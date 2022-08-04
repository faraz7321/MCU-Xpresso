/** @file link_scheduler.h
 *  @brief Scheduler module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_SCHEDULER_H_
#define LINK_SCHEDULER_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "wps_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Timeslot instance.
 */
typedef struct timeslot {
    wps_connection_t* connection_main;       /**< Main connection instance. */
    wps_connection_t* connection_auto_reply; /**< Auto-reply connection instance. */
    uint32_t          duration_pll_cycles;   /**< Timeslot duration, in PLL cycles. */
} timeslot_t;

/** @brief Schedule instance.
 */
typedef struct schedule {
    timeslot_t *timeslot; /**< Array containing every schedule timeslot. */
    uint32_t    size;     /**< Number of timeslot for the schedule. */
} schedule_t;

/** @brief Time slot type enumeration.
 */
typedef struct time_slot_cfg {
    uint32_t   duration_us; /**< Time slot duration in us */
    timeslot_t time_slot;   /**< Time slot */
} time_slot_cfg_t;

typedef struct scheduler {
    schedule_t *schedule;              /**< The schedule */
    uint8_t     current_time_slot_num; /**< Current time slot number */
    uint8_t     total_time_slot_count; /**< The total amount of time slots configured */
    uint32_t    sleep_cycles;          /**< Sleep time in PLL cycles */
    uint16_t    local_addr;            /**< Local Address. */
    bool        tx_disabled;           /**< TX disabled flag. */
    bool        timeslot_mismatch;     /**< Timeslot mismatch index flag. */
} scheduler_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize scheduler object.
 *
 *  @param[in] scheduler  Scheduler object.
 *  @param[in] schedule   Schedule.
 *  @param[in] local_addr Local address.
 *  @return None.
 */
void link_scheduler_init(scheduler_t *scheduler,
                         schedule_t  *schedule,
                         uint16_t     local_addr);

/** @brief Reset scheduler object.
 *
 *  @param[in] scheduler  Scheduler object.
 */
void link_scheduler_reset(scheduler_t *scheduler);

/** @brief Add a time slot to schedule.
 *
 *  @note Sleep cycle is computed based on number of
 *        timeslot increment. This function do not reset
 *        the sleep cycle. When computing a new time, user
 *        should always call link_scheduler_reset_sleep_time
 *        in order to reset the sleep_cycle computation.
 *
 *  @param[in] scheduler  Scheduler object.
 *  @return The number of timeslot incremented in the schedule.
 */
uint8_t link_scheduler_increment_time_slot(scheduler_t *scheduler);

/** @brief Set current time slot index.
 *
 *  @param[in] scheduler    Scheduler object.
 *  @param[in] time_slot_i  Time slot index.
 */
void link_scheduler_set_time_slot_i(scheduler_t *scheduler,
                                    uint8_t      time_slot_i);

/** @brief Enable transmissions.
 *
 *  @return The current time slot.
 */
void link_scheduler_enable_tx(scheduler_t *scheduler);

/** @brief Disable transmissions.
 *
 *  @param[in]  scheduler  Scheduler object.
 */
void link_scheduler_disable_tx(scheduler_t *scheduler);

/** @brief Get the current time slot.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The current time slot.
 */
timeslot_t *link_scheduler_get_current_timeslot(scheduler_t *scheduler);

/** @brief Get the total number of time slots.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The total number of time slots.
 */
uint8_t link_scheduler_get_total_timeslot_count(scheduler_t *scheduler);

/** @brief Get the current time slot index.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The current time slot index.
 */
uint8_t link_scheduler_get_next_timeslot_index(scheduler_t *scheduler);

/** @brief Get sleep the amount of time to sleep in PLL cycles.
 *
 *  @param[in]   scheduler  Scheduler object.
 *  @return time to sleep in PLL cycles.
 */
uint32_t link_scheduler_get_sleep_time(scheduler_t *scheduler);

/** @brief Start at the end of the schedule so the first effective time slot is the first one.
 *
 *  @param[in]   scheduler  Scheduler object.
 *  @return time to sleep in PLL cycles.
 */
void link_scheduler_set_first_time_slot(scheduler_t *scheduler);

/** @brief Reset the scheduler compute sleep time.
 *
 *  @note This should be called before incrementing the
 *        timeslot in order to reset the sleep_cycle computation.
 *
 *  @param[in] scheduler  Scheduler object.
 */
void link_scheduler_reset_sleep_time(scheduler_t *scheduler);

/** @brief  Set mismatch schedule index to true;
 *
 *  @param[in] scheduler  Scheduler object.
 */
void link_scheduler_set_mismatch(scheduler_t *scheduler);

/** @brief  Get the mismatch schedule index flag;
 *
 *  @param[in] scheduler  Scheduler object.
 */
bool link_scheduler_get_mismatch(scheduler_t *scheduler);

#ifdef __cplusplus
}
#endif
#endif /* LINK_SCHEDULER_H_ */
