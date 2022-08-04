/** @file  evk.c
 *  @brief Board Support Package for the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk.h"

/* CONSTANTS ******************************************************************/
#define FREE_RUNNING_TICK_FREQ 4000

/* EXTERNS ********************************************************************/
extern SPI_HandleTypeDef hradio_spi;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef hlp_uart;
extern DMA_HandleTypeDef hdma_sai1_b;
extern DMA_HandleTypeDef hdma_sai1_a;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_user_buttons_gpio(void);
static void init_led_gpio_push_pull(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
static void init_debug_pin_gpio_push_pull(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
static void init_board_led_gpio(void);
static void init_debug_pin_gpio_push_pull(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
static void init_board_debug_pin_gpio(void);
static void init_vdd_select_gpio(evk_vdd_t vdd);
static void init_radio_irq_gpio(void);
static void init_radio_shutdown_gpio(void);
static void init_radio_reset_gpio(void);
static void init_radio_debug_en_gpio(void);
static void init_radio_spi_peripheral(void);
static void init_radio_dma_controller(void);
static void init_radio_peripherals(void);
static void init_timer_us_clock(void);
static void init_free_running_timer_init(void);
static void init_exp_uart_init(void);
static void init_usb_detect_gpio(void);
static void init_all_gpio_clocks(void);
static void init_radio_pendsv(void);
static void init_dma1_clock(void);
static void init_dma2_clock(void);

/* PUBLIC FUNCTION ************************************************************/
void evk_init(void)
{
    HAL_Init();

    init_all_gpio_clocks();
    init_dma1_clock();
    init_dma2_clock();

    /* Set default system and USB clock frequency */
    evk_set_system_clock(CLK_169_98MHZ);

    /* Button initialization */
    init_user_buttons_gpio();

    /* LEDs init */
    init_board_led_gpio();

    /* Debug pins init */
    init_board_debug_pin_gpio();

    /* Power Management */
    init_vdd_select_gpio(VDD_3V3);

    /* USB detect init */
    init_usb_detect_gpio();
    evk_init_usb_clock();

    /* Timers */
    init_timer_us_clock();
    init_free_running_timer_init();

    /* Radio init */
    init_radio_peripherals();

    init_exp_uart_init();
}

void evk_system_reset(void)
{
    NVIC_SystemReset();
}

/* PRIVATE FUNCTIONS **********************************************************/
/* BUTTONS */
/** @brief Initialize the GPIOs for the buttons.
 */
static void init_user_buttons_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin  = BTN0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BTN0_PORT, &GPIO_InitStruct);

    /* Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin  = BTN1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BTN1_PORT, &GPIO_InitStruct);
}

/* LED */
/** @brief Initialize the GPIOs to use LEDs in push pull mode.
 *
 *  @param port   GPIO Port.
 *  @param pin    GPIO Pin.
 *  @param state  Pin state initialization.
 */
static void init_led_gpio_push_pull(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(port, pin, state);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/** @brief Initialize the GPIOs to use debug pins in push pull mode.
 *
 *  @param port   GPIO Port.
 *  @param pin    GPIO Pin.
 *  @param state  Pin state initialization.
 */
static void init_debug_pin_gpio_push_pull(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(port, pin, state);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/** @brief Initializes the on-board LEDs.
 */
static void init_board_led_gpio(void)
{
    init_led_gpio_push_pull(LED0_PORT, LED0_PIN, GPIO_PIN_RESET);
    init_led_gpio_push_pull(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
    init_led_gpio_push_pull(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
}

/** @brief Initializes the debug pins on the expansion board.
 */
static void init_board_debug_pin_gpio(void)
{
    init_debug_pin_gpio_push_pull(EXP_PC7_PORT,  EXP_PC7_PIN,  GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC8_PORT,  EXP_PC8_PIN,  GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC10_PORT, EXP_PC10_PIN, GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
}

/* POWER */
/** @brief Initialize GPIOs for the voltage select feature.
 *
 *  @param[in] evk_vdd_t  vdd
 *      @li VDD_1V8
 *      @li VDD_3V3
 */
static void init_vdd_select_gpio(evk_vdd_t vdd)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    evk_set_board_voltage(vdd);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = VOLTAGE_SEL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(VOLTAGE_SEL_PORT, &GPIO_InitStruct);
}

/* RADIO */
/** @brief Initialize IRQ pins.
 */
static void init_radio_irq_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = RADIO_IRQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_IRQ_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(NVIC_RADIO_IRQ, PRIO_RADIO_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_IRQ);
}

/** @brief Initialize the shutdown pin.
 */
static void init_radio_shutdown_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_SHUTDOWN_PORT, RADIO_SHUTDOWN_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_SHUTDOWN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_SHUTDOWN_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the reset pin.
 */
static void init_radio_reset_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_RESET_PORT, RADIO_RESET_PIN, GPIO_PIN_SET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_RESET_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_RESET_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the debug enable pin.
 */
static void init_radio_debug_en_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_DEBUG_EN_PORT, RADIO_DEBUG_EN_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_DEBUG_EN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_DEBUG_EN_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the SPI bus connected to the radio.
 */
static void init_radio_spi_peripheral(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    hradio_spi.Instance = SPI2;
    hradio_spi.Init.Mode = SPI_MODE_MASTER;
    hradio_spi.Init.Direction = SPI_DIRECTION_2LINES;
    hradio_spi.Init.DataSize = SPI_DATASIZE_8BIT;
    hradio_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hradio_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hradio_spi.Init.NSS = SPI_NSS_SOFT;
    hradio_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hradio_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hradio_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    hradio_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hradio_spi.Init.CRCPolynomial = 7;
    hradio_spi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hradio_spi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init(&hradio_spi) != HAL_OK) {
        Error_Handler();
    }

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_CS_PORT, RADIO_CS_PIN, GPIO_PIN_SET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_CS_PORT, &GPIO_InitStruct);
}

/** @brief Initialize DMA clock and ISR channel
 */
static void init_radio_dma_controller(void)
{
    /* DMA1_Channel2_IRQn interrupt configuration (SPI2_RX)*/
    HAL_NVIC_SetPriority(NVIC_RADIO_DMA_RX_CPLT, PRIO_RADIO_DMA_RX_CPLT, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_DMA_RX_CPLT);

    /* DMA1_Channel2_IRQn interrupt configuration (SPI2_TX)*/
    HAL_NVIC_SetPriority(NVIC_RADIO_DMA_TX_CPLT, PRIO_RADIO_DMA_TX_CPLT, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_DMA_TX_CPLT);
}

/** @brief Initialize all peripherals related to the SR10x0 radio.
 */
static void init_radio_peripherals(void)
{
    init_radio_dma_controller();
    init_radio_irq_gpio();
    init_radio_shutdown_gpio();
    init_radio_reset_gpio();
    init_radio_debug_en_gpio();
    init_radio_spi_peripheral();
    init_radio_pendsv();
    evk_init_ext_osc_clk();
    evk_init_xtal_clk();
    evk_radio_set_reset_pin();
    evk_radio_reset_reset_pin();
}

/* TIMERS */
/** @brief Initialize the microsecond timer.
 */
static void init_timer_us_clock(void)
{
    /* Enable TIM3 clock */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* Update Core Clock value */
    SystemCoreClockUpdate();

    htim3.Instance = TIM3;
    /* Dividing the frequency of the peripheral by what we want (1Mhz) gives the prescaler value for a
     * free running timer.
     */
    htim3.Init.Prescaler = ((HAL_RCC_GetPCLK2Freq() / 1000000)) - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 0xffff;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK) {
        Error_Handler();
    }
}

/** @brief Initialize the timer for the stop and wait ARQ.
 */
static void init_free_running_timer_init(void)
{
    /* Enable TIM6 clock */
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* Update Core Clock value */
    SystemCoreClockUpdate();

    htim6.Instance = TIM6;

    /* Dividing the frequency of the peripheral by what we want (4 kHz) gives the prescaler value for a
     * free running timer.
     */
    htim6.Init.Prescaler     = (evk_get_system_clock_freq() / FREE_RUNNING_TICK_FREQ) - 1;
    htim6.Init.CounterMode   = TIM_COUNTERMODE_UP;
    htim6.Init.Period        = 0xffff;
    htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
        Error_Handler();
    }
}

/* Expansion IO port */
/** @brief Initialize the UART bus connected to the expansion board.
 */
static void init_exp_uart_init(void)
{
    hlp_uart.Instance                    = LPUART1;
    hlp_uart.Init.BaudRate               = 1152000;
    hlp_uart.Init.WordLength             = UART_WORDLENGTH_8B;
    hlp_uart.Init.StopBits               = UART_STOPBITS_1;
    hlp_uart.Init.Parity                 = UART_PARITY_NONE;
    hlp_uart.Init.Mode                   = UART_MODE_TX_RX;
    hlp_uart.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    hlp_uart.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    hlp_uart.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    hlp_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
    hlp_uart.AdvancedInit.Swap           = UART_ADVFEATURE_SWAP_ENABLE;
    if (HAL_UART_Init(&hlp_uart) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&hlp_uart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlp_uart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&hlp_uart) != HAL_OK) {
        Error_Handler();
    }
}

/* USB */
/** @brief Initialize the GPIOs to detect a USB connection.
 */
static void init_usb_detect_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = USB_DETECT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(USB_DETECT_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(NVIC_USB_DET_IRQ, PRIO_USB_DET_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_USB_DET_IRQ);
}

/* CLOCK */
/** @brief Enable all GPIO's peripheral clock.
 */
static void init_all_gpio_clocks(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
}

static void init_radio_pendsv(void)
{
    HAL_NVIC_SetPriority(NVIC_PENDSV_IRQ, PRIO_PEND_SV_IRQ, 0);
    HAL_NVIC_ClearPendingIRQ(NVIC_PENDSV_IRQ);
    HAL_NVIC_EnableIRQ(NVIC_PENDSV_IRQ);
}

/** @brief Initialize the DMA1 peripheral clocks.
 *
 *  The DMA1 is used by multiple peripherals from different modules such as
 *  @li Radio Module
 *  @li UART Module
 */
static void init_dma1_clock(void)
{
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
}

/** @brief Initialize the DMA2 peripheral clocks.
 *
 *  The DMA2 is used by multiple peripherals from different modules such as
 *  @li Audio Module
 */
static void init_dma2_clock(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
}
