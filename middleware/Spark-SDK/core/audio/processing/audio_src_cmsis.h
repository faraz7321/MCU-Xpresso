/** @file  audio_src_cmsis.h
 *  @brief Sampling rate converter processing stage using the CMSIS DSP software library.
 *
 *  @note This processing stage requires an Arm Cortex-M processor based device.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_SRC_CMSIS_H_
#define AUDIO_SRC_CMSIS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "arm_math.h"
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SRC CMSIS Ratio.
 */
typedef enum src_cmsis_ratio {
    AUDIO_SRC_THREE = 3, /*!< Ratio of 3 between original and resulting sampling rate */
    AUDIO_SRC_SIX   = 6  /*!< Ratio of 6 between original and resulting sampling rate */
} src_cmsis_ratio_t;

/** @brief SRC CMSIS FIR Instance.
 */
typedef union src_cmsis_fir_instance {
    arm_fir_interpolate_instance_q15 interpolate_instance; /*!< Instance for the arm_fir interpolation */
    arm_fir_decimate_instance_q15 decimate_instance;       /*!< Instance for the arm_fir decimation */
} src_cmsis_fir_instance_t;

/** @brief SRC CMSIS Configuration.
 */
typedef struct src_cmsis_cfg {
    src_cmsis_ratio_t ratio;  /*!< Ratio to use for the SRC */
    bool is_divider;          /*!< True if SRC is divider, false if SRC is multiplier */
    uint16_t payload_size;    /*!< Size of the payload in bytes expected at input */
    uint8_t bit_depth;        /*!< Bit depth of each sample in the payload */
} src_cmsis_cfg_t;

/** @brief SRC CMSIS Instance.
 */
typedef struct src_cmsis_instance {
    src_cmsis_cfg_t cfg;                   /*!< SRC CMSIS user configuration */
    src_cmsis_fir_instance_t fir_instance; /*!< Instance for the arm_fir module */
} src_cmsis_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the SRC CMSIS processing stage.
 *
 *  @param[in] instance  SRC CMSIS instance.
 *  @param[in] mem_pool  Memory pool for memory allocation.
 */
void audio_src_cmsis_init(void *instance, mem_pool_t *mem_pool);

/** @brief Deinitialize the SRC CMSIS processing stage.
 *
 *  @param[in] instance  SRC CMSIS instance.
 */
void audio_src_cmsis_deinit(void *instance);

/** @brief Process SRC on an audio packet.
 *
 *  @param[in]  instance     SRC CMSIS instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed samples out.
 *  @return Number of bytes processed. Return 0 if no samples processed.
 */
uint16_t audio_src_cmsis_process(void *instance, sac_header_t *header, uint8_t *data_in,
                                 uint16_t bytes_count, uint8_t *data_out);

/** @brief SRC CMSIS control function.
 *
 *  @param[in] instance  SRC CMSIS instance.
 *  @param[in] cmd       Control command.
 *  @param[in] arg       Control argument.
 *  @return Value returned dependent on command.
 */
uint32_t audio_src_cmsis_ctrl(void *instance, uint8_t cmd, uint32_t arg);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_SRC_CMSIS_H_ */
