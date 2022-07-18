/** @file  audio_4-channel_coord.c
 *  @brief Application example to stream a single audio source to four audio outputs.
 *
 *  This application creates a unidirectional uncompressed audio stream.
 *  The coordinator sends a 48 kHz mono stream and four nodes playback the same audio stream.
 *  The use of the digital volume control and the sampling rate converter
 *  processing stages are demonstrated.

 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include "iface_audio.h"
#include "iface_audio_4-channel.h"
#include "iface_wireless.h"
#include "sac_api.h"
#include "swc_api.h"
#include "swc_cfg_coord.h"

/* CONSTANTS ******************************************************************/
#define SAC_MEM_POOL_SIZE      2800
#define SAC_PAYLOAD_SIZE       122
#define SAC_LATENCY_QUEUE_SIZE 10
#define SWC_MEM_POOL_SIZE      8400

/* PRIVATE GLOBALS ************************************************************/
/* ** Audio Core ** */
static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *audio_pipeline;

static sac_endpoint_t *max98091_producer;

static ep_swc_instance_t swc_consumer1_instance;
static sac_endpoint_t *swc_consumer1;
static ep_swc_instance_t swc_consumer2_instance;
static sac_endpoint_t *swc_consumer2;
static ep_swc_instance_t swc_consumer3_instance;
static sac_endpoint_t *swc_consumer3;
static ep_swc_instance_t swc_consumer4_instance;
static sac_endpoint_t *swc_consumer4;

/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_hal_t hal;
static swc_node_t *node;
static swc_connection_t *tx_to_node1_conn;
static swc_connection_t *tx_to_node2_conn;
static swc_connection_t *tx_to_node3_conn;
static swc_connection_t *tx_to_node4_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_to_node1_timeslots[] = TX_TO_NODE1_TIMESLOTS;
static int32_t tx_to_node2_timeslots[] = TX_TO_NODE2_TIMESLOTS;
static int32_t tx_to_node3_timeslots[] = TX_TO_NODE3_TIMESLOTS;
static int32_t tx_to_node4_timeslots[] = TX_TO_NODE4_TIMESLOTS;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(swc_error_t *err);
static void conn_tx_success_callback(void *conn);
static void app_audio_core_init(sac_error_t *audio_err);
static void audio_i2s_rx_complete_cb(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    swc_error_t swc_err;
    sac_error_t audio_err;

    iface_board_init();

    app_swc_core_init(&swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    app_audio_core_init(&audio_err);
    if (audio_err != SAC_ERR_NONE) {
        while (1);
    }

    iface_audio_coord_init();
    sac_pipeline_start(audio_pipeline);
    swc_connect();

    while (1) {
        sac_pipeline_process(audio_pipeline, &audio_err);
        sac_pipeline_consume(audio_pipeline, &audio_err);
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[out] err  Wireless Core error code.
 */
static void app_swc_core_init(swc_error_t *err)
{
    iface_swc_hal_init(&hal);
    iface_swc_handlers_init();

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .fast_sync_enabled = false,
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
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = LOCAL_ADDRESS,
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

    /* ** Coordinator sending to Node1 connection ** */
    swc_connection_cfg_t tx_to_node1_conn_cfg = {
        .name = "TX Connection to Node 1",
        .source_address = LOCAL_ADDRESS,
        .destination_address = REMOTE_ADDRESS_1,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = TX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = tx_to_node1_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node1_timeslots),
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
    tx_to_node1_conn = swc_connection_init(node, tx_to_node1_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node1_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node1_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node1_conn, node, tx_to_node1_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_to_node1_conn, conn_tx_success_callback);

    /* ** Coordinator sending to Node2 connection ** */
    swc_connection_cfg_t tx_to_node2_conn_cfg = {
        .name = "TX Connection to Node 2",
        .source_address = LOCAL_ADDRESS,
        .destination_address = REMOTE_ADDRESS_2,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = TX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = tx_to_node2_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node2_timeslots),
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
    tx_to_node2_conn = swc_connection_init(node, tx_to_node2_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node2_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node2_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node2_conn, node, tx_to_node2_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_to_node2_conn, conn_tx_success_callback);

    /* ** Coordinator sending to Node3 connection ** */
    swc_connection_cfg_t tx_to_node3_conn_cfg = {
        .name = "TX Connection to Node 3",
        .source_address = LOCAL_ADDRESS,
        .destination_address = REMOTE_ADDRESS_3,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = TX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = tx_to_node3_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node3_timeslots),
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
    tx_to_node3_conn = swc_connection_init(node, tx_to_node3_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node3_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node3_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node3_conn, node, tx_to_node3_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_to_node3_conn, conn_tx_success_callback);

    /* ** Coordinator sending to Node4 connection ** */
    swc_connection_cfg_t tx_to_node4_conn_cfg = {
        .name = "TX Connection to Node 4",
        .source_address = LOCAL_ADDRESS,
        .destination_address = REMOTE_ADDRESS_4,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = TX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = tx_to_node4_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node4_timeslots),
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
    tx_to_node4_conn = swc_connection_init(node, tx_to_node4_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node4_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node4_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node4_conn, node, tx_to_node4_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_to_node4_conn, conn_tx_success_callback);

    swc_setup(node);
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_success_callback(void *conn)
{
    (void)conn;

    iface_tx_conn_status();
}

/** @brief Initialize the Audio Core.
 *
 *  @param[out] err  Audio Core error code.
 */
static void app_audio_core_init(sac_error_t *audio_err)
{
    sac_endpoint_interface_t max98091_producer_iface;
    sac_endpoint_interface_t swc_consumer_iface;
    queue_critical_cfg_t queue_critical;

    iface_audio_swc_endpoint_init(NULL, &swc_consumer_iface);
    iface_audio_max98091_endpoint_init(&max98091_producer_iface, NULL);
    iface_audio_critical_section_init(&queue_critical);

    iface_audio_dma_complete_callbacks(NULL, audio_i2s_rx_complete_cb);

    swc_consumer1_instance.connection = tx_to_node1_conn;
    swc_consumer2_instance.connection = tx_to_node2_conn;
    swc_consumer3_instance.connection = tx_to_node3_conn;
    swc_consumer4_instance.connection = tx_to_node4_conn;

    sac_init(queue_critical, audio_memory_pool, SAC_MEM_POOL_SIZE);

    /*
     * Audio Pipeline
     * ==============
     *
     * Input:      Mono stream of 61 samples @ 48 kHz/16 bits is recorded using the MAX98091 hardware audio codec.
     * Processing: None.
     * Output:     Mono stream of 61 samples @ 48 kHz/16 bits is sequentially sent over the air to Node1, Node2, Node3 and Node4.
     *
     * +-------+     +-------------+
     * | Codec |  ├> | SWC (Node1) |
     * +-------+  |  +-------------+
     *            |    +-------------+
     *            ├--> | SWC (Node2) |
     *            |    +-------------+
     *            |      +-------------+
     *            ├----> | SWC (Node3) |
     *            |      +-------------+
     *            |        +-------------+
     *            └------> | SWC (Node4) |
     *                     +-------------+
     */
    audio_endpoint_cfg_t max98091_producer_cfg = {
        .use_encapsulation  = false,
        .delayed_action     = true,
        .channel_count      = 1,
        .bit_depth          = AUDIO_16BITS,
        .audio_payload_size = SAC_PAYLOAD_SIZE,
        .queue_size         = SAC_PRODUCER_QUEUE_SIZE
    };
    max98091_producer = sac_endpoint_init(NULL, "MAX98091 EP (Producer)",
                                          max98091_producer_iface, max98091_producer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    audio_endpoint_cfg_t swc_consumer_cfg = {
        .use_encapsulation  = true,
        .delayed_action     = false,
        .channel_count      = 1,
        .bit_depth          = AUDIO_16BITS,
        .audio_payload_size = SAC_PAYLOAD_SIZE,
        .queue_size         = SAC_LATENCY_QUEUE_SIZE
    };
    swc_consumer1 = sac_endpoint_init((void *)&swc_consumer1_instance, "SWC EP 1 (Consumer)",
                                      swc_consumer_iface, swc_consumer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    swc_consumer2 = sac_endpoint_init((void *)&swc_consumer2_instance, "SWC EP 2 (Consumer)",
                                      swc_consumer_iface, swc_consumer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    swc_consumer3 = sac_endpoint_init((void *)&swc_consumer3_instance, "SWC EP 3 (Consumer)",
                                      swc_consumer_iface, swc_consumer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    swc_consumer4 = sac_endpoint_init((void *)&swc_consumer4_instance, "SWC EP 4 (Consumer)",
                                      swc_consumer_iface, swc_consumer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    sac_pipeline_cfg_t pipeline_cfg = {
        .cdc_enable = false,
        .do_initial_buffering = true,
        .user_data_enable = false
    };
    audio_pipeline = sac_pipeline_init("Codec -> SWC", max98091_producer,
                                       pipeline_cfg, swc_consumer1, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    sac_pipeline_add_extra_consumer(audio_pipeline, swc_consumer2);
    sac_pipeline_add_extra_consumer(audio_pipeline, swc_consumer3);
    sac_pipeline_add_extra_consumer(audio_pipeline, swc_consumer4);

    sac_pipeline_setup(audio_pipeline, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }
}

/** @brief SAI DMA RX complete callback.
 *
 *  This receives audio packets from the codec. It needs to be
 *  executed every time a DMA transfer from the codec is completed
 *  in order to keep recording audio.
 */
static void audio_i2s_rx_complete_cb(void)
{
    sac_error_t audio_err;

    sac_pipeline_produce(audio_pipeline, &audio_err);
}
