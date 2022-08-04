/** @file  audio_compression.c
 *  @brief Audio ADPCM compression / decompression processing stage.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "audio_compression.h"

/* MACROS *********************************************************************/
#define AUDIO_NB_SAMPLE_TO_BYTE(nb_sample) (2 * (nb_sample))
#define AUDIO_BYTE_TO_NB_SAMPLE(nb_byte)   ((nb_byte) / 2)

/* PRIVATE FUNCTION PROTOTYPES *************************************************/
static uint16_t pack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_compression_init(void *instance, mem_pool_t *mem_pool)
{
    (void)mem_pool;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    compress_inst->compression_enabled = 1;
    adpcm_init_state(&(compress_inst->adpcm_left_state));
    adpcm_init_state(&(compress_inst->adpcm_right_state));
}

void audio_compression_deinit(void *instance)
{
    (void)instance;
}

uint32_t audio_compression_ctrl(void *instance, uint8_t cmd, uint32_t arg)
{
    (void)arg;
    uint32_t ret = 0;
    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    switch ((audio_compression_cmd_t)cmd) {
    case AUDIO_COMPRESSION_ENABLE:
        ret = compress_inst->compression_enabled = 1;
        break;
    case AUDIO_COMPRESSION_DISABLE:
        ret = compress_inst->compression_enabled = 0;
        break;
    case AUDIO_COMPRESSION_GET_STATE:
        ret = compress_inst->compression_enabled;
        break;
    default:
        break;
    }
    return ret;
}

uint16_t audio_compression_process(void *instance, sac_header_t *header,
                                   uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    (void)header;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *) instance;
    uint8_t output_size = 0;

    if (compress_inst->compression_enabled) {
        switch (compress_inst->compression_mode) {
        case AUDIO_COMPRESSION_PACK_STEREO:
            output_size = pack_stereo(instance, data_in, size, data_out);
            break;
        case AUDIO_COMPRESSION_UNPACK_STEREO:
            output_size = unpack_stereo(instance, data_in, size, data_out);
            break;
        case AUDIO_COMPRESSION_PACK_MONO:
            output_size = pack_mono(instance, data_in, size, data_out);
            break;
        case AUDIO_COMPRESSION_UNPACK_MONO:
            output_size = unpack_mono(instance, data_in, size, data_out);
            break;
        }
    }
    return output_size;
}

uint16_t audio_compression_process_discard(void *instance, sac_header_t *header,
                                           uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    (void)header;
    (void)data_out;
    uint8_t discard_size;
    uint8_t discard_buffer[10];

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *) instance;

    if (compress_inst->compression_enabled) {
        switch (compress_inst->compression_mode) {
        case AUDIO_COMPRESSION_PACK_STEREO:
            /* Only need the last stereo sample, to be added to the history */
            discard_size = AUDIO_NB_SAMPLE_TO_BYTE(2);
            data_in += (size - discard_size);
            pack_stereo(instance, data_in, discard_size, discard_buffer);
            break;
        case AUDIO_COMPRESSION_PACK_MONO:
            /* Only need the last sample, to be added to the history */
            discard_size = AUDIO_NB_SAMPLE_TO_BYTE(1);
            data_in += (size - discard_size);
            pack_mono(instance, data_in, discard_size, discard_buffer);
            break;
        case AUDIO_COMPRESSION_UNPACK_STEREO:
        case AUDIO_COMPRESSION_UNPACK_MONO:
            break;
        }
    }
    return 0;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Pack stereo uncompressed stream to stereo compressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the uncompressed stereo data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the compressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t pcm_sample_count;
    int16_t *input_buffer;
    uint8_t left_code;
    uint8_t right_code;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    input_buffer = (int16_t *)buffer_in;
    pcm_sample_count = AUDIO_BYTE_TO_NB_SAMPLE(buffer_in_size);

    /* Set left adpcm encoder status */
    memcpy(buffer_out, &(compress_inst->adpcm_left_state), sizeof(adpcm_state_t));

    /* Set right adpcm encoder status */
    memcpy(&buffer_out[sizeof(adpcm_state_t)], &(compress_inst->adpcm_right_state), sizeof(adpcm_state_t));

    buffer_out += sizeof(audio_compression_adpcm_stereo_header_t);

    /* Since two samples get compressed into a single byte,
     * the loop will work with two samples at the same time (left and right samples)
     */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        left_code  = adpcm_encode(*input_buffer++, &(compress_inst->adpcm_left_state));
        right_code = adpcm_encode(*input_buffer++, &(compress_inst->adpcm_right_state));
        /* Concatenate two ADPCM samples code per byte in the output buffer (4 bit msb, 4 bit lsb) */
        *buffer_out++ = (left_code & 0x0F) | ((right_code << 4) & 0xF0);
    }

    return (pcm_sample_count / 2) + sizeof(audio_compression_adpcm_stereo_header_t);
}

/** @brief Unpack stereo compressed stream to stereo uncompressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the input stereo compressed data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the uncompressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    int16_t *output_buffer;
    uint16_t pcm_sample_count;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    output_buffer = (int16_t *)buffer_out;

    /* Get left adpcm status */
    memcpy(&(compress_inst->adpcm_left_state), buffer_in, sizeof(adpcm_state_t));

    /* Get right adpcm encoder status */
    memcpy(&(compress_inst->adpcm_right_state), &buffer_in[sizeof(adpcm_state_t)], sizeof(adpcm_state_t));

    buffer_in += sizeof(audio_compression_adpcm_stereo_header_t);
    pcm_sample_count = (buffer_in_size - sizeof(audio_compression_adpcm_stereo_header_t)) * 2;

    /* Since two samples are compressed into a single byte, the loop will work with two compressed
     * samples at the same time (left and right samples)
     */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        *output_buffer++ = adpcm_decode((*buffer_in) & 0x0F, &(compress_inst->adpcm_left_state));
        *output_buffer++ = adpcm_decode(((*buffer_in++) >> 4) & 0x0F, &(compress_inst->adpcm_right_state));
    }
    return AUDIO_NB_SAMPLE_TO_BYTE(pcm_sample_count);
}

/** @brief Pack mono uncompressed stream to mono compressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the uncompressed mono data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the compressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t pcm_sample_count;
    int16_t *input_buffer;
    uint8_t left_code;
    uint8_t right_code;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    /* Set left adpcm encoder status */
    memcpy(buffer_out, &(compress_inst->adpcm_left_state), sizeof(adpcm_state_t));
    buffer_out += sizeof(adpcm_state_t);

    input_buffer = (int16_t *)buffer_in;
    pcm_sample_count = AUDIO_BYTE_TO_NB_SAMPLE(buffer_in_size);

    /* Since two samples get compressed into a single byte, the loop will work with two samples at the same time */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        left_code  = adpcm_encode(*input_buffer++, &(compress_inst->adpcm_left_state));
        right_code = adpcm_encode(*input_buffer++, &(compress_inst->adpcm_left_state));
        /* Concatenate two ADPCM samples code per byte in the output buffer (4 bit msb, 4 bit lsb) */
        *buffer_out++ = (left_code & 0x0F) | (right_code << 4);
    }

    /* Manage odd number of samples */
    if (pcm_sample_count & 0x01) {
        left_code  = adpcm_encode(*input_buffer, &(compress_inst->adpcm_left_state));
        *buffer_out = left_code & 0x0F;
    }

    return ((pcm_sample_count / 2) + (pcm_sample_count & 0x01)) * sizeof(uint8_t) + sizeof(state_variable_t);
}

/** @brief Unpack mono compressed stream to mono uncompressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the input mono compressed data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the uncompressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    int16_t *output_buffer;
    uint16_t pcm_sample_count;

    audio_compression_instance_t *compress_inst = (audio_compression_instance_t *)instance;

    output_buffer = (int16_t *)buffer_out;

    /* Get left adpcm status */
    memcpy(&(compress_inst->adpcm_left_state), buffer_in, sizeof(adpcm_state_t));
    buffer_in += sizeof(adpcm_state_t);

    /* Get number of mono samples */
    pcm_sample_count = (buffer_in_size -sizeof(adpcm_state_t)) * 2;

    /* Since two samples are compressed into a single byte, the loop will work with two compressed
     * samples at the same time (left and right samples)
     */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        *output_buffer++ = adpcm_decode((*buffer_in & 0x0F), &(compress_inst->adpcm_left_state));
        *output_buffer++ = adpcm_decode((*buffer_in++ >> 4) & 0x0F, &(compress_inst->adpcm_left_state));
    }

    return AUDIO_NB_SAMPLE_TO_BYTE(pcm_sample_count);
}
