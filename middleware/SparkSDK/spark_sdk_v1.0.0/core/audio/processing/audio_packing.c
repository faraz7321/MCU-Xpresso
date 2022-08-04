/** @file  audio_packing.c
 *  @brief Audio packing/unpacking for 20/24 bits audio processing stage.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "audio_packing.h"

/* MACROS *********************************************************************/
#define SAMPLE_SIZE_20BITS (2.5f)
#define PACKED_SIZE_20BITS ((uint8_t)(SAMPLE_SIZE_20BITS * 2.0f))
#define SAMPLE_SIZE_16BITS 2
#define SAMPLE_SIZE_24BITS 3
#define SAMPLE_SIZE_32BITS 4

/* PRIVATE FUNCTION PROTOTYPES *************************************************/
static void extend_msb_20bits_value(uint32_t *value);
static void extend_msb_24bits_value(uint32_t *value);
static uint16_t pack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t extend_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t extend_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_packing_init(void *instance, mem_pool_t *mem_pool)
{
    (void)mem_pool;

    audio_packing_instance_t *packing_inst = (audio_packing_instance_t *)instance;

    packing_inst->packing_enabled = 1;
}

void audio_packing_deinit(void *instance)
{
    audio_packing_instance_t *packing_inst = (audio_packing_instance_t *)instance;

    packing_inst->packing_enabled = 0;
}

uint32_t audio_packing_ctrl(void *instance, uint8_t cmd, uint32_t arg)
{
    (void)arg;
    uint32_t ret = 0;
    audio_packing_instance_t *packing_inst = (audio_packing_instance_t *)instance;

    switch ((audio_packing_cmd_t)cmd) {
    case AUDIO_PACKING_ENABLE:
        ret = packing_inst->packing_enabled = 1;
        break;
    case AUDIO_PACKING_DISABLE:
        ret = packing_inst->packing_enabled = 0;
        break;
    case AUDIO_PACKING_SET_MODE:
        packing_inst->packing_mode = arg;
        break;
    case AUDIO_PACKING_GET_MODE:
        ret = packing_inst->packing_mode;
        break;
    default:
        break;
    }
    return ret;
}

uint16_t audio_packing_process(void *instance, sac_header_t *header,
                               uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    (void)header;

    audio_packing_instance_t *packing_inst = (audio_packing_instance_t *) instance;
    uint16_t output_size = 0;

    if (!packing_inst->packing_enabled) {
        return 0;
    }

    switch (packing_inst->packing_mode) {
    case AUDIO_PACK_20BITS:
        output_size = pack_20bits(data_in, size, data_out);
        break;
    case AUDIO_PACK_24BITS:
        output_size = pack_24bits(data_in, size, data_out);
        break;
    case AUDIO_UNPACK_20BITS:
        output_size = unpack_20bits(data_in, size, data_out);
        break;
    case AUDIO_UNPACK_24BITS:
        output_size = unpack_24bits(data_in, size, data_out);
        break;
    case AUDIO_EXTEND_20BITS:
        output_size = extend_20bits(data_in, size, data_out);
        break;
    case AUDIO_EXTEND_24BITS:
        output_size = extend_24bits(data_in, size, data_out);
        break;
    case AUDIO_PACK_20BITS_16BITS:
        output_size = pack_20bits_16bits(data_in, size, data_out);
        break;
    case AUDIO_PACK_24BITS_16BITS:
        output_size = pack_24bits_16bits(data_in, size, data_out);
        break;
    case AUDIO_UNPACK_20BITS_16BITS:
        output_size = unpack_20bits_16bits(data_in, size, data_out);
        break;
    case AUDIO_UNPACK_24BITS_16BITS:
        output_size = unpack_24bits_16bits(data_in, size, data_out);
        break;
    }

    return output_size;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Pack 32 bits audio samples into 20 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 20 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 20 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 2) {
        *(uint32_t *)(&(data_out[0])) = 0;
        /* Check if sample #2 exists */
        if ((i + 1) < sample_count) {
            /* Copy Sample #2 */
            *(uint32_t *)(&(data_out[2])) = ((data32_in[1]) & 0xFFFFF);
            /* Shift Sample #2 by 4 bits */
            *(uint32_t *)(&(data_out[2])) <<= 4;
            /* Increment return size by (floor(2.5)) */
            ret += 2;
        }
        /* Copy Sample #1 without erasing Sample #2 */
        *(uint32_t *)(&(data_out[0])) |=  ((data32_in[0]) & 0xFFFFF);
        /* Increment return size by (ceil(2.5)) */
        ret += 3;

        /* Increment pointers */
        data_out += PACKED_SIZE_20BITS;
        data32_in += 2;
    }

    return ret;
}

/** @brief Pack 32 bits audio samples into 24 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 24 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 24 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i ++) {
        /* Copy Sample */
        *(uint32_t *)(&(data_out[0])) =  ((*data32_in) & 0xFFFFFF);
        /* Increment return size */
        ret += SAMPLE_SIZE_24BITS;

        /* Increment pointers */
        data_out += SAMPLE_SIZE_24BITS;
        data32_in++;
    }

    return ret;
}

/** @brief Pack 32 bits words containing 20 bits audio samples into 16 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 20 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 16 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint16_t *data_out = (uint16_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i ++) {
        /* Copy 16 bits MSB of input sample */
        data_out[i] =  ((data32_in[i] >> 4) & 0xFFFF);
        /* Increment return size */
        ret += SAMPLE_SIZE_16BITS;
    }

    return ret;
}

/** @brief Pack 32 bits words containing 24 bits audio samples into 16 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 24 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 16 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint16_t *data_out = (uint16_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i ++) {
        /* Copy 16 bits MSB of input sample */
        data_out[i] =  ((data32_in[i] >> 8) & 0xFFFF);
        /* Increment return size */
        ret += SAMPLE_SIZE_16BITS;
    }

    return ret;
}

/** @brief Unpack 20 bits audio samples into 32 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 20 bits samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 32 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint8_t *data_in = buffer_in;
    uint16_t sample_count = (uint16_t)((float)buffer_in_size / SAMPLE_SIZE_20BITS);
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 2) {

        /* Copy Sample #1 */
        (data32_out[0]) = (*(uint32_t *)(&(data_in[0])));
        extend_msb_20bits_value(&data32_out[0]);
        /* Increment return size */
        ret += SAMPLE_SIZE_32BITS;

        /* Check if sample #2 exists */
        if ((i + 1) < sample_count) {
            /* Copy Sample #2 */
            (data32_out[1]) = (*(uint32_t *)(&(data_in[2])) & 0xFFFFF0);
            /* Shift Sample #2 by 4 bits */
            (data32_out[1]) >>= 4;
            extend_msb_20bits_value(&data32_out[1]);
            /* Increment return size */
            ret += SAMPLE_SIZE_32BITS;
        }

        /* Increment pointers */
        data_in += PACKED_SIZE_20BITS;
        data32_out += 2;
    }

    return ret;
}

/** @brief Unpack 24 bits audio samples into 32 bits audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 24 bits samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 32 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint8_t *data_in = buffer_in;
    uint16_t sample_count = (uint16_t)((float)buffer_in_size / SAMPLE_SIZE_24BITS);
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i ++) {
        /* Copy Sample */
        (*data32_out) = (*(uint32_t *)(&(data_in[0])) & 0xFFFFFF);
        extend_msb_24bits_value(data32_out);
        /* Increment return size */
        ret += SAMPLE_SIZE_32BITS;

        /* Increment pointers */
        data_in += SAMPLE_SIZE_24BITS;
        data32_out++;
    }

    return ret;
}

/** @brief Unpack 16 bits audio samples into 32 bits words containing 20 bits audio.
 *
 *  @param[in]  buffer_in       Array of the input 16 bits audio samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 20 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t *data32_in = (uint16_t *)buffer_in;
    uint32_t *data_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_16BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy input sample */
        data_out[i] =  ((data32_in[i] << 4) & 0x000FFFF0);
        extend_msb_20bits_value(&data_out[i]);
        /* Increment return size */
        ret += SAMPLE_SIZE_32BITS;
    }

    return ret;
}

/** @brief Unpack 16 bits audio samples into 32 bits words containing 24 bits audio.
 *
 *  @param[in]  buffer_in       Array of the input 16 bits audio samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 24 bits stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t *data32_in = (uint16_t *)buffer_in;
    uint32_t *data_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_16BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy input sample */
        data_out[i] =  ((data32_in[i] << 8) & 0x00FFFF00);
        extend_msb_24bits_value(&data_out[i]);
        /* Increment return size */
        ret += SAMPLE_SIZE_32BITS;
    }

    return ret;
}

/** @brief Extend 20 bits audio samples sign bit into 32 bits word.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 20 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Output of the input 32 bits samples.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t extend_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;

    /* Copy data into output buffer */
    memcpy(buffer_out, buffer_in, buffer_in_size);

    /* Extend sign bit of output buffer samples */
    for (i = 0; i < sample_count; i++) {
        extend_msb_20bits_value(&data32_out[i]);
    }

    return buffer_in_size;
}

/** @brief Extend 24 bits audio samples sign bit into 32 bits word.
 *
 *  @param[in]  buffer_in       Array of the input 32 bits samples containing 24 bits audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Output of the input 32 bits samples.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t extend_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;

    /* Copy data into output buffer */
    memcpy(buffer_out, buffer_in, buffer_in_size);

    /* Extend sign bit of output buffer samples */
    for (i = 0; i < sample_count; i++) {
        extend_msb_24bits_value(&data32_out[i]);
    }

    return buffer_in_size;
}

static void extend_msb_20bits_value(uint32_t *value) {

    if ((*value) & (1 << 19)) {
        /* Negative value */
        *value |= 0xFFF00000;
    } else {
        /* Positive value */
        *value &= 0x000FFFFF;
    }
}

static void extend_msb_24bits_value(uint32_t *value) {

    if ((*value) & (1 << 23)) {
        /* Negative value */
        *value |= 0xFF000000;
    } else {
        /* Positive value */
        *value &= 0x00FFFFFF;
    }
}
