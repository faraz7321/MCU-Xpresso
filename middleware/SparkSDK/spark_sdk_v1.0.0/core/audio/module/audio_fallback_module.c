/** @file  audio_fallback_module.c
 *  @brief Audio Fallback Module is used to manage audio fallback.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_fallback_module.h"
#include <stddef.h>
#include "sac_api.h"

/* CONSTANTS ******************************************************************/
#define TXQ_LEN_HIGH_THRESHOLD 13 /* Threshold (x10) of tx queue len which triggers a switch to fallback */
#define TXQ_DECIMAL_FACTOR     10 /* Decimal factor used for tx queue length calculation */
#define LINK_MARGIN_COUNT_MAX  12 /* Max count of link margin average value readings, each count representing ~1/4 sec */

/* PRIVATE GLOBALS ************************************************************/
static sac_fallback_module_t *audio_fallback_module;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void init_txq_stats(void);
static void init_lm_stats(void);
static void update_txq_stats(void);
static void update_lm_stats(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_fallback_module_init(sac_fallback_module_cfg_t cfg, mem_pool_t *mem_pool, sac_error_t *audio_error)
{
    *audio_error = SAC_ERR_NONE;

    audio_fallback_module = (sac_fallback_module_t *)mem_pool_malloc(mem_pool, sizeof(sac_fallback_module_t));
    if (audio_fallback_module == NULL) {
        *audio_error = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }

    audio_fallback_module->fallback_flag = true; /* Start system in fallback mode */
    audio_fallback_module->fallback_threshold_default = cfg.link_margin_threshold;
    audio_fallback_module->fallback_threshold = cfg.link_margin_threshold;
    audio_fallback_module->fallback_threshold_hysteresis = cfg.link_margin_threshold_hysteresis;
    audio_fallback_module->pipeline = cfg.pipeline;
    audio_fallback_module->txq_max_size = cfg.txq_max_size * TXQ_DECIMAL_FACTOR;
    audio_fallback_module->fallback_packets_per_250ms = cfg.audio_packets_per_second / 4;
    audio_fallback_module->fallback_state = FB_STATE_FALLBACK_DISCONNECT;
    audio_fallback_module->fallback_count = 0;
    init_txq_stats();
    init_lm_stats();
}

bool fallback_gate_is_fallback_on(void *instance, sac_header_t *header, uint8_t *data_in, uint16_t size)
{
    (void)instance;
    (void)data_in;
    (void)size;

    if (audio_fallback_module->fallback_flag) {
        header->fallback = 1;
    } else {
        header->fallback = 0;
    }
    return audio_fallback_module->fallback_flag;
}

bool fallback_gate_is_fallback_off(void *instance, sac_header_t *header, uint8_t *data_in, uint16_t size)
{
    (void)instance;
    (void)header;
    (void)data_in;
    (void)size;

    return !audio_fallback_module->fallback_flag;
}

bool fallback_gate_fallback_detect(void *instance, sac_header_t *header, uint8_t *data_in, uint16_t size)
{
    bool ret;
    (void)instance;
    (void)data_in;
    (void)size;

    ret = (header->fallback == 1);
    if (ret) {
        fallback_set_fallback_flag();
    } else {
        fallback_clear_fallback_flag();
    }
    return ret;
}

void fallback_set_rx_link_margin(int16_t rx_lm)
{
    audio_fallback_module->rx_link_margin_data_rx = rx_lm;
    audio_fallback_module->rx_link_margin_data_valid = true;
}

void fallback_update_state(void)
{
    sac_link_margin_metrics_t *link_margin_metrics = &audio_fallback_module->link_margin_metrics;
    sac_txq_metrics_t *txq_metrics = &audio_fallback_module->txq_metrics;

    switch (audio_fallback_module->fallback_state) {
    case FB_STATE_NORMAL:
        if (txq_metrics->txq_len_avg == audio_fallback_module->txq_max_size) {
            /* TX Queue is full => Link is disconnected */
            audio_fallback_module->fallback_threshold = audio_fallback_module->fallback_threshold_default;
            init_lm_stats(); /* Clear the lm stats since they are from non-fallback mode */
            fallback_set_fallback_flag();
            audio_fallback_module->fallback_state = FB_STATE_FALLBACK_DISCONNECT;
        } else if ((txq_metrics->txq_len_avg > TXQ_LEN_HIGH_THRESHOLD) && !audio_fallback_module->fallback_flag) {
            init_lm_stats(); /* Clear the lm stats since they are from non-fallback mode */
            fallback_set_fallback_flag();
            audio_fallback_module->fallback_state = FB_STATE_WAIT_THRESHOLD;
        }
        break;
    case FB_STATE_WAIT_THRESHOLD:
        /* State entered due to a degrading link, waiting to measure return to normal threshold */
        if (txq_metrics->txq_len_avg == audio_fallback_module->txq_max_size) {
            /* TX Queue is full => Link is disconnected */
            audio_fallback_module->fallback_threshold = audio_fallback_module->fallback_threshold_default;
            audio_fallback_module->fallback_state = FB_STATE_FALLBACK_DISCONNECT;
        } else if (link_margin_metrics->link_margin_acc_avg > 0) {
            /* Averaging is complete. Use this value as a threshold to return to normal */
            audio_fallback_module->fallback_threshold = link_margin_metrics->link_margin_acc_avg;
            /* Make sure threshold is reasonable */
            if ((audio_fallback_module->fallback_threshold >
                (audio_fallback_module->fallback_threshold_default + audio_fallback_module->fallback_threshold_hysteresis)) ||
                (audio_fallback_module->fallback_threshold <
                (audio_fallback_module->fallback_threshold_default - audio_fallback_module->fallback_threshold_hysteresis))) {
                    audio_fallback_module->fallback_threshold = audio_fallback_module->fallback_threshold_default;
            }
            audio_fallback_module->fallback_state = FB_STATE_FALLBACK;
        }
        break;
    case FB_STATE_FALLBACK:
        /* State entered due to a degraded link */
        if (txq_metrics->txq_len_avg == audio_fallback_module->txq_max_size) {
            /* TX Queue is full => Link is disconnected */
            audio_fallback_module->fallback_threshold = audio_fallback_module->fallback_threshold_default;
            audio_fallback_module->fallback_state = FB_STATE_FALLBACK_DISCONNECT;
        } else if (link_margin_metrics->link_margin_count >= LINK_MARGIN_COUNT_MAX) {
            /* LM is continuously above threshold for 3 seconds, switch to normal */
            fallback_clear_fallback_flag();
            audio_fallback_module->fallback_state = FB_STATE_NORMAL;
        }
        break;
    case FB_STATE_FALLBACK_DISCONNECT:
        /* State entered due to a disconnected link */
        if (link_margin_metrics->link_margin_count >= LINK_MARGIN_COUNT_MAX) {
            /* LM is continuously above threshold for 3 seconds, switch to normal */
            fallback_clear_fallback_flag();
            audio_fallback_module->fallback_state = FB_STATE_NORMAL;
        }
        break;
    }
}

void fallback_update_stats(void)
{
    update_txq_stats();
    update_lm_stats();
}

bool fallback_is_active(void)
{
    return audio_fallback_module->fallback_flag;
}

void fallback_set_fallback_flag(void)
{
    audio_fallback_module->fallback_flag = true;
    audio_fallback_module->fallback_count++;
}

void fallback_clear_fallback_flag(void)
{
    audio_fallback_module->fallback_flag = false;
}

uint32_t fallback_get_fallback_count(void)
{
    return audio_fallback_module->fallback_count;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Clear the TXQ stats.
 */
static void init_txq_stats(void)
{
    memset(&audio_fallback_module->txq_metrics, 0, sizeof(audio_fallback_module->txq_metrics));
}

/** @brief Clear the link margin stats.
 */
static void init_lm_stats(void)
{
    memset(&audio_fallback_module->link_margin_metrics, 0, sizeof(audio_fallback_module->link_margin_metrics));
}

/** @brief Update the TXQ stats.
 *
 *  Call inside the main background loop.
 */
static void update_txq_stats(void)
{
    sac_txq_metrics_t *txq_metrics = &audio_fallback_module->txq_metrics;

    if (audio_fallback_module->pipeline->consumer->_buffering_complete) {
        txq_metrics->txq_len_sum -= txq_metrics->txq_len_arr[txq_metrics->txq_len_arr_idx];
        txq_metrics->txq_len_arr[txq_metrics->txq_len_arr_idx] = queue_get_length(audio_fallback_module->pipeline->consumer->_queue);
        txq_metrics->txq_len_sum += txq_metrics->txq_len_arr[txq_metrics->txq_len_arr_idx++];
        txq_metrics->txq_len_avg = (txq_metrics->txq_len_sum * TXQ_DECIMAL_FACTOR) / SAC_TXQ_ARR_LEN;
        txq_metrics->txq_len_arr_idx %= SAC_TXQ_ARR_LEN;
    }
}

/** @brief Update the link margin stats.
 *
 *  Call inside the main background loop.
 */
static void update_lm_stats(void)
{
    sac_link_margin_metrics_t *link_margin_metrics = &audio_fallback_module->link_margin_metrics;

    if (audio_fallback_module->rx_link_margin_data_valid) {
        link_margin_metrics->link_margin_accumulator += audio_fallback_module->rx_link_margin_data_rx;
        audio_fallback_module->rx_link_margin_data_valid = false; /* Rx'ed lm from node */
        if (++link_margin_metrics->link_margin_accumulator_count >= audio_fallback_module->fallback_packets_per_250ms) {
            /* Calculate 250ms average */
            link_margin_metrics->link_margin_acc_avg =
                        link_margin_metrics->link_margin_accumulator / link_margin_metrics->link_margin_accumulator_count;
            link_margin_metrics->link_margin_accumulator = 0;
            link_margin_metrics->link_margin_accumulator_count = 0;
            if (link_margin_metrics->link_margin_acc_avg >= (audio_fallback_module->fallback_threshold + audio_fallback_module->fallback_threshold_hysteresis)
                                       && audio_fallback_module->fallback_flag) {
                /* Average above threshold, inc the 3 second count */
                link_margin_metrics->link_margin_count = (link_margin_metrics->link_margin_count + 1) < LINK_MARGIN_COUNT_MAX ?
                                                                (link_margin_metrics->link_margin_count + 1) : LINK_MARGIN_COUNT_MAX;
            } else {
                link_margin_metrics->link_margin_count = 0; /* Below average, restart count */
            }
        }
    }
}

