/** @file  wps.h
 *  @brief SPARK Wireless Protocol Stack.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_H
#define WPS_H

/* INCLUDES *******************************************************************/
#include "wps_callback.h"
#include "wps_def.h"
#include "wps_error.h"
#include "wps_mac.h"
#include "wps_phy.h"
#include "wps_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define PROCESS_STATE_Q_SIZE 10 /*!< WPS process states machine queue size */

/* MACROS *********************************************************************/
#define BIT_AUTO_REPLY_TIMESLOT (1 << 7) /*!< Bit mask to identify Auto reply timeslot (Prime timeslot) */
#define MAIN_TIMESLOT(x) ((x) & 0x7F) /*!< User MACRO to identify primary timeslot */
#define AUTO_TIMESLOT(x) ((x) | BIT_AUTO_REPLY_TIMESLOT) /*!< User MACRO to identify data in auto-reply timeslot (Prime timeslot) */

/* TYPES **********************************************************************/

/** @brief Wireless protocol stack process signal.
 */
typedef enum process_signal {
    PROCESS_SIGNAL_EXECUTE, /*!< WPS process execute signal */
    PROCESS_SIGNAL_YIELD,   /*!< WPS process yield signal */
} process_signal_t;

/** @brief WPS instance.
 */
typedef struct wps wps_t;

/** @brief WPS process state machine function pointer type.
 */
typedef void (*wps_process_state_t)(wps_t *wps);

/** @brief Wireless Protocol Stack input signals.
 */
typedef enum wps_input_signal {
    WPS_NONE,              /*!< WPS no signal */
    WPS_RADIO_IRQ,         /*!< WPS radio IRQ signal */
    WPS_TRANSFER_COMPLETE, /*!< WPS transfer complete signal */
    WPS_CONNECT,           /*!< WPS complete signal */
    WPS_DISCONNECT,        /*!< WPS disconnect signal */
    WPS_HALT,              /*!< WPS halt signal */
    WPS_RESUME,            /*!< WPS resume signal */
} wps_input_signal_t;

/** @brief Wireless Protocol Stack status.
 */
typedef enum wps_state {
    WPS_IDLE,       /*!< WPS IDLE state */
    WPS_PROCESSING, /*!< WPS processing state */
} wps_status_t;

/** @brief Wireless Protocol Stack connection configuration.
 */
typedef struct wps_connection_config {
    /* Address */
    uint16_t source_address;      /*!< Current connection source address (Transmitting node address) */
    uint16_t destination_address; /*!< Current connection destination address (Receiving node address) */

    /* Buffer */
    xlayer_t  *fifo_buffer;       /*!< Queue buffer */
    uint32_t   fifo_buffer_size;  /*!< Queue size */
    uint8_t   *frame_buffer;      /*!< Frame buffer use for send/receive payload, depending on the nature of the connection */
    uint16_t   header_length;     /*!< Length of the WPS header in the current configuration */
    uint32_t   frame_length;      /*!< Frame length to send/receive. Set to header + max payload size.*/

    rf_channel_t (*channel_buffer)[WPS_NB_RF_CHANNEL][WPS_RADIO_COUNT];  /*!< Buffer to hold the channel config */
    uint8_t fallback_count;                                              /*!< Number of fallback threshold. */
    uint8_t *fallback_threshold;                                          /*!< Array of payload size fallback threshold. in descending order. */

    uint64_t (*get_tick_quarter_ms)(void);  /*!< Get free running timer tick in quarter ms */
} wps_connection_cfg_t;

/** @brief Wireless Protocol Stack connection header configuration.
 *
 */
typedef struct wps_header_cfg {
    bool main_connection;           /*!< main connection flag. */
    bool rdo_enabled;               /*!< rdo enabled flag. */
    bool ranging_phase_accumulator; /*!< ranging phase accumulator flag. */
    bool ranging_phase_provider;    /*!< ranging phase provider flag. */
} wps_header_cfg_t;

/** @brief Wireless Protocol Stack node configuration.
 */
typedef struct node_config {
    wps_role_t        role;                      /*!< Current node role : Coordinator or node */
    uint32_t          preamble_len;              /*!< Length of the preamble, in bits */
    sleep_lvl_t       sleep_lvl;                 /*!< Radio sleep level */
    uint32_t          crc_polynomial;            /*!< Radio CRC polynomial */
    uint16_t          local_address;             /*!< Node current address */
    uint32_t          syncword;                  /*!< Radio syncword */
    syncword_length_t syncword_len;              /*!< Sync word length */
    isi_mitig_t       isi_mitig;                 /*!< ISI mitigation level */
    uint8_t           rx_gain;                   /*!< Default radio RX gain */
} wps_node_cfg_t;

/** @brief Wireless Protocol Stack layer 7.
 */
typedef struct wps_l7 {
    circular_queue_t callback_queue; /*!< Circular queue instance to save the callbacks */
    circular_queue_t request_queue;  /*!< Circular queue to forward application request to WPS */
} wps_l7_t;

/** @brief Wireless Protocol Stack structure.
 */
typedef struct wps {
    wps_node_t *node;                     /*!< WPS node instance */
    schedule_t schedule;                  /*!< WPS link schedule (TDMA) */
    channel_sequence_t channel_sequence;  /*!< WPS channel sequence for channel hopping */
    bool random_channel_sequence_enabled; /*!< WPS random channel sequence enable flag */
    uint8_t network_id;                   /*!< WPS concurrent network ID */

    wps_l7_t l7;                    /*!< WPS Layer 7 instance */
    wps_mac_t mac;                  /*!< WPS MAC Layer instance */
    wps_phy_t phy[WPS_RADIO_COUNT]; /*!< WPS Layer 1 instance */

    process_signal_t process_signal;                            /*!< Wireless protocol stack process signal */
    wps_process_state_t  current_state;                         /*!< Current state machine state*/
    wps_process_state_t  end_state;                             /*!< end state machine state*/
    circular_queue_t next_states;                               /*!< Next state machine state queue*/
    wps_process_state_t *next_state_pool[PROCESS_STATE_Q_SIZE]; /*!< Next state machine state pool*/
    uint8_t state_step;                                         /*!< State machine state step*/

    wps_schedule_ratio_cfg_t throttle_cfg; /*!< WPS throttle feature configuration structure */
    circular_queue_t *write_request_queue; /*!< WPS write register request queue */
    circular_queue_t *read_request_queue;  /*!< WPS write register request queue */

    wps_input_signal_t signal;             /*!< WPS current signal */
    wps_status_t status;                   /*!< WPS status : Idle or processing */
    void (*callback_context_switch)(void); /*!< function pointer to trig a context switch to the callback process */
} wps_t;

/** @brief Process the WPS process state machine.
 *
 *  @param[in] wps  WPS instance.
 */
static inline void wps_process(wps_t *wps)
{
    wps->status = WPS_PROCESSING;

    do {
        wps->current_state(wps);
    } while (wps->process_signal == PROCESS_SIGNAL_EXECUTE);

    wps->status = WPS_IDLE;
}

/** @brief  WPS Ranging function pointer interface structure.
 */
typedef struct wps_phase_interface {
    void (*supply)(phase_info_t *local_info, phase_info_t *remote_info); /*!< Supply phases data to a module external */
    bool (*is_busy)(void);                                               /*!< Poll if external module is busy */
} wps_phase_interface_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Convert time in us to PLL cycles.
 *
 *  @param[in] time_us   Time in us.
 *  @param[in] chip_rate Chip rate.
 *
 *  @return Number of PLL cycles.
 */
uint32_t wps_us_to_pll_cycle(uint32_t time_us, chip_rate_cfg_t chip_rate);

/** @brief Set the WPS Phase interface.
 *
 *  @param wps        WPS instance.
 *  @param phase_itf  Phase interface.
 */
void wps_init_phase_interface(wps_t *wps, wps_phase_interface_t *phase_itf);

/** @brief Wireless Protocol Stack radio initialization.
 *
 *  Read and save NVM.
 *  radio_hal_t and spi buffer must be already be assigned in wps_radio_t.
 *
 *  @param[in]  wps_radio  WPS radio instance.
 *  @param[out] error      Error code.
 */
void wps_radio_init(wps_radio_t *wps_radio, wps_error_t *error);

/** @brief Set vref_tune_offset_enabled flag.
 *
 *  The vref_tune_offset_enabled flag is used to fine-tune the output voltage of the main
 *  transceiver's reference buffer. This may be used to compensate for frequency, gain and
 *  overall performance variations between individual units.
 *
 *  @param[in] node                      WPS node instance..
 *  @param[in] vref_tune_offset_enabled  Enable/disable vref tune offset.
 */
void wps_set_vref_tune_offset_enabled_flag(wps_radio_t *node, bool vref_tune_offset_enabled);

/** @brief Initialize the callback queue.
 *
 *  The callback queue is used to store event actions waiting to
 *  be executed by the application.
 *
 *  The size of the callback buffer should be equal to the size
 *  of the biggest Xlayer queue.
 *
 *  @param[in] wps              Wireless Protocol Stack instance.
 *  @param[in] callback_buffer  Array of callback function pointer.
 *  @param[in] size             Array size.
 *  @param[in] context_switch   Function that trigger the context switch to the callback process
 */
void wps_init_callback_queue(wps_t *wps,
                             wps_callback_inst_t *callback_buffer,
                             size_t size,
                             void (*context_switch)(void));

/** @brief Initialize the queue for the application request.
 *
 *  @param[in] wps             Wireless Protocol Stack instance.
 *  @param[in] request_buffer  Array of request pointer.
 *  @param[in] size            Size fo the request array.
 */
void wps_init_request_queue(wps_t *wps, wps_request_info_t *request_buffer, size_t size);

/** @brief Wireless Protocol Stack initialization.
 *
 *  Initialize the WPS and all the layers inside the WPS.
 *
 *  @param[in]  wps   Wireless Protocol Stack instance.
 *  @param[in]  node  Node instance.
 *  @param[out] err   Pointer to the error code.
 */
void wps_init(wps_t *wps, wps_node_t *node, wps_error_t *err);

/** @brief Wireless Protocol Stack process initialization.
 *
 *  Initialize the WPS process.
 *
 *  @param[in] wps   Wireless Protocol Stack instance.
 */
void wps_process_init(wps_t *wps);

/** @brief Set network syncing address.
 *
 *  @param[in]  wps      Wireless Protocol Stack instance.
 *  @param[in]  address  Syncing address.
 *  @param[out] err      Pointer to the error code.
 */
void wps_set_syncing_address(wps_t *wps, uint16_t address, wps_error_t *err);

/** @brief Set network ID.
 *
 *  @param[in]  wps         Wireless Protocol Stack instance.
 *  @param[in]  network_id  Network ID.
 *  @param[out] err         Pointer to the error code.
 */
void wps_set_network_id(wps_t *wps, uint8_t network_id, wps_error_t *err);

/** @brief Node configuration.
 *
 *  Configure the SPARK radio for proper communication. This goes
 *  through all of the packet configurations, the interrupt event,
 *  the sleep level and the internal radio timer source.
 *
 *  @param[in]  node   Node instance.
 *  @param[in]  radio  Radio instance.
 *  @param[in]  cfg    Node configuration.
 *  @param[out] err    Pointer to the error code.
 */
void wps_config_node(wps_node_t *node, wps_radio_t *radio, wps_node_cfg_t *cfg, wps_error_t *err);

/** @brief Configure network schedule.
 *
 *  Initialize the schedule object and convert the given duration
 *  to the time base of the SPARK radio.
 *
 *  @param[in]  wps                           Wireless Protocol Stack instance.
 *  @param[in]  timeslot_duration_pll_cycles  Timeslot duration array in pll cycles.
 *  @param[in]  timeslot                      Timeslot array used by schedule.
 *  @param[in]  schedule_size                 Schedule size. Timeslot's array size must match.
 *  @param[out] err                           Pointer to the error code.
 */
void wps_config_network_schedule(wps_t *wps,
                                 uint32_t *timeslot_duration_pll_cycles,
                                 timeslot_t *timeslot,
                                 uint32_t schedule_size,
                                 wps_error_t *err);

/** @brief Reset schedule.
 *
 *  @param[in]  wps   Wireless Protocol Stack instance.
 *  @param[out] err   Pointer to the error code.
 */
void wps_reset_schedule(wps_t *wps, wps_error_t *err);

/** @brief Configure network channel sequence.
 *
 *  Initialize the channel sequence for the Channel Hopping
 *  module inside the Layer 2 of the WPS.
 *
 *  @param[in]  wps               Wireless Protocol Stack instance.
 *  @param[in]  channel_sequence  Channel sequence.
 *  @param[in]  sequence_size     Channel sequence size.
 *  @param[out] err               Pointer to the error code.
 */
void wps_config_network_channel_sequence(wps_t *wps,
                                         uint32_t *channel_sequence,
                                         uint32_t sequence_size,
                                         wps_error_t *err);

/** @brief Enable random channel sequence.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_enable_random_channel_sequence(wps_t *wps, wps_error_t *err);

/** @brief Disable random channel sequence.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_random_channel_sequence(wps_t *wps, wps_error_t *err);

/** @brief Enable fast sync.
 *
 *  This allows the link to get synchronized faster when connections are not set to auto_sync.
 *
 *  The radio listens non-stop until it receives a frame then the TDMA schedule starts.
 *  WARNING: Available in IDLE sleep mode only.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_enable_fast_sync(wps_t *wps, wps_error_t *err);

/** @brief Disable fast sync.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_fast_sync(wps_t *wps, wps_error_t *err);

/** @brief Enable random data offset.
 *
 *  @param[in]  wps             Wireless Protocol Stack instance.
 *  @param[in]  rollover_value  Offset rollover value.
 *  @param[out] err             Pointer to the error code.
 */
void wps_enable_rdo(wps_t *wps, uint16_t rollover_value, wps_error_t *err);

/** @brief Disable random data offset.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_rdo(wps_t *wps, wps_error_t *err);

/** @brief Get the connection header size.
 *
 *  @param[in] wps        Wireless Protocol Stack instance.
 *  @param[in] header_cfg Header configuration.
 *  @return    Header size.
 */
uint8_t wps_get_connection_header_size(wps_t *wps, wps_header_cfg_t header_cfg);

/** @brief Create a connection between two nodes.
 *
 *  A connection is a unidirectional link between two nodes.
 *  The direction of the data will be determined by the relation
 *  between the source address of the connection and the current
 *  node address. For example, if source address is equal to
 *  this node address, the node will transmit during this
 *  connection.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  node        Node instance.
 *  @param[in]  config      Connection configuration.
 *  @param[out] err         Pointer to the error code.
 */
void wps_create_connection(wps_connection_t *connection,
                           wps_node_t *node,
                           wps_connection_cfg_t *config,
                           wps_error_t *err);

/** @brief Configure the connection header.
 *
 *  @note Must be called after wps_create_connection and all
 *        enable/disable connection features functions.
 *
 *  Main connection header :
 *   0                   1
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |S|  Timeslot ID|  Chan hop seq |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *   Header with ranging phase accumulator :
 *   0                   1                   2
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |S|  Timeslot ID|  Chan hop seq |      Count    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  WPS Header with ranging phase provider :
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Count     |     Phase 1   |     Phase 2   |     Phase 3   |     Phase 4   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  @param[in]  wps        Wireless Protocol Stack instance.
 *  @param[in]  connection Connection instance.
 *  @param[in]  header_cfg Header configuration.
 *  @param[out] err        Pointer to the error code.
 */
void wps_configure_header_connection(wps_t *wps,
                                     wps_connection_t *connection,
                                     wps_header_cfg_t header_cfg,
                                     wps_error_t *err);

/** @brief Set connection's timeslot.
 *
 *  A connection may send its payload via the MAIN_TIMESLOT or AUTO_TIMESLOT. It can't use both on the same timeslot.
 *  Feature like retransmission are not available when using AUTO_TIMESLOT.
 *
 *  @param[in]  connection    Connection instance.
 *  @param[in]  wps           Wireless Protocol Stack instance.
 *  @param[in]  timeslot_id   Timeslot id used by the connection.
 *  @param[in]  nb_timeslots  Number of timeslot used by the connection. Must match timeslot_id array size.
 *  @param[out] err           Pointer to the error code.
 */
void wps_connection_set_timeslot(wps_connection_t *connection,
                                 wps_t *wps,
                                 int32_t *timeslot_id,
                                 uint32_t nb_timeslots,
                                 wps_error_t *err);

/** @brief Configure connection's RF channel.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  node            Node instance.
 *  @param[in]  channel_x       Channel number.
 *  @param[in]  fallback_index  Fallback index.
 *  @param[in]  config          Channel configuration.
 *  @param[out] err             Pointer to the error code.
 */
void wps_connection_config_channel(wps_connection_t *connection,
                                   wps_node_t *node,
                                   uint8_t channel_x,
                                   uint8_t fallback_index,
                                   channel_cfg_t *config,
                                   wps_error_t *err);

/** @brief Configure connection's RF channel in a more convenient way.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  node        Node instance.
 *  @param[in]  frequency   Frequency to configure.
 *  @param[in]  tx_power    Channel's desired tranmission power level.
 *  @param[in]  channel_x   Channel number.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_preset_config_channel(wps_connection_t *connection,
                                          wps_node_t *node,
                                          uint16_t frequency,
                                          tx_power_t tx_power,
                                          uint8_t channel_x,
                                          wps_error_t *err);

/** @brief Configure connection's frame modulation and FEC level.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  modulation  Modulation.
 *  @param[in]  fec         FEC level.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_config_frame(wps_connection_t *connection,
                                 modulation_t modulation,
                                 fec_level_t fec,
                                 wps_error_t *err);

/** @brief Enable acknowledgment for connection's packet.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_ack(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable acknowledgment for connection's packet.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_ack(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable phases fetching in the radio.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_phases_aquisition(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable phases fetching in the radio.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_phases_aquisition(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable Stop and Wait (SaW) and Automatic Repeat Request (ARQ) for connection's packet.
 *
 *  @note This function must be called after wps_connection_enable_ack.
 *
 *  @param[in]  connection           Connection instance.
 *  @param[in]  local_address        Current node address.
 *  @param[in]  retry                Maximum number of retry.
 *  @param[in]  deadline_quarter_ms  Deadline in 1/4 ms. Packet is dropped when deadline is reached.
 *  @param[out] err                  Pointer to the error code.
 */
void wps_connection_enable_stop_and_wait_arq(wps_connection_t *connection,
                                             uint16_t local_address,
                                             uint32_t retry,
                                             uint32_t deadline_quarter_ms,
                                             wps_error_t *err);

/** @brief Disable Stop and Wait (SaW) and Automatic Repeat Request (ARQ) for connection's packet.
 *
 *  Retransmission are disabled when disabling the SaW ARQ.
 *  Incomplete transmission will be notified to user every
 *  time a packet is NACK.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_stop_and_wait_arq(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable auto-sync mode.
 *
 * If this mode is enabled, when the cross-layer queue is empty,
 * a small frame containing the L2 header will be sent over the air for the current time slot.
 * This can be useful to keep synchronization when no application data is being sent.
 *
 * Warning: When the auto-sync is disabled, the user should provide a consistent data rate
 * output on the coordinator in order to keep synchronization in the network.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_auto_sync(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable auto-sync mode.
 *
 * If this mode is disabled, when the cross-layer queue is empty,
 * no data will be sent over the air, the radio will simply wake up and
 * the WPS will prepare the next time slot.
 *
 * Warning: When the auto-sync is disabled, the user should provide a consistent data rate
 * output on the coordinator in order to keep synchronization in the network.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_auto_sync(wps_connection_t *connection, wps_error_t *err);

/** @brief Remove connection from network.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  wps         Wireless Protocol Stack instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_remove_connection(wps_connection_t *connection, wps_t *wps, wps_error_t *err);

/** @brief Enable connection fixed payload size mode.
 *
 * Warning: The use of auto replies to send user payload is not supported when this mode is enabled.
 *
 *  @param[in]  connection          Connection instance.
 *  @param[in]  fixed_payload_size  Payload size.
 *  @param[out] err                 Pointer to the error code.
 */
void wps_connection_enable_fixed_payload_size(wps_connection_t *connection,
                                              uint8_t fixed_payload_size,
                                              wps_error_t *err);

/** @brief Disable connection fixed payload size mode.
 *
 *  @note Warning: The use of auto replies to send user payload is not supported when this mode is enabled.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_fixed_payload_size(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable a connection's Clear Channel Assessment (CCA).
 *
 *  @param[in]  connection             Connection instance.
 *  @param[in]  threshold              CCA threshold.
 *  @param[in]  retry_time_pll_cycles  CCA retry time.
 *  @param[in]  try_count              CCA try count.
 *  @param[in]  fail_action            CCA fail action.
 *  @param[out] err                    Pointer to the error code.
 */
void wps_connection_enable_cca(wps_connection_t *connection,
                               uint8_t threshold,
                               uint16_t retry_time_pll_cycles,
                               uint8_t try_count,
                               cca_fail_action_t fail_action,
                               wps_error_t *err);

/** @brief Disable connection Clear Channel Assessment (CCA).
 *
 *  @note To properly disable CCA, the CCA module needs to be disabled with a threshold of 0xff.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_cca(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable connection's fixed gain (SR1120 feature only).
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  rx_gain     RX gain.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_fixed_gain(wps_connection_t *connection, uint8_t rx_gain, wps_error_t *err);

/** @brief Disable connection's fixed gain.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_fixed_gain(wps_connection_t *connection, wps_error_t *err);

/** @brief Set the callback function to execute when a payload is successfully transmitted.
 *
 *  @note The Core has successfully sent a frame. If ACKs are enabled, this callback is triggered when the
 *        ACK frame is received. If ACKs are disabled, it triggers every time the frame is sent.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS fail to transmit a frame.
 *
 *  @note An ACK frame was expected and was not received. If ACK is not enabled, this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS drops a frame.
 *
 *  @note The Core has discarded the frame because the deadline configured for the ARQ
 *        (either in time or in number of retries) has been met. If ARQ is not enabled,
 *        this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS successfully receives a frame.
 *
 *  @note The Core has successfully received a frame. The address matches its local address or the
 *        broadcast address and the CRC checked. The frame is ready to be picked up from the RX FIFO.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the event callback of a connection.
 *
 *  @note This callback report error that occurs in the WPS.
 *        Possible error are shown in the wps_error_t structure.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_event_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Connect node to network.
 *
 *  Setup the radio internal timer and reset every layer in the WPS.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_connect(wps_t *wps, wps_error_t *err);

/** @brief Disconnect node from network.
 *
 *  Put radio to sleep and disable internal radio timer
 *  to disconnect the radio from the network.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disconnect(wps_t *wps, wps_error_t *err);

/** @brief Reset the WPS when a crash occurs.
 *
 *  When a crash occurs the WPS is disconnected and then reconnected.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_reset(wps_t *wps, wps_error_t *err);

/** @brief Halt connection to network.
 *
 *  The node stays synchronized but doesn't send / receive application payload.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_halt(wps_t *wps, wps_error_t *err);

/** @brief Resume connection to network.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_resume(wps_t *wps, wps_error_t *err);

/** @brief Get buffer from the wps queue to hold the payload
 *
 *  The usage of this function is optional. If you don't want to
 *  use the WPS queue to hold the tx payload, when wps_create_connection
 *  is called, set the config.frame_length value to WPS_RADIO_HEADER_SIZE.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  payload     Pointer to the empty payload memory.
 *  @param[out] err         Pointer to the error code.
 */
void wps_get_free_slot(wps_connection_t *connection, uint8_t **payload, wps_error_t *err);

/** @brief Send payload over the air.
 *
 *  Enqueue a node in the connection Xlayer and WPS will
 *  send at next available timeslot.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  payload     Application payload to send over the air.
 *  @param[in]  size        Payload size in bytes.
 *  @param[out] err         Pointer to the error code.
 */
void wps_send(wps_connection_t *connection, uint8_t *payload, uint8_t size, wps_error_t *err);

/** @brief Read last received frame.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 *  @return WPS Received frame structure, including payload and size.
 */
wps_rx_frame wps_read(wps_connection_t *connection, wps_error_t *err);

/** @brief Remove the frame from the receiver FIFO.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_read_done(wps_connection_t *connection, wps_error_t *err);

/** @brief Return the used space of the connection Xlayer queue.
 *
 *  @param[in] connection  Connection instance.
 *  @return Xlayer queue's used space.
 */
uint32_t wps_get_fifo_size(wps_connection_t *connection);

/** @brief Return the free space of the connection Xlayer queue.
 *
 *  @param[in] connection  Connection instance.
 *  @return Xlayer queues's free space.
 */
uint32_t wps_get_fifo_free_space(wps_connection_t *connection);

/** @brief Get the current WPS error.
 *
 *  @note This function should only be called from
 *        the evt_callback_t that should be implemented
 *        in the application. The output error is of type
 *        wps_error_t.
 *
 *  @param[in] connection  Current connection with an error.
 *  @return Current WPS error.
 */
wps_error_t wps_get_error(wps_connection_t *connection);

/** @brief Initialize the connection throttle feature.
 *
 *  @note The pattern member of the connection struct need
 *        to be allocated to at least WPS_CONNECTION_THROTTLE_GRANULARITY * sizeof(bool).
 *
 *  @param[in]  connection  Current connection with an error.
 *  @param[out] err         Pointer to the error code.
 */
void wps_init_connection_throttle(wps_connection_t *connection, wps_error_t *err);

/** @brief Set the active timeslot ratio of the given connection.
 *
 *  @note Connection pattern member should been initialized and
 *        allocated before calling this method.
 *
 *  @param[in]  wps            Wireless Protocol Stack instance.
 *  @param[in]  connection     Current connection with an error.
 *  @param[in]  ratio_percent  Active timeslot ratio, in percent.
 *  @param[out] err            Pointer to the error code.
 */
void wps_set_active_ratio(wps_t *wps, wps_connection_t *connection, uint8_t ratio_percent, wps_error_t *err);

/** @brief Issue a write register request to the WPS.
 *
 *  @note Next time WPS is ready, it will write to the
 *        requested register(s).
 *
 *  @param[in]  wps           Wireless Protocol Stack instance.
 *  @param[in]  starting_reg  Starting register.
 *  @param[in]  data          Byte to send.
 *  @param[out] err           Pointer to the error code.
 */
void wps_request_write_register(wps_t *wps, uint8_t starting_reg, uint8_t data, wps_error_t *err);

/** @brief Issue a read register request to the WPS
 *
 *   @param[in] wps              Wireless Protocol Stack instance.
 *   @param[in] target_register  Radio Target register.
 *   @param[in] rx_buffer        Buffer containing radio register data.
 *   @param[in] xfer_cmplt       Bool to notify application that the transfer is complete.
 *   @param[in] err              WPS Error.
 */
void wps_request_read_register(wps_t *wps, uint8_t target_register, uint8_t *rx_buffer, bool *xfer_cmplt, wps_error_t *err);

/** @brief Process the wps callback
 *
 * This function should be called in a context with higher
 * priority than the application, but lower priority than the
 * radio IRQs. You can run it in a really high priority RTOS
 * task or something like pendsv.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
void wps_process_callback(wps_t *wps);

/** @brief Radio IRQ signal.
 *
 *  Notify the WPS of a context switch.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_radio_irq(wps_t *wps)
{
    if (wps->signal != WPS_CONNECT) {
        wps->signal = WPS_RADIO_IRQ;
        wps_phy_set_input_signal(wps->phy, PHY_SIGNAL_RADIO_IRQ);
        wps_phy_process(wps->phy);
    }
}

/** @brief SPI transfer complete.
 *
 *  Notify the WPS of a DMA transfer complete interrupt.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_transfer_complete(wps_t *wps)
{
    wps->signal = WPS_TRANSFER_COMPLETE;
    wps_phy_set_input_signal(wps->phy, PHY_SIGNAL_DMA_CMPLT);
    wps_phy_process(wps->phy);
}

#if WPS_RADIO_COUNT > 1

/** @brief Initialize the multi-radio BSP.
 *
 *  @param[in]  multi_cfg  Multi radio config.
 *  @param[out] err        Pointer to the error code.
 */
void wps_multi_init(wps_multi_cfg_t multi_cfg, wps_error_t *err);

/** @brief Process the MCU timer interrupt for Radio synchronization.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_multi_radio_timer_process(wps_t *wps)
{
    wps_phy_multi_process_radio_timer(wps->phy);
}

/** @brief Set which radio raises the SPI or radio interrupt.
 *
 *  @note This function should always be called before wps_process.
 *
 *  @param[in] index  Radio Index.
 */
static inline void wps_set_irq_index(uint8_t index)
{
    wps_phy_multi_set_current_radio_idx(index);
}

#endif


#ifdef __cplusplus
}
#endif

#endif /* WPS_H */
