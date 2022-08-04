#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_ecspi.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/* Definitions*/
#define ECSPI_MASTER_BASEADDR ECSPI2
#define EXAMPLE_ECSPI_DEALY_COUNT 1000000
#define ECSPI_MASTER_CLK_FREQ                                                                 \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootEcspi2)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootEcspi2)))
#define TRANSFER_SIZE 64U         /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */
// GPIO CONFIG
#define GPIO GPIO5
#define GPIO_PIN 13U // J103-p38

/* Variables*/
volatile bool isTransferCompleted = false;
bool flag = false;
volatile uint32_t g_systickCounter = 20U;
uint8_t rx_data[TRANSFER_SIZE];
uint8_t tx_data[TRANSFER_SIZE];

ecspi_master_handle_t *handle;

enum
{
    kECSPI_Idle = 0x0, /*!< ECSPI is idle state */
    kECSPI_Busy        /*!< ECSPI is busy tranferring data. */
};

/* Prototypes */
void init();

void evk_radio_spi_set_cs(void);
void evk_radio_spi_reset_cs(void);
void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

static void ECSPI_SendTransfer(ECSPI_Type *base, ecspi_master_handle_t *handle);
void ECSPI_MasterUserCallback(ECSPI_Type *base, ecspi_master_handle_t *handle, status_t status, void *userData);

void delay(uint32_t Counter);
void printData(uint8_t *tx_data, uint16_t size);
bool checkData(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

int main(void)
{
    init();

    PRINTF("\r\n\n\n\n\n\n******* SPI Transfer *******\r\n");

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
            // PRINTF("\r\nyo\r\n");
        }
        uint32_t *buf = handle->rxData;
        for (int i = 0; i < TRANSFER_SIZE; i++)
        {
            rx_data[i] = (uint8_t)*buf++;
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
    assert((handle != NULL));

    uint32_t txData[size];
    uint32_t rxData[size];
    for (int i = 0; i < size; i++)
    {
        txData[i] = tx_data[i];
        rxData[i] = rx_data[i];
    }

    /* Check if ECSPI is busy */
    if (handle->state == (uint32_t)kECSPI_Busy)
    {
        return;
    }

    /* Check if the input arguments valid */
    if (((tx_data == NULL) && (rx_data == NULL)) || (size == 0U))
    {
        return;
    }

    /* Set the handle information */
    handle->channel = kECSPI_Channel0;
    handle->txData = txData;
    handle->rxData = rxData;
    handle->transferSize = size;
    handle->txRemainingBytes = size;
    handle->rxRemainingBytes = size;

    /* Set the ECSPI state to busy */
    handle->state = kECSPI_Busy;

    ECSPI_SetChannelSelect(ECSPI_MASTER_BASEADDR, kECSPI_Channel0);

    evk_radio_spi_set_cs();
    /* First send data to Tx FIFO to start a ECSPI transfer */
    ECSPI_SendTransfer(ECSPI_MASTER_BASEADDR, handle);

    if (NULL != rx_data)
    {
        ECSPI_EnableInterrupts(
            ECSPI_MASTER_BASEADDR, (uint32_t)kECSPI_RxFifoReadyInterruptEnable | (uint32_t)kECSPI_RxFifoOverFlowInterruptEnable);
    }
    else
    {
        ECSPI_EnableInterrupts(ECSPI_MASTER_BASEADDR, kECSPI_TxFifoDataRequstInterruptEnable);
    }
}

static void ECSPI_SendTransfer(ECSPI_Type *base, ecspi_master_handle_t *handle)
{
    assert(base != NULL);
    assert(handle != NULL);

    uint32_t dataCounts = 0U;
    uint32_t txRemainingBytes = (uint32_t)(handle->txRemainingBytes);
    /* Caculate the data size to send */
    dataCounts = TRANSFER_SIZE;
    //((uint32_t)FSL_FEATURE_ECSPI_TX_FIFO_SIZEn(base) - (uint32_t)ECSPI_GetTxFifoCount(base)) < txRemainingBytes ? ((uint32_t)FSL_FEATURE_ECSPI_TX_FIFO_SIZEn(base) - (uint32_t)ECSPI_GetTxFifoCount(base)) : txRemainingBytes;
    uint32_t *buffer = handle->txData;
    while ((dataCounts--) != 0UL)
    {
        if (buffer != NULL)
        {
            base->TXDATA = *buffer++;
        }
        if (NULL != handle->txData)
        {
            handle->txData += 1U;
        }
        handle->txRemainingBytes -= 1U;
    }
}
void ECSPI_MasterUserCallback(ECSPI_Type *base, ecspi_master_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        isTransferCompleted = true;
        evk_radio_spi_reset_cs();
    }

    if (status == kStatus_ECSPI_HardwareOverFlow)
    {
        PRINTF("Hardware overflow occurred in this transfer. \r\n");
    }
}

void evk_radio_spi_set_cs(void)
{
    GPIO_PinWrite(GPIO, GPIO_PIN, 0U);
    // PRINTF("CS-> SET \r\n");
}

void evk_radio_spi_reset_cs(void)
{
    GPIO_PinWrite(GPIO, GPIO_PIN, 1U);
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

/* Initialize board */
void init()
{
    gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 0, kGPIO_IntRisingEdge};
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

    ECSPI_MasterInit(ECSPI_MASTER_BASEADDR, &masterConfig, ECSPI_MASTER_CLK_FREQ);
    GPIO_PinInit(GPIO, GPIO_PIN, &led_config);
    ECSPI_MasterTransferCreateHandle(ECSPI_MASTER_BASEADDR, handle, ECSPI_MasterUserCallback, NULL);
    evk_radio_spi_set_cs();
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
