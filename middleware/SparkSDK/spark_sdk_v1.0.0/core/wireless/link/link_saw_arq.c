/** @file link_saw_arq.c
 *  @brief Stop and Wait ARQ module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_saw_arq.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void link_saw_arq_init(saw_arq_t *saw_arq, uint16_t ttl_ms, uint16_t ttl_retries, bool init_board_seq, bool enable)
{
    saw_arq->ttl_ms          = ttl_ms;
    saw_arq->ttl_retries      = ttl_retries;
    saw_arq->seq_num         = init_board_seq;
    saw_arq->duplicate       = false;
    saw_arq->duplicate_count = 0;
    saw_arq->retry_count     = 0;
    saw_arq->enable          = enable;
}

bool link_saw_arq_is_frame_timeout(saw_arq_t *saw_arq, uint64_t time_stamp, uint16_t retry_count, uint64_t current_time)
{
    bool time_timeout;
    bool retries_timeout;
    bool timeout;
    uint16_t delta_t = current_time - time_stamp;

    if (!saw_arq->enable) {
        return true;
    }

    if (saw_arq->ttl_ms == 0) {
        time_timeout = false;
    } else {
        time_timeout = (delta_t >= saw_arq->ttl_ms);
    }

    if (saw_arq->ttl_retries == 0) {
        retries_timeout = false;
    } else {
        retries_timeout = (retry_count >= saw_arq->ttl_retries);
    }

    timeout = (time_timeout || retries_timeout);

    if ((retry_count > 0) && (timeout == false)) {
        saw_arq->retry_count++;
    }

    return timeout;
}

bool link_saw_arq_get_seq_num(saw_arq_t *saw_arq)
{
    return saw_arq->seq_num;
}

void link_saw_arq_inc_seq_num(saw_arq_t *saw_arq)
{
    saw_arq->seq_num = !saw_arq->seq_num;
}

void link_saw_arq_update_rx_seq_num(saw_arq_t *saw_arq, bool seq_num)
{
    saw_arq->duplicate = (seq_num == saw_arq->seq_num);
    saw_arq->seq_num = seq_num;
}

bool link_saw_arq_is_rx_frame_duplicate(saw_arq_t *saw_arq)
{
    if (!saw_arq->enable) {
        return false;
    }

    if (saw_arq->duplicate) {
        saw_arq->duplicate_count++;
    }

    return saw_arq->duplicate;
}

uint32_t link_saw_arq_get_duplicate_count(saw_arq_t *saw_arq)
{
    return saw_arq->duplicate_count;
}

uint32_t link_saw_arq_get_retry_count(saw_arq_t *saw_arq)
{
    return saw_arq->retry_count;
}

void link_saw_arq_reset_stats(saw_arq_t *saw_arq)
{
    saw_arq->retry_count     = 0;
    saw_arq->duplicate_count = 0;
}
