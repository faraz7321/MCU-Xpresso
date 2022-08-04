/** @file  evk_it.h
 *  @brief This module controls interrupt related features.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_IT_H_
#define EVK_IT_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Interrupt's module function callback type.
 */
typedef void (*irq_callback)(void);

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief This functions systick callback that is map to the TIM8 irq.
 */
void evk_it_set_systick_callback(irq_callback callback);

/** @brief This functions set the function callback for the every interrupt
 */
void evk_it_set_common_callback(irq_callback callback);

/** @brief This function sets the function callback for USB detection interrupt.
 */
void evk_set_usb_detect_callback(irq_callback callback);

/** @brief Set the application user timer callback.
 *
 *  @note This timer is used to mock application priority
 *        for WPS timing and behavior caracterization.
 *
 *  @param[in] callback  Timer user callback.
 */
void evk_set_app_timer_callback(irq_callback callback);

/** @brief This function set the function callback for the radio pin interrupt.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void evk_set_radio_irq_callback(irq_callback callback);

/** @brief This function sets the function callback for the DMA_RX ISR.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void evk_set_radio_dma_rx_callback(irq_callback callback);

/** @brief Set the application user timer callback.
 *
 *  @note This timer is used to mock application priority
 *        for WPS timing and behavior caracterization.
 *
 *  @param[in] callback  Timer user callback.
 */
void evk_set_app_timer_callback(irq_callback callback);

/** @brief This function sets the function callback for the pendsv.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void evk_set_pendsv_callback(irq_callback callback);

/** @brief Set USB IRQ callback.
 *
 *  @param[in] callback  USB IRQ callback.
 */
void evk_set_usb_irq_callback(irq_callback callback);

/** @brief Disable IRQ Interrupts
 */
void evk_enter_critical(void);

/** @brief Enable IRQ Interrupts
 */
void evk_exit_critical(void);

/** @brief Error handle used by STM32 HAL.
 */
void Error_Handler(void);


#ifdef __cplusplus
}
#endif

#endif /* EVK_IT_H_ */

