/** @file  iface_app_pairing.h
 *  @brief This file contains the prototypes of functions configuring the
 *         app_pairing common layer functions which calls the underlying BSP functions.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef IFACE_APP_PAIRING_H_
#define IFACE_APP_PAIRING_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))
#define CONCATENATE(a, b) (a << 8 | b)

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Set the pairing watchdog timer application callback.
 *
 *  @param[in] callback  Function callback
 */
void iface_set_watchdog_timer_callback(void (*callback)(void));

/** @brief Set the watchdog timer limit.
 *
 * @param[in] us_limit  Timer limit in us.
 */
void iface_set_watchdog_timer_limit(uint32_t us_limit);

/** @brief Start watchdog timer.
 */
void iface_start_watchdog_timer(void);

/** @brief Stop watchdog timer.
 */
void iface_stop_watchdog_timer(void);

/** @brief Blocking delay with a 1ms resolution.
 *
 *  @param[in] ms_delay  milli secondes delay.
 */
void iface_delay_ms(uint32_t ms_delay);


#ifdef __cplusplus
}
#endif

#endif /* IFACE_APP_PAIRING_H_ */
