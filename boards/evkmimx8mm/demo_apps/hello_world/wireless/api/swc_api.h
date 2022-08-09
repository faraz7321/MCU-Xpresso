/** @file  swc_api.h
 *  @brief SPARK Wireless Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_API_H_
#define SWC_API_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "swc_error.h"
#include "wps.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* TYPES **********************************************************************/
    /** @brief Wireless Core configuration.
     */
    typedef struct swc_cfg
    {
        uint32_t *timeslot_sequence;          /*!< Network schedule as an array of timeslot durations in microseconds */
        uint32_t timeslot_sequence_length;    /*!< Number of timeslots in the timeslot sequence */
        uint32_t *channel_sequence;           /*!< RF channels as an array of channel numbers */
        uint32_t channel_sequence_length;     /*!< Number of channels in the channel sequence */
        bool fast_sync_enabled;               /*!< Enable fast synchronization for low data rate links */
        bool random_channel_sequence_enabled; /*!< Enable random channel sequence concurrency mechanism */
        uint8_t *memory_pool;                 /*!< Memory pool instance from which memory allocation is done */
        uint32_t memory_pool_size;            /*!< Memory pool size in bytes */
    } swc_cfg_t;

    /** @brief Wireless Core hardware abstraction layer.
     */
    typedef struct swc_hal
    {
        radio_hal_t radio_hal[WPS_RADIO_COUNT]; /*!< Radio HAL */
        void (*context_switch)(void);           /*!< Context switch function pointer */
        uint64_t (*get_tick_quarter_ms)(void);  /*!< Get tick quarter ms function pointer */
#if (WPS_RADIO_COUNT == 2)
        wps_multi_cfg_t multi_cfg; /*!< MCU timer functions for dual radio configuration */
#endif
    } swc_hal_t;

    /** @brief Wireless node configuration.
     */
    typedef struct swc_node_cfg
    {
        wps_role_t role;             /*!< Network role */
        uint16_t pan_id;             /*!< Personal area network 12-bit ID */
        uint8_t coordinator_address; /*!< Coordinator device's 8-bit address; Same as local_address if local device is the Coordinator */
        uint8_t local_address;       /*!< Local device's 8-bit address */
        sleep_lvl_t sleep_level;     /*!< Sleep depth the transceiver will put itself when not active */
    } swc_node_cfg_t;

    /** @brief Wireless node.
     */
    typedef struct swc_node
    {
        uint8_t radio_count;           /*!< Number of radio added to the node */
        swc_node_cfg_t cfg;            /*!< Wireless node configuration */
        wps_node_t *wps_node_handle;   /*!< Low-level node handler */
        wps_radio_t *wps_radio_handle; /*!< Low-level radio handler */
    } swc_node_t;

    /** @brief Wireless statistics.
     */
    typedef struct swc_statistics
    {
        uint32_t packet_sent_and_acked_count;        /*!< Increments when an acknowledge frame is received after sending a packet */
        uint32_t packet_sent_and_not_acked_count;    /*!< Increments when an acknowledge frame is not received after sending a packet */
        uint32_t no_packet_tranmission_count;        /*!< Increments when there is nothing to send at the start of a TX timeslot */
        uint32_t packet_dropped_count;               /*!< Increments when a packet is dropped by the Wireless Core due to its timeout mechanism */
        uint32_t tx_timeslot_occurrence;             /*!< Increments for every TX timeslot the connection goes through */
        float tx_used_capacity_pc;                   /*!< Percentage of TX timeslots used for transmission over the total number of TX timeslots */
        uint32_t packet_successfully_received_count; /*!< Increments when a packet is received and the CRC checks */
        uint32_t no_packet_reception_count;          /*!< Increment when nothing is received at the start of a RX timeslot */
        uint32_t rx_timeslot_occurrence;             /*!< Increments for every RX timeslot the connection goes through */
        uint32_t packet_duplicated_count;            /*!< Increments when a packet is received but is discarded because it is a duplicate of a previously received packet */
        uint32_t packet_overrun_count;               /*!< Increments when a packet is received but is discarded because the Wireless Core reception queue is full */
    } swc_statistics_t;

    /** @brief Wireless radio configuration.
     */
    typedef struct swc_radio_cfg
    {
        irq_polarity_t irq_polarity; /*!< State of the radio IRQ pin when asserted */
        std_spi_t std_spi;           /*!< Radio SPI interface timing setting */
    } swc_radio_cfg_t;

    /** @brief Wireless connection configuration.
     */
    typedef struct swc_connection_cfg
    {
        char *name;                   /*!< Name of the connection as a character string */
        uint8_t source_address;       /*!< Address of the transmitting node */
        uint8_t destination_address;  /*!< Address of the receiving node */
        uint8_t max_payload_size;     /*!< Maximum size in bytes the payload can ever be */
        uint32_t queue_size;          /*!< Queue size in number of frames */
        modulation_t modulation;      /*!< Frame modulation */
        fec_level_t fec;              /*!< Frame forward error correction level */
        int32_t *timeslot_id;         /*!< ID of timeslots used by the connection */
        uint32_t timeslot_count;      /*!< Number of timeslots used by the connection */
        bool allocate_payload_memory; /*!< Whether or not payload memory allocation is managed by the connection (true) or the application (false) */
        bool ack_enabled;             /*!< Whether or not ACK frames are sent (RX connection) or receive (TX connection) on the connection */
        bool arq_enabled;             /*!< Whether or not retransmissions are enabled (needs ACK enabled) */
        struct
        {
            uint32_t retry_count;   /*!< Maximum number of retries (0 is infinite) before a frame is dropped (needs ARQ enabled) */
            uint32_t time_deadline; /*!< Maximum amount of time (0 is infinite) in increment of 250 us (e.g. 4 is 1 ms) a frame can sit in the queue before it is dropped (needs ARQ enabled) */
        } arq_settings;             /*!< Settings for the automatic repeat request feature (Set only if ARQ is enabled) */
        bool auto_sync_enabled;     /*!< Whether or not dummy frames are sent out by the Wireless Core if there is no application data in the queue */
        bool cca_enabled;           /*!< Whether or not energy is sensed on the channel before trying to transmit */
        struct
        {
            uint8_t threshold;             /*!< Energy level considered too high for a successful transmission, from 0 (extremely unsensitive, i.e. lots of energy sensed on the channel will still yield to a transmission)
                                                to 47 (extremely sensitive, i.e. little energy sensed on the channel will make the transmission abort) */
            uint8_t try_count;             /*!< Number of energy sensings to do before the fail action is executed */
            uint16_t retry_time;           /*!< Amount of time between energy sensings in increments of 48.8 us (e.g. 10 is ~500 us) */
            cca_fail_action_t fail_action; /*!< Action to do when the energy level sensed is still too high after the last energy sensing try */
        } cca_settings;                    /*!< Settings for the clear channel assessment feature (Set only if CCA is enabled) */
        bool throttling_enabled;           /*!< Whether or not this connection supports throttling */
        bool rdo_enabled;                  /*!< Whether or not the random data offset is used on this connection */
        bool fallback_enabled;             /*!< Whether ot not this connection can dynamically change its power settings based on the sent payload size */
        struct
        {
            uint8_t threshold;            /*!< Size of the payload in bytes that triggers the fallback mode */
            int8_t tx_pulse_count_offset; /*!< Offset on the pulse count to apply when going into fallback mode */
            int8_t tx_pulse_width_offset; /*!< Offset on the pulse width to apply when going into fallback mode */
            int8_t tx_pulse_gain_offset;  /*!< Offset on the pulse gain to apply when going into fallback mode */
        } fallback_settings;              /*!< Settings for the fallback mode feature (Set only if fallback is enabled) */
    } swc_connection_cfg_t;

    /** @brief Wireless connection.
     */
    typedef struct swc_connection
    {
        uint8_t channel_count;             /*!< Number of channels added to the connection */
        swc_connection_cfg_t cfg;          /*!< Wireless connection configuration */
        swc_statistics_t stats;            /*!< Wireless connection statistics */
        wps_connection_t *wps_conn_handle; /*!< Low-level connection handler */
    } swc_connection_t;

    /** @brief Wireless channel configuration.
     */
    typedef struct swc_channel_cfg
    {
        uint8_t frequency;      /*!< Frequency of the channel in increments of 40.96 MHz (e.g., 183 for 7.5 GHz) */
        uint8_t tx_pulse_count; /*!< Pulses number of the transmitted frames, from 1 to 3 */
        uint8_t tx_pulse_width; /*!< Pulses width of the transmitted frames, from 0 (narrow) to 7 (large) */
        uint8_t tx_pulse_gain;  /*!< Pulses amplitude of the transmitted frames, from 0 (max gain: 0 dB) to 3 (min gain: -1.8 dB) */
        uint8_t rx_pulse_count; /*!< Pulses number of the received frames, from 1 to 3, corresponding to the tx_pulse_count of the incoming frames */
    } swc_channel_cfg_t;

    /** @brief Wireless fallback information.
     */
    typedef struct swc_fallback_info
    {
        int32_t link_margin; /*!< Link margin value */
    } swc_fallback_info_t;

    /* PUBLIC FUNCTION PROTOTYPES *************************************************/
    /** @brief Wireless Core initialization.
     *
     *  This is the first API call that needs to be made when initializing and
     *  configuring the Wireless Core.
     *
     *  @param[in]  cfg  Wireless Core configuration.
     *  @param[in]  hal  Board specific functions.
     *  @param[out] err  Wireless Core error code.
     */
    void swc_init(swc_cfg_t cfg, swc_hal_t *hal, swc_error_t *err);

    /** @brief Initialize the local device.
     *
     *  This sets global device parameters.
     *
     *  @param[in]  cfg  Wireless radio configuration.
     *  @param[out] err  Wireless Core error code.
     *  @return Node handle.
     */
    swc_node_t *swc_node_init(swc_node_cfg_t cfg, swc_error_t *err);

    /** @brief Initialize and line a radio wireless node.
     *
     *  This must be called for every radio the wireless node is using. Most systems
     *  operate a single radio, but a dual radio configuration is also supported
     *  for specific use cases.
     *
     *  @param[in]  node  Node handle.
     *  @param[in]  cfg   Wireless radio configuration.
     *  @param[in]  hal   Board specific functions.
     *  @param[out] err   Wireless Core error code.
     */
    void swc_node_add_radio(swc_node_t *node, swc_radio_cfg_t cfg, swc_hal_t *hal, swc_error_t *err);

    /** @brief Initialize a connection.
     *
     *  A connection abstracts a one-way data flow between 2 devices (e.g., a coordinator and a node).
     *
     *  @param[in]  hal   Board specific functions.
     *  @param[in]  node  Node handle.
     *  @param[in]  cfg   Wireless connection configuration.
     *  @param[out] err   Wireless Core error code.
     *  @return Connection handle.
     */
    swc_connection_t *swc_connection_init(swc_node_t *node, swc_connection_cfg_t cfg, swc_hal_t *hal, swc_error_t *err);

    /** @brief Configure channels to use for a wireless connection.
     *
     *  @param[in]  conn  Connection handle.
     *  @param[in]  node  Node handle.
     *  @param[in]  cfg   Wireless channel configuration.
     *  @param[out] err   Wireless Core error code.
     */
    void swc_connection_add_channel(swc_connection_t *conn, swc_node_t *node, swc_channel_cfg_t cfg, swc_error_t *err);

    /** @brief Set the callback function to execute after a successful transmission.
     *
     *  @note If ACKs are enabled, this callback is triggered when the ACK frame is received.
     *        If ACKs are disabled, it triggers every time the frame is sent (if the callback is configured).
     *
     *  @param[in] conn  Connection handle.
     *  @param[in] cb    Callback function.
     */
    void swc_connection_set_tx_success_callback(swc_connection_t *conn, void (*cb)(void *conn));

    /** @brief Set the callback function to execute after an unsuccessful transmission.
     *
     *  @note If ACKs are enabled, this callback is triggered if an ACK is not received after a transmission.
     *        If ACKs are disabled, it never triggers since every transmission is considered a success.
     *
     *  @param[in] conn  Connection handle.
     *  @param[in] cb    Callback function.
     */
    void swc_connection_set_tx_fail_callback(swc_connection_t *conn, void (*cb)(void *conn));

    /** @brief Set the callback function to execute after the ARQ module discards a frame.
     *
     *  @note If ARQ is enabled, the frame can be dropped because the time deadline or the retry count deadline has been reached.
     *        If ARQ is disabled, it never triggers.
     *
     *  @param[in] conn  Connection handle.
     *  @param[in] cb    Callback function.
     */
    void swc_connection_set_tx_dropped_callback(swc_connection_t *conn, void (*cb)(void *conn));

    /** @brief Set the callback function to execute after a successful frame reception.
     *
     *  @note A reception is considered successful when the frame destination address matches the
     *        local address or the broadcast address and the CRC checks.
     *
     *  @param[in] conn  Connection handle.
     *  @param[in] cb    Callback function.
     */
    void swc_connection_set_rx_success_callback(swc_connection_t *conn, void (*cb)(void *conn));

    /** @brief Set the percentage of allocated timeslots to use.
     *
     *  The throttling feature reduces the usable bandwidth in order to reduce power consumption.
     *  By default, the active ratio is set to 100%. For example. if a ratio of 50% is set, only
     *  1 timeslot out of 2 will be usable by the connection. The transceiver will stay asleep
     *  in unused timeslots.
     *
     *  @param[in]  conn          Connection handle.
     *  @param[in]  active_ratio  Percentage of the allocated timeslots to use, from 0 to 100.
     *  @param[out] err           Wireless Core error code.
     */
    void swc_connection_set_throttling_active_ratio(swc_connection_t *conn, uint8_t active_ratio, swc_error_t *err);

    /** @brief Get a buffer from the connection queue.
     *
     *  @param[in]  conn            Connection handle.
     *  @param[out] payload_buffer  Free payload buffer if available, NULL otherwise.
     *  @param[out] err             Wireless Core error code.
     */
    void swc_connection_get_payload_buffer(swc_connection_t *conn, uint8_t **payload_buffer, swc_error_t *err);

    /** @brief Enqueue a payload buffer in the connection transmission queue.
     *
     *  @param[in]  conn            Connection handle.
     *  @param[in]  payload_buffer  Buffer containing the payload to transmit.
     *  @param[in]  size            Size of the payload.
     *  @param[out] err             Wireless Core error code.
     */
    void swc_connection_send(swc_connection_t *conn, uint8_t *payload_buffer, uint8_t size, swc_error_t *err);

    /** @brief Retrieve a payload buffer from the connection reception queue.
     *
     *  @param[in]  conn            Connection handle.
     *  @param[in]  payload_buffer  Address of the buffer where to put the payload.
     *  @param[out] err             Wireless Core error code.
     *  @return Size of the payload.
     */
    uint8_t swc_connection_receive(swc_connection_t *conn, uint8_t **payload_buffer, swc_error_t *err);

    /** @brief Free the last received payload buffer from the connection reception queue.
     *
     *  @param[in]  conn  Connection handle.
     *  @param[out] err   Wireless Core error code.
     */
    void swc_connection_receive_complete(swc_connection_t *conn, swc_error_t *err);

    /** @brief Wireless Core setup.
     *
     *  This is the last API call that needs to be made when initializing and
     *  configuring the Wireless Core.
     *
     *  @param[in] node  Node handle.
     */
    void swc_setup(swc_node_t *node);

    /** @brief Start Wireless Core network activity.
     *
     *  This is called once the Wireless Core initialization and configuration
     *  is done (i.e., after swc_setup()).
     */
    void swc_connect(void);

    /** @brief Stop Wireless Core network activity.
     */
    void swc_disconnect(void);

    /** @brief Get information used when the fallback mode is enabled.
     *
     *  This is used by a node receiving data through a connection
     *  with fallback enabled. The information retrieved here must
     *  be sent back to the original sender usually through an
     *  auto-reply timeslot.
     *
     *  @param[in] conn  Connection handle.
     *  @return Fallback information.
     */
    swc_fallback_info_t swc_connection_get_fallback_info(swc_connection_t *conn);

    /** @brief Get the number of bytes allocated in the memory pool.
     *
     *  @return Number of bytes allocated in the memory pool.
     */
    uint32_t swc_get_allocated_bytes(void);

    /** @brief Free the memory to reconfigure the wireless core.
     */
    void swc_free_memory(void);

    /** @brief Function to call to process the Wireless Core callback queue.
     *
     *  The callbacks are TX ACK, TX NACK and RX. It is suggested to configure
     *  a software interrupt (SWI) and put this function into its IRQ handler.
     *  The Wireless Core can then trigger it with a call to the context_switch()
     *  function from its HAL structure.
     */
    void swc_connection_callbacks_processing_handler(void);

#if (WPS_RADIO_COUNT == 1)
    /** @brief Function to call in the external interrupt handler servicing the radio IRQ.
     */
    void swc_radio_irq_handler(void);

    /** @brief Function to call in the radio DMA SPI receive complete IRQ handler.
     */
    void swc_radio_spi_receive_complete_handler(void);
#elif (WPS_RADIO_COUNT == 2)
/** @brief Function to call in the external interrupt handler servicing radio #1 IRQ.
 */
void swc_radio1_irq_handler(void);

/** @brief Function to call in radio #1 DMA SPI receive complete IRQ handler.
 */
void swc_radio1_spi_receive_complete_handler(void);

/** @brief Function to call in the external interrupt handler servicing radio #2 IRQ.
 */
void swc_radio2_irq_handler(void);

/** @brief Function to call in radio #2 DMA SPI receive complete IRQ handler.
 */
void swc_radio2_spi_receive_complete_handler(void);

/** @brief Callback for the Radio sync timer.
 */
void swc_radio_synchronization_timer_callback(void);
#else
#error "Number of radios must be either 1 or 2"
#endif

#ifdef __cplusplus
}
#endif

#endif /* SWC_API_H_ */
