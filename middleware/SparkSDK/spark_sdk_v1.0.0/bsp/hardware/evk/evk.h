/** @file  evk.h
 *  @brief Board Support Package for SPARK EVK board.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_H_
#define EVK_H_

/* INCLUDES *******************************************************************/
#include "evk_audio.h"
#include "evk_button.h"
#include "evk_clock.h"
#include "evk_dbg.h"
#include "evk_flash.h"
#include "evk_it.h"
#include "evk_led.h"
#include "evk_power.h"
#include "evk_radio.h"
#include "evk_timer.h"
#include "evk_uart.h"
#include "evk_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the board's peripherals.
 */
void evk_init(void);

/** @brief Reset the MCU.
 */
void evk_system_reset(void);


#ifdef __cplusplus
}
#endif

#endif /* EVK_H_ */

