#include "SR1020.h"

void iface_swc_hal_init(swc_hal_t *hal)
{
    hal->radio_hal[0].set_shutdown_pin = evk_radio_set_shutdown_pin;
    hal->radio_hal[0].reset_shutdown_pin = evk_radio_reset_shutdown_pin;
    hal->radio_hal[0].set_reset_pin = evk_radio_set_reset_pin;
    hal->radio_hal[0].reset_reset_pin = evk_radio_reset_reset_pin;
    hal->radio_hal[0].read_irq_pin = evk_radio_read_irq_pin;
    hal->radio_hal[0].set_cs = evk_radio_spi_set_cs;
    hal->radio_hal[0].reset_cs = evk_radio_spi_reset_cs;
    hal->radio_hal[0].delay_ms = evk_timer_delay_ms;

    hal->radio_hal[0].transfer_full_duplex_blocking = evk_radio_spi_transfer_full_duplex_blocking;
    hal->radio_hal[0].transfer_full_duplex_non_blocking = evk_radio_spi_transfer_full_duplex_non_blocking;
    hal->radio_hal[0].is_spi_busy = evk_radio_is_spi_busy;
    hal->radio_hal[0].context_switch = evk_radio_context_switch;
    hal->radio_hal[0].disable_radio_irq = evk_radio_disable_irq_it;
    hal->radio_hal[0].enable_radio_irq = evk_radio_enable_irq_it;
    hal->radio_hal[0].disable_radio_dma_irq = evk_radio_disable_dma_irq_it;
    hal->radio_hal[0].enable_radio_dma_irq = evk_radio_enable_dma_irq_it;

    hal->context_switch = evk_radio_callback_context_switch;

    hal->get_tick_quarter_ms = evk_timer_get_free_running_tick_ms;
}
uint64_t evk_timer_get_free_running_tick_ms(void)
{
    return (uint64_t)  g_systickCounter;
}
void iface_swc_handlers_init(void)
{
    PRINTF("called %s", __func__);
    evk_set_radio_irq_callback(swc_radio_irq_handler);
    evk_set_radio_dma_rx_callback(swc_radio_spi_receive_complete_handler);
    evk_set_pendsv_callback(swc_connection_callbacks_processing_handler);
}
bool evk_radio_read_irq_pin(void)
{
    return GPIO_PinRead(GPIO3, 21U);
}

void evk_radio_enable_irq_it(void)
{
    EnableIRQ(GPIO3_Combined_16_31_IRQn);
    PRINTF("RADIO_IRQ_PIN enabled");
}

void evk_radio_disable_irq_it(void)
{
    // GPIO_PortDisableInterrupts(GPIO3, (1 << 21U));
    DisableIRQ(GPIO3_Combined_16_31_IRQn);
    PRINTF("RADIO_IRQ_PIN disabled");
}
void evk_radio_enable_dma_irq_it(void)
{
    PRINTF("Enable DMA");
}

void evk_radio_disable_dma_irq_it(void)
{
    PRINTF("Disable DMA");
}
void evk_radio_context_switch(void)
{
    NVIC_SetPendingIRQ(GPIO3_Combined_16_31_IRQn);
}

void evk_radio_callback_context_switch(void)
{
    SET_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
}

void GPIO3_Combined_16_31_IRQHandler(void)
{
    PRINTF("Radio INTERRUPT");
    GPIO_ClearPinsInterruptFlags(GPIO3, (1 << 21U));
    exti2_irq_callback();
}

void default_irq_callback(void)
{
    return;
}
void evk_set_radio_dma_rx_callback(irq_callback callback)
{
    radio1_dma_callback = callback;
}
void evk_set_radio_irq_callback(irq_callback callback)
{
    PRINTF("setting irq callback");
    exti2_irq_callback = callback;
}
void evk_set_pendsv_callback(irq_callback callback)
{
    pendsv_irq_callback = callback;
}
void evk_radio_set_shutdown_pin(void)
{
    PRINTF("RADIO SET Shutdown");
    GPIO_PinWrite(GPIO3, 23U, 1U);
}
void evk_radio_reset_shutdown_pin(void)
{
    PRINTF("RADIO RESET Shutdown");
    GPIO_PinWrite(GPIO3, 23U, 0U);
}
void evk_radio_set_reset_pin(void)
{
    PRINTF("RADIO SET reset_pin");
    GPIO_PinWrite(GPIO3, 22U, 1U);
}

void evk_radio_reset_reset_pin(void)
{
    PRINTF("RADIO RESET reset_pin");
    GPIO_PinWrite(GPIO3, 22U, 0U);
}
void evk_radio_spi_set_cs(void)
{
    GPIO_PinWrite(GPIO5, 13U, 1U);
}

void evk_radio_spi_reset_cs(void)
{
    GPIO_PinWrite(GPIO5, 13U, 0U);
}
