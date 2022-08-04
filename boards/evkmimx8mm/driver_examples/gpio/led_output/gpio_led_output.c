/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This example uses GPIO as interrupt
 * Hello is printed on console when input
 * is detected on pin 38
 * of the expansion connector on EVK
 */

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_LED_GPIO GPIO3
#define EXAMPLE_LED_GPIO_PIN 21U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* The PIN status */
volatile bool g_pinSet = false;
/*******************************************************************************
 * Code
 ******************************************************************************/

void GPIO3_Combined_16_31_IRQHandler(void)
{
    PRINTF("\r\n hello\r\n");
    // GPIO_DisableInterrupts(EXAMPLE_LED_GPIO, EXAMPLE_LED_GPIO_PIN);
    GPIO_ClearPinsInterruptFlags(EXAMPLE_LED_GPIO, (1 << EXAMPLE_LED_GPIO_PIN));
}
/*!
 * @brief Main function
 */
int main(void)
{
    /* Define the init structure for the output LED pin*/
    gpio_pin_config_t led_config = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge};

    /* Board pin, clock, debug console init */
    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitMemory();

    /* Print a note to terminal. */
    PRINTF("\r\n GPIO Driver example\r\n");
    // PRINTF("\r\n The LED is blinking.\r\n");

    /* Init output LED GPIO. */
    GPIO_PinInit(EXAMPLE_LED_GPIO, EXAMPLE_LED_GPIO_PIN, &led_config);
    GPIO_PortEnableInterrupts(EXAMPLE_LED_GPIO, (1 << EXAMPLE_LED_GPIO_PIN));
    EnableIRQ(GPIO3_Combined_16_31_IRQn);

    while (1)
    {
    }
}
