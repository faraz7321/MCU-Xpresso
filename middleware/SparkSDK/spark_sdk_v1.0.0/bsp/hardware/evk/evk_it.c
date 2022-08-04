/** @file  evk_it.c
 *  @brief This module controls interrupt related features.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* Includes ------------------------------------------------------------------*/
#include "evk_it.h"
#include "evk_dbg.h"

/* EXTERNS ********************************************************************/
extern PCD_HandleTypeDef  hpcd_USB_FS;
extern TIM_HandleTypeDef  htim2;
extern TIM_HandleTypeDef  htim8;
extern TIM_HandleTypeDef  htim20;
extern DMA_HandleTypeDef  hradio_dma_spi_rx;
extern UART_HandleTypeDef hlp_uart;
extern DMA_HandleTypeDef  hdma_lpuart1_tx;
extern DMA_HandleTypeDef  hdma_lpuart1_rx;
extern DMA_HandleTypeDef  hdma_sai1_b;
extern DMA_HandleTypeDef  hdma_sai1_a;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void default_irq_callback(void);

/* PRIVATE GLOBALS ************************************************************/
static uint32_t nested_critical;
static irq_callback exti0_irq_callback       = default_irq_callback;
static irq_callback exti2_irq_callback       = default_irq_callback;
static irq_callback radio1_dma_callback      = default_irq_callback;
static irq_callback pendsv_irq_callback      = default_irq_callback;
static irq_callback systick_callback         = default_irq_callback;
static irq_callback app_timer_callback       = default_irq_callback;
static irq_callback common_callback          = default_irq_callback;
static irq_callback usb_irq_callback         = default_irq_callback;

/* PUBLIC FUNCTION ***********************************************************/
void evk_it_set_systick_callback(irq_callback callback)
{
    systick_callback = callback;
}

void evk_it_set_common_callback(irq_callback callback)
{
    common_callback = callback;
}

void evk_set_usb_detect_callback(irq_callback callback)
{
    exti0_irq_callback = callback;
}

void evk_set_radio_irq_callback(irq_callback callback)
{
    exti2_irq_callback = callback;
}

void evk_set_radio_dma_rx_callback(irq_callback callback)
{
    radio1_dma_callback = callback;
}

void evk_set_app_timer_callback(irq_callback callback)
{
    app_timer_callback = callback;
}

void evk_set_pendsv_callback(irq_callback callback)
{
    pendsv_irq_callback = callback;
}

void evk_set_usb_irq_callback(irq_callback callback)
{
    usb_irq_callback = callback;
}

void evk_enter_critical(void)
{
    if (!nested_critical) {
        /* First time enter critical */
        __disable_irq();
    }
    ++nested_critical;
}

void evk_exit_critical(void)
{
    --nested_critical;
    if (!nested_critical) {
        /* Last time exit critical */
        __enable_irq();
    }
}

void Error_Handler(void)
{

}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Default interrupt used when initializing callbacks.
 */
static void default_irq_callback(void)
{
    return;
}

/** @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    while (1) {

    }
}

/** @brief This function handles Memory Management fault.
 */
void MemManage_Handler(void)
{
    while (1) {

    }
}

/** @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
    while (1) {

    }
}


/** @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
    while (1) {

    }
}

/** @brief This function handles System Service call via SWI instruction.
 */
void SVC_Handler(void)
{

}

/** @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{

}

/** @brief This function handles Pendable request for System Service.
 */
void PendSV_Handler(void)
{
    CLEAR_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
    pendsv_irq_callback();
}

/** @brief This function handles System Tick timer.
 */
void SysTick_Handler(void)
{

}

/** @brief This function handles EXTI line0 interrupt.
 */
void EXTI0_IRQHandler(void)
{
    common_callback();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    exti0_irq_callback();
}

/** @brief This function handles EXTI line2 interrupt.
 */
void EXTI2_IRQHandler(void)
{
    common_callback();
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
    exti2_irq_callback();
}

/** @brief DMA1 Channel 2 IRQ Handler.
 */
void DMA1_Channel2_IRQHandler(void)
{
    common_callback();
    /* Change the DMA state */
    (&hradio_dma_spi_rx)->State = HAL_DMA_STATE_READY;

    /* Clear the transfer complete flag */
    (&hradio_dma_spi_rx)->DmaBaseAddress->IFCR = ((uint32_t)DMA_ISR_TCIF1 << ((&hradio_dma_spi_rx)->ChannelIndex & 0x1FU));

    /* Process Unlocked */
    __HAL_UNLOCK(&hradio_dma_spi_rx);

    radio1_dma_callback();
}

/** @brief This function handles DMA2 channel5 global interrupt.
 */
void DMA2_Channel5_IRQHandler(void)
{
    common_callback();
    HAL_DMA_IRQHandler(&hdma_sai1_a);
}

/** @brief This function handles DMA2 channel6 global interrupt.
 */
void DMA2_Channel6_IRQHandler(void)
{
    common_callback();
    HAL_DMA_IRQHandler(&hdma_sai1_b);
}

/** @brief This function handles DMA1 channel5 global interrupt.
 */
void DMA1_Channel5_IRQHandler(void)
{
    common_callback();
  HAL_DMA_IRQHandler(&hdma_lpuart1_tx);
}

/** @brief This function handles DMA1 channel6 global interrupt.
 */
void DMA1_Channel6_IRQHandler(void)
{
    common_callback();
    HAL_DMA_IRQHandler(&hdma_lpuart1_rx);
}

/** @brief This function handles USB low priority interrupt remap.
 */
void USB_LP_IRQHandler(void)
{
    common_callback();
    usb_irq_callback();
}

/** @brief This function handles Timer 2 interrupt.
 */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
    HAL_IncTick();
}

void TIM8_UP_IRQHandler(void)
{
    __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);
    common_callback();
    systick_callback();
}

/** @brief This function handles Timer 20 interrupt.
 */
void TIM20_UP_IRQHandler(void)
{
    __HAL_TIM_CLEAR_IT(&htim20, TIM_IT_UPDATE);
    app_timer_callback();
}

/** @brief This function handles LPUART1 global interrupt.
 */
void LPUART1_IRQHandler(void)
{
    common_callback();
    HAL_UART_IRQHandler(&hlp_uart);
}
