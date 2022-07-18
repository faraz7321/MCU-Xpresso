/** @file  iface_audio_high_resolution.h
 *  @brief This file contains the prototypes of functions configuring the
 *         audio high resolution application which calls the underlying BSP functions.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef IFACE_AUDIO_HIGH_RESOLUTION_H_
#define IFACE_AUDIO_HIGH_RESOLUTION_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize hardware drivers in the underlying board support package.
 */
void iface_board_init(void);

/** @brief Initialize the coordinator's SAI peripheral.
 *
 *  Configure the serial audio interface to MONO or Stereo
 */
void iface_audio_coord_init(void);

/** @brief Initialize the node's SAI peripheral.
 *
 *  Configure the serial audio interface to MONO or Stereo
 */
void iface_audio_node_init(void);

/** @brief Set the serial audio interface transfer complete callbacks
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param tx_callback  Audio i2s tx complete callback
 *  @param rx_callback  Audio i2s rx complete callback
 */
void iface_set_sai_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void));

/** @brief Poll for button presses.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param[in] button1_callback  Function to execute when pressing button #1.
 *  @param[in] button2_callback  Function to execute when pressing button #2.
 */
void iface_button_handling(void (*button1_callback)(void), void (*button2_callback)(void));

/** @brief Notify user of the wireless TX connection status.
 */
void iface_tx_conn_status(void);

/** @brief Notify user of the wireless RX connection status.
 */
void iface_rx_conn_status(void);


#ifdef __cplusplus
}
#endif

#endif /* IFACE_AUDIO_HIGH_RESOLUTION_H_ */
