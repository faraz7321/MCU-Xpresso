/** @file  swc_api.c
 *  @brief SPARK Wireless Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_api.h"
#include "mem_pool.h"
#include "swc_error.h"
#include "wps.h"
#include "wps_stats.h"

/* CONSTANTS ******************************************************************/
#define WPS_DEFAULT_PREAMBLE_LEN         94
#define WPS_DEFAULT_CRC                  0xBAAD
#define WPS_DEFAULT_SYNC_WORD_LEN        SYNCWORD_LENGTH_32
#define WPS_DEFAULT_CALLBACK_QUEUE_SIZE  20
#define WPS_DEFAULT_FREQ_SHIFT           true
#define WPS_DEFAULT_PULSE_START_POS      2
#define WPS_DEFAULT_RND_PHASE            RND_PHASE_ENABLE
#define WPS_DEFAULT_PULSE_SPACING        1
#define WPS_DEFAULT_RDO_ROLLOVER_VAL     15

#define WPS_INTEGGAIN_ONE_PULSE_VAL      1
#define WPS_INTEGGAIN_MANY_PULSES_VAL    0

/* MACROS *********************************************************************/
#define HW_ADDR(net_id, node_id) ((net_id << 8) | node_id)
#define NET_ID_FROM_PAN_ID(pan_id) (pan_id & 0x0ff)
#define SYNCWORD_ID_FROM_PAN_ID(pan_id) ((pan_id & 0xf00) >> 8)

#define MEM_ALLOC_CHECK_RETURN_NULL(ptr, size, err) { \
    ptr = mem_pool_malloc(&mem_pool, size); \
    if (ptr == NULL) { \
        *err = SWC_ERR_NOT_ENOUGH_MEMORY; \
        return NULL; \
    } \
}
#define MEM_ALLOC_CHECK_RETURN_VOID(ptr, size, err) { \
    ptr = mem_pool_malloc(&mem_pool, size); \
    if (ptr == NULL) { \
        *err = SWC_ERR_NOT_ENOUGH_MEMORY; \
        return; \
    } \
}

/* PRIVATE GLOBALS ************************************************************/
static wps_t wps;
static mem_pool_t mem_pool;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool has_main_timeslot(int32_t *timeslot_id, uint32_t timeslot_count);

/* PUBLIC FUNCTIONS ***********************************************************/
void swc_init(swc_cfg_t cfg, swc_hal_t *hal, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    uint32_t timeslot_sequence_pll_cycle[cfg.timeslot_sequence_length];
    timeslot_t *timeslots;
    wps_callback_inst_t *callback_queue;
    wps_request_info_t *request;

    mem_pool_init(&mem_pool, cfg.memory_pool, (size_t)cfg.memory_pool_size);

    MEM_ALLOC_CHECK_RETURN_VOID(timeslots, sizeof(timeslot_t) * cfg.timeslot_sequence_length, err);
    MEM_ALLOC_CHECK_RETURN_VOID(callback_queue, sizeof(wps_callback_inst_t) * WPS_DEFAULT_CALLBACK_QUEUE_SIZE, err);
    MEM_ALLOC_CHECK_RETURN_VOID(request, sizeof(wps_request_info_t) * WPS_REQUEST_MEMORY_SIZE, err);

    /* Initialize the callback queue which will be used to accumulate and run the WPS callbacks asynchronously */
    wps_init_callback_queue(&wps, callback_queue, WPS_DEFAULT_CALLBACK_QUEUE_SIZE, hal->context_switch);

    /* Initialize the request queue which will be used to accumulate request from the application to the WPS */
    wps_init_request_queue(&wps, request, WPS_REQUEST_MEMORY_SIZE);

#if WPS_RADIO_COUNT == 2
    /* Initialize MCU timer functions used for timing when in dual radio configuration */
    wps_multi_init(hal->multi_cfg, &wps_err);
#endif

    for (uint32_t i = 0; i < cfg.timeslot_sequence_length; i++) {
        timeslot_sequence_pll_cycle[i] = wps_us_to_pll_cycle(cfg.timeslot_sequence[i], CHIP_RATE_20_48_MHZ);
    }

    wps_config_network_schedule(&wps, timeslot_sequence_pll_cycle, timeslots, cfg.timeslot_sequence_length, &wps_err);
    wps_config_network_channel_sequence(&wps, cfg.channel_sequence, cfg.channel_sequence_length, &wps_err);

    /* Enable/disable global miscellaneous WPS features */
#if (WPS_RADIO_COUNT == 1)
    if (cfg.fast_sync_enabled) {
        wps_enable_fast_sync(&wps, &wps_err);
    } else {
        wps_disable_fast_sync(&wps, &wps_err);
    }
#endif
    if (cfg.random_channel_sequence_enabled) {
        wps_enable_random_channel_sequence(&wps, &wps_err);
    } else {
        wps_disable_random_channel_sequence(&wps, &wps_err);
    }

    /* RDO at the WPS level can always be enabled but it will only
     * do something if RDO is also enabled on a connection.
     */
    wps_enable_rdo(&wps, WPS_DEFAULT_RDO_ROLLOVER_VAL, &wps_err);
}

swc_node_t *swc_node_init(swc_node_cfg_t cfg, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_node_cfg_t wps_node_cfg;
    swc_node_t *node;

    *err = SWC_ERR_NONE;

    MEM_ALLOC_CHECK_RETURN_NULL(node, sizeof(swc_node_t), err);
    MEM_ALLOC_CHECK_RETURN_NULL(node->wps_node_handle, sizeof(wps_node_t), err);
    MEM_ALLOC_CHECK_RETURN_NULL(node->wps_radio_handle, sizeof(wps_radio_t) * WPS_RADIO_COUNT, err);

    node->radio_count = 0;
    node->cfg = cfg;

    wps_node_cfg.role           = cfg.role;
    wps_node_cfg.preamble_len   = WPS_DEFAULT_PREAMBLE_LEN;
    wps_node_cfg.sleep_lvl      = cfg.sleep_level;
    wps_node_cfg.crc_polynomial = WPS_DEFAULT_CRC;
    wps_node_cfg.local_address  = HW_ADDR(NET_ID_FROM_PAN_ID(cfg.pan_id), cfg.local_address);
    wps_node_cfg.syncword       = sync_word_table[SYNCWORD_ID_FROM_PAN_ID(cfg.pan_id)];
    wps_node_cfg.syncword_len   = WPS_DEFAULT_SYNC_WORD_LEN;

    wps_set_network_id(&wps, NET_ID_FROM_PAN_ID(cfg.pan_id), &wps_err);
    wps_set_syncing_address(&wps, HW_ADDR(NET_ID_FROM_PAN_ID(cfg.pan_id), cfg.coordinator_address), &wps_err);
    wps_config_node(node->wps_node_handle, node->wps_radio_handle, &wps_node_cfg, &wps_err);

    return node;
}

void swc_node_add_radio(swc_node_t *node, swc_radio_cfg_t cfg, swc_hal_t *hal, swc_error_t *err)
{
    wps_error_t wps_err;
    uint8_t radio_id = node->radio_count;

    *err = SWC_ERR_NONE;

    node->wps_radio_handle[radio_id].radio_hal          = hal->radio_hal[radio_id];
    node->wps_radio_handle[radio_id].radio.irq_polarity = cfg.irq_polarity;
    node->wps_radio_handle[radio_id].radio.std_spi      = cfg.std_spi;

    MEM_ALLOC_CHECK_RETURN_VOID(node->wps_radio_handle[radio_id].nvm, sizeof(nvm_t), err);
    MEM_ALLOC_CHECK_RETURN_VOID(node->wps_radio_handle[radio_id].spectral_calib_vars, sizeof(calib_vars_t), err);

    /* Disable MCU external interrupt servicing the radio IRQ before initializing the WPS.
     * It will be later re-activated with a call to the swc_connect() function.
     */
    hal->radio_hal[radio_id].disable_radio_irq();

    wps_radio_init(&node->wps_radio_handle[radio_id], &wps_err);

    node->radio_count++;
}

swc_connection_t *swc_connection_init(swc_node_t *node, swc_connection_cfg_t cfg, swc_hal_t *hal, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_connection_cfg_t wps_conn_cfg;
    wps_header_cfg_t wps_header_cfg;
    uint8_t header_size;
    uint8_t conn_frame_length;
    uint8_t threshold_count = 0;
    uint8_t *frame_queue;
    uint8_t *fallback_threshold = NULL;
    swc_connection_t *conn;
    xlayer_t *xlayer_queue;
    rf_channel_t (*channel_buffer)[WPS_NB_RF_CHANNEL][WPS_RADIO_COUNT];

    *err = SWC_ERR_NONE;

    /* Set to true if a connection has at least one main timeslot */
    wps_header_cfg.main_connection = has_main_timeslot(cfg.timeslot_id, cfg.timeslot_count);

    wps_header_cfg.rdo_enabled = cfg.rdo_enabled;

    /* Wireless Core API does not support ranging yet, so hardcode to false for now */
    wps_header_cfg.ranging_phase_accumulator = false;
    wps_header_cfg.ranging_phase_provider = false;

    header_size = wps_get_connection_header_size(&wps, wps_header_cfg);
    conn_frame_length = cfg.allocate_payload_memory ? cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE :
                                                      header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE;

    /* Allocate memory */
    MEM_ALLOC_CHECK_RETURN_NULL(conn, sizeof(swc_connection_t), err);
    MEM_ALLOC_CHECK_RETURN_NULL(conn->wps_conn_handle, sizeof(wps_connection_t), err);
    MEM_ALLOC_CHECK_RETURN_NULL(xlayer_queue, sizeof(xlayer_t) * cfg.queue_size, err);
    MEM_ALLOC_CHECK_RETURN_NULL(frame_queue, sizeof(uint8_t) * cfg.queue_size * conn_frame_length, err);
    if (cfg.fallback_enabled) {
        threshold_count = 1;
        MEM_ALLOC_CHECK_RETURN_NULL(fallback_threshold, sizeof(uint8_t) * threshold_count, err);
        memcpy(fallback_threshold, &cfg.fallback_settings.threshold, threshold_count);
    }
    if (cfg.throttling_enabled) {
        MEM_ALLOC_CHECK_RETURN_NULL(conn->wps_conn_handle->pattern, sizeof(bool) * WPS_CONNECTION_THROTTLE_GRANULARITY, err);
    }
    MEM_ALLOC_CHECK_RETURN_NULL(channel_buffer, sizeof(rf_channel_t[WPS_NB_RF_CHANNEL][WPS_RADIO_COUNT]) * (threshold_count + 1), err);

    conn->channel_count = 0;
    conn->cfg = cfg;

    wps_conn_cfg.source_address       = HW_ADDR(NET_ID_FROM_PAN_ID(node->cfg.pan_id), cfg.source_address);
    wps_conn_cfg.destination_address  = HW_ADDR(NET_ID_FROM_PAN_ID(node->cfg.pan_id), cfg.destination_address);
    wps_conn_cfg.header_length        = header_size;
    wps_conn_cfg.frame_length         = conn_frame_length;
    wps_conn_cfg.get_tick_quarter_ms  = hal->get_tick_quarter_ms;
    wps_conn_cfg.fifo_buffer          = xlayer_queue;
    wps_conn_cfg.fifo_buffer_size     = cfg.queue_size;
    wps_conn_cfg.frame_buffer         = frame_queue;
    wps_conn_cfg.fallback_count       = threshold_count;
    wps_conn_cfg.fallback_threshold   = fallback_threshold;
    wps_conn_cfg.channel_buffer       = channel_buffer;

    wps_create_connection(conn->wps_conn_handle, node->wps_node_handle, &wps_conn_cfg, &wps_err);

    wps_connection_config_frame(conn->wps_conn_handle, cfg.modulation, cfg.fec, &wps_err);

    wps_connection_set_timeslot(conn->wps_conn_handle, &wps, cfg.timeslot_id, cfg.timeslot_count, &wps_err);

    if (cfg.ack_enabled) {
        wps_connection_enable_ack(conn->wps_conn_handle, &wps_err);
    } else {
        wps_connection_disable_ack(conn->wps_conn_handle, &wps_err);
    }

    if (cfg.arq_enabled) {
        wps_connection_enable_stop_and_wait_arq(conn->wps_conn_handle, node->wps_node_handle->local_address,
                                                cfg.arq_settings.try_count, cfg.arq_settings.time_deadline, &wps_err);
    } else {
        wps_connection_disable_stop_and_wait_arq(conn->wps_conn_handle, &wps_err);
    }

    if (cfg.auto_sync_enabled) {
        wps_connection_enable_auto_sync(conn->wps_conn_handle, &wps_err);
    } else {
        wps_connection_disable_auto_sync(conn->wps_conn_handle, &wps_err);
    }

    if (cfg.cca_enabled) {
        wps_connection_enable_cca(conn->wps_conn_handle, conn->cfg.cca_settings.threshold, conn->cfg.cca_settings.retry_time,
                                  conn->cfg.cca_settings.try_count, conn->cfg.cca_settings.fail_action, &wps_err);
    } else {
        wps_connection_disable_cca(conn->wps_conn_handle, &wps_err);
    }

    if (cfg.throttling_enabled) {
        wps_init_connection_throttle(conn->wps_conn_handle, &wps_err);
    }

    wps_configure_header_connection(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);

    return conn;
}

void swc_connection_add_channel(swc_connection_t *conn, swc_node_t *node, swc_channel_cfg_t cfg, swc_error_t *err)
{
    wps_error_t wps_err;
    channel_cfg_t wps_chann_cfg;

    *err = SWC_ERR_NONE;

    MEM_ALLOC_CHECK_RETURN_VOID(wps_chann_cfg.power, sizeof(tx_power_settings_t), err);

    /* Configure RF channels the connection will use */
    wps_chann_cfg.frequency          = cfg.frequency;
    wps_chann_cfg.power->pulse_count = cfg.tx_pulse_count;
    wps_chann_cfg.power->pulse_width = cfg.tx_pulse_width;
    wps_chann_cfg.power->tx_gain     = cfg.tx_pulse_gain;
    wps_chann_cfg.freq_shift_enable  = WPS_DEFAULT_FREQ_SHIFT;
    wps_chann_cfg.pulse_spacing      = WPS_DEFAULT_PULSE_SPACING;
    wps_chann_cfg.pulse_start_pos    = WPS_DEFAULT_PULSE_START_POS;
    wps_chann_cfg.rdn_phase_enable   = WPS_DEFAULT_RND_PHASE;
    wps_chann_cfg.integgain          = (cfg.rx_pulse_count == 1) ? WPS_INTEGGAIN_ONE_PULSE_VAL : WPS_INTEGGAIN_MANY_PULSES_VAL;

    wps_connection_config_channel(conn->wps_conn_handle, node->wps_node_handle, conn->channel_count, 0, &wps_chann_cfg, &wps_err);

    if (conn->cfg.fallback_enabled) {
        wps_chann_cfg.power->pulse_count += conn->cfg.fallback_settings.tx_pulse_count_offset;
        wps_chann_cfg.power->pulse_width += conn->cfg.fallback_settings.tx_pulse_width_offset;
        wps_chann_cfg.power->tx_gain     += conn->cfg.fallback_settings.tx_pulse_gain_offset;
        wps_connection_config_channel(conn->wps_conn_handle, node->wps_node_handle, conn->channel_count, 1, &wps_chann_cfg, &wps_err);
    }

    conn->channel_count++;
}

void swc_connection_set_tx_success_callback(swc_connection_t *conn, void (*cb)(void *conn))
{
    conn->wps_conn_handle->tx_success_callback_t = cb;
    conn->wps_conn_handle->tx_success_parg_callback_t = conn;
}

void swc_connection_set_tx_fail_callback(swc_connection_t *conn, void (*cb)(void *conn))
{
    conn->wps_conn_handle->tx_fail_callback_t = cb;
    conn->wps_conn_handle->tx_fail_parg_callback_t = conn;
}

void swc_connection_set_tx_dropped_callback(swc_connection_t *conn, void (*cb)(void *conn))
{
    conn->wps_conn_handle->tx_drop_callback_t = cb;
    conn->wps_conn_handle->tx_drop_parg_callback_t = conn;
}

void swc_connection_set_rx_success_callback(swc_connection_t *conn, void (*cb)(void *conn))
{
    conn->wps_conn_handle->rx_success_callback_t = cb;
    conn->wps_conn_handle->rx_success_parg_callback_t = conn;
}

void swc_connection_set_throttling_active_ratio(swc_connection_t *conn, uint8_t active_ratio, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    wps_set_active_ratio(&wps, conn->wps_conn_handle, active_ratio, &wps_err);
}

void swc_connection_get_payload_buffer(swc_connection_t *conn, uint8_t **payload_buffer, swc_error_t *err)
{
    wps_error_t wps_err;

   *err = SWC_ERR_NONE;

    wps_get_free_slot(conn->wps_conn_handle, payload_buffer, &wps_err);
    if (wps_err != WPS_NO_ERROR) {
        *payload_buffer = NULL;
    }
}

void swc_connection_send(swc_connection_t *conn, uint8_t *payload_buffer, uint8_t size, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    wps_send(conn->wps_conn_handle, payload_buffer, size, &wps_err);
}

uint8_t swc_connection_receive(swc_connection_t *conn, uint8_t **payload, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_rx_frame frame;

    *err = SWC_ERR_NONE;

    frame = wps_read(conn->wps_conn_handle, &wps_err);
    *payload = frame.payload;

    return frame.size;
}

void swc_connection_receive_complete(swc_connection_t *conn, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    wps_read_done(conn->wps_conn_handle, &wps_err);
}

void swc_setup(swc_node_t *node)
{
    wps_error_t wps_err;

    wps_init(&wps, node->wps_node_handle, &wps_err);
}

void swc_connect(void)
{
    wps_error_t wps_err;

    wps_connect(&wps, &wps_err);
}

void swc_disconnect(void)
{
    wps_error_t wps_err;

    wps_disconnect(&wps, &wps_err);
}

swc_fallback_info_t swc_connection_get_fallback_info(swc_connection_t *conn)
{
    swc_fallback_info_t info;

    info.link_margin = wps_stats_get_inst_phy_margin(conn->wps_conn_handle);

    return info;
}

uint32_t swc_get_allocated_bytes(void)
{
    return mem_pool_get_allocated_bytes(&mem_pool);
}

void swc_free_memory(void)
{
    mem_pool_free(&mem_pool);
}

void swc_connection_callbacks_processing_handler(void)
{
    wps_process_callback(&wps);
}

#if (WPS_RADIO_COUNT == 1)
void swc_radio_irq_handler(void)
{
    wps_radio_irq(&wps);
    wps_process(&wps);
}

void swc_radio_spi_receive_complete_handler(void)
{
    wps_transfer_complete(&wps);
    wps_process(&wps);
}
#elif (WPS_RADIO_COUNT == 2)
void swc_radio1_irq_handler(void)
{
    wps_set_irq_index(0);
    wps_radio_irq(&wps);
    wps_process(&wps);
}

void swc_radio1_spi_receive_complete_handler(void)
{
    wps_set_irq_index(0);
    wps_transfer_complete(&wps);
    wps_process(&wps);
}

void swc_radio2_irq_handler(void)
{
    wps_set_irq_index(1);
    wps_radio_irq(&wps);
    wps_process(&wps);
}

void swc_radio2_spi_receive_complete_handler(void)
{
    wps_set_irq_index(1);
    wps_transfer_complete(&wps);
    wps_process(&wps);
}

void swc_radio_synchronization_timer_callback(void)
{
    wps_multi_radio_timer_process(&wps);
}
#else
#error "Number of radios must be either 1 or 2"
#endif

/** @brief Check if at least one timeslot is a main timeslot.
 *
 *  @param[in] timeslot_id     ID of timeslots used by the connection.
 *  @param[in] timeslot_count  Number of timeslots used by the connection.
 *  @retval true  There is a least one main timeslot.
 *  @retval false There is no main timeslots.
 */
static bool has_main_timeslot(int32_t *timeslot_id, uint32_t timeslot_count)
{
    bool main_timeslot = false;

    for (uint32_t i = 0; i < timeslot_count; i++) {
        main_timeslot = !(timeslot_id[i] & BIT_AUTO_REPLY_TIMESLOT);
        if (main_timeslot) {
            break;
        }
    }

    return main_timeslot;
}

