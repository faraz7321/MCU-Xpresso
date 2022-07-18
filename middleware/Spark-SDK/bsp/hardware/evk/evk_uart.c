/** @file  evk_uart.c
 *  @brief This module controls UART features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_it.h"
#include "evk_uart.h"

/* PRIVATE GLOBALS ************************************************************/
UART_HandleTypeDef hlp_uart;
DMA_HandleTypeDef  hdma_lpuart1_tx;
DMA_HandleTypeDef  hdma_lpuart1_rx;
void (*stlink_uart_tx_callback)(void) = NULL;
void (*stlink_uart_rx_callback)(void) = NULL;
void (*exp_uart_rx_callback)(void) = NULL;
void (*exp_uart_tx_callback)(void) = NULL;

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_exp_uart_write_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit(&hlp_uart, data, size, LOG_UART_TIMEOUT_MS);
}

void evk_exp_uart_write_non_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit_DMA(&hlp_uart, data, size);
}

void evk_exp_uart_read_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Receive(&hlp_uart, data, size, LOG_UART_TIMEOUT_MS);
}

void evk_exp_uart_read_byte_non_blocking(uint8_t *data)
{
    HAL_UART_Receive_DMA(&hlp_uart, data, sizeof(*data));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hlp_uart) {
        if (stlink_uart_rx_callback != NULL) {
            stlink_uart_rx_callback();
        }
    } else {
        if (exp_uart_rx_callback != NULL) {
            exp_uart_rx_callback();
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hlp_uart) {
        if (stlink_uart_tx_callback != NULL) {
            stlink_uart_tx_callback();
        }
    } else {
        if (exp_uart_tx_callback != NULL) {
            exp_uart_tx_callback();
        }
    }
}

void evk_set_stlink_uart_tx_callback(void (*callback)(void))
{
    stlink_uart_tx_callback = callback;
}

void evk_set_stlink_uart_rx_callback(void (*callback)(void))
{
    stlink_uart_rx_callback = callback;
}

void evk_set_exp_uart_tx_callback(void (*callback)(void))
{
    exp_uart_tx_callback = callback;
}

void evk_set_exp_uart_rx_callback(void (*callback)(void))
{
    exp_uart_rx_callback = callback;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the UART pins.
 *
 *  @param[in] huart  UART handler pointer.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == LPUART1) {
        /* Peripheral clock enable */
        __HAL_RCC_LPUART1_CLK_ENABLE();

        /* LPUART1 GPIO Configuration
         * PC0     ------> LPUART1_RX
         * PC1     ------> LPUART1_TX
         */
        GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* LPUART1 DMA Init */
        /* LPUART1_TX Init */
        hdma_lpuart1_tx.Instance                 = DMA1_CHANNEL_LPUART_TX;
        hdma_lpuart1_tx.Init.Request             = DMA_REQUEST_LPUART1_TX;
        hdma_lpuart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma_lpuart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_lpuart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_lpuart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_lpuart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_lpuart1_tx.Init.Mode                = DMA_NORMAL;
        hdma_lpuart1_tx.Init.Priority            = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_lpuart1_tx) != HAL_OK) {
            Error_Handler();
        }

        hdma_lpuart1_rx.Instance                 = DMA1_CHANNEL_LPUART_RX;
        hdma_lpuart1_rx.Init.Request             = DMA_REQUEST_LPUART1_RX;
        hdma_lpuart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_lpuart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_lpuart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_lpuart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_lpuart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_lpuart1_rx.Init.Mode                = DMA_NORMAL;
        hdma_lpuart1_rx.Init.Priority            = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_lpuart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_lpuart1_tx);
        __HAL_LINKDMA(huart, hdmarx, hdma_lpuart1_rx);

        HAL_NVIC_SetPriority(NVIC_LPUART_IRQ, PRIO_LPUART_IRQ, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_IRQ);

        /* DMA interrupt init */
        /* DMA1_Channel5_IRQn interrupt configuration */
        HAL_NVIC_SetPriority(NVIC_LPUART_TX_CPLT, PRIO_LPUART_DMA_TX_CPLT, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_TX_CPLT);

        HAL_NVIC_SetPriority(NVIC_LPUART_RX_CPLT, PRIO_LPUART_DMA_RX_CPLT, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_RX_CPLT);
    }
}

/** @brief De-initializes the UART pins.
 *
 *  @param[in] huart  UART handler pointer.
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1) {
        /* Peripheral clock disable */
        __HAL_RCC_LPUART1_CLK_DISABLE();

        /* LPUART1 GPIO Configuration
         * PC0     ------> LPUART1_RX
         * PC1     ------> LPUART1_TX
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1);
    }
}
