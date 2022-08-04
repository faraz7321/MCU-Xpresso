/** @file  evk_usb.c
 *  @brief This module controls USB features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_usb.h"

/* PUBLIC FUNCTIONS ***********************************************************/
bool evk_is_usb_detected(void)
{
    if (HAL_GPIO_ReadPin(USB_DETECT_PORT, USB_DETECT_PIN) == GPIO_PIN_RESET) {
        return true;
    } else {
        return false;
    }
}

void evk_usb_enter_critical(void)
{
    NVIC_DisableIRQ(NVIC_USB_LP_IRQ);
}

void evk_usb_exit_critical(void)
{
    NVIC_EnableIRQ(NVIC_USB_LP_IRQ);
}