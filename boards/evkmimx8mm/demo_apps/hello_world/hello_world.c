#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "SR1020.h"
#include "core_cm4.h"

/* ECSPI APPLICATION SPECIFIC */
#define ECSPI_MASTER_BASEADDR ECSPI2
#define EXAMPLE_ECSPI_DEALY_COUNT 1000000
#define ECSPI_MASTER_CLK_FREQ                                                                 \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootEcspi2)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootEcspi2)))
#define TRANSFER_SIZE 16U         /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */
/* ** ECSPI Globals ** */
volatile bool isTransferCompleted = false;
volatile uint32_t g_systickCounter = 0;
ecspi_master_handle_t *handle;
uint8_t rx_data[TRANSFER_SIZE];
uint8_t tx_data[TRANSFER_SIZE];
uint32_t tx[TRANSFER_SIZE];
uint32_t rx[TRANSFER_SIZE];

//bool g_enable_dma_irq = false;

/* ** Function Prototypes ** */
void BOARD_Init_All(void);
void ECSPI_MasterUserCallback(ECSPI_Type *base, ecspi_master_handle_t *handle, status_t status, void *userData);

/* SPARK APPLICATION SPECIFIC */
#define SWC_MEM_POOL_SIZE 4600
#define MAX_PAYLOAD_SIZE_BYTE TRANSFER_SIZE
/* Spark Globals */
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_hal_t hal;
static swc_node_t *node;
static swc_connection_t *rx_conn;
static swc_connection_t *tx_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t rx_timeslots[] = RX_TIMESLOTS;
static int32_t tx_timeslots[] = TX_TIMESLOTS;

/* ** Application Specific ** */
static char stats_string[500];
static const char *tx_payload = "Hello, World!\r\n";
static const char *rx_payload[MAX_PAYLOAD_SIZE_BYTE];
static uint32_t tx_count;
static bool print_stats_now = false;
static bool reset_stats_now = false;

/* ** Spark Function Protypes ** */
static void app_swc_core_init(swc_error_t *err);
static void conn_tx_success_callback(void *conn);
static void conn_tx_fail_callback(void *conn);
static void conn_rx_success_callback(void *conn);
static void print_stats(void);
static void reset_stats(void);

/**
 * @brief driver code
 * Sends data over UWB radio through SPI
 *
 * @return int
 */
int main(void)
{
    BOARD_Init_All();
    PRINTN("\r\n\n\n\n\n");
    PRINTF("******* UWB Transfer *******");
    swc_error_t swc_err;

    uint8_t *hello_world_buf = NULL;

    app_swc_core_init(&swc_err);
    if (swc_err != SWC_ERR_NONE)
    {
        while (1)
            ;
    }
    PRINTF("SWC Connecting!");
    swc_connect();
    //__enable_irq();
    // PRINTF("SWC connected!");

    while (1)
    {
        /* PRINTF("swc_connection_get_payload_buffer");
        swc_connection_get_payload_buffer(tx_conn, &hello_world_buf, &swc_err);
        if (hello_world_buf != NULL)
        {
            // PRINTF(" before swc_connection_send");
            swc_connection_send(tx_conn, (uint8_t *)tx_payload, MAX_PAYLOAD_SIZE_BYTE, &swc_err);
        }
        Print received string and stats every 1000 transmissions */
        if ((g_systickCounter % 5000) == 0)
        {
            PRINTF("still alive");
        }
    }
}
void BOARD_Init_All(void)
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

    gpio_pin_config_t CS_config = {kGPIO_DigitalOutput, 1, kGPIO_IntRisingEdge};
    gpio_pin_config_t INT_config = {kGPIO_DigitalInput, 1, kGPIO_IntRisingEdge};
    gpio_pin_config_t RST_config = {kGPIO_DigitalOutput, 0, kGPIO_IntRisingEdge};
    gpio_pin_config_t SHTDN_config = {kGPIO_DigitalOutput, 0, kGPIO_IntRisingEdge};

    GPIO_PinInit(GPIO5, 13U, &CS_config);    // SPI chip select
    GPIO_PinInit(GPIO3, 21U, &INT_config);   // radio interrupt
    GPIO_PinInit(GPIO3, 22U, &RST_config);   // radio reset
    GPIO_PinInit(GPIO3, 23U, &SHTDN_config); // radio shutdown
    // GPIO_PortEnableInterrupts(GPIO3, (1 << 21U));
    ECSPI_MasterTransferCreateHandle(ECSPI_MASTER_BASEADDR, handle, ECSPI_MasterUserCallback, NULL);
    GPIO_PortEnableInterrupts(GPIO3, (1 << 21U));
    SysTick_Config(SystemCoreClock / 1000U);
}

void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    ECSPI_Type *base = ECSPI_MASTER_BASEADDR;
    assert((base != NULL));
    // PRINTF("Starting full duplex blocking");

    /* Check if the argument is legal */
    if ((size == 0) && (rx_data == NULL))
    {
        return;
    }
    ECSPI_SetChannelSelect(base, kECSPI_Channel0);
    evk_radio_spi_reset_cs();
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
                PRINTF("Starting full duplex blocking");
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
    evk_radio_spi_set_cs();
}

void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    ecspi_transfer_t masterXfer;
    PRINTF("Starting full duplex non blocking");

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
        PRINTF("TRANSFER COMPLETED!");
        isTransferCompleted = true;
        radio1_dma_callback();
        //if (g_enable_dma_irq)
            //swc_radio_spi_receive_complete_handler();
        evk_radio_spi_reset_cs();
    }

    if (status == kStatus_ECSPI_HardwareOverFlow)
    {
        PRINTF("Hardware overflow occurred in this transfer. \r\n");
    }
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
    g_systickCounter++;
}

void PendSV_Handler(void)
{
    CLEAR_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
    pendsv_irq_callback();
}
/* Adds delay in milliseconds */
void delay(uint32_t Counter)
{
    uint32_t target = g_systickCounter + Counter;

    while (g_systickCounter < target)
    {
    }
}
void evk_timer_delay_ms(uint32_t ms)
{
    PRINTF("Delay %u", ms);
    delay(ms);
}

static void app_swc_core_init(swc_error_t *err)
{
    iface_swc_hal_init(&hal);
    iface_swc_handlers_init();
    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .fast_sync_enabled = false,
        .random_channel_sequence_enabled = false,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE};
    swc_init(core_cfg, &hal, err);
    PRINTF("swc_init");
    if (*err != SWC_ERR_NONE)
    {
        return;
    }

    swc_node_cfg_t node_cfg = {
        .role = NETWORK_ROLE,
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = LOCAL_ADDRESS,
        .sleep_level = SWC_SLEEP_LEVEL};
    node = swc_node_init(node_cfg, err);
    PRINTF("swc_node_init completed!");
    if (*err != SWC_ERR_NONE)
    {
        return;
    }

    swc_radio_cfg_t radio_cfg = {
        .irq_polarity = IRQ_ACTIVE_HIGH,
        .std_spi = SPI_STANDARD};
    swc_node_add_radio(node, radio_cfg, &hal, err);
    PRINTF("swc_node_add_ratio completed!");
    if (*err != SWC_ERR_NONE)
    {
        return;
    }

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = LOCAL_ADDRESS,
        .destination_address = REMOTE_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
        .allocate_payload_memory = true,
        .ack_enabled = true,
        .arq_enabled = true,
        .arq_settings.retry_count = 0,
        .arq_settings.time_deadline = 0,
        .auto_sync_enabled = false,
        .cca_enabled = false,
        .throttling_enabled = false,
        .rdo_enabled = false,
        .fallback_enabled = false};
    tx_conn = swc_connection_init(node, tx_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE)
    {
        return;
    }
    PRINTF("TX swc_connection_init completed!");
    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT};
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++)
    {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, node, tx_channel_cfg, err);
        if (*err != SWC_ERR_NONE)
        {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback);
    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback);

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = REMOTE_ADDRESS,
        .destination_address = LOCAL_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .modulation = SWC_MODULATION,
        .fec = SWC_FEC_LEVEL,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
        .allocate_payload_memory = true,
        .ack_enabled = true,
        .arq_enabled = true,
        .arq_settings.retry_count = 0,
        .arq_settings.time_deadline = 0,
        .auto_sync_enabled = false,
        .cca_enabled = false,
        .throttling_enabled = false,
        .rdo_enabled = false,
        .fallback_enabled = false};
    rx_conn = swc_connection_init(node, rx_conn_cfg, &hal, err);
    if (*err != SWC_ERR_NONE)
    {
        return;
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT};
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++)
    {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, node, rx_channel_cfg, err);
        if (*err != SWC_ERR_NONE)
        {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback);

    swc_setup(node);
    PRINTF("swc_setup complete");
}
static void conn_tx_success_callback(void *conn)
{
    (void)conn;

    PRINTF("TX Success");
    /* Print stats every 1000 transmissions */
    tx_count++;
    if ((tx_count % 1000) == 0)
    {
        print_stats_now = true;
    }
}

static void conn_tx_fail_callback(void *conn)
{
    (void)conn;

    PRINTF("TX Failed");
    /* Print stats every 1000 transmissions */
    tx_count++;
    if ((tx_count % 1000) == 0)
    {
        print_stats_now = true;
    }
}

static void conn_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t err;
    uint8_t *payload = NULL;

    /* Get new payload */
    swc_connection_receive(rx_conn, &payload, &err);
    memcpy(rx_payload, payload, sizeof(rx_payload));

    /* Free the payload memory */
    swc_connection_receive_complete(rx_conn, &err);

    PRINTF("RX Success");
}

bool evk_radio_is_spi_busy(void)
{
    if (handle->state == 0)
    {
        PRINTF("SPI Idle");
    }
    else
        PRINTF("SPI Busy");

    return handle->state; //(&hradio_spi)->Instance->SR & SPI_SR_BSY;
}

static void print_stats(void)
{
    int string_length;

    swc_connection_update_stats(tx_conn);
    string_length = swc_connection_format_stats(tx_conn, node, stats_string, sizeof(stats_string));
    swc_connection_update_stats(rx_conn);
    swc_connection_format_stats(rx_conn, node, stats_string + string_length, sizeof(stats_string) - string_length);

    PRINTF("%s", stats_string);
}

static void reset_stats(void)
{
    if (reset_stats_now == false)
    {
        reset_stats_now = true;
    }
}
