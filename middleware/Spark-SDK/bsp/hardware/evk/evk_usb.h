/** @file  evk_usb.h
 *  @brief This module controls USB features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_USB_H_
#define EVK_USB_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define USBD_VID                   1155                        /* STMicroelectronics */
#define USBD_LANGID_STRING         1033                        /* ENGLISH US */
#define USBD_MANUFACTURER_STRING   "SPARK Microsystems Inc."
#define USBD_PID                   22336                       /* Virtual COM Port */
#define USBD_PRODUCT_STRING        "EVK1.4"
#define USBD_CONFIGURATION_STRING  "CDC Config"
#define USBD_INTERFACE_STRING      "CDC Interface"

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Disable USB interrupt.
 */
void evk_usb_enter_critical(void);

/** @brief Enable USB interrupt.
 */
void evk_usb_exit_critical(void);

/** @brief Check if a powered USB cable is connected to the board.
 *
 *  @retval True   USB is connected.
 *  @retval False  USB is not connected.
 */
bool evk_is_usb_detected(void);


#ifdef __cplusplus
}
#endif

#endif /* EVK_USB_H_ */

