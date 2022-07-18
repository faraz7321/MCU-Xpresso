/** @file  iface_audio_4-channel.h
 *  @brief This file contains the prototypes of functions configuring the
 *         audio 4 channel application which calls the underlying BSP functions.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef IFACE_AUDIO_4_CHANNEL_H_
#define IFACE_AUDIO_4_CHANNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize hardware drivers in the underlying board support package.
 */
void iface_board_init(void);

/** @brief Initialize the coordinator's peripheral.
 *
 *  Configure the serial audio interface to MONO;
 *  Activate DMA peripheral;
 *  Codec sampling rate to 48Khz;
 *  Codec record enabled;
 *  Codec playback disabled;
 *  Codec playback and record filter disabled.
 */
void iface_audio_coord_init(void);

/** @brief Initialize the node's peripheral.
 *
 *  Configure the serial audio interface to MONO;
 *  Activate DMA peripheral;
 *  Codec sampling rate to 48Khz;
 *  Codec record disabled;
 *  Codec playback enable;
 *  Codec playback and record filter disabled.
 */
void iface_audio_node_init(void);

/** @brief Set the serial audio interface transfer complete callbacks
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param tx_callback  Audio i2s tx complete callback
 *  @param rx_callback  Audio i2s rx complete callback
 */
void iface_audio_dma_complete_callbacks(void (*tx_callback)(void), void (*rx_callback)(void));

/** @brief Poll for button presses.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param[in] button1_callback  Function to execute when pressing button #1.
 *  @param[in] button2_callback  Function to execute when pressing button #2.
 */
void iface_volume_handling(void (*button1_callback)(void), void (*button2_callback)(void));

/** @brief Notify user of the wireless TX connection status.
 */
void iface_tx_conn_status(void);

/** @brief Notify user of the wireless RX connection status.
 */
void iface_rx_conn_status(void);


#ifdef __cplusplus
}
#endif

#endif /* IFACE_AUDIO_4_CHANNEL_H_ */
