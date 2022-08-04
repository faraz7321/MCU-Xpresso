/** @file  audio_mixer_module.h
 *  @brief Audio Mixer Module is used to mix multiples audio stream inputs into one output audio stream.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_MIXER_MODULE_H_
#define AUDIO_MIXER_MODULE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "mem_pool.h"
#include "sac_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define MIN_NB_OF_INPUTS 2 /*!< The minimum number of input audio streams to be mixed */
#define MAX_NB_OF_INPUTS 3 /*!< The maximum supported number of input audio streams to be mixed */
#define MIN_NB_OF_BYTES_PER_PAYLOAD 2   /*!< The minimum number of bytes a payload can contain */
#define MAX_NB_OF_BYTES_PER_PAYLOAD 122 /*!< The maximum number of bytes a payload can contain  */
#define MAX_NB_OF_BYTES_PER_BUFFER (MAX_NB_OF_BYTES_PER_PAYLOAD * 2) /*!< Give a buffer to have at least 2 packets */

/* TYPES **********************************************************************/
/** @brief The Audio Mixer Module configurations.
 */
typedef struct audio_mixer_module_cfg {
    uint8_t nb_of_inputs; /*!< The number of inputs to be mixed */
    uint8_t payload_size; /*!< The audio payload size in bytes which must match the output consuming endpoint */
    uint8_t bit_depth;    /*!< Bit depth of each sample in the payload */
} audio_mixer_module_cfg_t;

/** @brief The Audio Mixer queue.
 */
typedef struct audio_mixer_queue {
    uint8_t samples[MAX_NB_OF_BYTES_PER_BUFFER]; /*!< Can have up to 2x the maximum payload in bytes */
    uint8_t current_size;                        /*!< The current size of the queue in bytes */
} audio_mixer_queue_t;

/** @brief The Audio Mixer Module instance.
 */
typedef struct audio_mixer_module {
    audio_mixer_queue_t input_samples_queue[MAX_NB_OF_INPUTS]; /*!< Pointer to the input samples to be mixed */
    uint8_t output_packet_buffer[MAX_NB_OF_BYTES_PER_PAYLOAD]; /*!< The mixed output packets array */
    audio_mixer_module_cfg_t cfg;                              /*!< Audio Mixer Module configurations */
} audio_mixer_module_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize Audio Mixer Module.
 *
 *  @param[in]  cfg          Used to configure the Audio Mixer Module.
 *  @param[in]  mem_pool     Memory pool for memory allocation.
 *  @param[out] audio_error  Error code.
 *  @return Audio Mixer Module instance.
 */
audio_mixer_module_t *audio_mixer_module_init(audio_mixer_module_cfg_t cfg, mem_pool_t *mem_pool, sac_error_t *audio_error);

/** @brief The Audio Mixer Module uses an algo to mix samples.
 *
 *  @param[in] audio_mixer_module  Audio Mixer Module instance.
 */
void audio_mixer_module_mix_packets(audio_mixer_module_t *audio_mixer_module);

/** @brief A payload is added to the input queue.
 *
 *  @param[in] input_samples_queue  The samples are stored in this queue.
 *  @param[in] samples              The stored audio samples.
 *  @param[in] size                 The stored audio samples size in bytes.
 */
void audio_mixer_module_append_samples(audio_mixer_queue_t *input_samples_queue, uint8_t *samples, uint8_t size);

/** @brief Silence samples are added to the input queue.
 *
 *  @param[in] input_samples_queue  The samples are stored in this queue.
 *  @param[in] size                 The stored audio samples size in bytes.
 */
void audio_mixer_module_append_silence(audio_mixer_queue_t *input_samples_queue, uint8_t size);

/** @brief Put the remainder in front of the queue.
 *
 *  @param[in] audio_mixer_module  Audio Mixer Module instance.
 */
void audio_mixer_module_handle_remainder(audio_mixer_module_t *audio_mixer_module);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_MIXER_MODULE_H_ */
