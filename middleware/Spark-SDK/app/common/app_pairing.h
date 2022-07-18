/** @file  app_pairing.h
 *  @brief The pairing module is used exchange data between two devices and make them connect with each other.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef APP_PAIRING_H_
#define APP_PAIRING_H_

/* INCLUDES *******************************************************************/
#include "wps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Paired Device List */
#define PAIRING_DEVICE_LIST_MAX_COUNT 10 /* Up to 255 devices are supported */

/* Output power configuration */
#define PAIRING_TX_DATA_PULSE_COUNT 2
#define PAIRING_TX_DATA_PULSE_WIDTH 4
#define PAIRING_TX_DATA_PULSE_GAIN  0
#define PAIRING_TX_ACK_PULSE_COUNT  3
#define PAIRING_TX_ACK_PULSE_WIDTH  6
#define PAIRING_TX_ACK_PULSE_GAIN   0

/* Input power configuration */
#define PAIRING_RX_ACK_PULSE_COUNT  3 /* Pulses configuration of received ACK frames */
#define PAIRING_RX_DATA_PULSE_COUNT 3 /* Pulses configuration of received data frames */

/* SWC queue size */
#define PAIRING_DATA_QUEUE_SIZE 2

/* Misc configurations */
#define PAIRING_SWC_MODULATION  MODULATION_IOOK
#define PAIRING_SWC_FEC_LEVEL   FEC_LVL_2
#define PAIRING_SWC_SLEEP_LEVEL SLEEP_IDLE

/* Schedule configuration */
#define PAIRING_SCHEDULE { \
    500, 500 \
}
#define COORD_TO_NODE_TIMESLOTS { \
    MAIN_TIMESLOT(0) \
}
#define NODE_TO_COORD_TIMESLOTS { \
    MAIN_TIMESLOT(1) \
}

/* Channels */
#define PAIRING_CHANNEL_FREQ { \
    164, 171, 178, 185, 192 \
}
#define PAIRING_CHANNEL_SEQUENCE { \
    0, 1, 2, 3, 4 \
}

/* TYPES **********************************************************************/
typedef struct paired_device {
    uint16_t node_address;
    uint64_t unique_id;
} paired_device_t;

typedef struct app_pairing {
    wps_role_t network_role;
    uint16_t pan_id;
    uint8_t coordinator_address;
    uint8_t node_address;
    uint8_t device_role;
    paired_device_t paired_device[PAIRING_DEVICE_LIST_MAX_COUNT];
} app_pairing_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Function used in an application to pair devices together.
 *
 *  @param app_pairing_device  Pairing structure to exchange data between applications.
 *  @return If the pairing was successful or not.
 */
bool app_pairing_start_pairing_process(app_pairing_t *app_pairing_device);


#ifdef __cplusplus
}
#endif

#endif /* APP_PAIRING_H_ */
