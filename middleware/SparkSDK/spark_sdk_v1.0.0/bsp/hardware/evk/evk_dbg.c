/** @file  evk_db.c
 *  @brief This module controls debug io of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_dbg.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_dbg_on(evk_dbg_t io)
{
    switch (io) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_SET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_SET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_SET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_SET);
        break;
    default:
        break;
    }
}

void evk_dbg_off(evk_dbg_t io)
{
    switch (io) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_RESET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_RESET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

void evk_dbg_toggle(evk_dbg_t io)
{
    switch (io) {
    case DBG0:
        HAL_GPIO_TogglePin(EXP_PC8_PORT, EXP_PC8_PIN);
        break;
    case DBG1:
        HAL_GPIO_TogglePin(EXP_PC7_PORT, EXP_PC7_PIN);
        break;
    case DBG2:
        HAL_GPIO_TogglePin(EXP_PA3_PORT, EXP_PA3_PIN);
        break;
    case DBG3:
        HAL_GPIO_TogglePin(EXP_PC11_PORT, EXP_PC11_PIN);
        break;
    default:
        break;
    }
}

void evk_dbg_pulse(evk_dbg_t io)
{
    switch (io) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_RESET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_RESET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}
