/** @file  wps_def.h
 *  @brief Wireless Protocol Stack definitions used by multiple modules.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_DEF_H
#define WPS_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include "circular_queue.h"
#include "link_cca.h"
#include "link_fallback.h"
#include "link_gain_loop.h"
#include "link_lqi.h"
#include "link_protocol.h"
#include "link_random_datarate_offset.h"
#include "link_saw_arq.h"
#include "sr_api.h"
#include "sr_spectral.h"
#include "sr_def.h"
#include "wps_error.h"

/* CONSTANTS ******************************************************************/
#ifndef WPS_RADIO_COUNT
#define WPS_RADIO_COUNT 1 /*!< WPS radio count */
#endif

#define WPS_RADIO_FIFO_SIZE        128 /*!< WPS radio FIFO size */
#define WPS_PAYLOAD_SIZE_BYTE_SIZE 1   /*!< Size of the payload size automatically loaded in the FIFO. */

#ifndef WPS_NB_RF_CHANNEL
#define WPS_NB_RF_CHANNEL 5 /*!< Number of RF channels */
#endif

#define WPS_CONNECTION_THROTTLE_GRANULARITY 20  /*!< WPS throttle ratio granularity (100 / value) */
#define WPS_REQUEST_MEMORY_SIZE             2   /*!< WPS request queue size */
#define WPS_RADIO_SPI_BUFFER_SIZE           200 /*!< WPS radio SPI buffer size */

/* TYPES **********************************************************************/
/** @brief WPS Connection.
 */
typedef struct wps_connection wps_connection_t;

/** @brief WPS statistics function.
 */
typedef struct wps_stats {
    uint32_t tx_success;       /*!< Number of payload sent */
    uint32_t tx_byte_sent;     /*!< Number of byte sent */
    uint32_t tx_drop;          /*!< Number of payload dropped */
    uint32_t tx_fail;          /*!< Number of TX payload fail */
    uint32_t rx_received;      /*!< Number of payload received */
    uint32_t rx_byte_received; /*!< Number of byte received */
    uint32_t rx_overrun;       /*!< Number of payload dropped because of an RX buffer overrun */
    uint32_t cca_pass;         /*!< Number of CCA TX abort */
    uint32_t cca_fail;         /*!< Number of CCA TX anyway */
} wps_stats_t;

/** @brief WPS Connection
 */
struct wps_connection {
    uint16_t source_address;        /*!< Source address */
    uint16_t destination_address;   /*!< Destination address */

    bool fixed_payload_size_enable; /*!< Fixed payload size enable flag (necessary for Frames lager than RX FIFO) */
    uint8_t fixed_payload_size;     /*!< Frame size (only used if fixed frame size mode is enabled) */
    wps_error_t wps_error;          /*!< WPS error */

    /* Layer 2 */
    bool phases_exchange;                 /*!< Send phases data in the header to other device */
    bool ack_enable;                      /*!< Ack received frame or expect ack when sending frame */
    bool auto_sync_enable;                /*!< Auto sync mode enable flag*/
    uint8_t header_size;                  /*!< Header size in bytes */
    link_protocol_t link_protocol;        /*!< Internal connection protocol */
    saw_arq_t stop_and_wait_arq;          /*!< Stop and Wait (SaW) and Automatic Repeat Query (ARQ) */
    link_cca_t cca;                       /*!< Clear Channel Assessment */
    link_fallback_t link_fallback;        /*!< Fallback Module instance */
    lqi_t lqi;                            /*!< Link quality indicator */
    lqi_t used_frame_lqi;                 /*!< WPS frames Link quality indicator (Excludes unused or sync timeslots)*/
    lqi_t channel_lqi[WPS_NB_RF_CHANNEL]; /*!< Channel frames Link quality indicator */
    wps_stats_t wps_stats;                /*!< Wireless protocol stack statistics */

    /* Link throttle */
    uint8_t pattern_count;                /*!< Current pattern array index count */
    uint8_t active_ratio;                 /*!< Active timeslot ratio, in percent */
    uint8_t pattern_total_count;          /*!< Total pattern array count based on reduced ratio fraction */
    bool *pattern;                        /*!< Pattern array pointer, need to be allocated by application and initialized to 1 */

    /* Gain loop */
    gain_loop_t gain_loop[WPS_NB_RF_CHANNEL][WPS_RADIO_COUNT];  /*!< Gain loop */

    /* Queue */
    circular_queue_t xlayer_queue; /*!< Cross layer queue */

    /* Layer 1 */
    frame_cfg_t  frame_cfg;                                      /*!< Connection frame config */
    sleep_lvl_t  sleep_lvl;                                      /*!< Connection sleep level */
    rf_channel_t (*channel)[WPS_NB_RF_CHANNEL][WPS_RADIO_COUNT]; /*!< RF Channel information */
    packet_cfg_t packet_cfg;                                     /*!< Packet configuration */

    /* Callback */
    void (*tx_success_callback_t)(void *parg); /*!< Function called by the wps to indicate the frame has been successfully transmitted */
    void (*tx_fail_callback_t)(void *parg);    /*!< Function called by the wps to indicate the transmission failed */
    void (*tx_drop_callback_t)(void *parg);    /*!< Function called by the wps to indicate a frame is dropped */
    void (*rx_success_callback_t)(void *parg); /*!< Function called by the wps to indicate the frame has been received */
    void (*evt_callback_t)(void *parg);        /*!< Function called by the wps to indicate that a WPS event appened */

    void *tx_success_parg_callback_t; /*!< TX success callback void pointer argument. */
    void *tx_fail_parg_callback_t;    /*!< TX fail callback void pointer argument. */
    void *tx_drop_parg_callback_t;    /*!< TX drop callback void pointer argument. */
    void *rx_success_parg_callback_t; /*!< RX success callback void pointer argument. */
    void *evt_parg_callback_t;        /*!< Event callback void pointer argument. */
    uint64_t (*get_tick_quarter_ms)(void); /*!< Get free running timer tick in quarter ms */
};

/** @brief WPS role enumeration.
 */
typedef enum wps_role {
    NETWORK_COORDINATOR = 0, /*!< Coordinator dictate the time to the whole network */
    NETWORK_NODE             /*!< Node re-adjust its timer to stay in sync */
} wps_role_t;

/** @brief Wireless Protocol Stack radio.
 *
 *  @note This is the parameter to setup one
 *        radio instance.
 */
typedef struct wps_radio {
    radio_t       radio;                                    /*!< Radio instance */
    radio_hal_t   radio_hal;                                /*!< Radio HAL instance */
    calib_vars_t *spectral_calib_vars;                      /*!< Calibration variables */
    nvm_t        *nvm;                                      /*!< NVM variables */
    uint8_t       spi_rx_buffer[WPS_RADIO_SPI_BUFFER_SIZE]; /*!< SPI RX transfer buffer */
    uint8_t       spi_tx_buffer[WPS_RADIO_SPI_BUFFER_SIZE]; /*!< SPI TX transfer buffer */
} wps_radio_t;

/** @brief WPS Node definition.
 *
 *  @note This is the parameters used to
 *        setup on node instance. One node
 *        can contains multiple radio.
 */
typedef struct node {
    wps_radio_t    *radio;              /*!< Wireless Protocol Stack radio */
    wps_role_t      role;               /*!< WPS role, Node or Coordinator */
    uint16_t        local_address;      /*!< Node address */
    syncword_cfg_t  syncword_cfg;       /*!< Radio(s) configuration syncword */
    uint32_t        preamble_len;       /*!< Radio(s) preamble lenght */
    uint8_t         rx_gain;            /*!< Radio(s) default rx gain */
    sleep_lvl_t     sleep_lvl;          /*!< Radio(s) sleep level */
    uint32_t        crc_polynomial;     /*!< Radio(s) crc polynomial */
    isi_mitig_t     isi_mitig;          /*!< Radio(s) ISI mitigation level */
    chip_rate_cfg_t chip_rate;          /*!< Chip rate (SR1120 only) */
} wps_node_t;

/** @brief Received frame.
 */
typedef struct rx_frame {
    uint8_t *payload;  /*!< Pointer to payload */
    uint8_t  size;     /*!< Size of payload */
} wps_rx_frame;

/** @brief WPS request type enumeration.
 */
typedef enum wps_request {
    REQUEST_NONE = 0,                  /*!< No request */
    REQUEST_MAC_CHANGE_SCHEDULE_RATIO, /*!< Request allowing application to change active timeslot ratio */
    REQUEST_PHY_WRITE_REG,             /*!< Request allowing application to write to a register */
    REQUEST_PHY_READ_REG,              /*!< Request allowing application to read a register */
} wps_request_t;

/** @brief WPS schedule request configuration.
 */
typedef struct wps_schedule_ratio_cfg {
    uint8_t active_ratio;               /*!< Target connection current active ratio */
    uint8_t pattern_total_count;        /*!< Target connection total pattern array size */
    uint8_t pattern_current_count;      /*!< Target connection pattern index count */
    wps_connection_t *target_conn;      /*!< Connection to change active timeslot ratio */
    circular_queue_t *pattern_queue;    /*!< Pattern buffer queue */
} wps_schedule_ratio_cfg_t;

/** @brief WPS write register request info structure.
 */
typedef struct wps_write_request_info {
    uint8_t target_register; /**< Target register to write data */
    uint8_t data;            /**< Data to send to the radio register */
    bool pending_request;    /**< Flag to notify that a request is pending */
} wps_write_request_info_t;

/** @brief WPS read register request info structure.
 */
typedef struct wps_read_request_info {
    uint8_t target_register; /*!< Target register to read. */
    uint8_t *rx_buffer;      /*!< RX buffer containing register value */
    bool pending_request;    /**< Flag to notify that a request is pending */
    bool *xfer_cmplt;        /*!< Bool to notify that read register is complete */
} wps_read_request_info_t;

/** @brief WPS request structure configuration.
 *
 *  @note Available choice for configuration structure are
 *      - wps_schedule_ratio_cfg_t
 *      - wps_write_request_info_t
 *      - wps_read_request_info_t
 */
typedef struct wps_request_info {
    void *config;       /*!< WPS request structure configuration. */
    wps_request_t type; /*!< WPS request type enumeration */
} wps_request_info_t;

/** @brief WPS events callback.
 */
typedef void (*wps_callback_t)(void *parg);


#ifdef __cplusplus
}
#endif

#endif /* WPS_DEF_H */
