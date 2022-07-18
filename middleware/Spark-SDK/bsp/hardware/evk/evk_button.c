/** @file  evk_button.c
 *  @brief This module controls button feature of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_button.h"

/* PUBLIC FUNCTIONS ***********************************************************/
bool evk_read_btn_state(evk_btn_t btn)
{
    bool btn_state = false;

    switch (btn) {
    case BTN1:
        btn_state = HAL_GPIO_ReadPin(BTN0_PORT, BTN0_PIN);
        break;
    case BTN2:
        btn_state = HAL_GPIO_ReadPin(BTN1_PORT, BTN1_PIN);
        break;
    default:
        break;
    }
    return btn_state;
}
