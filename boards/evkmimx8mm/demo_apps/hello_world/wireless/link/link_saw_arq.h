/** @file link_saw_arq.h
 *  @brief Stop and wait ARQ module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_SAW_ARQ_H_
#define LINK_SAW_ARQ_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Stop and Wait ARQ
 */
typedef struct saw_arq {
    uint64_t ttl_ms;          /**< Time to live in ms */
    uint16_t ttl_retries;     /**< Time to live in amount of retries */
    bool     seq_num;         /**< 1 bit sequence number */
    bool     duplicate;       /**< Duplicate flag */
    uint32_t duplicate_count; /**< Duplicate count */
    uint32_t retry_count;     /**< Retry count */
    bool     enable;          /**< Module enable flag */
} saw_arq_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize SAW ARQ Object.
 *
 *  @param[in] saw_arq      SAW ARQ Object.
 *  @param[in] ttl_ms       Time to live in ms.
 *  @param[in] ttl_retries  Time to live in amount of retries.
 *  @param[in] board_seq    Board sequence based on source/dest address for sequence number init.
 *  @param[in] enable       Enable flag.
 */
void link_saw_arq_init(saw_arq_t *saw_arq, uint16_t ttl_ms, uint16_t ttl_retries, bool init_board_seq, bool enable);

/** @brief Is the current frame timeout.
 *
 *  @param[in] saw_arq     SAW ARQ Object.
 *  @param[in] frame       The frame.
 *  @param[in] time_stamp  Time stamp.
 */
bool link_saw_arq_is_frame_timeout(saw_arq_t *saw_arq, uint64_t time_stamp, uint16_t retry_count, uint64_t current_time);

/** @brief Get sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 *  @return sequence number.
 */
bool link_saw_arq_get_seq_num(saw_arq_t *saw_arq);

/** @brief Increment sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
void link_saw_arq_inc_seq_num(saw_arq_t *saw_arq);

/** @brief Update RX sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 *  @param[in] seq_num  Sequence number.
 */
void link_saw_arq_update_rx_seq_num(saw_arq_t *saw_arq, bool seq_num);

/** @brief Is the received frame a duplicate.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
bool link_saw_arq_is_rx_frame_duplicate(saw_arq_t *saw_arq);

/** @brief Get duplicate count.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
uint32_t link_saw_arq_get_duplicate_count(saw_arq_t *saw_arq);

/** @brief Get total number of retries.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
uint32_t link_saw_arq_get_retry_count(saw_arq_t *saw_arq);

/** @brief Reset the SaW ARQ object stats.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
void link_saw_arq_reset_stats(saw_arq_t *saw_arq);

#ifdef __cplusplus
}
#endif
#endif /* LINK_CHANNEL_HOPPING_H_ */
