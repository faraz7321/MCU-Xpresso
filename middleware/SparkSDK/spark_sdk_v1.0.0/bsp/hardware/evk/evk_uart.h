/** @file  evk_uart.h
 *  @brief This module controls UART features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_UART_H_
#define EVK_UART_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Read data on the LPUART which is on the expansion board.
 *
 *  @param[in]  data  Data buffer to write.
 *  @param[in]  size  Size of the data to write.
 */
void evk_exp_uart_read_blocking(uint8_t *data, uint16_t size);

/** @brief Write data on the UART log.
 *
 *  @param[in]  data  Data buffer to write.
 *  @param[in]  size  Size of the data to write.
 */
void evk_exp_uart_write_blocking(uint8_t *data, uint16_t size);

/** @brief Write data on the UART log in non blocking mode.
 *
 *  @param[in]  data  Data buffer to write.
 *  @param[in]  size  Size of the data to write.
 */
void evk_exp_uart_write_non_blocking(uint8_t *data, uint16_t size);

/** @brief Read byte on the UART in non blocking mode.
 *
 *  @param[in] data  Data byte to write.
 */
void evk_exp_uart_read_byte_non_blocking(uint8_t *data);

/** @brief Set the LPUART TX callback used by the ST-Link.
 *
 *  @param[in] callback  Pointer to the LPUART TX callback.
 */
void evk_set_stlink_uart_tx_callback(void (*callback)(void));

/** @brief Set the LPUART RX callback used by the ST-Link.
 *
 *  @param[in] callback  Pointer to the LPUART RX callback.
 */
void evk_set_stlink_uart_rx_callback(void (*callback)(void));

/** @brief Set the UART RX callback.
 *
 *  @param[in] callback  Pointer to the UART RX callback
 */
void evk_set_exp_uart_rx_callback(void (*callback)(void));

/** @brief Set the UART TX callback.
 *
 *  @param[in] callback  Pointer to the UART TX callback.
 */
void evk_set_exp_uart_tx_callback(void (*callback)(void));


#ifdef __cplusplus
}
#endif

#endif /* EVK_UART_H_ */

