/** @file  audio_fallback_module.h
 *  @brief Audio Fallback Module is used to manage audio fallback.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_FALLBACK_MODULE_H_
#define AUDIO_FALLBACK_MODULE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "mem_pool.h"
#include "sac_api.h"
#include "sac_error.h"

#ifdef __cplusplus
extern "C" {
#endif


/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize fallback.
 *
 *  @param[in] cfg           Fallback configuration.
 *  @param[in] mem_pool      Memory pool.
 *  @param[out] audio_error  Audio error.
 */
void audio_fallback_module_init(sac_fallback_module_cfg_t cfg, mem_pool_t *mem_pool, sac_error_t *audio_error);

/** @brief Gate function for tx compress processing stage. It will modify the fallback flag in the audio
 *         header based on the fallback_flag.
 *
 *  @param[in] instance      Process instance.
 *  @param[in] header        Audio header.
 *  @param[in] payload       Audio payload.
 *  @param[in] payload_size  Audio payload size.
 *  @return Fallback flag status.
 */
bool fallback_gate_is_fallback_on(void *instance, sac_header_t *header, uint8_t *payload, uint16_t payload_size);

/** @brief Gate function for tx compress discard processing stage. It is called by the compress discard processing
 *         stage. When fallback (compression is off), this forces the compression instance to process packets
 *         to keep the compression buffer primed for a switch to compression on.
 *
 *  @param[in] instance      Process instance.
 *  @param[in] header        Audio header.
 *  @param[in] payload       Audio payload.
 *  @param[in] payload_size  Audio payload size.
 *  @return Inverse of fallback_flag status.
 */
bool fallback_gate_is_fallback_off(void *instance, sac_header_t *header, uint8_t *payload, uint16_t payload_size);

/** @brief Gate function for rx compress processing stage. It will return the state of the audio header
 *         fallback flag.
 *
 *  @param[in] instance      Process instance.
 *  @param[in] header        Audio header.
 *  @param[in] payload       Audio payload.
 *  @param[in] payload_size  Audio payload size.
 *  @return Fallback_flag status.
 */
bool fallback_gate_fallback_detect(void *instance, sac_header_t *header, uint8_t *payload, uint16_t payload_size);

/** @brief Set the rx'ed rx link margin value from the node.
 *
 *  @param[in] rx_lm  RX link margin value.
 */
void fallback_set_rx_link_margin(int16_t rx_lm);

/** @brief Function for coordinator to update the fallback state machine. Called from inside the main background loop.
 */
void fallback_update_state(void);

/** @brief Update the txq and lm stats.
 */
void fallback_update_stats(void);

/** @brief Return status of fallback flag.
 *
 *  @retval true If active.
 *  @retval false If not active.
 */
bool fallback_is_active(void);

/** @brief Set fallback flag.
 */
void fallback_set_fallback_flag(void);

/** @brief Clear fallback flag.
 */
void fallback_clear_fallback_flag(void);


/** @brief Get fallback count.
 *
 *  @return Fallback count.
 */
uint32_t fallback_get_fallback_count(void);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_FALLBACK_MODULE_H_ */

