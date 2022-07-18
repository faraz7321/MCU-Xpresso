/** @file  evk_led.c
 *  @brief This module controls LED features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_led.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_led_on(evk_led_t led)
{
    switch (led) {
    case LED0:
        HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_SET);
        break;

    case LED1:
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
        break;

    case LED2:
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
        break;

    default:
        break;
    }
}

void evk_led_off(evk_led_t led)
{
    switch (led) {
    case LED0:
        HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_RESET);
        break;

    case LED1:
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
        break;

    case LED2:
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

void evk_led_toggle(evk_led_t led)
{
    switch (led) {
    case LED0:
        HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN);
        break;

    case LED1:
        HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
        break;

    case LED2:
        HAL_GPIO_TogglePin(LED2_PORT, LED2_PIN);
        break;
    default:
        break;
    }
}
