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

typedef void (*irq_callback)(void);

enum
{
    kECSPI_Idle = 0x0, /*!< ECSPI is idle state */
    kECSPI_Busy        /*!< ECSPI is busy tranferring data. */
};

void evk_radio_spi_set_cs(void);
void evk_radio_spi_reset_cs(void);
void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

void iface_swc_handlers_init(void);
void iface_swc_hal_init(swc_hal_t *hal);

// Radio Callback
void default_irq_callback(void);
void evk_set_radio_dma_rx_callback(irq_callback callback);
void evk_set_radio_irq_callback(irq_callback callback);
void evk_set_pendsv_callback(irq_callback callback);
void spi_transfer(void);

static irq_callback radio_irq_callback = default_irq_callback;
static irq_callback pendsv_irq_callback = default_irq_callback;
static irq_callback radio1_dma_callback = default_irq_callback;

#endif /* SR1020_H_ */