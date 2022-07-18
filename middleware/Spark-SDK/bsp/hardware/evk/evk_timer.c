/** @file  evk_timer.c
 *  @brief This module controls timer features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_it.h"
#include "evk_timer.h"
#include "evk_clock.h"

/* CONSTANT *******************************************************************/
#define MAX_VALUE_UINT16_T  0xffff
#define APP_TIMER_PRESCALER 3
#define SYSTICK_TIMER_PRESCALER 3
#define SYSTICK_TIMER_FREQ 1000

/* TYPES **********************************************************************/
TIM_HandleTypeDef htim3 = {
    .Instance           = TIM3,
    .Init.CounterMode   = TIM_COUNTERMODE_UP,
    .Init.Period        = 0xffff,
    .Init.ClockDivision = TIM_CLOCKDIVISION_DIV1,
};
TIM_HandleTypeDef htim6 = {
    .Instance           = TIM6,
    .Init.CounterMode   = TIM_COUNTERMODE_UP,
    .Init.Period        = 0xffff,
    .Init.ClockDivision = TIM_CLOCKDIVISION_DIV1,
};
TIM_HandleTypeDef  htim8 = {
    .Instance         = TIM8,
    .Init.Prescaler   = SYSTICK_TIMER_PRESCALER,
    .Init.CounterMode = TIM_COUNTERMODE_UP,
    .Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE
};
TIM_HandleTypeDef htim20  = {
    .Instance           = TIM20,
    .Init.Prescaler   = APP_TIMER_PRESCALER - 1,
    .Init.CounterMode   = TIM_COUNTERMODE_UP,
    .Init.ClockDivision = TIM_CLOCKDIVISION_DIV1,
    .Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
};

/* PUBLIC FUNCTIONS ***********************************************************/
uint32_t evk_timer_get_ms_tick(void)
{
    return HAL_GetTick();
}

void evk_timer_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t evk_timer_get_tick_us(void)
{
    return __HAL_TIM_GET_COUNTER(&htim3);
}

void evk_timer_delay_us(uint32_t delay)
{
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    while(__HAL_TIM_GET_COUNTER(&htim3) < delay);
}

uint64_t evk_timer_get_free_running_tick_ms(void)
{
    static uint64_t count;
    static uint32_t last_time;
    uint64_t ret_count;

    evk_enter_critical();
    uint32_t current_time = __HAL_TIM_GET_COUNTER(&htim6) / 4;
    if(last_time > current_time)
    {
        count += (((MAX_VALUE_UINT16_T) / 4) - last_time) + current_time ;
    } else {
        count += current_time - last_time;
    }
    last_time = current_time;
    ret_count = count;
    evk_exit_critical();
    return ret_count;
}

void evk_timer_app_timer_init(uint32_t period_us)
{
    uint32_t uwTimclock = 0;
    uint32_t frequency  = 1000000 / period_us;

    /* Enable TIM20 clock */
    __HAL_RCC_TIM20_CLK_ENABLE();
    uwTimclock = HAL_RCC_GetPCLK1Freq() / APP_TIMER_PRESCALER;
    htim20.Init.Period      = (uwTimclock / frequency) - 1; /* 1/4000 s*/
    if (HAL_TIM_Base_Init(&htim20) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(NVIC_TIMER_20, PRIO_APP_TIMER_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_TIMER_20);
}

void evk_timer_app_timer_start(void)
{
    if (HAL_TIM_Base_Start_IT(&htim20) != HAL_OK) {
        Error_Handler();
    }
}

void evk_timer_app_timer_stop(void)
{
    if (HAL_TIM_Base_Stop_IT(&htim20) != HAL_OK) {
        Error_Handler();
    }
}

void evk_timer_systick_timer_init(void)
{
    /* Enable TIM8 clock */
    __HAL_RCC_TIM8_CLK_ENABLE();

    htim8.Init.Period = evk_get_system_clock_freq() / ((SYSTICK_TIMER_PRESCALER + 1) * SYSTICK_TIMER_FREQ) - 1;
    if (HAL_TIM_Base_Init(&htim8) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(NVIC_SYSTICK_TIMER, PRIO_SYSTICK_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_SYSTICK_TIMER);
    evk_timer_systick_timer_start();
}

void evk_timer_systick_timer_start(void)
{
    if (HAL_TIM_Base_Start_IT(&htim8) != HAL_OK) {
        Error_Handler();
    }
}

void evk_timer_systick_timer_stop(void)
{
    if (HAL_TIM_Base_Stop_IT(&htim8) != HAL_OK) {
        Error_Handler();
    }
}
