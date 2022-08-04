/** @file  audio_cdc_module.c
 *  @brief Audio CDC Module is used to manage clock drift compensation.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_cdc_module.h"
#include <string.h>
#include "resampling.h"

/* CONSTANTS ******************************************************************/
#define BIT_PER_BYTE   8
#define DECIMAL_FACTOR 100

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void cdc_update_queue_status(sac_cdc_instance_t *instance, queue_node_t *in_node);
static uint16_t cdc_compensate(sac_cdc_instance_t *instance, queue_node_t *in_node, queue_node_t *out_node);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_cdc_module_init(sac_pipeline_t *pipeline, mem_pool_t *mem_pool, sac_error_t *err)
{
    sac_cdc_instance_t *cdc_instance;

    *err = SAC_ERR_NONE;

    pipeline->_cdc_instance = mem_pool_malloc(mem_pool, sizeof(sac_cdc_instance_t));
    if (pipeline->_cdc_instance == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }
    cdc_instance = pipeline->_cdc_instance;

    cdc_instance->avg_sum = 0;
    cdc_instance->avg_val = 0;
    cdc_instance->avg_idx = 0;
    cdc_instance->queue_avg_size = pipeline->cfg.cdc_queue_avg_size;

    /* Allocate rolling average memory */
    cdc_instance->avg_arr = (uint8_t *)mem_pool_malloc(mem_pool, cdc_instance->queue_avg_size);
    if (cdc_instance->avg_arr == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }
    memset(cdc_instance->avg_arr, 0, cdc_instance->queue_avg_size);

    /* Initialize resampling configuration */
    switch (pipeline->consumer->cfg.bit_depth) {
    case AUDIO_16BITS:
        cdc_instance->size_of_buffer_type = AUDIO_16BITS_BYTE;
        break;
    case AUDIO_20BITS:
        cdc_instance->size_of_buffer_type = AUDIO_20BITS_BYTE;
        break;
    case AUDIO_24BITS:
        cdc_instance->size_of_buffer_type = AUDIO_24BITS_BYTE;
        break;
    default:
        *err = SAC_ERR_CDC_INIT_FAILURE;
        return;
    }

    resampling_config_t resampling_config = {
        .nb_sample = (pipeline->consumer->cfg.audio_payload_size / cdc_instance->size_of_buffer_type),
        .nb_channel = pipeline->consumer->cfg.channel_count,
        .resampling_length = pipeline->cfg.cdc_resampling_length
    };

    resampling_config.buffer_type = pipeline->consumer->cfg.bit_depth - 1;

    /* Initialize the resampling instance */
    if (resampling_init(&cdc_instance->resampling_instance, &resampling_config) != RESAMPLING_NO_ERROR) {
        *err = SAC_ERR_CDC_INIT_FAILURE;
        return;
    }

    /* Configure threshold */
    cdc_instance->normal_queue_size = pipeline->consumer->cfg.queue_size * DECIMAL_FACTOR;
    uint8_t sample_amount = pipeline->consumer->cfg.audio_payload_size /
                            (pipeline->consumer->cfg.channel_count * cdc_instance->size_of_buffer_type);
    cdc_instance->max_queue_offset = DECIMAL_FACTOR / sample_amount;
}

queue_node_t *audio_cdc_module_process(sac_pipeline_t *pipeline, queue_node_t *in_node, sac_error_t *err)
{
    queue_node_t *out_node;

    *err = SAC_ERR_NONE;

    cdc_update_queue_status(pipeline->_cdc_instance, in_node);

    out_node = queue_get_free_node(pipeline->producer->_free_queue);
    if (out_node == NULL) {
        *err = SAC_ERR_PRODUCER_Q_FULL;
        return in_node;
    }
    if (cdc_compensate(pipeline->_cdc_instance, in_node, out_node) > 0) {
        /* AUDIO_CDC is running, use output node */
        queue_free_node(in_node);
        return out_node;
    } else {
        /* No resampling happened, return input node */
        queue_free_node(out_node);
        return in_node;
    }
}

void audio_cdc_module_update_queue_avg(sac_pipeline_t *pipeline)
{
    uint16_t current_queue_length = queue_get_length(pipeline->consumer->_queue);
    sac_cdc_instance_t *cdc_instance = pipeline->_cdc_instance;
    uint16_t avg_idx = cdc_instance->avg_idx;

    /* Update Rolling Avg */
    cdc_instance->avg_sum -= cdc_instance->avg_arr[avg_idx]; /* Remove oldest value */
    cdc_instance->avg_arr[avg_idx] = current_queue_length;
    cdc_instance->avg_sum += cdc_instance->avg_arr[avg_idx]; /* Add new value */
    if (++avg_idx >= cdc_instance->queue_avg_size) {
        avg_idx = 0;
    }
    cdc_instance->avg_val = (cdc_instance->avg_sum * DECIMAL_FACTOR) / cdc_instance->queue_avg_size; /* decimals are part of int value */
    cdc_instance->avg_idx = avg_idx;
}

/* PRIVATE FUNCTIONS **********************************************************/
static void cdc_update_queue_status(sac_cdc_instance_t *instance, queue_node_t *in_node)
{
    if ((sac_node_get_header(in_node)->tx_queue_level_high == 1) &&
        (resample_get_state(&instance->resampling_instance) == RESAMPLING_IDLE)) {
        instance->wait_for_queue_full = true;
    }

    if (resample_get_state(&instance->resampling_instance) == RESAMPLING_WAIT_QUEUE_FULL) {
        if (instance->avg_val >= (instance->normal_queue_size - DECIMAL_FACTOR)) {
            instance->resampling_instance.status = RESAMPLING_IDLE;
            instance->wait_for_queue_full = false;
        }
    } else if (resample_get_state(&instance->resampling_instance) == RESAMPLING_IDLE) {
        /* Verify if queue is increasing or depleting */
        if (instance->wait_for_queue_full) {
            instance->resampling_instance.status = RESAMPLING_WAIT_QUEUE_FULL;
        } else {
            if (instance->count > instance->queue_avg_size) {
                if (instance->avg_val > (instance->normal_queue_size + instance->max_queue_offset)) {
                    resampling_start(&instance->resampling_instance, RESAMPLING_REMOVE_SAMPLE);
                    instance->count = 0;
                } else if (instance->avg_val < (instance->normal_queue_size - instance->max_queue_offset)) {
                    resampling_start(&instance->resampling_instance, RESAMPLING_ADD_SAMPLE);
                    instance->count = 0;
                }
            } else {
                /* Give time to the avg to stabilize before checking */
                instance->count++;
            }
        }
    }
}

static uint16_t cdc_compensate(sac_cdc_instance_t *instance, queue_node_t *in_node, queue_node_t *out_node)
{
    uint16_t sample_count = 0;

    sample_count = sac_node_get_payload_size(in_node) / instance->size_of_buffer_type;

    /* Resample and save buffer_size in output node header */
    sample_count = resample(&instance->resampling_instance,
                            (void *)sac_node_get_data(in_node),
                            (void *)sac_node_get_data(out_node),
                            sample_count);
    sac_node_set_payload_size(out_node, sample_count * instance->size_of_buffer_type);

    return sac_node_get_payload_size(out_node);
}
