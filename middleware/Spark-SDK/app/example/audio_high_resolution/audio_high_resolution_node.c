/** @file  audio_high_resolution_node.c
 *  @brief This application creates a unidirectional uncompressed high resolution
 *         audio stream from the MAX98091 audio codec Line-In of the coordinator to the
 *         Headphone-Out of the node.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include "audio_packing.h"
#include "iface_audio_high_resolution.h"
#include "iface_audio.h"
#include "iface_wireless.h"
#include "sac_api.h"
#include "swc_api.h"
#include "swc_cfg_node.h"

/* CONSTANTS ******************************************************************/
#define SAC_MEM_POOL_SIZE      8000
#define SAC_PAYLOAD_SIZE       78
#define SAC_LATENCY_QUEUE_SIZE 32
#define SWC_MEM_POOL_SIZE      3200

/* PRIVATE GLOBALS ************************************************************/
/* ** Audio Core ** */
static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *audio_pipeline;

static audio_volume_instance_t volume_instance;
static sac_processing_t *volume_processing;

static audio_packing_instance_t audio_packing_instance;
static sac_processing_t *audio_packing_processing;

static sac_endpoint_t *max98091_consumer;

static ep_swc_instance_t swc_producer_instance;
static sac_endpoint_t *swc_producer;

/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_hal_t hal;
static swc_node_t *node;
static swc_connection_t *rx_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t rx_timeslots[] = RX_TIMESLOTS;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(swc_error_t *err);
static void conn_rx_success_callback(void *conn);

static void app_audio_core_init(sac_error_t *audio_err);
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void audio_i2s_tx_complete_cb(void);

static void volume_up(void);
static void volume_down(void);

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

    iface_audio_node_init();

    sac_pipeline_start(audio_pipeline);

    swc_connect();

    while (1) {
        sac_pipeline_process(audio_pipeline, &audio_err);
        iface_button_handling(volume_up, volume_down);
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

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = REMOTE_ADDRESS,
        .destination_address = LOCAL_ADDRESS,
        .max_payload_size = SAC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = RX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
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
    rx_conn = swc_connection_init(node, rx_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain  = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, node, rx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback);

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
    sac_processing_interface_t volume_iface;
    sac_processing_interface_t packing_iface;
    queue_critical_cfg_t queue_critical;

    iface_audio_swc_endpoint_init(&swc_producer_iface, NULL);
    iface_audio_max98091_endpoint_init(NULL, &max98091_consumer_iface);
    iface_audio_critical_section_init(&queue_critical);

    app_audio_core_volume_interface_init(&volume_iface);
    app_audio_core_packing_interface_init(&packing_iface);

    iface_set_sai_complete_callback(audio_i2s_tx_complete_cb, NULL);

    swc_producer_instance.connection = rx_conn;

    sac_init(queue_critical, audio_memory_pool, SAC_MEM_POOL_SIZE);

    /*
     * Audio Pipeline
     * ==============
     *
     * Input:      Mono stream of 30 samples @ 96 kHz/24 bits is received over the air from the Coordinator.
     * Processing: Unpacking from 24 bits aligned to 32 bits aligned audio samples.
     * Processing: Digital volume control followed by clock drift compensation.
     * Output:     Mono stream of 40 samples @ 96 kHz/24 bits is played using the MAX98091 hardware audio codec.
     *
     * +-----+    +----------------+    +-----+    +-------+
     * | SWC | -> | Digital Volume | -> | CDC | -> | Codec |
     * +-----+    +----------------+    +-----+    +-------+
     */
    audio_endpoint_cfg_t swc_producer_cfg = {
        .use_encapsulation  = true,
        .delayed_action     = false,
        .channel_count      = 1,
        .bit_depth          = AUDIO_24BITS,
        .audio_payload_size = SAC_PAYLOAD_SIZE,
        .queue_size         = SAC_PRODUCER_QUEUE_SIZE
    };
    swc_producer = sac_endpoint_init((void *)&swc_producer_instance, "SWC EP (Producer)",
                                     swc_producer_iface, swc_producer_cfg, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    audio_packing_instance.packing_mode = AUDIO_UNPACK_24BITS;
    audio_packing_processing = sac_processing_stage_init((void *)&audio_packing_instance, "Audio Unpacking", packing_iface, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    volume_instance.initial_volume_level = 100;
    volume_instance.bit_depth = AUDIO_24BITS;
    volume_processing = sac_processing_stage_init((void *)&volume_instance, "Digital Volume Control", volume_iface, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    audio_endpoint_cfg_t max98091_consumer_cfg = {
        .use_encapsulation  = false,
        .delayed_action     = true,
        .channel_count      = 1,
        .bit_depth          = AUDIO_24BITS,
        .audio_payload_size = 104,
        .queue_size         = SAC_LATENCY_QUEUE_SIZE
    };
    max98091_consumer = sac_endpoint_init(NULL, "MAX98091 EP (Consumer)",
                                          max98091_consumer_iface, max98091_consumer_cfg, audio_err);
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
    audio_pipeline = sac_pipeline_init("SWC -> Codec", swc_producer,
                                       pipeline_cfg, max98091_consumer, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }

    sac_pipeline_add_processing(audio_pipeline, audio_packing_processing);
    sac_pipeline_add_processing(audio_pipeline, volume_processing);

    sac_pipeline_setup(audio_pipeline, audio_err);
    if (*audio_err != SAC_ERR_NONE) {
        return;
    }
}

/** @brief Initialize the digital volume control audio processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface)
{
    iface->init = audio_volume_init;
    iface->deinit = audio_volume_deinit;
    iface->ctrl = audio_volume_ctrl;
    iface->process = audio_volume_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio packing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface)
{
    iface->init = audio_packing_init;
    iface->deinit = audio_packing_deinit;
    iface->ctrl = audio_packing_ctrl;
    iface->process = audio_packing_process;
    iface->gate = NULL;
}

/** @brief Increase the audio output volume level.
 *
 *  This affects the audio pipeline the digital volume
 *  processing stage is added to.
 */
static void volume_up(void)
{
    sac_processing_ctrl(volume_processing, AUDIO_VOLUME_INCREASE, SAC_NO_ARG);
}

/** @brief Decrease the audio output volume level.
 *
 *  This affects the audio pipeline the digital volume
 *  processing stage is added to.
 */
static void volume_down(void)
{
    sac_processing_ctrl(volume_processing, AUDIO_VOLUME_DECREASE, SAC_NO_ARG);
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
