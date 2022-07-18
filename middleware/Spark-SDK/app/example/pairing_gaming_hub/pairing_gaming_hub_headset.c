/** @file  pairing_gaming_hub_headset.c
 *  @brief The headset device is part of the gaming hub to demonstrate a point to multipoint pairing application.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include "app_pairing.h"
#include "iface_audio.h"
#include "iface_pairing_gaming_hub.h"
#include "iface_wireless.h"
#include "sac_api.h"
#include "swc_api.h"
#include "swc_cfg_headset.h"

/* CONSTANTS ******************************************************************/
#define SAC_MEM_POOL_SIZE      5000
#define SAC_PAYLOAD_SIZE       84
#define SAC_LATENCY_QUEUE_SIZE 18
#define SWC_MEM_POOL_SIZE      3400
#define PAIRING_DEVICE_ROLE    1

/* PRIVATE GLOBALS ************************************************************/
/* ** Audio Core ** */
static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *audio_pipeline;
static sac_endpoint_t *max98091_consumer;
static ep_swc_instance_t swc_producer_instance;
static sac_endpoint_t *swc_producer;

/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_hal_t hal;
static swc_node_t *node;
static swc_connection_t *rx_from_dongle_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t rx_from_dongle_timeslots[] = RX_FROM_DONGLE_TIMESLOTS;

/* ** Application Specific ** */
static bool is_device_paired;
static app_pairing_t app_pairing;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(app_pairing_t *app_pairing, swc_error_t *err);
static void conn_rx_success_callback(void *conn);

static void app_audio_core_init(sac_error_t *audio_err);
static void audio_i2s_tx_complete_cb(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    swc_error_t swc_err;
    sac_error_t audio_err;

    iface_board_init();

    /* Give the default addresses when entering pairing */
    app_pairing.coordinator_address = COORDINATOR_ADDRESS;
    app_pairing.node_address = NODE_ADDRESS;
    app_pairing.pan_id = PAN_ID;
    app_swc_core_init(&app_pairing, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    app_audio_core_init(&audio_err);
    if (audio_err != SAC_ERR_NONE) {
        while (1);
    }

    iface_audio_node_init();

    sac_pipeline_start(audio_pipeline);

    swc_connect();

    while (1) {
        sac_pipeline_process(audio_pipeline, &audio_err);

        if (!is_device_paired) {
            /* Since the device is unpaired the send_keyboard_press_button should do nothing */
            iface_button_handling(NULL, enter_pairing_mode);
        } else {
            iface_button_handling(NULL, unpair_device);
        }
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[out] err  Wireless Core error code.
 */
static void app_swc_core_init(app_pairing_t *app_pairing, swc_error_t *err)
{
    uint16_t local_address;
    uint16_t remote_address;

    iface_swc_hal_init(&hal);
    iface_swc_handlers_init();

    local_address = app_pairing->node_address;
    remote_address = app_pairing->coordinator_address;

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .fast_sync_enabled = true,
        .random_channel_sequence_enabled = false,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE
    };
    swc_init(core_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_node_cfg_t node_cfg = {
        .role = NETWORK_ROLE,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = app_pairing->coordinator_address,
        .local_address = local_address,
        .sleep_level = SWC_SLEEP_LEVEL
    };
    node = swc_node_init(node_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_radio_cfg_t radio_cfg = {
        .irq_polarity = IRQ_ACTIVE_HIGH,
        .std_spi = SPI_STANDARD
    };
    swc_node_add_radio(node, radio_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    /* ** RX from dongle Connection ** */
    swc_connection_cfg_t rx_from_dongle_conn_cfg = {
        .name = "RX from dongle Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = RX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = rx_from_dongle_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_from_dongle_timeslots),
        .allocate_payload_memory = true,
        .ack_enabled = true,
        .arq_enabled = true,
        .arq_settings.try_count = 0,
        .arq_settings.time_deadline = 0,
        .auto_sync_enabled = false,
        .cca_enabled = false,
        .throttling_enabled = false,
        .rdo_enabled = false,
        .fallback_enabled = false
    };
    rx_from_dongle_conn = swc_connection_init(node, rx_from_dongle_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_from_dongle_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_from_dongle_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_from_dongle_conn, node, rx_from_dongle_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_from_dongle_conn, conn_rx_success_callback);

    swc_setup(node);
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_success_callback(void *conn)
{
    (void)conn;
    sac_error_t audio_err;

    iface_rx_conn_status();

    sac_pipeline_produce(audio_pipeline, &audio_err);
}

/** @brief Initialize the Audio Core.
 *
 *  @param[out] err  Audio Core error code.
 */
static void app_audio_core_init(sac_error_t *audio_err)
{
    sac_endpoint_interface_t max98091_consumer_iface;
    sac_endpoint_interface_t swc_producer_iface;
    queue_critical_cfg_t queue_critical;

    iface_audio_swc_endpoint_init(&swc_producer_iface, NULL);
    iface_audio_max98091_endpoint_init(NULL, &max98091_consumer_iface);
    iface_audio_critical_section_init(&queue_critical);

    iface_set_sai_complete_callback(audio_i2s_tx_complete_cb, NULL);

    swc_producer_instance.connection = rx_from_dongle_conn;

    sac_init(queue_critical, audio_memory_pool, SAC_MEM_POOL_SIZE);

    /*
     * Audio Pipeline
     * ==============
     *
     * Input:      Stereo stream of 42 samples @ 48 kHz/16 bits is received over the air from the Coordinator.
     * Processing: None.
     * Output:     Stereo stream of 42 samples @ 48 kHz/16 bits is played using the MAX98091 hardware audio codec.
     *
     * +-----+    +-----+    +-------+
     * | SWC | -> | CDC | -> | Codec |
     * +-----+    +-----+    +-------+
     */
    audio_endpoint_cfg_t swc_producer_cfg = {
        .use_encapsulation  = true,
        .delayed_action     = false,
        .channel_count      = 2,
        .bit_depth          = AUDIO_16BITS,
        .audio_payload_size = SAC_PAYLOAD_SIZE,
        .queue_size         = SAC_PRODUCER_QUEUE_SIZE
    };
    swc_producer = sac_endpoint_init((void *)&swc_producer_instance,
                                     "SWC EP (Producer)",
                                     swc_producer_iface,
                                     swc_producer_cfg,
                                     audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    audio_endpoint_cfg_t max98091_consumer_cfg = {
        .use_encapsulation  = false,
        .delayed_action     = true,
        .channel_count      = 2,
        .bit_depth          = AUDIO_16BITS,
        .audio_payload_size = SAC_PAYLOAD_SIZE,
        .queue_size         = SAC_LATENCY_QUEUE_SIZE
    };
    max98091_consumer = sac_endpoint_init(NULL,
                                          "MAX98091 EP (Consumer)",
                                          max98091_consumer_iface,
                                          max98091_consumer_cfg,
                                          audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    sac_pipeline_cfg_t pipeline_cfg = {
        .cdc_enable = true,
        .cdc_resampling_length = 5760,
        .cdc_queue_avg_size = 1000,
        .do_initial_buffering = false,
        .user_data_enable = false
    };
    audio_pipeline = sac_pipeline_init("Codec -> SWC",
                                       swc_producer,
                                       pipeline_cfg,
                                       max98091_consumer,
                                       audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    sac_pipeline_setup(audio_pipeline, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }
}

/** @brief SAI DMA TX complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be
 *  executed every time a DMA transfer to the codec is completed
 *  in order to keep the audio playing.
 */
static void audio_i2s_tx_complete_cb(void)
{
    sac_error_t audio_err;

    sac_pipeline_consume(audio_pipeline, &audio_err);
}

/** @brief Enter in Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err;
    sac_error_t audio_err;

    iface_notify_enter_pairing();

    /* Give the information to the Pairing Application */
    app_pairing.network_role = node->cfg.role;
    app_pairing.device_role  = PAIRING_DEVICE_ROLE;

    is_device_paired = app_pairing_start_pairing_process(&app_pairing);

    if (is_device_paired) {
        iface_notify_pairing_successful();

        /* Reconfigure the Coordinator and Node addresses */
        swc_disconnect();
        app_swc_core_init(&app_pairing, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }
        app_audio_core_init(&audio_err);
        if (audio_err != SAC_ERR_NONE) {
            while (1);
        }
        swc_connect();
    } else {
        /* Indicate that the pairing process was unsuccessful */
        iface_notify_not_paired();
    }
}

/** @brief Unpair the device, this will put its connection addresses back to default value.
 */
static void unpair_device(void)
{
    swc_error_t swc_err;
    sac_error_t audio_err;

    is_device_paired = false;

    /* Give the default addresses */
    swc_disconnect();
    memset(&app_pairing, 0, sizeof(app_pairing_t));
    app_pairing.coordinator_address = COORDINATOR_ADDRESS;
    app_pairing.node_address = NODE_ADDRESS;
    app_pairing.pan_id = PAN_ID;
    app_swc_core_init(&app_pairing, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
    app_audio_core_init(&audio_err);
    if (audio_err != SAC_ERR_NONE) {
        while (1);
    }
    swc_connect();

    iface_notify_not_paired();
}
