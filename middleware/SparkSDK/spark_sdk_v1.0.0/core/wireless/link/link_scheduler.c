/** @file link_scheduler.c
 *  @brief Scheduler module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "link_scheduler.h"
#include "link_utils.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline bool time_slot_is_empty(scheduler_t *scheduler, timeslot_t *time_slot);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_scheduler_init(scheduler_t *scheduler,
                         schedule_t  *schedule,
                         uint16_t     local_addr)
{
    memset(scheduler, 0, sizeof(scheduler_t));
    scheduler->total_time_slot_count = schedule->size;
    scheduler->schedule = schedule;
    scheduler->local_addr = local_addr;
}

void link_scheduler_reset(scheduler_t *scheduler)
{
    scheduler->current_time_slot_num = 0;
    scheduler->total_time_slot_count = 0;
    scheduler->sleep_cycles = 0;
    scheduler->tx_disabled = false;

    for (uint8_t i = 0; i < scheduler->schedule->size; i++) {
        memset(&scheduler->schedule->timeslot[i], 0, sizeof(timeslot_t));
    }
}

uint8_t link_scheduler_increment_time_slot(scheduler_t *scheduler)
{
    uint8_t i = scheduler->current_time_slot_num;
    uint8_t inc_count = 0;

    scheduler->timeslot_mismatch = false;

    if (scheduler->total_time_slot_count != 0) {
        timeslot_t *next_time_slot = &scheduler->schedule->timeslot[(i + 1) % scheduler->total_time_slot_count];

        if (time_slot_is_empty(scheduler, next_time_slot)) {
            scheduler->sleep_cycles += scheduler->schedule->timeslot[i].duration_pll_cycles;
            i = (i + 1) % scheduler->total_time_slot_count;
            inc_count++;
            while (time_slot_is_empty(scheduler, &scheduler->schedule->timeslot[i])) {
                scheduler->sleep_cycles += scheduler->schedule->timeslot[i].duration_pll_cycles;
                i = (i + 1) % scheduler->total_time_slot_count;
                inc_count++;
            };
        } else {
            scheduler->sleep_cycles += scheduler->schedule->timeslot[i].duration_pll_cycles;
            i = (i + 1) % scheduler->total_time_slot_count;
            inc_count++;
        }

        scheduler->current_time_slot_num = i;
    }
    return inc_count;
}

void link_scheduler_set_time_slot_i(scheduler_t *scheduler, uint8_t time_slot_i)
{
    scheduler->current_time_slot_num = time_slot_i;
}

void link_scheduler_enable_tx(scheduler_t *scheduler)
{
    scheduler->tx_disabled = false;
}

void link_scheduler_disable_tx(scheduler_t *scheduler)
{
    scheduler->tx_disabled = true;
}

timeslot_t *link_scheduler_get_current_timeslot(scheduler_t *scheduler)
{
    return &scheduler->schedule->timeslot[scheduler->current_time_slot_num];
}

uint8_t link_scheduler_get_total_timeslot_count(scheduler_t *scheduler)
{
    return scheduler->total_time_slot_count;
}

uint8_t link_scheduler_get_next_timeslot_index(scheduler_t *scheduler)
{
    return scheduler->current_time_slot_num;
}

uint32_t link_scheduler_get_sleep_time(scheduler_t *scheduler)
{
    return scheduler->sleep_cycles;
}

void link_scheduler_set_first_time_slot(scheduler_t *scheduler)
{
    if (scheduler->schedule->size > 1) {
        link_scheduler_set_time_slot_i(scheduler, scheduler->schedule->size - 1);
    }
}

void link_scheduler_reset_sleep_time(scheduler_t *scheduler)
{
    scheduler->sleep_cycles = 0;
}

void link_scheduler_set_mismatch(scheduler_t *scheduler)
{
    scheduler->timeslot_mismatch = true;
}

bool link_scheduler_get_mismatch(scheduler_t *scheduler)
{
    return scheduler->timeslot_mismatch;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get time slot empty flag.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @param[in]  time_slot  Time slot.
 */
static inline bool time_slot_is_empty(scheduler_t *scheduler, timeslot_t *time_slot)
{
    if (time_slot->connection_main == NULL) {
        return true;
    } else if ((scheduler->tx_disabled) &&
        (time_slot->connection_main->source_address == scheduler->local_addr)) {
        return true;
    } else {
        return false;
    }
}
