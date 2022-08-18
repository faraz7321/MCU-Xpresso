/* Spark Stack */

#ifndef SR1020_H_
#define SR1020_H_

/* Spark interface */

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "fsl_ecspi.h"
#include "fsl_gpio.h"
#include "swc_api.h"
#include "swc_cfg_coord.h"
#include "fsl_debug_console.h"
#include "swc_stats.h"

#define SET_BIT(REG, BIT) ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))
typedef void (*irq_callback)(void);
extern volatile uint32_t g_systickCounter;
enum
{
    kECSPI_Idle = 0x0, /*!< ECSPI is idle state */
    kECSPI_Busy        /*!< ECSPI is busy tranferring data. */
};

void iface_swc_handlers_init(void);
void iface_swc_hal_init(swc_hal_t *hal);

// Radio Callback
void default_irq_callback(void);
void evk_set_radio_dma_rx_callback(irq_callback callback);
void evk_set_radio_irq_callback(irq_callback callback);
void evk_set_pendsv_callback(irq_callback callback);
void evk_timer_delay_ms(uint32_t ms);

bool evk_radio_read_irq_pin(void);
void evk_radio_enable_irq_it(void);
void evk_radio_disable_irq_it(void);
void evk_radio_enable_dma_irq_it(void);
void evk_radio_disable_dma_irq_it(void);
void evk_radio_set_shutdown_pin(void);
void evk_radio_reset_shutdown_pin(void);
void evk_radio_set_reset_pin(void);
void evk_radio_reset_reset_pin(void);
void evk_radio_spi_set_cs(void);
void evk_radio_spi_reset_cs(void);
uint64_t evk_timer_get_free_running_tick_ms(void);

// void evk_radio_spi_write_blocking(uint8_t *data, uint8_t size);
// void evk_radio_spi_read_blocking(uint8_t *data, uint8_t size);
void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

bool evk_radio_is_spi_busy(void);
void evk_radio_context_switch(void);
void evk_radio_callback_context_switch(void);

void delay(uint32_t ms);
void GPIO3_Combined_16_31_IRQHandler(void);

static irq_callback exti2_irq_callback = default_irq_callback;
static irq_callback pendsv_irq_callback = default_irq_callback;
static irq_callback radio1_dma_callback = default_irq_callback;

#endif /* SR1020_H_ */