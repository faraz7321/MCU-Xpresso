/** @file  audio_cdc_module.h
 *  @brief Audio CDC Module is used to manage clock drift compensation.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_CDC_MODULE_H_
#define AUDIO_CDC_MODULE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "queue.h"
#include "sac_api.h"
#include "resampling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define CDC_QUEUE_SIZE_INFLATION 3

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize audio clock drift compensation (CDC) module.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Mem pool handle.
 *  @param[out] err       Error code.
 */
void audio_cdc_module_init(sac_pipeline_t *pipeline,  mem_pool_t *mem_pool, sac_error_t *err);

/** @brief Process the CDC in the node.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  node      Node containing samples.
 *  @param[out] err       Error code.
 *  @return Processed node.
 */
queue_node_t *audio_cdc_module_process(sac_pipeline_t *pipeline, queue_node_t *in_node, sac_error_t *err);

/** @brief Run rolling average with new queue length.
 *
 *  @param[in] pipeline  Pipeline instance.
 */
void audio_cdc_module_update_queue_avg(sac_pipeline_t *pipeline);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_CDC_MODULE_H_ */
