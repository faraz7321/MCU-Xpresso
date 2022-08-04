/** @file  evk_timer.h
 *  @brief This module controls timer features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_TIMER_H_
#define EVK_TIMER_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get timebase tick value.
 *
 *  @return Tick value.
 */
uint32_t evk_timer_get_ms_tick(void);

/** @brief Blocking delay with a 1ms resolution.
 */
void evk_timer_delay_ms(uint32_t ms);

/** @brief Get the current tick in micro seconds.
 *
 *  @return Current value of the micro seconds timer
 */
uint32_t evk_timer_get_tick_us(void);

/** @brief Produce a delay in micro seconds.
 *
 *  @param[in] Delay  Delay in micro seconds to wait.
 */
void evk_timer_delay_us(uint32_t delay);

/** @brief Free running timer with a tick of 1 ms.
 *
 *  @return Tick count.
 */
uint64_t evk_timer_get_free_running_tick_ms(void);

/** @brief Initialize the application timer.
 *
 *  @note This timer is used to mock an application
 *        interrupt time and priority since the datacom
 *        task do not use any peripherals/interrupt that
 *        that have a priority higher than the WPS.
 *
 *  @param[in] period_us Timer period, in us.
 */
void evk_timer_app_timer_init(uint32_t period_us);

/** @brief Start the application timer.
 *
 *  @note This starts the interrupt generation
 *        of the application timer.
 */
void evk_timer_app_timer_start(void);

/** @brief Stop the application timer.
 *
 *  @note This stops the interrupt generation
 *        of the application timer.
 */
void evk_timer_app_timer_stop(void);

/** @brief Initialize the systick timer.
 *
 *  @note This timer is used to replace systick timer that the OS use.
 *
 */
void evk_timer_systick_timer_init(void);

/** @brief Start systick timer
 *
 *  @note This starts the interrupt generation
 *        of the systick timer.
 */
void evk_timer_systick_timer_start(void);

/** @brief Stop the systick timer.
 *
 *  @note This stops the interrupt generation
 *        of the systick timer.
 */
void evk_timer_systick_timer_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* EVK_TIMER_H_ */

