/** @file  iface_pairing_evk.c
 *  @brief This file contains the implementation of functions configuring the
 *         pairing common code which calls the functions of the BSP of the EVK1.4.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "iface_pairing.h"
#include "evk.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void iface_set_watchdog_timer_callback(void (*callback)(void))
{
    evk_set_app_timer_callback((irq_callback)callback);
}

void iface_set_watchdog_timer_limit(uint32_t us_limit)
{
    evk_timer_app_timer_init(us_limit);
}

void iface_start_watchdog_timer(void)
{
    evk_timer_app_timer_start();
}

void iface_stop_watchdog_timer(void)
{
    evk_timer_app_timer_stop();
}

void iface_delay_ms(uint32_t ms_delay)
{
    evk_timer_delay_ms(ms_delay);
}
/* PRIVATE FUNCTIONS **********************************************************/
