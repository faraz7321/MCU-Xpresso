/** @file  audio_volume.c
 *  @brief Audio processing functions related to the software volume control.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "audio_volume.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void audio_volume_increase(audio_volume_instance_t *volume_ctrl);
static void audio_volume_decrease(audio_volume_instance_t *volume_ctrl);
static void audio_volume_mute(audio_volume_instance_t *volume_ctrl);
static float audio_volume_get_level(audio_volume_instance_t *volume_ctrl);
static void adjust_volume_factor(audio_volume_instance_t *volume);
static void apply_volume_factor_16bits(int16_t *audio_samples_in, uint16_t samples_count,
                                       int16_t *audio_samples_out, float volume_factor);
static void apply_volume_factor_32bits(int32_t *audio_samples_in, uint16_t samples_count,
                                       int32_t *audio_samples_out, float volume_factor);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_volume_init(void *instance, mem_pool_t *mem_pool)
{
    (void)mem_pool;
    audio_volume_instance_t *vol_inst = (audio_volume_instance_t *) instance;

    vol_inst->_volume_factor = (vol_inst->initial_volume_level / 100.0);
    vol_inst->_volume_threshold = (vol_inst->initial_volume_level / 100.0);
}

void audio_volume_deinit(void *instance)
{
    audio_volume_instance_t *vol_inst = (audio_volume_instance_t *)instance;

    audio_volume_mute(vol_inst);
}

uint32_t audio_volume_ctrl(void *instance, uint8_t cmd, uint32_t arg)
{
    (void)arg;
    uint32_t ret = 0;
    audio_volume_instance_t *vol_inst = (audio_volume_instance_t *)instance;

    switch ((audio_volume_cmd_t)cmd) {
    case AUDIO_VOLUME_INCREASE:
        audio_volume_increase(vol_inst);
        break;
    case AUDIO_VOLUME_DECREASE:
        audio_volume_decrease(vol_inst);
        break;
    case AUDIO_VOLUME_MUTE:
        audio_volume_mute(vol_inst);
        break;
    case AUDIO_VOLUME_GET_FACTOR:
        ret = (uint32_t)(audio_volume_get_level(vol_inst) * 10000.0);
        break;
    }
    return ret;
}

uint16_t audio_volume_process(void *instance, sac_header_t *header,
                              uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    (void)header;
    audio_volume_instance_t *vol_inst = (audio_volume_instance_t *)instance;

    if ((vol_inst->_volume_threshold != AUDIO_VOLUME_MAX) || (vol_inst->_volume_factor != AUDIO_VOLUME_MAX)) {
        adjust_volume_factor(vol_inst);

        switch (vol_inst->bit_depth) {
        case AUDIO_16BITS:
            apply_volume_factor_16bits((int16_t *)data_in, (size / AUDIO_16BITS_BYTE), (int16_t *)data_out, vol_inst->_volume_factor);
            break;
        case AUDIO_20BITS:
        case AUDIO_24BITS:
            apply_volume_factor_32bits((int32_t *)data_in, (size / AUDIO_32BITS_BYTE), (int32_t *)data_out, vol_inst->_volume_factor);
            break;
        default:
            return 0;
        }

        return size;
    } else {
        return 0;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Increase the audio volume.
 *
 *  @param[in] instance  Volume instance.
 */
static void audio_volume_increase(audio_volume_instance_t *instance)
{
    instance->_volume_threshold += AUDIO_VOLUME_TICK;
    if (instance->_volume_threshold >= AUDIO_VOLUME_MAX) {
        instance->_volume_threshold = AUDIO_VOLUME_MAX;
    }
}

/** @brief Decrease the audio volume.
 *
 *  @param[in] instance  Volume instance.
 */
static void audio_volume_decrease(audio_volume_instance_t *instance)
{
    instance->_volume_threshold -= AUDIO_VOLUME_TICK;
    if (instance->_volume_threshold <= AUDIO_VOLUME_MIN) {
        instance->_volume_threshold = AUDIO_VOLUME_MIN;
    }
}

/** @brief Mute the audio.
 *
 *  @param[in] instance  Volume instance.
 */
static void audio_volume_mute(audio_volume_instance_t *instance)
{
    instance->_volume_factor = 0;
    instance->_volume_threshold = 0;
}

/** @brief Get the audio volume level.
 *
 *  @param[in] instance  Volume instance.
 *  @return Volume level.
 */
static float audio_volume_get_level(audio_volume_instance_t *instance)
{
    return instance->_volume_factor;
}

/** @brief Adjust volume factor to tend toward volume threshold.
 *
 *  @param[in]  instance  Volume instance.
 */
static void adjust_volume_factor(audio_volume_instance_t *instance)
{
    /* Test if factor increases or decreases and reaches the desired value */
    /* The value is correct if overflow */
    if (instance->_volume_factor < instance->_volume_threshold) {
        instance->_volume_factor += AUDIO_VOLUME_GRAD;
        if (instance->_volume_factor >= instance->_volume_threshold) {
            instance->_volume_factor = instance->_volume_threshold;
        }
    } else if (instance->_volume_factor > instance->_volume_threshold) {
        instance->_volume_factor -= AUDIO_VOLUME_GRAD;
        if (instance->_volume_factor <= instance->_volume_threshold) {
            instance->_volume_factor = instance->_volume_threshold;
        }
    }
}

/** @brief Process a volume factor on each sample.
 *
 *  @param[in]  audio_samples_in   16bits samples pointer of data in.
 *  @param[in]  samples_count      Number of samples to process.
 *  @param[out] audio_samples_out  16bits samples pointer of data out.
 *  @param[in]  volume_factor      Volume factor to apply.
 */
static void apply_volume_factor_16bits(int16_t *audio_samples_in, uint16_t samples_count,
                                       int16_t *audio_samples_out, float volume_factor)
{
    uint16_t count;

    for (count = 0; count < samples_count; count++) {
        audio_samples_out[count] = audio_samples_in[count] * volume_factor;
    }
}

/** @brief Process a volume factor on each sample.
 *
 *  @param[in]  audio_samples_in   32bits samples pointer of data in.
 *  @param[in]  samples_count      Number of samples to process.
 *  @param[out] audio_samples_out  32bits samples pointer of data out.
 *  @param[in]  volume_factor      Volume factor to apply.
 */
static void apply_volume_factor_32bits(int32_t *audio_samples_in, uint16_t samples_count,
                                       int32_t *audio_samples_out, float volume_factor)
{
    uint16_t count;

    for (count = 0; count < samples_count; count++) {
        audio_samples_out[count] = audio_samples_in[count] * volume_factor;
    }
}
