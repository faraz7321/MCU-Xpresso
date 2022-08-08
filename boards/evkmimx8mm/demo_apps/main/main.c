#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_ecspi.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "evk_audio.h"

/* Definitions*/
#define ECSPI_MASTER_BASEADDR ECSPI2
#define EXAMPLE_ECSPI_DEALY_COUNT 1000000
#define ECSPI_MASTER_CLK_FREQ                                                                 \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootEcspi2)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootEcspi2)))
#define TRANSFER_SIZE 64U         /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */

typedef void (*irq_callback)(void);
enum
{
    kECSPI_Idle = 0x0, /*!< ECSPI is idle state */
    kECSPI_Busy        /*!< ECSPI is busy tranferring data. */
};

/* Prototypes */
void evk_radio_spi_set_cs(void);
void evk_radio_spi_reset_cs(void);
void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

// Radio Callback
void iface_swc_handlers_init(void);
void default_irq_callback(void);
void evk_set_radio_dma_rx_callback(irq_callback callback);
void evk_set_radio_irq_callback(irq_callback callback);
void evk_set_pendsv_callback(irq_callback callback);
void spi_transfer(void);

// SPI and GPIOs
void ECSPI_MasterUserCallback(ECSPI_Type *base, ecspi_master_handle_t *handle, status_t status, void *userData);
void GPIO3_Combined_16_31_IRQHandler(void);

// Utility
void delay(uint32_t Counter);
void printData(uint8_t *tx_data, uint16_t size);
bool checkData(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/* Variables*/
volatile bool isTransferCompleted = false;
volatile uint32_t g_systickCounter = 20U;

uint8_t rx_data[TRANSFER_SIZE];
uint8_t tx_data[TRANSFER_SIZE];
uint32_t tx[TRANSFER_SIZE];
uint32_t rx[TRANSFER_SIZE];

ecspi_master_handle_t *handle;

static irq_callback radio_irq_callback = default_irq_callback;
static irq_callback pendsv_irq_callback = default_irq_callback;
static irq_callback radio1_dma_callback = default_irq_callback;

int main(void)
{
    /* Board specific RDC settings */
    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitMemory();

    CLOCK_SetRootMux(kCLOCK_RootEcspi2, kCLOCK_EcspiRootmuxSysPll1); /* Set ECSPI2 source to SYSTEM PLL1 800MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootEcspi2, 2U, 5U);                 /* Set root clock to 800MHZ / 10 = 80MHZ */
    ecspi_master_config_t masterConfig;

    ECSPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = TRANSFER_BAUDRATE;
    // masterConfig.burstLength = 1;

    ECSPI_MasterInit(ECSPI_MASTER_BASEADDR, &masterConfig, ECSPI_MASTER_CLK_FREQ);

    // Setup callback
    iface_swc_handlers_init();

    gpio_pin_config_t CS_config = {kGPIO_DigitalOutput, 1, kGPIO_IntRisingEdge};
    gpio_pin_config_t GPIO_config = {kGPIO_DigitalInput, 1, kGPIO_IntRisingEdge};
    GPIO_PinInit(GPIO5, 13U, &CS_config);   // SPI chip select
    GPIO_PinInit(GPIO3, 21U, &GPIO_config); // radio interrupt
    GPIO_PortEnableInterrupts(GPIO3, (1 << 21U));
    EnableIRQ(GPIO3_Combined_16_31_IRQn);

    ECSPI_MasterTransferCreateHandle(ECSPI_MASTER_BASEADDR, handle, ECSPI_MasterUserCallback, NULL);

    PRINTF("\r\n\n\n\n\n\n******* SPI Transfer2 *******\r\n");

    /* Set up the transfer data */
    while (1)
    {

        for (uint16_t i = 0; i < TRANSFER_SIZE; i++)
        {
            tx_data[i] = (uint8_t)i;
            rx_data[i] = (uint8_t)0xAA;
        }
        PRINTF("\r\nBefore Transfer\r\nTX=");
        printData(tx_data, TRANSFER_SIZE);
        PRINTF("\r\nRX=");
        printData(rx_data, TRANSFER_SIZE);

        isTransferCompleted = false;
        evk_radio_spi_transfer_full_duplex_non_blocking(tx_data, rx_data, TRANSFER_SIZE);

        while (!isTransferCompleted)
        {
        }
        PRINTF("\r\nAfter Transfer\r\nTX=");
        printData(tx_data, TRANSFER_SIZE);
        PRINTF("\r\nRX=");
        printData(rx_data, TRANSFER_SIZE);

        if (checkData(tx_data, rx_data, TRANSFER_SIZE))
        {
            PRINTF("\r\nLoopback successful\r\n");
        }
        GETCHAR();
        // delay(10000);
    }
}

void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    ECSPI_Type *base = ECSPI_MASTER_BASEADDR;
    assert((base != NULL));

    /* Check if the argument is legal */
    if ((size == 0) && (rx_data == NULL))
    {
        return;
    }
    ECSPI_SetChannelSelect(base, kECSPI_Channel0);
    evk_radio_spi_set_cs();
    while (size > 0)
    {
        while ((base->STATREG & ECSPI_STATREG_TE_MASK) == 0UL)
        {
        }
        // writing to Tx //
        base->TXDATA = *tx_data++;
        while ((base->STATREG & ECSPI_STATREG_RR_MASK) == 0UL)
        {
            uint32_t
                /* Get status flags of ECSPI */
                state = ECSPI_GetStatusFlags(ECSPI_MASTER_BASEADDR);
            /* If hardware overflow happen */
            if ((ECSPI_STATREG_RO_MASK & state) != 0UL)
            {
                /* Clear overflow flag for next transfer */
                ECSPI_ClearStatusFlags(ECSPI_MASTER_BASEADDR, kECSPI_RxFifoOverFlowFlag);
                return;
            }
        }
        // reading from Rx //
        *rx_data = (uint8_t)(base->RXDATA);
        rx_data++;
        size--;
    }
    evk_radio_spi_reset_cs();
}

void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    ecspi_transfer_t masterXfer;

    for (uint16_t i = 0; i < size; i++)
    {
        tx[i] = tx_data[i];
        rx[i] = rx_data[i];
    }

    masterXfer.txData = tx;
    masterXfer.rxData = rx;
    masterXfer.dataSize = size;
    masterXfer.channel = kECSPI_Channel0;
    evk_radio_spi_set_cs();
    ECSPI_MasterTransferNonBlocking(ECSPI_MASTER_BASEADDR, handle, &masterXfer);
}

void ECSPI_MasterUserCallback(ECSPI_Type *base, ecspi_master_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        for (int i = 0; i < TRANSFER_SIZE; i++)
        {
            rx_data[i] = rx[i];
        }
        isTransferCompleted = true;
        radio1_dma_callback();
        evk_radio_spi_reset_cs();
    }

    if (status == kStatus_ECSPI_HardwareOverFlow)
    {
        PRINTF("Hardware overflow occurred in this transfer. \r\n");
    }
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

void printData(uint8_t *data, uint16_t size)
{
    for (int i = 0; i < size; i++)
    {
        PRINTF("%02x,", data[i]);
    }
    PRINTF("\r\n");
}

bool checkData(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    bool flag = true;
    for (int i = 0; i < size; i++)
    {
        if (tx_data[i] != rx_data[i])
        {
            flag = false;
        }
    }
    return flag;
}

void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}
/* Adds delay in milliseconds */
void delay(uint32_t Counter)
{
    /* Delay to wait slave is ready */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        while (1)
        {
        }
    }
    g_systickCounter = Counter;
    while (g_systickCounter != 0U)
    {
    }
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
void swc_radio_irq_handler(void)
{
    PRINTF("\r\nexample spark callback");
}
void spi_transfer(void)
{
    PRINTF("\r\nTransfer complete");
}
void iface_swc_handlers_init(void)
{
    evk_set_radio_irq_callback(swc_radio_irq_handler);
    evk_set_radio_dma_rx_callback(spi_transfer);
    // evk_set_pendsv_callback(swc_connection_callbacks_processing_handler);
}