/** @file link_tdma_sync.c
 *  @brief TDMA sync module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <string.h>
#include "link_utils.h"
#include "link_tdma_sync.h"

/* CONSTANTS ******************************************************************/
#define CCA_THRESHOLD_WATCHDOG_COUNT   3000

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline void sync_update(tdma_sync_t *tdma_sync,
                               uint32_t     duration_pll_cycles,
                               link_cca_t  *cca);
static inline void slave_adjust_frame_rx(tdma_sync_t *tdma_sync,
                                         uint16_t     rx_waited_pll_cycles,
                                         link_cca_t  *cca,
                                         uint8_t      rx_cca_retry_count);
static inline void slave_adjust_frame_lost(tdma_sync_t *tdma_sync);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_tdma_sync_init(tdma_sync_t *tdma_sync,
                         sleep_lvl_t sleep_mode,
                         uint16_t setup_time_pll_cycles,
                         uint16_t frame_lost_max_count,
                         uint8_t sync_word_size_bits,
                         uint16_t preamble_size_bits,
                         uint8_t pll_startup_xtal_cycles,
                         isi_mitig_t isi_mitig,
                         uint8_t isi_mitig_pauses,
                         bool fast_sync_enable)
{
    (void)isi_mitig;
    (void)isi_mitig_pauses;
    (void)fast_sync_enable;

    memset(tdma_sync, 0, sizeof(tdma_sync_t));
    tdma_sync->sleep_mode                       = sleep_mode;
    tdma_sync->timeout_pll_cycles               = 2 * setup_time_pll_cycles + preamble_size_bits + sync_word_size_bits;
    tdma_sync->setup_time_pll_cycles            = setup_time_pll_cycles;
    tdma_sync->base_target_rx_waited_pll_cycles = setup_time_pll_cycles +
                                                  preamble_size_bits +
                                                  sync_word_size_bits;
    tdma_sync->frame_lost_max_count             = frame_lost_max_count;
    tdma_sync->slave_sync_state                 = STATE_SYNCING;
    tdma_sync->cca_unsync_watchdog_count        = 0;

    switch (tdma_sync->sleep_mode) {
    case SLEEP_IDLE:
        tdma_sync->sleep_offset_pll_cycles = 1;
        break;
    case SLEEP_SHALLOW:
        tdma_sync->sleep_offset_pll_cycles = PLL_RATIO;
        break;
    case SLEEP_DEEP:
        tdma_sync->sleep_offset_pll_cycles = (PLL_RATIO + ((pll_startup_xtal_cycles + 2) * PLL_RATIO));
        break;
    default:
        break;
    }
}

void link_tdma_sync_update_tx(tdma_sync_t *tdma_sync,
                              uint32_t     duration_pll_cycles,
                              link_cca_t  *cca)
{
    duration_pll_cycles += tdma_sync->sync_slave_offset;

    if (tdma_sync->previous_frame_type == FRAME_RX) {
        duration_pll_cycles += tdma_sync->setup_time_pll_cycles;
    }
    tdma_sync->previous_frame_type = FRAME_TX;

    sync_update(tdma_sync, duration_pll_cycles, cca);
    tdma_sync->sync_slave_offset = 0;
}

void link_tdma_sync_update_rx(tdma_sync_t *tdma_sync,
                              uint32_t     duration_pll_cycles,
                              link_cca_t  *cca)
{
    duration_pll_cycles += tdma_sync->sync_slave_offset;

    if (tdma_sync->previous_frame_type == FRAME_TX) {
        duration_pll_cycles -= tdma_sync->setup_time_pll_cycles;
    }
    tdma_sync->previous_frame_type = FRAME_RX;

    sync_update(tdma_sync, duration_pll_cycles, cca);
    tdma_sync->sync_slave_offset = 0;
}

void link_tdma_sync_slave_adjust(tdma_sync_t    *tdma_sync,
                                 frame_outcome_t frame_outcome,
                                 uint16_t        rx_waited_pll_cycles,
                                 link_cca_t     *cca,
                                 uint8_t         rx_cca_retry_count)
{
    (void)rx_cca_retry_count;

    if (frame_outcome == FRAME_RECEIVED) {
        slave_adjust_frame_rx(tdma_sync, rx_waited_pll_cycles, cca, 0);
    } else {
        slave_adjust_frame_lost(tdma_sync);
    }
}

void link_tdma_sync_slave_find(tdma_sync_t    *tdma_sync,
                               frame_outcome_t frame_outcome,
                               uint16_t        rx_waited_pll_cycles,
                               link_cca_t     *cca,
                               uint8_t         rx_cca_retry_count)
{
    (void)rx_cca_retry_count;

    if (frame_outcome == FRAME_RECEIVED) {
        slave_adjust_frame_rx(tdma_sync, rx_waited_pll_cycles, cca, 0);
    } else {
        tdma_sync->sync_slave_offset = -UNSYNC_OFFSET_PLL_CYCLES;
    }
}

uint32_t link_tdma_sync_get_sleep_cycles(tdma_sync_t *tdma_sync)
{
    return tdma_sync->sleep_cycles_value;
}

uint32_t link_tdma_sync_get_timeout(tdma_sync_t *tdma_sync)
{
    return tdma_sync->timeout_value;
}

uint16_t link_tdma_sync_get_pwr_up(tdma_sync_t *tdma_sync)
{
    return tdma_sync->pwr_up_value;
}

bool link_tdma_sync_is_slave_synced(tdma_sync_t *tdma_sync)
{
    return tdma_sync->slave_sync_state;
}

uint8_t link_tdma_sync_get_isi_mitigation_pauses(isi_mitig_t isi_mitig_reg_val)
{
    (void)isi_mitig_reg_val;

    return 0;
}

uint32_t link_tdma_get_preamble_length(uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val)
{
    (void)isi_mitig_pauses;

    return preamble_len_reg_val;
}

uint32_t link_tdma_get_syncword_length(uint8_t isi_mitig_pauses, syncword_length_t syncword_len_reg_val)
{
    (void)isi_mitig_pauses;

    switch (syncword_len_reg_val) {
    case SYNCWORD_LENGTH_16:
        return 16;
    default:
        return 32;
    }
}
/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Update TDMA sync module.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @return None.
 */
static inline void sync_update(tdma_sync_t *tdma_sync,
                               uint32_t     duration_pll_cycles,
                               link_cca_t  *cca)
{
    uint32_t timeout_pll_cycles;

    if (cca->enable) {
        timeout_pll_cycles = tdma_sync->timeout_pll_cycles + cca->max_try_count * cca->retry_time_pll_cycles;
    } else {
        timeout_pll_cycles = tdma_sync->timeout_pll_cycles;
    }

    switch (tdma_sync->sleep_mode) {
    case SLEEP_SHALLOW:
    case SLEEP_DEEP:
        duration_pll_cycles -= tdma_sync->sleep_offset_pll_cycles;
        tdma_sync->sleep_cycles_value = duration_pll_cycles / PLL_RATIO;
        tdma_sync->pwr_up_value += duration_pll_cycles % PLL_RATIO;
        if (tdma_sync->pwr_up_value > PLL_RATIO) {
            tdma_sync->sleep_cycles_value++;
            tdma_sync->pwr_up_value = tdma_sync->pwr_up_value % PLL_RATIO;
        }
        tdma_sync->timeout_value = timeout_pll_cycles + tdma_sync->pwr_up_value;
        break;
    case SLEEP_IDLE:
    default:
        tdma_sync->sleep_cycles_value = duration_pll_cycles - tdma_sync->sleep_offset_pll_cycles;
        tdma_sync->pwr_up_value       = 0;
        tdma_sync->timeout_value      = timeout_pll_cycles;
        break;
    }
}

/** @brief Update Adjust slave sync when frame is received.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] rx_waited_pll_cycles RX waited value in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] rx_cca_retry_count   RX CCA retry count.
 *  @return None.
 */
static inline void slave_adjust_frame_rx(tdma_sync_t *tdma_sync,
                                         uint16_t     rx_waited_pll_cycles,
                                         link_cca_t   *cca,
                                         uint8_t      rx_cca_retry_count)
{
    uint16_t target_rx_waited_pll_cycles = tdma_sync->base_target_rx_waited_pll_cycles;

    (void)rx_cca_retry_count;

    if (tdma_sync->sleep_mode != SLEEP_IDLE) {
        rx_waited_pll_cycles -= tdma_sync->pwr_up_value;
    }

    tdma_sync->frame_lost_count = 0;
    /* Determine by how long the frame transmission was delayed due to CCA failure */
    if (rx_waited_pll_cycles > (tdma_sync->base_target_rx_waited_pll_cycles +
                                cca->retry_time_pll_cycles -
                               (cca->retry_time_pll_cycles / 2)) && cca->enable) {
        tdma_sync->cca_unsync_watchdog_count++;
        for (int8_t i = cca->max_try_count; i >= 0; i--) {
            if ((rx_waited_pll_cycles < (tdma_sync->base_target_rx_waited_pll_cycles +
                                        (cca->retry_time_pll_cycles * (i + 1))) -
                                         cca->retry_time_pll_cycles / 2) &&
                (rx_waited_pll_cycles > (tdma_sync->base_target_rx_waited_pll_cycles +
                                        (cca->retry_time_pll_cycles * i)) -
                                         cca->retry_time_pll_cycles / 2)) {
                target_rx_waited_pll_cycles =  tdma_sync->base_target_rx_waited_pll_cycles + (cca->retry_time_pll_cycles * i);
                break;
            }
        }
    }
    if (target_rx_waited_pll_cycles == tdma_sync->base_target_rx_waited_pll_cycles) {
        tdma_sync->slave_sync_state = STATE_SYNCED;
        tdma_sync->cca_unsync_watchdog_count = 0;
    }
    /* Slave woke up too early */
    if (rx_waited_pll_cycles > target_rx_waited_pll_cycles) {
        tdma_sync->sync_slave_offset = (rx_waited_pll_cycles - target_rx_waited_pll_cycles);
    /* Slave woke up too late */
    } else if (rx_waited_pll_cycles < target_rx_waited_pll_cycles) {
        tdma_sync->sync_slave_offset = -(target_rx_waited_pll_cycles - rx_waited_pll_cycles);
    /* Slave woke up in time */
    } else {
        tdma_sync->sync_slave_offset = 0;
    }

    if ((tdma_sync->cca_unsync_watchdog_count > CCA_THRESHOLD_WATCHDOG_COUNT) || (tdma_sync->slave_sync_state == STATE_SYNCING)) {
        tdma_sync->sync_slave_offset = rx_waited_pll_cycles - tdma_sync->base_target_rx_waited_pll_cycles;
    }
}

/** @brief Update Adjust slave sync when frame is lost.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return None.
 */
static inline void slave_adjust_frame_lost(tdma_sync_t *tdma_sync)
{
    tdma_sync->frame_lost_count++;
    tdma_sync->sync_slave_offset = 0;
    if (tdma_sync->frame_lost_count >= tdma_sync->frame_lost_max_count) {
        tdma_sync->slave_sync_state = STATE_SYNCING;
        tdma_sync->frame_lost_count = tdma_sync->frame_lost_max_count;
    }
}
