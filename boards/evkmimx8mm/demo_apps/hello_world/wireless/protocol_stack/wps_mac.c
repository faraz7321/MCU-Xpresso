/** @file wps_mac.c
 *  @brief Wireless protocol stack MAC.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"
#include "wps_callback.h"
#ifdef SPARK_WPS_CFG_FILE_EXISTS
#include "spark_wps_cfg.h"
#endif

/* CONSTANTS ******************************************************************/
#define RADIO_MAX_PACKET_SIZE               255
#define SYNC_PLL_STARTUP_CYCLES   (uint32_t)0x60
#define SYNC_RX_SETUP_PLL_CYCLES  (uint32_t)147
#define SYNC_FRAME_LOST_MAX_COUNT (uint32_t)100
#define HEADER_BYTE0_SEQ_NUM_MASK           BIT(7)
#define HEADER_BYTE0_TIME_SLOT_ID_MASK      BITS8(6, 0)
#define MULTI_RADIO_BASE_IDX                0

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void state_link_quality(void *signal_data);
static void state_sync(void *signal_data);
static void state_post_rx(void *signal_data);
static void state_post_tx(void *signal_data);
static void state_stop_wait_arq(void *signal_data);
static void state_mac_prepare_frame(void *signal_data);
static void state_setup_primary_link(void *signal_data);
static void state_setup_ack_link(void *signal_data);
static void state_setup_prime_link(void *signal_data);
static void state_scheduler(void *signal_data);
static void end(void *signal_data);

static bool outcome_is_frame_received(wps_mac_t *wps_mac);
static bool outcome_is_tx_sent_ack(wps_mac_t *wps_mac);
static bool outcome_is_tx_not_sent(wps_mac_t *wps_mac);
static bool is_network_node(wps_mac_t *wps_mac);
static bool is_current_timeslot_tx(wps_mac_t *wps_mac);
static bool is_current_prime_timeslot_tx(wps_mac_t *wps_mac);
static bool is_current_prime_timeslot_rx(wps_mac_t *wps_mac);
static bool is_current_timeslot_prime(wps_mac_t *wps_mac);
static bool is_saw_arq_enable(wps_connection_t *connection);
static bool is_phase_accumulation_enable(wps_mac_t *wps_mac);
static void update_phases_data(wps_mac_phase_data_t *phase_data, uint16_t rx_wait_time);
static bool is_phase_data_valid(wps_mac_phase_data_t *phase_data);
static bool header_space_available(xlayer_t *current_queue);
static bool no_payload_received(xlayer_t *current_queue);
static bool scheduling_needed(wps_mac_t *wps_mac);
static bool no_space_available_for_rx(wps_mac_t *wps_mac);
static void update_xlayer_sync(wps_mac_t *wps_mac, xlayer_t *current_xlayer);
static void update_main_xlayer_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer);
static void update_prime_xlayer_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer);
static void update_xlayer_modem_feat(wps_mac_t *wps_mac, xlayer_t *current_xlayer);
static void extract_header(wps_mac_t *wps_mac, xlayer_t *current_queue);
static void fill_header(wps_mac_t *wps_mac, xlayer_t *current_queue);
static void setup_prime_timeslot_status(wps_mac_t *wps_mac);
static void process_scheduler(wps_mac_t *wps_mac);
static void process_rx_tx_outcome(wps_mac_t *wps_mac);
static void flush_timeout_frames_before_sending(wps_mac_t *wps_mac, wps_connection_t *connection);
static xlayer_t *get_xlayer_for_tx(wps_mac_t *wps_mac, wps_connection_t *connection);
static xlayer_t *get_xlayer_for_rx(wps_mac_t *wps_mac, wps_connection_t *connection);
static bool send_done(wps_connection_t *connection);
static void handle_link_throttle(wps_mac_t *wps_mac, uint8_t *inc_count);
#ifndef WPS_DISABLE_LINK_STATS
static void update_wps_stats(wps_mac_t *MAC);
#endif /* WPS_DISABLE_LINK_STATS */
/* TYPES **********************************************************************/
wps_mac_state rx_frame_sm[]               = {state_link_quality, state_post_rx, state_sync, end};
wps_mac_state rx_frame_miss_sm[]          = {state_link_quality, state_sync, state_post_rx, end};
wps_mac_state tx_sm[]                     = {state_stop_wait_arq, state_post_tx, end};
wps_mac_state prepare_frame_sm[]          = {state_mac_prepare_frame, end};
wps_mac_state setup_link_sm[]             = {state_setup_primary_link, state_setup_ack_link, state_setup_prime_link, end};
wps_mac_state schedule_sm[]               = {state_scheduler, end};
wps_mac_state empty_sm[]                  = {end};

/* PRIVATE GLOBALS ************************************************************/
uint8_t overrun_buffer[RADIO_MAX_PACKET_SIZE];

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_mac_init(wps_mac_t *wps_mac,
                 circular_queue_t *callback_queue,
                 schedule_t *schedule,
                 channel_sequence_t *channel_sequence,
                 wps_mac_sync_cfg_t *sync_cfg,
                 uint16_t local_address,
                 wps_role_t node_role,
                 bool random_channel_sequence_enabled,
                 uint8_t network_id)
{
    wps_mac->state_machine[MAC_SIGNAL_RX_FRAME]               = rx_frame_sm;
    wps_mac->state_machine[MAC_SIGNAL_RX_FRAME_MISS]          = rx_frame_miss_sm;
    wps_mac->state_machine[MAC_SIGNAL_TX_SENT_ACK]            = tx_sm;
    wps_mac->state_machine[MAC_SIGNAL_TX_SENT_NACK]           = tx_sm;
    wps_mac->state_machine[MAC_SIGNAL_TX_NOT_SENT]            = tx_sm;
    wps_mac->state_machine[MAC_SIGNAL_TX]                     = tx_sm;
    wps_mac->state_machine[MAC_SIGNAL_PREPARE_FRAME]          = prepare_frame_sm;
    wps_mac->state_machine[MAC_SIGNAL_SETUP_LINK]             = setup_link_sm;
    wps_mac->state_machine[MAC_SIGNAL_SCHEDULE]               = schedule_sm;
    wps_mac->state_machine[MAC_SIGNAL_EMPTY]                  = empty_sm;

    wps_mac->state_process_idx                = 0;
    wps_mac->current_ts_prime                 = false;
    wps_mac->current_ts_prime_tx              = false;
    wps_mac->local_address                    = local_address;
    wps_mac->node_role                        = node_role;
    wps_mac->callback_queue                   = callback_queue;

    wps_mac->random_channel_sequence_enabled = random_channel_sequence_enabled;
    wps_mac->network_id          = network_id;

    /* Scheduler init */
    link_scheduler_init(&wps_mac->scheduler, schedule, wps_mac->local_address);
    wps_mac->scheduler.total_time_slot_count = wps_mac->scheduler.schedule->size;
    link_scheduler_set_first_time_slot(&wps_mac->scheduler);
    link_scheduler_enable_tx(&wps_mac->scheduler);

    link_channel_hopping_init(&wps_mac->channel_hopping, channel_sequence, wps_mac->random_channel_sequence_enabled, wps_mac->network_id);

    /* Sync module init */
    link_tdma_sync_init(&wps_mac->tdma_sync,
                        sync_cfg->sleep_level,
                        SYNC_RX_SETUP_PLL_CYCLES * (sync_cfg->isi_mitig_pauses + 1),
                        SYNC_FRAME_LOST_MAX_COUNT,
                        sync_cfg->syncword_len,
                        sync_cfg->preamble_len,
                        SYNC_PLL_STARTUP_CYCLES,
                        sync_cfg->isi_mitig,
                        sync_cfg->isi_mitig_pauses,
                        wps_mac->fast_sync_enabled);
}

void wps_mac_reset(wps_mac_t *wps_mac)
{
    /* Sync module reset */
    wps_mac->tdma_sync.frame_lost_count  = 0;
    wps_mac->tdma_sync.sync_slave_offset = 0;
    wps_mac->tdma_sync.slave_sync_state  = STATE_SYNCING;

    /* Internal state machine reset */
    wps_mac->current_input  = MAC_SIGNAL_EMPTY;
    wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;
}
#include "fsl_debug_console.h"
void wps_mac_process(wps_mac_t *wps_mac)
{
    //PRINTF("wps_mac_process");
    wps_mac->state_process_idx = 0;
    wps_mac->current_input     = wps_mac->input_signal.main_signal;
    wps_mac->current_output    = MAC_SIGNAL_WPS_EMPTY;

    if (scheduling_needed(wps_mac)) {
        process_scheduler(wps_mac);
    } else {
        process_rx_tx_outcome(wps_mac);
    }
#ifndef WPS_DISABLE_LINK_STATS
    update_wps_stats(wps_mac);
#endif /* WPS_DISABLE_LINK_STATS */
}

void wps_mac_enable_fast_sync(wps_mac_t *wps_mac)
{
    wps_mac->fast_sync_enabled = true;
}

void wps_mac_disable_fast_sync(wps_mac_t *wps_mac)
{
    wps_mac->fast_sync_enabled = false;
}

void wps_mac_set_phase_interface(wps_mac_t *wps_mac, wps_mac_phase_interface_t *phase_itf)
{
    wps_mac->phase_intf.is_busy =  phase_itf->is_busy;
    wps_mac->phase_intf.supply  =  phase_itf->supply;
}

/* PRIVATE STATE FUNCTIONS ****************************************************/
/** @brief Link quality state.
 *
 * This state handle LQI and gain loop module update.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_link_quality(void *signal_data)
{
    wps_mac_t    *wps_mac             = (wps_mac_t *)signal_data;
    gain_loop_t *current_gain_loop  = NULL;
    lqi_t       *current_lqi        = NULL;
    lqi_t *current_channel_lqi      = NULL;
    uint8_t gain_index;

    if (is_current_prime_timeslot_rx(wps_mac)) {
        /* RX prime frame, update LQI and gain loop */
        current_lqi         = &wps_mac->current_timeslot->connection_auto_reply->lqi;
        current_channel_lqi = &wps_mac->current_timeslot->connection_auto_reply->channel_lqi[wps_mac->current_channel_index];
        current_gain_loop   =  wps_mac->current_timeslot->connection_auto_reply->gain_loop[wps_mac->current_channel_index];
    } else {
        /* Timeslot is not prime OR is prime but TX */
        current_lqi         = &wps_mac->current_timeslot->connection_main->lqi;
        current_channel_lqi = &wps_mac->current_timeslot->connection_main->channel_lqi[wps_mac->current_channel_index];
        current_gain_loop   =  wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
    }

    gain_index = link_gain_loop_get_gain_index(current_gain_loop);
#ifndef WPS_DISABLE_PHY_STATS
    /* Update LQI */
    link_lqi_update(current_lqi,
                    gain_index,
                    wps_mac->current_xlayer->frame.frame_outcome,
                    wps_mac->current_xlayer->config.rssi_raw,
                    wps_mac->current_xlayer->config.rnsi_raw,
                    wps_mac->current_xlayer->config.phase_offset);
#ifdef WPS_ENABLE_PHY_STATS_PER_BANDS
    link_lqi_update(current_channel_lqi,
                    gain_index,
                    wps_mac->current_xlayer->frame.frame_outcome,
                    wps_mac->current_xlayer->config.rssi_raw,
                    wps_mac->current_xlayer->config.rnsi_raw,
                    wps_mac->current_xlayer->config.phase_offset);
#else
    (void)current_channel_lqi;
    (void)gain_index;
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */
#else
    (void)current_channel_lqi;
    (void)current_lqi;
    (void)gain_index;
#endif /* WPS_DISABLE_PHY_STATS */
}

/** @brief Sync state.
 *
 * This state handle sync module update when node role
 * is set to NETWORK_NODE.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_sync(void *signal_data)
{
    wps_mac_t *wps_mac = (wps_mac_t *)signal_data;

    if (wps_mac->output_signal.main_signal == MAC_SIGNAL_SYNCING) {
        wps_mac->current_xlayer->config.rx_wait_time = 0;
    }

    if (is_network_node(wps_mac)) {
        if ((!wps_mac->current_ts_prime) || (wps_mac->current_ts_prime_tx)) {
            if (!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync)) {
                link_tdma_sync_slave_find(&wps_mac->tdma_sync,
                                          wps_mac->current_xlayer->frame.frame_outcome,
                                          wps_mac->current_xlayer->config.rx_wait_time,
                                         &wps_mac->current_timeslot->connection_main->cca,
                                          wps_mac->current_xlayer->config.rx_cca_retry_count);
            } else if (wps_mac->current_timeslot->connection_main->source_address == wps_mac->syncing_address) {
                link_tdma_sync_slave_adjust(&wps_mac->tdma_sync,
                                            wps_mac->current_xlayer->frame.frame_outcome,
                                            wps_mac->current_xlayer->config.rx_wait_time,
                                           &wps_mac->current_timeslot->connection_main->cca,
                                            wps_mac->current_xlayer->config.rx_cca_retry_count);
            }
        }
    }
}

/** @brief Post RX state.
 *
 * This state handle header extraction and operation after
 * the reception of valid frame.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_post_rx(void *signal_data)
{
    wps_mac_t *wps_mac = (wps_mac_t *)signal_data;
    gain_loop_t *current_gain_loop = NULL;
    lqi_t *current_wps_lqi         = NULL;
    uint8_t gain_index;

    if (outcome_is_frame_received(wps_mac)) {
        if (no_payload_received(wps_mac->current_xlayer)) {
            wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;
        } else {

            /* Good frame received */
            wps_mac->current_output = MAC_SIGNAL_WPS_FRAME_RX_SUCCESS;

            /* Frame Received */
            extract_header(wps_mac, wps_mac->current_xlayer);

            /* After header extraction, if no payload received, then do not elevate payload to WPS */
            if (no_payload_received(wps_mac->current_xlayer)) {
                /* Frame received is internal to MAC */
                wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;
            }


            /* Assign proper callback */
            if (is_current_prime_timeslot_rx(wps_mac)) {
                /* RX prime frame, update LQI and gain loop */
                current_wps_lqi   = &wps_mac->current_timeslot->connection_auto_reply->used_frame_lqi;
                current_gain_loop = wps_mac->current_timeslot->connection_auto_reply->gain_loop[wps_mac->current_channel_index];
                wps_mac->current_xlayer->config.callback = wps_mac->current_timeslot->connection_auto_reply->rx_success_callback_t;
                wps_mac->current_xlayer->config.parg_callback = wps_mac->current_timeslot->connection_auto_reply->rx_success_parg_callback_t;
                if (!no_payload_received(wps_mac->current_xlayer)) {
                    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_FRAME_RX_SUCCESS;
                }
            } else {
                /* Timeslot is not prime OR is prime but TX */
                current_wps_lqi                         = &wps_mac->current_timeslot->connection_main->used_frame_lqi;
                current_gain_loop                       = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
                wps_mac->current_xlayer->config.callback = wps_mac->current_timeslot->connection_main->rx_success_callback_t;
                wps_mac->current_xlayer->config.parg_callback = wps_mac->current_timeslot->connection_main->rx_success_parg_callback_t;
            }

            gain_index = link_gain_loop_get_gain_index(current_gain_loop);

            if (!no_payload_received(wps_mac->current_xlayer)) {
                /* Update WPS LQI */
#ifndef WPS_DISABLE_STATS_USED_TIMESLOTS
                link_lqi_update(current_wps_lqi,
                                gain_index,
                                wps_mac->current_xlayer->frame.frame_outcome,
                                wps_mac->current_xlayer->config.rssi_raw,
                                wps_mac->current_xlayer->config.rnsi_raw,
                                wps_mac->current_xlayer->config.phase_offset);
#else
                (void)current_wps_lqi;
                (void)gain_index;
#endif /* WPS_DISABLE_STATS_USED_TIMESLOTS */
            }
        }
        if (no_space_available_for_rx(wps_mac)) {
            wps_mac->current_xlayer->config.callback = wps_mac->current_timeslot->connection_main->evt_callback_t;
            wps_mac->current_xlayer->config.parg_callback = wps_mac->current_timeslot->connection_main->evt_parg_callback_t;
            wps_mac->current_output                  = MAC_SIGNAL_WPS_FRAME_RX_OVERRUN;
        }
    } else {
        wps_mac->current_output = MAC_SIGNAL_WPS_FRAME_RX_FAIL;
    }

    if (link_scheduler_get_mismatch(&wps_mac->scheduler)) {
        wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;
    }
}

/** @brief Post TX state.
 *
 * This state handle operation after
 * the transmission of a frame with or
 * without acknowledgement.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_post_tx(void *signal_data)
{
    wps_mac_t *wps_mac                  = (wps_mac_t *)signal_data;
    frame_outcome_t  xlayer_outcome     = wps_mac->current_xlayer->frame.frame_outcome;
    uint32_t         ack_rssi           = wps_mac->current_xlayer->config.rssi_raw;
    uint32_t         ack_rnsi           = wps_mac->current_xlayer->config.rnsi_raw;
    uint8_t         *ack_phase_offset   = wps_mac->current_xlayer->config.phase_offset;
    gain_loop_t      *current_gain_loop;
    lqi_t            *current_lqi;
    lqi_t            *current_channel_lqi;
    wps_connection_t *current_connection;
    uint8_t gain_index;

    if (wps_mac->current_xlayer != &wps_mac->empty_frame_tx) {
        if (outcome_is_tx_sent_ack(wps_mac)) {
            current_connection                            = wps_mac->current_timeslot->connection_main;
            wps_mac->current_output                       = MAC_SIGNAL_WPS_TX_SUCCESS;
            wps_mac->current_xlayer->config.callback      = wps_mac->current_timeslot->connection_main->tx_success_callback_t;
            wps_mac->current_xlayer->config.parg_callback = wps_mac->current_timeslot->connection_main->tx_success_parg_callback_t;
            current_gain_loop                             = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
            current_lqi                                   = &wps_mac->current_timeslot->connection_main->lqi;
            current_channel_lqi                           = &wps_mac->current_timeslot->connection_main->channel_lqi[wps_mac->current_channel_index];
            if (is_current_timeslot_tx(wps_mac)) {
                link_saw_arq_inc_seq_num(&wps_mac->current_timeslot->connection_main->stop_and_wait_arq);
            }
        } else {
            if (is_current_timeslot_tx(wps_mac)) {
                if (!wps_mac->current_timeslot->connection_main->ack_enable) {
                    wps_mac->current_output = MAC_SIGNAL_WPS_TX_SUCCESS;
                    wps_mac->current_xlayer->config.callback = wps_mac->current_timeslot->connection_main->tx_success_callback_t;
                    wps_mac->current_xlayer->config.parg_callback = wps_mac->current_timeslot->connection_main->tx_success_parg_callback_t;
                } else {
                    wps_mac->current_output = MAC_SIGNAL_WPS_TX_FAIL;
                    wps_mac->current_xlayer->config.callback   = wps_mac->current_timeslot->connection_main->tx_fail_callback_t;
                    wps_mac->current_xlayer->config.parg_callback   = wps_mac->current_timeslot->connection_main->tx_fail_parg_callback_t;
                }
                current_connection  = wps_mac->current_timeslot->connection_main;
                current_gain_loop   = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
                current_lqi         = &wps_mac->current_timeslot->connection_main->lqi;
                current_channel_lqi = &wps_mac->current_timeslot->connection_main->channel_lqi[wps_mac->current_channel_index];
            } else {
                /* SaW only for main connection */
                xlayer_outcome                             = outcome_is_tx_not_sent(wps_mac) ? FRAME_WAIT : FRAME_SENT_ACK_LOST;
                current_connection                         = wps_mac->current_timeslot->connection_auto_reply;
                current_gain_loop                          = wps_mac->current_timeslot->connection_auto_reply->gain_loop[wps_mac->current_channel_index];
                current_lqi                                = &wps_mac->current_timeslot->connection_auto_reply->lqi;
                current_channel_lqi = &wps_mac->current_timeslot->connection_auto_reply->channel_lqi[wps_mac->current_channel_index];
                if (outcome_is_tx_not_sent(wps_mac)) {
                    wps_mac->current_output = MAC_SIGNAL_WPS_TX_FAIL;
                    wps_mac->current_xlayer->config.callback   = wps_mac->current_timeslot->connection_auto_reply->tx_fail_callback_t;
                    wps_mac->current_xlayer->config.parg_callback   = wps_mac->current_timeslot->connection_auto_reply->tx_fail_parg_callback_t;
                } else {
                    wps_mac->current_output = MAC_SIGNAL_WPS_TX_SUCCESS;
                    wps_mac->current_xlayer->config.callback   = wps_mac->current_timeslot->connection_auto_reply->tx_success_callback_t;
                    wps_mac->current_xlayer->config.parg_callback   = wps_mac->current_timeslot->connection_auto_reply->tx_success_parg_callback_t;
                }
            }
        }
        gain_index = link_gain_loop_get_gain_index(current_gain_loop);
#ifndef WPS_DISABLE_STATS_USED_TIMESLOTS
        link_lqi_update(&current_connection->used_frame_lqi, gain_index, xlayer_outcome, ack_rssi, ack_rnsi, ack_phase_offset);
#else
    (void)current_connection;
    (void)gain_index;
    (void)xlayer_outcome;
    (void)ack_rssi;
    (void)ack_rnsi;
    (void)ack_phase_offset;
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS */
    } else {
        wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;
        if (wps_mac->current_ts_prime_tx) {
            xlayer_outcome         = FRAME_SENT_ACK_LOST;
            current_lqi            = &wps_mac->current_timeslot->connection_auto_reply->lqi;
            current_channel_lqi    = &wps_mac->current_timeslot->connection_auto_reply->channel_lqi[wps_mac->current_channel_index];
            current_gain_loop = wps_mac->current_timeslot->connection_auto_reply->gain_loop[wps_mac->current_channel_index];
        } else {
            current_lqi            = &wps_mac->current_timeslot->connection_main->lqi;
            current_channel_lqi    = &wps_mac->current_timeslot->connection_main->channel_lqi[wps_mac->current_channel_index];
            if (outcome_is_tx_sent_ack(wps_mac)) {
                current_gain_loop = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
            } else {
                current_gain_loop = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
            }
        }
    }
    gain_index = link_gain_loop_get_gain_index(current_gain_loop);
#ifndef WPS_DISABLE_PHY_STATS
    link_lqi_update(current_lqi, gain_index, xlayer_outcome, ack_rssi, ack_rnsi, ack_phase_offset);
#ifdef WPS_ENABLE_PHY_STATS_PER_BANDS
    link_lqi_update(current_channel_lqi, gain_index, xlayer_outcome, ack_rssi, ack_rnsi, ack_phase_offset);
#else
    (void)current_channel_lqi;
    (void)gain_index;
    (void)xlayer_outcome;
    (void)ack_rssi;
    (void)ack_rnsi;
    (void)ack_phase_offset;
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */
#else
    (void)current_channel_lqi;
    (void)current_lqi;
    (void)gain_index;
    (void)xlayer_outcome;
    (void)ack_rssi;
    (void)ack_rnsi;
    (void)ack_phase_offset;
#endif /* WPS_ENABLE_PHY_STATS */
}

/** @brief Stop and wait ARQ state.
 *
 * This state handle ACK algorithm and status.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_stop_wait_arq(void *signal_data)
{
    (void)signal_data;
}

/** @brief Prepare frame state.
 *
 * This state handle preparation of MAC header
 * for frame transmition.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_mac_prepare_frame(void *signal_data)
{
    wps_mac_t *wps_mac = (wps_mac_t *)signal_data;

    /* TODO Add packet fragmentation module */
    if (is_current_timeslot_tx(wps_mac) || wps_mac->current_ts_prime) {

        /* TX timeslot / TX timeslot prime */
        if (header_space_available(wps_mac->current_xlayer)) {

            fill_header(wps_mac, wps_mac->current_xlayer);
        }
    }
}

/** @brief Prepare frame state.
 *
 * This state handle preparation of MAC header
 * for frame transmition.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_setup_primary_link(void *signal_data)
{
    //PRINTF("state_setup_primary_link");
    wps_mac_t *wps_mac      = (wps_mac_t *)signal_data;
    uint32_t next_channel = link_channel_hopping_get_channel(&wps_mac->channel_hopping);
    uint16_t rdo_value    = link_rdo_get_offset(&wps_mac->link_rdo);

    if (is_current_timeslot_tx(wps_mac)) {
        link_tdma_sync_update_tx(&wps_mac->tdma_sync,
                                 link_scheduler_get_sleep_time(&wps_mac->scheduler) + rdo_value,
                                &wps_mac->current_timeslot->connection_main->cca);

        wps_mac->main_xlayer = get_xlayer_for_tx(wps_mac, wps_mac->current_timeslot->connection_main);
        wps_mac->auto_xlayer = NULL;

        /* Set current xlayer for the prepare frame */
        wps_mac->current_xlayer = wps_mac->main_xlayer;

        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;

    } else {
        link_tdma_sync_update_rx(&wps_mac->tdma_sync,
                                 link_scheduler_get_sleep_time(&wps_mac->scheduler) + rdo_value,
                                 &wps_mac->current_timeslot->connection_main->cca);

        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;
        wps_mac->main_xlayer               = get_xlayer_for_rx(wps_mac, wps_mac->current_timeslot->connection_main);
        wps_mac->auto_xlayer               = NULL;

        if ((!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync)) &&
            (wps_mac->node_role == NETWORK_NODE) &&
             wps_mac->current_timeslot->connection_main->source_address == wps_mac->syncing_address &&
             wps_mac->fast_sync_enabled) {
            link_gain_loop_reset_gain_index(wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index]);
            wps_mac->output_signal.main_signal = MAC_SIGNAL_SYNCING;
        }
    }

    uint8_t payload_size = wps_mac->main_xlayer->frame.payload_end_it - wps_mac->main_xlayer->frame.payload_begin_it;
    uint8_t fallback_index = link_fallback_get_channel_index(&wps_mac->current_timeslot->connection_main->link_fallback, payload_size);

    wps_mac->main_xlayer->config.channel = &wps_mac->current_timeslot->connection_main->channel[fallback_index][MULTI_RADIO_BASE_IDX][next_channel];
    wps_mac->main_xlayer->config.packet_cfg = wps_mac->current_timeslot->connection_main->packet_cfg;
    wps_mac->main_xlayer->config.cca_threshold   = wps_mac->current_timeslot->connection_main->cca.threshold;
    wps_mac->main_xlayer->config.cca_retry_time  = wps_mac->current_timeslot->connection_main->cca.retry_time_pll_cycles;
    wps_mac->main_xlayer->config.cca_max_try_count = wps_mac->current_timeslot->connection_main->cca.max_try_count;
    wps_mac->main_xlayer->config.cca_try_count = 0;
    wps_mac->main_xlayer->config.cca_fail_action = wps_mac->current_timeslot->connection_main->cca.fail_action;
    wps_mac->main_xlayer->config.sleep_level = wps_mac->tdma_sync.sleep_mode;
    wps_mac->main_xlayer->config.gain_loop = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
    wps_mac->main_xlayer->config.fixed_payload_size_enable = wps_mac->current_timeslot->connection_main->fixed_payload_size_enable;
    wps_mac->main_xlayer->config.phases_info = &wps_mac->phase_data.local_phases_info;
    wps_mac->main_xlayer->config.isi_mitig = wps_mac->tdma_sync.isi_mitig;

    update_main_xlayer_link_parameter(wps_mac, wps_mac->main_xlayer);
    update_xlayer_sync(wps_mac, wps_mac->main_xlayer);
    update_xlayer_modem_feat(wps_mac, wps_mac->main_xlayer);
    //PRINTF("r state_setup_primary_link");
}

/** @brief Setup ack link state.
 *
 * This state handle preparation of xlayer for proper
 * ACK reception/transmition.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_setup_ack_link(void *signal_data)
{
    (void)signal_data;
}

/** @brief Setup prime link state.
 *
 * This state handle preparation of xlayer for proper
 * prime timeslot frame reception/transmition.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_setup_prime_link(void *signal_data)
{
    wps_mac_t *wps_mac = (wps_mac_t *)signal_data;
    uint32_t next_channel = link_channel_hopping_get_channel(&wps_mac->channel_hopping);

    if (wps_mac->current_ts_prime) {
        /* Prime status */
        if (wps_mac->current_ts_prime_tx) {
            /* TX Prime */
            /* TODO Add tx pulse/Channel cfg */
            wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
            wps_mac->auto_xlayer = get_xlayer_for_tx(wps_mac, wps_mac->current_timeslot->connection_auto_reply);

            /* Set current xlayer for Prepare frame */
            wps_mac->current_xlayer = wps_mac->auto_xlayer;
        } else {
            /* RX Prime*/
            wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
            wps_mac->auto_xlayer = get_xlayer_for_rx(wps_mac, wps_mac->current_timeslot->connection_auto_reply);
        }
        update_prime_xlayer_link_parameter(wps_mac, wps_mac->auto_xlayer);
    }
    if( wps_mac->auto_xlayer != NULL){
        uint8_t payload_size = wps_mac->main_xlayer->frame.payload_end_it - wps_mac->main_xlayer->frame.payload_begin_it;
        uint8_t fallback_index = link_fallback_get_channel_index(&wps_mac->current_timeslot->connection_main->link_fallback, payload_size);

        wps_mac->auto_xlayer->config.channel = &wps_mac->current_timeslot->connection_main->channel[fallback_index][MULTI_RADIO_BASE_IDX][next_channel];
        wps_mac->auto_xlayer->config.packet_cfg = wps_mac->main_xlayer->config.packet_cfg;
        wps_mac->auto_xlayer->config.gain_loop = wps_mac->current_timeslot->connection_main->gain_loop[wps_mac->current_channel_index];
	    wps_mac->auto_xlayer->config.fixed_payload_size_enable = wps_mac->main_xlayer->config.fixed_payload_size_enable;
    }
    /* Reassign input for loop back */
    wps_mac->current_input = MAC_SIGNAL_PREPARE_FRAME;
    wps_mac->state_process_idx   = 0;
}

/** @brief Scheduler state.
 *
 * This state get the next timeslot to
 * handle.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void state_scheduler(void *signal_data)
{
    wps_mac_t *wps_mac = (wps_mac_t *)signal_data;
    uint8_t inc_count;

    link_scheduler_reset_sleep_time(&wps_mac->scheduler);
    inc_count = link_scheduler_increment_time_slot(&wps_mac->scheduler);
    handle_link_throttle(wps_mac, &inc_count);
    link_channel_hopping_increment_sequence(&wps_mac->channel_hopping, inc_count);

    wps_mac->current_channel_index = link_channel_hopping_get_channel(&wps_mac->channel_hopping);
    wps_mac->current_timeslot      = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
    setup_prime_timeslot_status(wps_mac);

    wps_mac->current_input     = MAC_SIGNAL_SETUP_LINK;
    wps_mac->state_process_idx = 0;
}

/** @brief End state.
 *
 * This state is use in the jump tables to
 * know when jump table are ending.
 *
 *  @param[in] signal_data  MAC structure.
 */
static void end(void *signal_data)
{
    (void)signal_data;
}

void wps_mac_send_channel_index(void *wps_mac, uint8_t *index)
{
    wps_mac_t *mac = wps_mac;
    *index = link_channel_hopping_get_seq_index(&mac->channel_hopping);
}

void wps_mac_receive_channel_index(void *wps_mac, uint8_t *index)
{
    wps_mac_t *mac = wps_mac;
    if (mac->node_role == NETWORK_NODE) {
        link_channel_hopping_set_seq_index(&mac->channel_hopping, *index);
    }
}

uint8_t wps_mac_get_channel_index_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->channel_hopping.hop_seq_index);
}

void wps_mac_send_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw)
{
    wps_mac_t *mac = wps_mac;
    uint16_t  index = link_scheduler_get_next_timeslot_index(&mac->scheduler);

    *timeslot_id_saw  = MASK2VAL(index, HEADER_BYTE0_TIME_SLOT_ID_MASK) |
                        MOV2MASK(link_saw_arq_get_seq_num(&mac->current_timeslot->connection_main->stop_and_wait_arq),
                        HEADER_BYTE0_SEQ_NUM_MASK);
}

void wps_mac_receive_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw)
{
    wps_mac_t *mac = wps_mac;
    uint8_t time_slot_id;

    if (is_network_node(mac)) {
        time_slot_id = MASK2VAL(*timeslot_id_saw, HEADER_BYTE0_TIME_SLOT_ID_MASK);
        if (time_slot_id < mac->scheduler.schedule->size) {
            if (link_scheduler_get_next_timeslot_index(&mac->scheduler) != time_slot_id) {
                link_scheduler_set_mismatch(&mac->scheduler);
            }

            link_scheduler_set_time_slot_i(&mac->scheduler, time_slot_id);
        }
    }

    /* Check if received frame is an auto sync frame */
    if (mac->current_xlayer->frame.header_begin_it + 2 != mac->current_xlayer->frame.payload_end_it) {
        link_saw_arq_update_rx_seq_num(&mac->current_timeslot->connection_main->stop_and_wait_arq,
                                    MASK2VAL(*timeslot_id_saw, HEADER_BYTE0_SEQ_NUM_MASK));
        if (link_saw_arq_is_rx_frame_duplicate(&mac->current_timeslot->connection_main->stop_and_wait_arq)) {
            /* Frame is duplicate */
            mac->current_output = MAC_SIGNAL_WPS_EMPTY;
        }
    }
}

uint8_t wps_mac_get_timeslot_id_saw_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;
    return sizeof(mac->scheduler.current_time_slot_num);
}

void wps_mac_send_rdo(void *wps_mac, uint8_t *rdo)
{
    wps_mac_t *mac = wps_mac;

    link_rdo_send_offset(&mac->link_rdo, rdo);
}

void wps_mac_receive_rdo(void *wps_mac, uint8_t *rdo)
{
    wps_mac_t *mac = wps_mac;

    link_rdo_set_offset(&mac->link_rdo, rdo);
}

uint8_t wps_mac_get_rdo_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->link_rdo.offset);
}

void wps_mac_send_ranging_phases(void *wps_mac, uint8_t *phases)
{
    wps_mac_t *mac = wps_mac;

    *phases = mac->phase_data.local_phases_count;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase1;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase2;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase3;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase4;
}

void wps_mac_receive_ranging_phases(void *wps_mac, uint8_t *phases)
{
    wps_mac_t *mac = wps_mac;

    mac->phase_data.remote_phases_count = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase1 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase2 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase3 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase4 = *phases;

    if (is_phase_accumulation_enable(mac)) {
        if (is_phase_data_valid(&mac->phase_data) && !mac->phase_intf.is_busy()) {
            mac->phase_intf.supply(&mac->phase_data.last_local_phases_info, &mac->phase_data.remote_phases_info);
        }
        update_phases_data(&mac->phase_data, mac->main_xlayer->config.rx_wait_time);
    }
}

uint8_t wps_mac_get_ranging_phases_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->phase_data.local_phases_count)       +
           sizeof(mac->phase_data.local_phases_info.phase1) +
           sizeof(mac->phase_data.local_phases_info.phase2) +
           sizeof(mac->phase_data.local_phases_info.phase3) +
           sizeof(mac->phase_data.local_phases_info.phase4);
}

void wps_mac_send_ranging_phase_count(void *wps_mac, uint8_t *phase_count)
{
    wps_mac_t *mac = wps_mac;

    /* Transmit count */
    *phase_count = mac->phase_data.local_phases_count;
}

void wps_mac_receive_ranging_phase_count(void *wps_mac, uint8_t *phase_count)
{
    wps_mac_t *mac = wps_mac;

    mac->phase_data.local_phases_count = *phase_count;
}

uint8_t wps_mac_get_ranging_phase_count_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->phase_data.local_phases_count);
}

/* PRIVATE FUNCTIONS *********************************************************/
/** @brief Output if input signal is RX_FRAME.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Input signal is RX_FRAME.
 *  @retval False  Input signal is not RX_FRAME.
 */
static bool outcome_is_frame_received(wps_mac_t *wps_mac)
{
    return (wps_mac->current_input == MAC_SIGNAL_RX_FRAME);
}

/** @brief Output if sent frame has been acknowledge.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Frame sent has been acknowledge.
 *  @retval False  Frame sent has not been acknowledge.
 */
static bool outcome_is_tx_sent_ack(wps_mac_t *wps_mac)
{
    return (wps_mac->current_input == MAC_SIGNAL_TX_SENT_ACK);
}

/** @brief Output if frame has not been sent
 *
 *  @param[in] wps_mac
 *  @return True   The frame is not sent.
 *  @return False  The frame is sent.
 */
static bool outcome_is_tx_not_sent(wps_mac_t *wps_mac)
{
    return (wps_mac->current_input == MAC_SIGNAL_TX_NOT_SENT);
}

/** @brief Output if node role is NETWORK_NODE.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Node role is NETWORK_NODE.
 *  @retval False  Node role is not NETWORK_NODE.
 */
static bool is_network_node(wps_mac_t *wps_mac)
{
    return (wps_mac->node_role == NETWORK_NODE);
}

/** @brief Output if current main connection timeslot is TX.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Main connection timeslot is TX.
 *  @retval False  Main connection timeslot is RX.
 */
static bool is_current_timeslot_tx(wps_mac_t *wps_mac)
{
    return (wps_mac->current_timeslot->connection_main->source_address == wps_mac->local_address);
}

/** @brief Output if auto-reply connection timeslot is TX.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Auto-reply connection timeslot is TX.
 *  @retval False  Auto-reply connection timeslot is RX.
 */
static bool is_current_prime_timeslot_tx(wps_mac_t *wps_mac)
{
    return (wps_mac->current_timeslot->connection_auto_reply->source_address == wps_mac->local_address);
}

/** @brief Output if current timeslot is prime RX.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval true   Current timeslot is not prime RX.
 *  @retval false  Current timeslot is prime RX.
 */
static bool is_current_prime_timeslot_rx(wps_mac_t *wps_mac)
{
    return (wps_mac->current_ts_prime && !wps_mac->current_ts_prime_tx);
}

/** @brief Output if auto-reply connection exist in timeslot.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   There is an auto-reply connection.
 *  @retval False  There is not an auto-reply connection.
 */
static bool is_current_timeslot_prime(wps_mac_t *wps_mac)
{
    return (wps_mac->current_timeslot->connection_auto_reply != NULL);
}

/** @brief Return if stop and wait is enable or not.
 *
 *  @param wps_mac  MAC structure.
 *  @retval True   Stop and wait ARQ is enable.
 *  @retval False  Stop and wait ARQ is disable.
 */
static bool is_saw_arq_enable(wps_connection_t *connection)
{
    return connection->stop_and_wait_arq.enable;
}

/** @brief Return if the current situation allows for a phase accumulation.
 *
 *  @param[in] connection  Current connection.
 *  @retval True   Current situation is valid.
 *  @retval False  Current is not valid.
 */
static bool is_phase_accumulation_enable(wps_mac_t *wps_mac)
{
    bool ret = false;

    if (is_current_timeslot_prime(wps_mac) &&
        !is_current_prime_timeslot_tx(wps_mac) &&
        wps_mac->current_timeslot->connection_main->phases_exchange) {
        ret = true;
    }
    return ret;
}

/** @brief Update phases data.
 *
 *  @param[in] phase_data   Phase data.
 *  @param[in] rx_wait_time Reception wait time.
 */
static void update_phases_data(wps_mac_phase_data_t *phase_data, uint16_t rx_wait_time)
{
    phase_data->last_local_phases_info.phase1 = phase_data->local_phases_info.phase1;
    phase_data->last_local_phases_info.phase2 = phase_data->local_phases_info.phase2;
    phase_data->last_local_phases_info.phase3 = phase_data->local_phases_info.phase3;
    phase_data->last_local_phases_info.phase4 = phase_data->local_phases_info.phase4;
    phase_data->last_local_phases_info.rx_waited0 = rx_wait_time & 0x00ff;
    phase_data->last_local_phases_info.rx_waited1 = (rx_wait_time & 0x7f00) >> 8;
    phase_data->local_phases_count++;
}

/** @brief Return if current phase data are valid.
 *
 *  @param[in] phase_data   Phase data.
 *  @retval True   Current situationis valid.
 *  @retval False  Current is not valid.
 */
static bool is_phase_data_valid(wps_mac_phase_data_t *phase_data)
{
    return (((uint8_t)(phase_data->remote_phases_count + 1)) == phase_data->local_phases_count);
}

/** @brief Update the xlayer sync module value for PHY.
 *
 *  @param[in]  wps_mac         MAC structure.
 *  @param[out] current_xlayer  Current xlayer node to update.
 */
static void update_xlayer_sync(wps_mac_t *wps_mac, xlayer_t *current_xlayer)
{
    current_xlayer->config.power_up_delay = link_tdma_sync_get_pwr_up(&wps_mac->tdma_sync);
    current_xlayer->config.rx_timeout     = link_tdma_sync_get_timeout(&wps_mac->tdma_sync);
    current_xlayer->config.sleep_time     = link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync);
}

/** @brief Update the main connection xlayer gain loop value for PHY.
 *
 *  @param[in] wps_mac MAC structure.
 *  @param[in] xlayer  xlayer node to update.
 */
static void update_main_xlayer_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer)
{
    xlayer->config.destination_address = wps_mac->current_timeslot->connection_main->destination_address;
    xlayer->config.source_address      = wps_mac->current_timeslot->connection_main->source_address;
    xlayer->config.expect_ack          = wps_mac->current_timeslot->connection_main->ack_enable;
}

/** @brief Update the main connection xlayer gain loop value for PHY.
 *
 *  @param[in] wps_mac MAC structure.
 *  @param[in] xlayer  xlayer node to update.
 */
static void update_prime_xlayer_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer)
{
    xlayer->config.destination_address = wps_mac->current_timeslot->connection_auto_reply->destination_address;
    xlayer->config.source_address      = wps_mac->current_timeslot->connection_auto_reply->source_address;
}

/** @brief Update the main connection xlayer modem feat value for PHY.
 *
 *  @param[in] wps_mac MAC structure.
 *  @param[in] xlayer  xlayer node to update.
 */
static void update_xlayer_modem_feat(wps_mac_t *wps_mac, xlayer_t *current_xlayer)
{
    current_xlayer->config.fec        = wps_mac->current_timeslot->connection_main->frame_cfg.fec;
    current_xlayer->config.modulation = wps_mac->current_timeslot->connection_main->frame_cfg.modulation;
}

/** @brief Return the corresponding queue for TX depending on the input connection.
 *
 * For TX timeslot, Application should have enqueue a node
 * inside the queue, so the MAC only need to get the front of
 * the queue in order to get the good node for the process.
 *
 *  @param[in] connection  Queue node connection.
 *  @return Current pending node in queue.
 */
static xlayer_t *get_xlayer_for_tx(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    xlayer_t *free_xlayer;
    bool unsync = ((wps_mac->tdma_sync.slave_sync_state == STATE_SYNCING) && (wps_mac->node_role == NETWORK_NODE));

    if (is_saw_arq_enable(connection)) {
        flush_timeout_frames_before_sending(wps_mac, connection);
    }

    free_xlayer = circular_queue_front(&connection->xlayer_queue);

    if (free_xlayer == NULL || unsync) {
        if (connection->auto_sync_enable && !unsync) {
            wps_mac->empty_frame_tx.frame.header_memory = overrun_buffer;
            wps_mac->empty_frame_tx.frame.header_end_it = overrun_buffer + connection->header_size;
        } else {
            wps_mac->empty_frame_tx.frame.header_memory    = NULL;
            wps_mac->empty_frame_tx.frame.header_end_it    = NULL;
        }
        wps_mac->empty_frame_tx.frame.header_begin_it      = wps_mac->empty_frame_tx.frame.header_end_it;
        wps_mac->empty_frame_tx.frame.payload_end_it       = wps_mac->empty_frame_tx.frame.header_end_it;
        wps_mac->empty_frame_tx.frame.payload_begin_it     = wps_mac->empty_frame_tx.frame.header_end_it;
        free_xlayer                                       = &wps_mac->empty_frame_tx;
        wps_mac->empty_frame_tx.frame.time_stamp           = connection->get_tick_quarter_ms();

    } else {
        free_xlayer->frame.header_begin_it = free_xlayer->frame.header_end_it;
    }

    return free_xlayer;
}

/** @brief Return the corresponding queue for RX depending on the input connection.
 *
 * For RX timeslot, MAC should get the first free slot, WPS
 * should enqueue for application .
 *
 *  @param[in] connection  Queue node connection.
 *  @return Current pending node in queue.
 */
static xlayer_t *get_xlayer_for_rx(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    xlayer_t *free_xlayer;
    xlayer_t *empty_free_xlayer;

    free_xlayer = circular_queue_get_free_slot(&connection->xlayer_queue);
    if (free_xlayer == NULL) {
        empty_free_xlayer = circular_queue_front(&connection->xlayer_queue);
        free_xlayer = &wps_mac->empty_frame_rx;

        wps_mac->empty_frame_rx.frame.payload_memory      = overrun_buffer;
        wps_mac->empty_frame_rx.frame.payload_memory_size = connection->fixed_payload_size_enable
                                                         ? (connection->fixed_payload_size + connection->header_size + 1)
                                                         : empty_free_xlayer->frame.payload_memory_size;
        wps_mac->empty_frame_rx.frame.payload_begin_it    = overrun_buffer;
        wps_mac->empty_frame_rx.frame.payload_end_it      = connection->fixed_payload_size_enable
                                                         ? overrun_buffer + (connection->fixed_payload_size + connection->header_size + 1)
                                                         : overrun_buffer + empty_free_xlayer->frame.payload_memory_size;
        wps_mac->empty_frame_rx.frame.header_memory       = overrun_buffer;
        wps_mac->empty_frame_rx.frame.header_memory_size  = connection->header_size;
        wps_mac->empty_frame_rx.frame.header_begin_it     = overrun_buffer;
        wps_mac->empty_frame_rx.frame.header_end_it       = overrun_buffer;
    }

    return free_xlayer;
}

/** @brief Output if there is enough space for MAC header in xlayer.
 *
 *  @param[in] current_queue  Current header xlayer.
 *  @retval True   Header have enough space for MAC header.
 *  @retval False  Header have not enough space for MAC header.
 */
static bool header_space_available(xlayer_t *current_queue)
{
    int8_t header_space_available = current_queue->frame.header_begin_it - current_queue->frame.header_memory;

    /* Ensure begin_it is available AND that enough space is available to write header */
    return (current_queue->frame.header_begin_it != NULL && (header_space_available >= current_queue->frame.header_memory_size));
}

/** @brief Extract the header fields from a received node queue.
 *
 *  @param[out] wps_mac        Frame header MAC instance.
 *  @param[in]  current_queue  xlayer header.
 *  @retval True   The received timeslot ID was different from the expected one.
 *  @retval False  The received timeslot ID was identical to the expected one.
 */
static void extract_header(wps_mac_t *wps_mac, xlayer_t *current_queue)
{
    /* MAC should always be the first to extract */
    current_queue->frame.header_begin_it = current_queue->frame.header_memory;
    if (current_queue->frame.header_begin_it != NULL) {
        /* First byte should always be the radio automatic response */
        current_queue->frame.header_begin_it++;

        if(wps_mac->current_ts_prime && !wps_mac->current_ts_prime_tx) {
            link_protocol_receive_buffer(&wps_mac->current_timeslot->connection_auto_reply->link_protocol,
                                        wps_mac->current_xlayer->frame.header_begin_it,
                                        wps_mac->current_timeslot->connection_auto_reply->header_size);
            wps_mac->current_xlayer->frame.header_begin_it += wps_mac->current_timeslot->connection_auto_reply->header_size;
        } else {
            link_protocol_receive_buffer(&wps_mac->current_timeslot->connection_main->link_protocol,
                                        wps_mac->current_xlayer->frame.header_begin_it,
                                        wps_mac->current_timeslot->connection_main->header_size);
            wps_mac->current_xlayer->frame.header_begin_it += wps_mac->current_timeslot->connection_main->header_size;
        }

        /* Assign payload pointer as if there is no other layer on top. */
        current_queue->frame.payload_begin_it = current_queue->frame.header_begin_it;
    }
}

/** @brief Fill the header fields for a TX node queue.
 *
 *  @param[in]  wps_mac        Frame header MAC instance.
 *  @param[in]  current_queue  header xlayer.
 */
static void fill_header(wps_mac_t *wps_mac, xlayer_t *current_queue)
{
    uint32_t size = 0;

    if(wps_mac->current_ts_prime_tx) {
        current_queue->frame.header_begin_it -= wps_mac->current_timeslot->connection_auto_reply->header_size;
        link_protocol_send_buffer(&wps_mac->current_timeslot->connection_auto_reply->link_protocol, current_queue->frame.header_begin_it, &size);
    } else {
        current_queue->frame.header_begin_it -= wps_mac->current_timeslot->connection_main->header_size;
        link_protocol_send_buffer(&wps_mac->current_timeslot->connection_main->link_protocol, current_queue->frame.header_begin_it, &size);
    }
}

/** @brief Fill the header fields for a TX node queue.
 *
 *  @param[in] current_queue  Current xlayer.
 *  @retval True   No payload have been received.
 *  @retval False  Payload have been received.
 */
static bool no_payload_received(xlayer_t *current_queue)
{
    return (current_queue->frame.header_begin_it == current_queue->frame.payload_end_it);
}

/** @brief Setup MAC instance flag for timeslot prime status.
 *
 *  @param[in] wps_mac  MAC instance.
 */
static void setup_prime_timeslot_status(wps_mac_t *wps_mac)
{
    if (is_current_timeslot_prime(wps_mac)) {
        wps_mac->current_ts_prime    = true;
        if (is_current_prime_timeslot_tx(wps_mac)) {
            wps_mac->current_ts_prime_tx = true;
        } else {
            wps_mac->current_ts_prime_tx = false;
        }
    } else {
        wps_mac->current_ts_prime    = false;
        wps_mac->current_ts_prime_tx = false;
    }
}

/** @brief Output if current input signal is MAC_SIGNAL_SCHEDULE.
 *
 *  @param[in] wps_mac  WPS MAC instance.
 *  @retval true   Scheduling needed.
 *  @retval false  No scheduling needed.
 */
static bool scheduling_needed(wps_mac_t *wps_mac)
{
    return (wps_mac->current_input == MAC_SIGNAL_SCHEDULE);
}

/** @brief Process the MAC_SIGNAL_SCHEDULE input.
 *
 *  @param[in] wps_mac  WPS MAC instance.
 */
static void process_scheduler(wps_mac_t *wps_mac)
{
    do {
         //PRINTF("process_scheduler %d %d", wps_mac->current_input, wps_mac->state_process_idx);
        wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx++](wps_mac);

    } while (wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx] != end);
}

/** @brief Process frame outcome coming from WPS PHY layer.
 *
 *  @param[in] wps_mac  WPS MAC instance.
 */
static void process_rx_tx_outcome(wps_mac_t *wps_mac)
{
    wps_mac->current_xlayer = wps_mac->main_xlayer;
    /* Handle main xlayer processing */
    do {
        wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx++](wps_mac);
    } while (wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx] != end);

    /* Assign main output */
    wps_mac->output_signal.main_signal  = wps_mac->current_output;

    /* Reset state process for state machine */
    wps_mac->state_process_idx = 0;

    wps_mac->current_output = MAC_SIGNAL_WPS_EMPTY;

    if (wps_mac->auto_xlayer != NULL) {
        /* Get WPS input xlayer and reset output */
        wps_mac->current_input  = wps_mac->input_signal.auto_signal;
        wps_mac->current_xlayer = wps_mac->auto_xlayer;
        /* Handle auto xlayer processing*/
        while (wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx] != end) {
            wps_mac->state_machine[wps_mac->current_input][wps_mac->state_process_idx++](wps_mac);
        }
    }
    /* Assign auto output */
    wps_mac->output_signal.auto_signal = wps_mac->current_output;
}

/** @brief  Check if current use queue is the internal one.
 *
 * This is use to know if there is still space available
 * in queue, since when we use the internal one, it means there
 * is no more space available.
 *
 *  @param[in] wps_mac  WPS MAC instance.
 *  @return true   No space is available in queue for reception.
 *  @return false  Space is available in queue for reception.
 */
static bool no_space_available_for_rx(wps_mac_t *wps_mac)
{
    return (wps_mac->current_xlayer == &wps_mac->empty_frame_rx);
}

/** @brief Finish a transmission.
 *
 *  @param[in] connection Connection instance.
 *  @retval true   On sucess.
 *  @retval false  On error.
 */
static bool send_done(wps_connection_t *connection)
{
    return circular_queue_dequeue(&connection->xlayer_queue);
}

/** @brief  Check and flush timeout frame before sending to PHY.
 *
 *  @param wps_mac  WPS MAC instance.
 */
static void flush_timeout_frames_before_sending(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    bool timeout = false;
    xlayer_t *xlayer;

    do {
        xlayer = circular_queue_front(&connection->xlayer_queue);
        if (xlayer != NULL) {
            timeout = link_saw_arq_is_frame_timeout(&connection->stop_and_wait_arq,
                                                    xlayer->frame.time_stamp,
                                                    xlayer->frame.retry_count++,
                                                    connection->get_tick_quarter_ms());
            if (timeout) {
                xlayer->config.callback = connection->tx_drop_callback_t;
                xlayer->config.parg_callback = connection->tx_drop_parg_callback_t;
                wps_callback_enqueue(wps_mac->callback_queue, xlayer);
                wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_TX_DROP;
#ifndef WPS_DISABLE_LINK_STATS
                update_wps_stats(wps_mac);
#endif /* WPS_DISABLE_LINK_STATS */
                send_done(connection);
            }
        } else {
            timeout = false;
        }
    } while (timeout);
}

#ifndef WPS_DISABLE_LINK_STATS
/** @brief Update WPS statistics
 *
 *  @param[in] MAC  WPS MAC instance.
 */
static void update_wps_stats(wps_mac_t *mac)
{
    xlayer_t *current_xlayer;

    current_xlayer = mac->main_xlayer;
    switch (mac->output_signal.main_signal) {
    case MAC_SIGNAL_WPS_FRAME_RX_SUCCESS:
        mac->current_timeslot->connection_main->wps_stats.rx_received++;
        mac->current_timeslot->connection_main->wps_stats.rx_byte_received +=
            (current_xlayer->frame.payload_end_it - current_xlayer->frame.payload_begin_it);
        break;
    case MAC_SIGNAL_WPS_FRAME_RX_OVERRUN:
        mac->current_timeslot->connection_main->wps_stats.rx_overrun++;
        break;
    case MAC_SIGNAL_WPS_TX_SUCCESS:
        mac->current_timeslot->connection_main->wps_stats.tx_success++;
        mac->current_timeslot->connection_main->wps_stats.tx_byte_sent +=
            (current_xlayer->frame.payload_end_it - current_xlayer->frame.payload_begin_it);
        if (mac->current_timeslot->connection_main->cca.enable) {
            if (current_xlayer->config.cca_try_count >= current_xlayer->config.cca_max_try_count) {
                mac->current_timeslot->connection_main->wps_stats.cca_fail++;
            } else if (current_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->current_timeslot->connection_main->wps_stats.cca_pass++;
            }
        }
        break;
    case MAC_SIGNAL_WPS_TX_FAIL:

        mac->current_timeslot->connection_main->wps_stats.tx_fail++;

        if (mac->current_timeslot->connection_main->cca.enable) {
            if (current_xlayer->config.cca_try_count >= current_xlayer->config.cca_max_try_count) {
                mac->current_timeslot->connection_main->wps_stats.cca_fail++;
            } else if (current_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->current_timeslot->connection_main->wps_stats.cca_pass++;
            }
        }
        break;
    case MAC_SIGNAL_WPS_TX_DROP:
            mac->current_timeslot->connection_main->wps_stats.tx_drop++;
        break;
    case MAC_SIGNAL_WPS_EMPTY:
        /* PHY NACK signal occured but SAW has not yet trigger, handle CCA stats only. */
        if (mac->current_timeslot->connection_main->cca.enable) {
            if (current_xlayer->config.cca_try_count >= current_xlayer->config.cca_max_try_count) {
                mac->current_timeslot->connection_main->wps_stats.cca_fail++;
            } else if (current_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->current_timeslot->connection_main->wps_stats.cca_pass++;
            }
        }
        break;
    default:
        break;
    }
    current_xlayer = mac->auto_xlayer;
    switch (mac->output_signal.auto_signal) {
    case MAC_SIGNAL_WPS_FRAME_RX_SUCCESS:
        mac->current_timeslot->connection_auto_reply->wps_stats.rx_received++;
        mac->current_timeslot->connection_auto_reply->wps_stats.rx_byte_received +=
            (current_xlayer->frame.payload_end_it - current_xlayer->frame.payload_begin_it);
        break;
    case MAC_SIGNAL_WPS_FRAME_RX_OVERRUN:
        mac->current_timeslot->connection_auto_reply->wps_stats.rx_overrun++;
        break;
    case MAC_SIGNAL_WPS_TX_SUCCESS:
        mac->current_timeslot->connection_auto_reply->wps_stats.tx_success++;
        mac->current_timeslot->connection_auto_reply->wps_stats.tx_byte_sent +=
            (current_xlayer->frame.payload_end_it - current_xlayer->frame.payload_begin_it);
        break;
    case MAC_SIGNAL_WPS_TX_FAIL:
        mac->current_timeslot->connection_auto_reply->wps_stats.tx_fail++;
        break;
    case MAC_SIGNAL_WPS_TX_DROP:
            mac->current_timeslot->connection_auto_reply->wps_stats.tx_drop++;
        break;
    default:
        break;
    }
}
#endif /* WPS_DISABLE_LINK_STATS */

/** @brief Handle link throttle.
 *
 *  @param[in] wps_mac    WPS MAC instance.
 *  @param[in] inc_count  Increment count.
 */
static void handle_link_throttle(wps_mac_t *wps_mac, uint8_t *inc_count)
{
    timeslot_t *candidate_timeslot = link_scheduler_get_current_timeslot(&wps_mac->scheduler);

    if (candidate_timeslot->connection_main->pattern != NULL) {
        uint8_t total_pattern_count = candidate_timeslot->connection_main->pattern_total_count;
        uint8_t current_pattern_count = (candidate_timeslot->connection_main->pattern_count + 1) % total_pattern_count;

        candidate_timeslot->connection_main->pattern_count = current_pattern_count;
        if (candidate_timeslot->connection_main->pattern[current_pattern_count] == 0) {
            do {
                *inc_count += link_scheduler_increment_time_slot(&wps_mac->scheduler);
                candidate_timeslot = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
                total_pattern_count = candidate_timeslot->connection_main->pattern_total_count;
                current_pattern_count = (candidate_timeslot->connection_main->pattern_count + 1) % total_pattern_count;
                candidate_timeslot->connection_main->pattern_count = current_pattern_count;
            } while (candidate_timeslot->connection_main->pattern[current_pattern_count] == 0);
        }
    }
}
