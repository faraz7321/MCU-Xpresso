/** @file  wps.c
 *  @brief SPARK Wireless Protocol Stack.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_calib.h"
#include "sr_spectral.h"
#include "wps.h"

/* CONSTANTS ******************************************************************/
#define EXTRACT_NETWORK_ID(addr, msbits_count) (addr >> (16 - msbits_count))
#define PERCENT_DENOMINATOR       100
#define TIMESLOT_VALUE_MASK       0x7F
#define US_TO_PLL_FACTOR          1000
#define CCA_ON_TIME_PLL_CYCLE     16

/* PRIVATE GLOBALS ************************************************************/
static bool pattern_cfg[WPS_CONNECTION_THROTTLE_GRANULARITY * WPS_REQUEST_MEMORY_SIZE];
static wps_write_request_info_t write_request_buffer[WPS_REQUEST_MEMORY_SIZE];
static wps_read_request_info_t read_request_buffer[WPS_REQUEST_MEMORY_SIZE];
static circular_queue_t pattern_cfg_queue;
static circular_queue_t write_request_queue;
static circular_queue_t read_request_queue;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/

static bool is_main_timeslot(int8_t id);
static uint32_t auto_reply_id_to_id(int8_t id);
static void set_tx_conn_queue_memory(uint8_t *buffer,
                                     circular_queue_t *queue,
                                     uint32_t queue_size,
                                     uint32_t hdr_len,
                                     uint32_t max_frame_len);
static void set_rx_conn_queue_memory(uint8_t *buffer,
                                     circular_queue_t *queue,
                                     uint32_t queue_size,
                                     uint32_t hdr_len,
                                     uint32_t max_frame_len);
static uint8_t generate_active_pattern(bool *pattern, uint8_t active_ratio);
static void initialize_request_queues(wps_t *wps);

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
uint32_t wps_us_to_pll_cycle(uint32_t time_us, chip_rate_cfg_t chip_rate)
{
    return (time_us * PLL_FREQ_KHZ(chip_rate) / US_TO_PLL_FACTOR) - 1;
}

void wps_init_phase_interface(wps_t *wps, wps_phase_interface_t *phase_itf)
{
    wps_mac_set_phase_interface(&wps->mac, (wps_mac_phase_interface_t *)phase_itf);
}

void wps_radio_init(wps_radio_t *wps_radio, wps_error_t *error)
{
    sr_api_error_t     uwb_err;

    *error = WPS_NO_ERROR;

    uwb_init(&wps_radio->radio,
             &wps_radio->radio_hal,
             wps_radio->spi_rx_buffer,
             wps_radio->spi_tx_buffer,
             WPS_RADIO_SPI_BUFFER_SIZE,
             &uwb_err);

    if (uwb_err != API_ERR_NONE) {
        *error = WPS_RADIO_NOT_INITIALIZED_ERROR;
        return;
    }

    sr_nvm_init(&wps_radio->radio, wps_radio->nvm);
    sr_calibrate(&wps_radio->radio, wps_radio->spectral_calib_vars, wps_radio->nvm);
}

void wps_init_callback_queue(wps_t *wps,
                             wps_callback_inst_t *callback_buffer,
                             size_t size,
                             void (*context_switch)(void))
{
    wps->callback_context_switch = context_switch;
    circular_queue_init(&wps->l7.callback_queue, callback_buffer, size, sizeof(wps_callback_inst_t));
}

void wps_init_request_queue(wps_t *wps, wps_request_info_t *request_buffer, size_t size)
{
    circular_queue_init(&wps->l7.request_queue, request_buffer, size, sizeof(wps_request_info_t));
}

void wps_init(wps_t *wps, wps_node_t *node, wps_error_t *err)
{
    wps_mac_sync_cfg_t mac_sync_cfg = {0};

    *err = WPS_NO_ERROR;

    if (node->radio == NULL) {
        *err = WPS_RADIO_NOT_INITIALIZED_ERROR;
        return;
    } else if (wps->channel_sequence.channel == NULL) {
        *err = WPS_CHANNEL_SEQUENCE_NOT_INITIALIZED_ERROR;
        return;
    }

    wps->node   = node;
    wps->status = WPS_IDLE;
    wps->signal = WPS_NONE;

    mac_sync_cfg.sleep_level      = wps->node->sleep_lvl;
    mac_sync_cfg.isi_mitig        = wps->node->isi_mitig;
    mac_sync_cfg.isi_mitig_pauses = link_tdma_sync_get_isi_mitigation_pauses(mac_sync_cfg.isi_mitig);
    mac_sync_cfg.preamble_len     = link_tdma_get_preamble_length(mac_sync_cfg.isi_mitig_pauses,
                                                                  wps->node->preamble_len);
    mac_sync_cfg.syncword_len     = link_tdma_get_syncword_length(mac_sync_cfg.isi_mitig_pauses,
                                                                  wps->node->syncword_cfg.syncword_length);
    wps_mac_init(&wps->mac,
                 &wps->l7.callback_queue,
                 &wps->schedule,
                 &wps->channel_sequence,
                 &mac_sync_cfg,
                 wps->node->local_address,
                 wps->node->role,
                 wps->random_channel_sequence_enabled,
                 wps->network_id);

    /* Initialize request type */
    initialize_request_queues(wps);
}

void wps_set_syncing_address(wps_t *wps, uint16_t address, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->mac.syncing_address = address;
}

void wps_set_network_id(wps_t *wps, uint8_t network_id, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->network_id = network_id;
}

void wps_config_node(wps_node_t *node, wps_radio_t *radio, wps_node_cfg_t *cfg, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    node->radio                     = radio;
    node->role                      = cfg->role;
    node->local_address             = cfg->local_address;
    node->crc_polynomial            = cfg->crc_polynomial;
    node->preamble_len              = cfg->preamble_len;
    node->sleep_lvl                 = cfg->sleep_lvl;
    node->isi_mitig                 = cfg->isi_mitig;
    node->rx_gain                   = cfg->rx_gain;

    /* Syncword */
    node->syncword_cfg.syncword_length    = cfg->syncword_len;
    node->syncword_cfg.syncword_bit_cost  = 2;
    node->syncword_cfg.syncword_tolerance = 0xC;
    node->syncword_cfg.syncword           = cfg->syncword;
}

void wps_config_network_schedule(wps_t          *wps,
                                 uint32_t       *timeslot_duration_pll_cycles,
                                 timeslot_t     *timeslot,
                                 uint32_t        schedule_size,
                                 wps_error_t    *err)
{
    *err = WPS_NO_ERROR;

    wps->schedule.size = schedule_size;
    wps->schedule.timeslot = timeslot;

    for (uint32_t i = 0; i < schedule_size; ++i) {
        timeslot[i].duration_pll_cycles = timeslot_duration_pll_cycles[i];
    }
}

void wps_reset_schedule(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_scheduler_reset(&wps->mac.scheduler);
}

void wps_config_network_channel_sequence(wps_t *wps,
                                         uint32_t *channel_sequence,
                                         uint32_t sequence_size,
                                         wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->channel_sequence.channel = channel_sequence;
    wps->channel_sequence.sequence_size = sequence_size;
}

void wps_enable_random_channel_sequence(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->random_channel_sequence_enabled = true;
}

void wps_disable_random_channel_sequence(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->random_channel_sequence_enabled = false;
}

uint8_t wps_get_connection_header_size(wps_t *wps, wps_header_cfg_t header_cfg)
{
    uint8_t header_size = 0;

    header_size += header_cfg.main_connection ? wps_mac_get_channel_index_proto_size(&wps->mac) +
                                                wps_mac_get_timeslot_id_saw_proto_size(&wps->mac) : 0;
    header_size += header_cfg.rdo_enabled ? sizeof(wps->mac.link_rdo.offset) : 0;
    header_size += header_cfg.ranging_phase_provider ? wps_mac_get_ranging_phases_proto_size(&wps->mac) : 0;
    header_size += header_cfg.ranging_phase_accumulator ? wps_mac_get_ranging_phase_count_proto_size(&wps->mac) : 0;

    return header_size;
}

void wps_configure_header_connection(wps_t *wps,
                                     wps_connection_t *connection,
                                     wps_header_cfg_t header_cfg,
                                     wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_protocol_init_cfg_t proto_init_cfg;
    link_error_t link_err;

    proto_init_cfg.buffer_size = wps_get_connection_header_size(wps, header_cfg);

    link_protocol_init(&connection->link_protocol, &proto_init_cfg, &link_err);

    link_protocol_cfg_t link_proto_cfg;

    if (header_cfg.main_connection == true) {
        link_proto_cfg.instance = &wps->mac;
        link_proto_cfg.send     = wps_mac_send_timeslot_id_saw;
        link_proto_cfg.receive  = wps_mac_receive_timeslot_id_saw;
        link_proto_cfg.size     = wps_mac_get_timeslot_id_saw_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_cfg, &link_err);

        link_proto_cfg.instance = &wps->mac;
        link_proto_cfg.send     = wps_mac_send_channel_index;
        link_proto_cfg.receive  = wps_mac_receive_channel_index;
        link_proto_cfg.size     = wps_mac_get_channel_index_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_cfg, &link_err);
    }

    if (header_cfg.rdo_enabled == true) {

        link_proto_cfg.instance = &wps->mac;
        link_proto_cfg.send     = wps_mac_send_rdo;
        link_proto_cfg.receive  = wps_mac_receive_rdo;
        link_proto_cfg.size     = wps_mac_get_rdo_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_cfg, &link_err);
    }

    if (header_cfg.ranging_phase_provider == true) {
        link_proto_cfg.instance = &wps->mac;
        link_proto_cfg.send     = wps_mac_send_ranging_phases;
        link_proto_cfg.receive  = wps_mac_receive_ranging_phases;
        link_proto_cfg.size     = wps_mac_get_ranging_phases_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_cfg, &link_err);
    }

    if (header_cfg.ranging_phase_accumulator == true) {
        link_proto_cfg.instance = &wps->mac;
        link_proto_cfg.send     = wps_mac_send_ranging_phase_count;
        link_proto_cfg.receive  = wps_mac_receive_ranging_phase_count;
        link_proto_cfg.size     = wps_mac_get_ranging_phase_count_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_cfg, &link_err);
    }
}

void wps_create_connection(wps_connection_t *connection,
                           wps_node_t *node,
                           wps_connection_cfg_t *config,
                           wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->source_address      = config->source_address;
    connection->destination_address = config->destination_address;
    connection->auto_sync_enable    = true;

    connection->header_size = config->header_length;

    circular_queue_init(&connection->xlayer_queue, config->fifo_buffer, config->fifo_buffer_size, sizeof(xlayer_t));

    if (node->local_address == connection->source_address) {
        /* TX */
        set_tx_conn_queue_memory(config->frame_buffer,
                                 &connection->xlayer_queue,
                                 config->fifo_buffer_size,
                                 config->header_length,
                                 config->frame_length);
    } else {
        /* RX */
        set_rx_conn_queue_memory(config->frame_buffer,
                                 &connection->xlayer_queue,
                                 config->fifo_buffer_size,
                                 config->header_length,
                                 config->frame_length);
    }

    connection->tx_success_callback_t       = NULL;
    connection->tx_fail_callback_t          = NULL;
    connection->tx_drop_callback_t          = NULL;
    connection->rx_success_callback_t       = NULL;
    connection->evt_callback_t              = NULL;
    connection->get_tick_quarter_ms  = config->get_tick_quarter_ms;
    connection->packet_cfg           = DEFAULT_PACKET_CONFIGURATION;
    connection->channel              = config->channel_buffer;

    link_fallback_init(&connection->link_fallback, config->fallback_threshold, config->fallback_count);
}

void wps_connection_set_timeslot(wps_connection_t *connection,
                                 wps_t *network,
                                 int32_t *timeslot_id,
                                 uint32_t nb_timeslots,
                                 wps_error_t *err)
{
    uint32_t id;

    *err = WPS_NO_ERROR;

    for (uint32_t i = 0; i < nb_timeslots; ++i) {
        id = timeslot_id[i];

        if (is_main_timeslot(id)) {
            network->schedule.timeslot[id].connection_main = connection;
        } else {
            id = auto_reply_id_to_id(id);
            network->schedule.timeslot[id].connection_auto_reply = connection;
        }
    }
}

void wps_connection_config_channel(wps_connection_t *connection,
                                   wps_node_t *node,
                                   uint8_t channel_x,
                                   uint8_t fallback_index,
                                   channel_cfg_t *config,
                                   wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        config_spectrum_advance(node->radio[i].spectral_calib_vars, config, &connection->channel[fallback_index][i][channel_x]);
    }
}

void wps_connection_preset_config_channel(wps_connection_t *connection,
                                          wps_node_t *node,
                                          uint16_t frequency,
                                          tx_power_t tx_power,
                                          uint8_t channel_x,
                                          wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    if (WPS_RADIO_COUNT == 1) {
        config_spectrum(node->radio->spectral_calib_vars, frequency, tx_power, &connection->channel[0][0][channel_x]);
    }
}

void wps_connection_config_frame(wps_connection_t *connection,
                                 modulation_t modulation,
                                 fec_level_t fec,
                                 wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->frame_cfg.modulation = modulation;
    connection->frame_cfg.fec = fec;
}

void wps_connection_enable_ack(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->ack_enable = true;
}

void wps_connection_disable_ack(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->ack_enable = false;
}

void wps_connection_enable_phases_aquisition(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->phases_exchange = true;
}

void wps_connection_disable_phases_aquisition(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->phases_exchange = false;
}

void wps_connection_enable_stop_and_wait_arq(wps_connection_t *connection,
                                             uint16_t local_address,
                                             uint32_t retry,
                                             uint32_t deadline_quarter_ms,
                                             wps_error_t *err)
{
    bool board_seq;

    *err = WPS_NO_ERROR;

    if (connection->ack_enable == false) {
        *err = WPS_ACK_DISABLED_ERROR;
        return;
    }

    if (local_address == connection->destination_address) {
        board_seq = true;
    } else {
        board_seq = false;
    }

    link_saw_arq_init(&connection->stop_and_wait_arq, deadline_quarter_ms, retry, board_seq, true);
}

void wps_connection_disable_stop_and_wait_arq(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_saw_arq_init(&connection->stop_and_wait_arq, 0, 0, false, false);
}

void wps_connection_enable_auto_sync(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->auto_sync_enable = true;
}

void wps_connection_disable_auto_sync(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->auto_sync_enable = false;
}

void wps_connection_enable_fixed_payload_size(wps_connection_t *connection,
                                              uint8_t fixed_payload_size,
                                              wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->fixed_payload_size_enable = true;
    connection->fixed_payload_size        = fixed_payload_size;
    connection->packet_cfg               |= ENABLE_FIXED_PAYLOAD_SIZE;
}

void wps_connection_disable_fixed_payload_size(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->fixed_payload_size_enable = false;
    connection->packet_cfg               |= DISABLE_FIXED_PAYLOAD_SIZE;
}

void wps_connection_enable_cca(wps_connection_t *connection,
                               uint8_t threshold,
                               uint16_t retry_time_pll_cycles,
                               uint8_t try_count,
                               cca_fail_action_t fail_action,
                               wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    link_cca_init(&connection->cca, threshold, retry_time_pll_cycles, CCA_ON_TIME_PLL_CYCLE, try_count, fail_action, true);
}

void wps_connection_disable_cca(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    link_cca_init(&connection->cca, 0xff, 0, 0, 0, CCA_FAIL_ACTION_TX, false);
}

void wps_connection_enable_fixed_gain(wps_connection_t *connection, uint8_t rx_gain, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    for (uint8_t i = 0; i < WPS_NB_RF_CHANNEL; i++) {
        for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
            link_gain_loop_init(&connection->gain_loop[i][j], true, rx_gain);
        }
    }
}

void wps_connection_disable_fixed_gain(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    for (uint8_t i = 0; i < WPS_NB_RF_CHANNEL; i++) {
        for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
            link_gain_loop_init(&connection->gain_loop[i][j], false, 0);
        }
    }
}

void wps_enable_rdo(wps_t *wps, uint16_t rollover_value, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_rdo_init(&wps->mac.link_rdo, rollover_value);
}

void wps_disable_rdo(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->mac.link_rdo.enabled = false;
}

void wps_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    if (connection != NULL) {
        connection->tx_success_callback_t = callback;
        connection->tx_success_parg_callback_t = parg;
    }
}

void wps_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    if (connection != NULL) {
        connection->tx_fail_callback_t = callback;
        connection->tx_fail_parg_callback_t = parg;
    }
}

void wps_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    if (connection != NULL) {
        connection->tx_drop_callback_t = callback;
        connection->tx_drop_parg_callback_t = parg;
    }
}

void wps_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    if (connection != NULL) {
        connection->rx_success_callback_t = callback;
        connection->rx_success_parg_callback_t = parg;
    }
}

void wps_set_event_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    if (connection != NULL) {
        connection->evt_callback_t = callback;
        connection->evt_parg_callback_t = parg;
    }
}

void wps_connect(wps_t *wps, wps_error_t *err)
{
    wps_phy_cfg_t phy_cfg = {0};

    *err = WPS_NO_ERROR;

    if (!((wps->signal == WPS_DISCONNECT) || (wps->signal == WPS_NONE))) {
        *err = WPS_ALREADY_CONNECTED_ERROR;
        return;
    }

    wps->signal = WPS_CONNECT;

    if (WPS_RADIO_COUNT == 1) {
        wps->node->radio->radio_hal.reset_reset_pin();
        wps->node->radio->radio_hal.reset_shutdown_pin();
        wps->node->radio->radio_hal.delay_ms(25);
        wps->node->radio->radio_hal.set_reset_pin();
    }
    wps->node->radio->radio_hal.delay_ms(100);

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_cfg.radio                     = &wps->node->radio[i].radio;
        phy_cfg.local_address             = wps->node->local_address;
        phy_cfg.syncword_cfg              = wps->node->syncword_cfg;
        phy_cfg.preamble_len              = wps->node->preamble_len;
        phy_cfg.sleep_lvl                 = wps->node->sleep_lvl;
        phy_cfg.crc_polynomial            = wps->node->crc_polynomial;
        phy_cfg.rx_gain                   = wps->node->rx_gain;

        wps_phy_init(&wps->phy[i], &phy_cfg);
    }

    wps_process_init(wps);
    wps_mac_reset(&wps->mac);
    wps_phy_connect(wps->phy);

}

void wps_disconnect(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    if (wps->signal == WPS_DISCONNECT) {
        *err = WPS_ALREADY_DISCONNECTED_ERROR;
        return;
    }

    wps_phy_disconnect(wps->phy);
    wps->signal = WPS_DISCONNECT;
}

void wps_reset(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    if (wps->signal == WPS_DISCONNECT) {
        *err = WPS_ALREADY_DISCONNECTED_ERROR;
        return;
    }

    wps_disconnect(wps, err);
    wps_connect(wps, err);
}

void wps_halt(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    (void)wps;
}

void wps_resume(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    (void)wps;
}

void wps_init_connection_throttle(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    if (connection->pattern != NULL) {
        memset(connection->pattern, 1, WPS_CONNECTION_THROTTLE_GRANULARITY * sizeof(bool));
        connection->pattern_count = 0;
        connection->pattern_total_count = WPS_CONNECTION_THROTTLE_GRANULARITY;
        connection->active_ratio = 100;
    }
}

void wps_set_active_ratio(wps_t *wps, wps_connection_t *connection, uint8_t ratio_percent, wps_error_t *err)
{
    wps_request_info_t *request;
    bool *pattern = circular_queue_get_free_slot(wps->throttle_cfg.pattern_queue);

    *err = WPS_NO_ERROR;

    if (pattern == NULL) {
        *err = WPS_CONN_THROTTLE_NOT_INITIALIZED_ERROR;
    }

    wps->throttle_cfg.active_ratio          = ratio_percent;
    wps->throttle_cfg.pattern_total_count   = generate_active_pattern(pattern, ratio_percent);
    wps->throttle_cfg.pattern_current_count = 0;
    wps->throttle_cfg.target_conn           = connection;
    request                                 = circular_queue_get_free_slot(&wps->l7.request_queue);

    if (request == NULL) {
        *err = WPS_CONN_THROTTLE_NOT_INITIALIZED_ERROR;
    }

    request->config = &wps->throttle_cfg;
    request->type   = REQUEST_MAC_CHANGE_SCHEDULE_RATIO;
    circular_queue_enqueue(wps->throttle_cfg.pattern_queue);
    circular_queue_enqueue(&wps->l7.request_queue);
}

void wps_get_free_slot(wps_connection_t *connection, uint8_t **payload, wps_error_t *err)
{
    xlayer_t *frame = circular_queue_get_free_slot(&connection->xlayer_queue);

    *err = WPS_NO_ERROR;

    if (frame == NULL) {
        *err = WPS_QUEUE_FULL_ERROR;
        return;
    }

    *payload = frame->frame.payload_begin_it;
}

void wps_send(wps_connection_t *connection, uint8_t *payload, uint8_t size, wps_error_t *err)
{
    xlayer_t *frame = circular_queue_get_free_slot(&connection->xlayer_queue);

    *err = WPS_NO_ERROR;

    if (connection->fixed_payload_size_enable && (connection->fixed_payload_size != size)) {
        *err = WPS_WRONG_TX_SIZE_ERROR;
        return;
    }

    if (frame) {
        frame->frame.retry_count         = 0;
        frame->frame.time_stamp          = connection->get_tick_quarter_ms();
        frame->frame.payload_memory      = payload;
        frame->frame.payload_memory_size = size;
        frame->frame.payload_begin_it    = payload;
        frame->frame.payload_end_it      = payload + size;

        circular_queue_enqueue(&connection->xlayer_queue);
    } else {
        *err = WPS_QUEUE_FULL_ERROR;
        return;
    }
}

wps_rx_frame wps_read(wps_connection_t *connection, wps_error_t *err)
{
    wps_rx_frame frame_out;

    *err = WPS_NO_ERROR;

    if (circular_queue_is_empty(&connection->xlayer_queue)) {
        *err              = WPS_QUEUE_EMPTY_ERROR;
        frame_out.payload = NULL;
        frame_out.size = 0;
        return frame_out;
    }

    xlayer_t *frame = circular_queue_front(&connection->xlayer_queue);

    frame_out.payload = (frame->frame.payload_begin_it);
    frame_out.size = frame->frame.payload_end_it - frame->frame.payload_begin_it;

    return frame_out;
}

void wps_read_done(wps_connection_t *connection, wps_error_t *err)
{
    *err = circular_queue_dequeue(&connection->xlayer_queue) ? WPS_NO_ERROR : WPS_QUEUE_EMPTY_ERROR;
}

uint32_t wps_get_fifo_size(wps_connection_t *connection)
{
    return circular_queue_size(&connection->xlayer_queue);
}

uint32_t wps_get_fifo_free_space(wps_connection_t *connection)
{
    return circular_queue_free_space(&connection->xlayer_queue);
}

wps_error_t wps_get_error(wps_connection_t *connection)
{
    return connection->wps_error;
}

void wps_request_write_register(wps_t *wps, uint8_t starting_reg, uint8_t data, wps_error_t *err)
{
    wps_request_info_t *request;
    wps_write_request_info_t *write_request = circular_queue_get_free_slot(wps->write_request_queue);

    *err = WPS_NO_ERROR;

    if (write_request != NULL) {
        request = circular_queue_get_free_slot(&wps->l7.request_queue);
        if (request != NULL) {
            request->config = write_request;
            request->type   = REQUEST_PHY_WRITE_REG;
            circular_queue_enqueue(&wps->l7.request_queue);

            write_request->target_register = starting_reg;
            write_request->data            = data;
            circular_queue_enqueue(wps->write_request_queue);
        } else {
            *err = WPS_REQUEST_QUEUE_FULL;
        }
    } else {
        *err = WPS_WRITE_REQUEST_QUEUE_FULL;
    }
}

void wps_request_read_register(wps_t *wps, uint8_t target_register, uint8_t *rx_buffer, bool *xfer_cmplt, wps_error_t *err)
{
    wps_request_info_t *request;
    wps_read_request_info_t *read_request = circular_queue_get_free_slot(wps->read_request_queue);

    *err = WPS_NO_ERROR;

    if (read_request != NULL) {
        request = circular_queue_get_free_slot(&wps->l7.request_queue);
        if (request != NULL) {
            request->config = read_request;
            request->type   = REQUEST_PHY_READ_REG;
            circular_queue_enqueue(&wps->l7.request_queue);

            *xfer_cmplt                   = false;
            read_request->rx_buffer       = rx_buffer;
            read_request->target_register = target_register;
            read_request->xfer_cmplt      = xfer_cmplt;
            circular_queue_enqueue(wps->read_request_queue);
        } else {
            *err = WPS_REQUEST_QUEUE_FULL;
        }
    } else {
        *err = WPS_READ_REQUEST_QUEUE_FULL;
    }
}

#if WPS_RADIO_COUNT == 1

void wps_enable_fast_sync(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_mac_enable_fast_sync(&wps->mac);
}

void wps_disable_fast_sync(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_mac_disable_fast_sync(&wps->mac);
}

#elif WPS_RADIO_COUNT > 1

void wps_multi_init(wps_multi_cfg_t multi_cfg, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_multi_radio_init(multi_cfg);
}

#endif

/* PRIVATE FUNCTION ***********************************************************/
/** @brief Check if ID is main or auto timeslot
 *
 * @retval false  Connection is on auto-reply.
 * @retval true   Connection is on main.
 */
static bool is_main_timeslot(int8_t id)
{
    if (id & (BIT_AUTO_REPLY_TIMESLOT)) {
        return false;
    } else {
        return true;
    }
}

/** @brief Convert auto reply ID to timeslot ID.
 *
 * @return Timeslot ID.
 */
static uint32_t auto_reply_id_to_id(int8_t id)
{
    return (id & TIMESLOT_VALUE_MASK);
}

/** @brief Convert auto-reply ID to timeslot ID.
 *
 * @param[in] buffer        Frame's header buffer, must be 2D array [queue size][max header size].
 * @param[in] queue         Cross layer queue.
 * @param[in] queue_size    Cross layer queue size.
 * @param[in] hdr_len       Length of the WPS header
 * @param[in] max_frame_len Total frame buffer length (Header + Payload).
 */
static void set_tx_conn_queue_memory(uint8_t *buffer,
                                     circular_queue_t *queue,
                                     uint32_t queue_size,
                                     uint32_t hdr_len,
                                     uint32_t max_frame_len)
{
    xlayer_t *buffer_begin = (xlayer_t *)queue->buffer_begin;

    if (max_frame_len > hdr_len) {
        for (uint32_t i = 0; i < queue_size; ++i) {
            buffer_begin[i].frame.payload_memory      = (buffer + i * max_frame_len);
            buffer_begin[i].frame.payload_memory_size = max_frame_len - hdr_len;
            buffer_begin[i].frame.payload_begin_it    = buffer_begin[i].frame.payload_memory + hdr_len;
        }
    }

    for (uint32_t i = 0; i < queue_size; ++i) {
        buffer_begin[i].frame.header_memory      = (buffer + i * max_frame_len);
        buffer_begin[i].frame.header_memory_size = hdr_len;
        buffer_begin[i].frame.header_end_it      = (buffer + i * max_frame_len + hdr_len);
        buffer_begin[i].frame.header_begin_it    = buffer_begin[i].frame.header_end_it;
    }
}

/** @brief Convert auto-reply ID to timeslot ID.
 *
 * @param[in] buffer         Frame's header + payload buffer, must be 2D array [queue size][max header + payload size].
 * @param[in] queue          Cross layer queue.
 * @param[in] queue_size     Cross layer queue size.
 * @param[in] max_frame_len  Total frame buffer length (Header + Payload).
 */
static void set_rx_conn_queue_memory(uint8_t *buffer,
                                     circular_queue_t *queue,
                                     uint32_t queue_size,
                                     uint32_t hdr_len,
                                     uint32_t max_frame_len)
{
    xlayer_t *buffer_begin = (xlayer_t *)queue->buffer_begin;

    for (uint32_t i = 0; i < queue_size; ++i) {
        buffer_begin[i].frame.payload_memory      = (buffer + i * max_frame_len);
        buffer_begin[i].frame.payload_memory_size = max_frame_len;
        buffer_begin[i].frame.payload_begin_it    = buffer_begin[i].frame.payload_memory;

        buffer_begin[i].frame.header_memory      = buffer_begin[i].frame.payload_memory;
        buffer_begin[i].frame.header_memory_size = hdr_len;

        buffer_begin[i].frame.header_begin_it = buffer_begin[i].frame.payload_memory;
        buffer_begin[i].frame.header_end_it   = buffer_begin[i].frame.payload_memory;
    }
}

/** @brief Generate active pattern based on given ratio.
 *
 *  @note This will generate a bool array that properly
 *        distribute 1 and 0 through all the array size.
 *
 *  @note Number of active timeslot is the nominator of
 *        the reduced fraction (active_ratio / 100).
 *
 *  @note Total pattern size is the denominator of the
 *        reduced fraction (active_ratio / 100).
 *
 *
 *  @param[in] pattern       Allocated bool pattern array. Size should be WPS_CONNECTION_THROTTLE_GRANULARITY.
 *  @param[in] active_ratio  Active timeslot ratio, in percent.
 *  @return Total pattern size.
 */
static uint8_t generate_active_pattern(bool *pattern, uint8_t active_ratio)
{
    uint8_t current_gcd         = wps_utils_gcd(active_ratio, PERCENT_DENOMINATOR);
    uint8_t active_elements     = active_ratio / current_gcd;
    uint8_t total_number_of_val = PERCENT_DENOMINATOR / current_gcd;
    uint16_t pos                = 0;

    memset(pattern, 0, total_number_of_val);

    for (uint8_t i = 0; i < active_elements; i++) {
        pos = ((i * total_number_of_val) / active_elements);
        pattern[pos % total_number_of_val] = 1;
    }

    return total_number_of_val;

}

static void initialize_request_queues(wps_t *wps)
{
    /* Initialize pattern queue for throttling */
    memset(pattern_cfg, 1, sizeof(pattern_cfg));
    wps->throttle_cfg.pattern_queue = &pattern_cfg_queue;
    circular_queue_init(wps->throttle_cfg.pattern_queue,
                        pattern_cfg,
                        WPS_REQUEST_MEMORY_SIZE,
                        sizeof(bool) * WPS_CONNECTION_THROTTLE_GRANULARITY);

    /* Initialize write request buffer and queue */
    memset(write_request_buffer, 0, sizeof(write_request_buffer));
    wps->write_request_queue = &write_request_queue;
    circular_queue_init(wps->write_request_queue,
                        write_request_buffer,
                        WPS_REQUEST_MEMORY_SIZE,
                        sizeof(wps_write_request_info_t));

    /* Initialize read request buffer and queue */
    memset(read_request_buffer, 0, sizeof(read_request_buffer));
    wps->read_request_queue = &read_request_queue;
    circular_queue_init(wps->read_request_queue,
                        read_request_buffer,
                        WPS_REQUEST_MEMORY_SIZE,
                        sizeof(wps_read_request_info_t));
}
