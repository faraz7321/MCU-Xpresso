/** @file  audio_volume.h
 *  @brief Audio processing functions related to the software volume control.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_VOLUME_H_
#define AUDIO_VOLUME_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define AUDIO_VOLUME_MAX  1      /*!< Maximum possible value for the audio volume */
#define AUDIO_VOLUME_MIN  0      /*!< Minimum possible value for the audio volume */
#define AUDIO_VOLUME_GRAD 0.0003 /*!< Step value to use when gradually increasing the volume towards
                                      the desired value */
#define AUDIO_VOLUME_TICK 0.1    /*!< Step value to use when increasing or decreasing the volume */

/* TYPES **********************************************************************/
/** @brief Volume Commands.
 */
typedef enum audio_volume_cmd {
    AUDIO_VOLUME_INCREASE,  /*!< Increase the volume by one tick */
    AUDIO_VOLUME_DECREASE,  /*!< Decrease the volume by one tick */
    AUDIO_VOLUME_MUTE,      /*!< Set the volume to 0 */
    AUDIO_VOLUME_GET_FACTOR /*!< Get the current volume value (between 0 and 10000) */
} audio_volume_cmd_t;

/** @brief Volume Instance.
 */
typedef struct audio_volume_instance {
    sac_bit_depth_t bit_depth;    /*!< Bit depth selected from the sac_bit_depth_t enum */
    uint8_t initial_volume_level; /*!< Initial volume level from 0 to 100 */
    float   _volume_factor;       /*!< Internal: factor used for calculation */
    float   _volume_threshold;    /*!< Internal: threshold set by user that _volume_factor will tend towards */
} audio_volume_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the digital volume control processing stage.
 *
 *  @param[in] volume    Volume control instance.
 *  @param[in] mem_pool  Memory pool for memory allocation.
 */
void audio_volume_init(void *instance, mem_pool_t *mem_pool);

/** @brief Deinitialize the digital volume control processing stage.
 *
 *  @param[in] volume  Volume control instance.
 */
void audio_volume_deinit(void *instance);

/** @brief Process volume on each audio sample.
 *
 *  @param[in]  volume       Volume instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed samples out.
 *  @return Number of samples processed. Return 0 if no samples processed.
 */
uint16_t audio_volume_process(void *instance, sac_header_t *header, uint8_t *data_in,
                              uint16_t bytes_count, uint8_t *data_out);

/** @brief Volume Control function.
 *
 *  @param[in] volume  Volume instance.
 *  @param[in] cmd     Control command.
 *  @param[in] arg     Control argument.
 *  @return Value returned dependent on command.
 */
uint32_t audio_volume_ctrl(void *instance, uint8_t cmd, uint32_t arg);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_VOLUME_H_ */
