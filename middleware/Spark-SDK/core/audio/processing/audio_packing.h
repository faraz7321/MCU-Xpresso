/** @file  audio_packing.h
 *  @brief Audio packing/unpacking for 20/24 bits audio processing stage.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_PACKING_H_
#define AUDIO_PACKING_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef enum audio_packing_cmd {
    AUDIO_PACKING_ENABLE,
    AUDIO_PACKING_DISABLE,
    AUDIO_PACKING_SET_MODE,
    AUDIO_PACKING_GET_MODE
} audio_packing_cmd_t;

typedef enum audio_packing_mode {
    AUDIO_PACK_20BITS,
    AUDIO_PACK_24BITS,
    AUDIO_PACK_20BITS_16BITS,
    AUDIO_PACK_24BITS_16BITS,
    AUDIO_UNPACK_20BITS,
    AUDIO_UNPACK_24BITS,
    AUDIO_UNPACK_20BITS_16BITS,
    AUDIO_UNPACK_24BITS_16BITS,
    AUDIO_EXTEND_20BITS,
    AUDIO_EXTEND_24BITS
} audio_packing_mode_t;

typedef struct audio_packing_instance {
    uint8_t packing_enabled;
    audio_packing_mode_t packing_mode;
} audio_packing_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize packing process.
 *
 *  @param[in] instance  Packing instance.
 *  @param[in] mem_pool  Memory pool for memory allocation.
 */
void audio_packing_init(void *instance, mem_pool_t *mem_pool);

/** @brief Deinitialize packing process.
 *
 *  @param[in] instance  Packing instance.
 */
void audio_packing_deinit(void *instance);

/** @brief Audio packing control function.
 *
 *  @param[in] instance  Packing instance.
 *  @param[in] cmd       Control command.
 *  @param[in] arg       Command argument.
 *  @return Audio Packing Status.
 */
uint32_t audio_packing_ctrl(void *instance, uint8_t cmd, uint32_t arg);

/** @brief Process audio samples packing.
 *
 *  @param[in]  instance     Packing instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @return Number of bytes processed.
 */
uint16_t audio_packing_process(void *instance, sac_header_t *header,
                               uint8_t *data_in, uint16_t size, uint8_t *data_out);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PACKING_H_ */
