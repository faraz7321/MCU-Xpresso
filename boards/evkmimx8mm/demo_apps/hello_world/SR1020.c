#include "SR1020.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void iface_swc_hal_init(swc_hal_t *hal)
{
    hal->radio_hal[0].set_cs = evk_radio_spi_set_cs;
    hal->radio_hal[0].reset_cs = evk_radio_spi_reset_cs;

    hal->radio_hal[0].transfer_full_duplex_blocking = evk_radio_spi_transfer_full_duplex_blocking;
    hal->radio_hal[0].transfer_full_duplex_non_blocking = evk_radio_spi_transfer_full_duplex_non_blocking;
}
void iface_swc_handlers_init(void)
{
    // evk_set_radio_irq_callback(swc_radio_irq_handler);
    // evk_set_radio_dma_rx_callback(swc_radio_spi_receive_complete_handler);
    // evk_set_pendsv_callback(swc_connection_callbacks_processing_handler);
}

void evk_radio_spi_set_cs(void)
{
    GPIO_PinWrite(GPIO5, 13U, 0U);
    // PRINTF("CS-> SET \r\n");
}

void evk_radio_spi_reset_cs(void)
{
    GPIO_PinWrite(GPIO5, 13U, 1U);
    // PRINTF("CS-> RESET \r\n");
}
void GPIO3_Combined_16_31_IRQHandler(void)
{
    // PRINTF("\r\n hello\r\n");
    radio_irq_callback();
    // make call to the provided spark sdk function
    GPIO_ClearPinsInterruptFlags(GPIO3, (1 << 21U));
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
    radio_irq_callback = callback;
}
void evk_set_pendsv_callback(irq_callback callback)
{
    pendsv_irq_callback = callback;
}
