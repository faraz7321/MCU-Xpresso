/** @file  audio_compression.h
 *  @brief Audio ADPCM compression / decompression processing stage.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_COMPRESSION_H_
#define AUDIO_COMPRESSION_H_

/* INCLUDES *******************************************************************/
#include "adpcm.h"
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define AUDIO_COMPRESSION_OVERHEAD_MAX_SIZE 6 /*!< 3 bytes for the decoder state per channel */

/* TYPES **********************************************************************/
/** @brief Audio Compression Commands.
 */
typedef enum audio_compression_cmd {
    AUDIO_COMPRESSION_ENABLE,   /*!< Enable the Audio Compression */
    AUDIO_COMPRESSION_DISABLE,  /*!< Disable the Audio Compression */
    AUDIO_COMPRESSION_GET_STATE /*!< Get the Audio Compression state */
} audio_compression_cmd_t;

/** @brief Audio Compression Mode.
 */
typedef enum audio_compression_mode {
    AUDIO_COMPRESSION_PACK_STEREO,   /*!< Pack stereo uncompressed stream to stereo compressed stream mode */
    AUDIO_COMPRESSION_UNPACK_STEREO, /*!< Unpack stereo compressed stream to stereo uncompressed stream mode */
    AUDIO_COMPRESSION_PACK_MONO,     /*!< Pack mono uncompressed stream to mono compressed stream mode */
    AUDIO_COMPRESSION_UNPACK_MONO    /*!< Unpack mono compressed stream to mono uncompressed stream mode */
} audio_compression_mode_t;

/** @brief Audio Compression Instance.
 */
typedef struct audio_compression_instance {
    uint8_t compression_enabled;               /*!< Audio Compression enabled/disabled status */
    adpcm_state_t adpcm_left_state;            /*!< Left ADPCM encoder state */
    adpcm_state_t adpcm_right_state;           /*!< Right ADPCM encoder state */
    audio_compression_mode_t compression_mode; /*!< Audio Compression mode */
} audio_compression_instance_t;

/** @brief Audio Compression Stereo Header.
 */
typedef struct audio_compression_adpcm_stereo_header {
    adpcm_state_t adpcm_header_left_state;  /*!< Audio compression left channel state */
    adpcm_state_t adpcm_header_right_state; /*!< Audio compression right channel state */
} audio_compression_adpcm_stereo_header_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize compression process.
 *
 *  @param[in] instance  Compression instance.
 *  @param[in] mem_pool  Memory pool for memory allocation.
 */
void audio_compression_init(void *instance, mem_pool_t *mem_pool);

/** @brief Deinitialize compression process.
 *
 *  @param[in] instance  Compression instance.
 */
void audio_compression_deinit(void *instance);

/** @brief Audio compression control function.
 *
 *  @param[in] instance  Compression instance.
 *  @param[in] cmd       Control command.
 *  @param[in] arg       Control argument.
 *  @return Audio Compression Status.
 */
uint32_t audio_compression_ctrl(void *instance, uint8_t cmd, uint32_t arg);

/** @brief Process audio samples compression.
 *
 *  @param[in]  instance     Compression instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @return Number of bytes processed.
 */
uint16_t audio_compression_process(void *instance, sac_header_t *header,
                                   uint8_t *data_in, uint16_t size, uint8_t *data_out);

/** @brief Process the last audio samples to maintain the sample history. Thus, any subsequent switch to
 *         audio_compression_process will provide a clean audio output. Output is discarded and
 *         the function returns 0. Only valid for packing.
 *
 *  @param[in]  instance     Compression instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @return 0.
 */
uint16_t audio_compression_process_discard(void *instance, sac_header_t *header,
                                           uint8_t *data_in, uint16_t size, uint8_t *data_out);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_COMPRESSION_H_ */
