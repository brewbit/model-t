
#include "ch.h"
#include "hal.h"

#include "hci.h"
#include "evnt_handler.h"
#include "cc3000_spi.h"


#define ASSERT_CS()    do { \
                         spiSelect(SPI_WLAN); \
                       } while (0)


#define DEASSERT_CS()  do { \
                         spiUnselect(SPI_WLAN); \
                       } while (0)


#define HEADERS_SIZE_EVNT       (SPI_HEADER_SIZE + 5)

#define SPI_WRITE_OP 1
#define SPI_READ_OP  3


typedef enum {
  SPI_STATE_POWERUP,
  SPI_STATE_IDLE,
} spi_state_t;


static void
wifi_irq_cb(EXTDriver *extp, expchannel_t channel);

static void
wait_io_ready(void);

static void
spi_read_payload(void);

static msg_t
spi_io_thread(void* arg);

static void
spi_first_write(uint8_t *ucBuf, uint16_t usLength);




static spi_state_t spiState;
static uint8_t* txPacket;
static uint16_t txPacketLength;
static uint16_t rxPacketLength;
static Semaphore sem_init;
Semaphore sem_io_ready;
Semaphore sem_write_complete;
Thread* io_thread;

int irq_count, missed_irq_count, irq_timeout_count, io_thread_loc;

uint8_t wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
uint8_t wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

static const SPIConfig wlan_spi_cfg = {
    .end_cb = NULL,
    .ssport = PORT_WIFI_CS,
    .sspad = PAD_WIFI_CS,
    .cr1 = SPI_CR1_CPHA
};

static const EXTConfig extcfg = {
    {
        [12] = {EXT_CH_MODE_FALLING_EDGE, wifi_irq_cb},
    },
    EXT_MODE_EXTI(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        EXT_MODE_GPIOD, 0, 0, 0)
};

// Static buffer for 5 bytes of SPI HEADER
static const uint8_t tSpiReadHeader[] = {SPI_READ_OP, 0, 0, 0, 0};

//*****************************************************************************
//
//!  spi_close
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Close Spi interface
//
//*****************************************************************************
void
spi_close(void)
{
  // Disable Interrupt
  if (EXTD1.state == EXT_ACTIVE)
    extChannelDisable(&EXTD1, 12);

  // Shut down I/O thread
  chThdTerminate(io_thread);
  chSemSignal(&sem_io_ready);
  chThdWait(io_thread);
  io_thread = NULL;
}

//*****************************************************************************
//
//!  spi_open
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Open Spi interface
//
//*****************************************************************************
void
spi_open()
{
  spiStart(SPI_WLAN, &wlan_spi_cfg);
  extStart(&EXTD1, &extcfg);

  chSemInit(&sem_init, 0);
  chSemInit(&sem_io_ready, 0);
  chSemInit(&sem_write_complete, 0);

  spiState = SPI_STATE_POWERUP;
  txPacketLength = 0;
  txPacket = NULL;
  rxPacketLength = 0;

  // Enable interrupt on WLAN IRQ pin
  extChannelEnable(&EXTD1, 12);

  io_thread = chThdCreateFromHeap(NULL, 1024, HIGHPRIO, spi_io_thread, NULL);
}

static void
wait_io_ready()
{
  while (1) {
    /* Wait for the I/O ready semaphore to be signalled. We need to include
     * a timeout because occasionally we seem to be missing IRQ edges. We
     * handle this by checking the IRQ line manually when the semaphore wait
     * times out.
     */
    msg_t rdy = chSemWaitTimeout(&sem_io_ready, S2ST(2));

    /* If the semaphore has been signalled, we are ready to read or write */
    if (rdy == RDY_OK) {
      break;
    }
    /* Check if IRQ is asserted */
    else if (palReadPad(PORT_WIFI_IRQ, PAD_WIFI_IRQ) == 0) {
      irq_timeout_count++;
      break;
    }
  }
}

static msg_t
spi_io_thread(void* arg)
{
  (void)arg;

  chRegSetThreadName("spi_read");

  while (TRUE) {
    wait_io_ready();

    if (chThdShouldTerminate())
      break;

    /* If a write is pending, handle that first */
    if (txPacket != NULL && txPacketLength > 0) {
      /* Assert the CS line to indicate to the CC3000 that we have data to send */
      ASSERT_CS();

      /* Wait for the CC3000 to respond by asserting the IRQ line */
      wait_io_ready();

      /* Commence write operation */
      spiSend(SPI_WLAN, txPacketLength, txPacket);

      DEASSERT_CS();

      /* Clear the pending write vars */
      txPacket = NULL;
      txPacketLength = 0;
      
      /* Signal the waiting thread that the write is complete */
      chSemSignal(&sem_write_complete);
    }
    else {
      /* Assert CS to start read */
      ASSERT_CS();

      /* Read header */
      spiExchange(SPI_WLAN, SPI_HEADER_SIZE, tSpiReadHeader, wlan_rx_buffer);

      /* Read payload */
      spi_read_payload();

      DEASSERT_CS();

      /* Dispatch the data to the HCI module */
      hci_dispatch_packet(wlan_rx_buffer + SPI_HEADER_SIZE);
    }
  }

  return 0;
}

//*****************************************************************************
//
//! spi_first_write
//!
//!  @param  ucBuf     buffer to write
//!  @param  usLength  buffer's length
//!
//!  @return none
//!
//!  @brief  enter point for first write flow
//
//*****************************************************************************
static void
spi_first_write(uint8_t *ucBuf, uint16_t usLength)
{
  // workaround for first transaction
  ASSERT_CS();

  chThdSleepMicroseconds(50);

  // SPI writes first 4 bytes of data
  spiSend(SPI_WLAN, 4, ucBuf);

  chThdSleepMicroseconds(50);

  spiSend(SPI_WLAN, usLength - 4, ucBuf + 4);

  // From this point on - operate in a regular way
  spiState = SPI_STATE_IDLE;

  DEASSERT_CS();
}

//*****************************************************************************
//
//!  spi_write
//!
//!  @param  pUserBuffer  buffer to write
//!  @param  usLength     buffer's length
//!
//!  @return none
//!
//!  @brief  Spi write operation
//
//*****************************************************************************
void
spi_write(uint8_t *pUserBuffer, uint16_t usLength)
{
  if((usLength & 1) == 0)
    usLength++;

  pUserBuffer[0] = SPI_WRITE_OP;
  pUserBuffer[1] = (usLength >> 8) & 0xFF;
  pUserBuffer[2] = (usLength) & 0xFF;
  pUserBuffer[3] = 0;
  pUserBuffer[4] = 0;

  usLength += SPI_HEADER_SIZE;

  if (spiState == SPI_STATE_POWERUP) {
    chSemWait(&sem_init);

    // This is time for first TX/RX transactions over SPI: the IRQ is down -
    // so need to send read buffer size command
    spi_first_write(pUserBuffer, usLength);
  }
  else {
    txPacketLength = usLength;
    txPacket = pUserBuffer;

    /* Signal the I/O thread to handle the write */
    chSemSignal(&sem_io_ready);

    /* Wait for write to complete */
    chSemWait(&sem_write_complete);
  }
}

//*****************************************************************************
//
//!  spi_read_payload
//!
//!  @param  None
//!
//!  @return None
//!
//!  @brief  This function processes received SPI Header and in accordance with
//!              it - continues reading the packet
//
//*****************************************************************************
static void
spi_read_payload(void)
{
  uint16_t data_to_recv = (wlan_rx_buffer[3] << 8) | (wlan_rx_buffer[4]);

  if (data_to_recv)
    spiReceive(SPI_WLAN, data_to_recv, wlan_rx_buffer + SPI_HEADER_SIZE);
}

//*****************************************************************************
//
//!  wifi_irq_cb
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  GPIO A interrupt handler. When the external SSI WLAN device is
//!          ready to interact with Host CPU it generates an interrupt signal.
//!          After that Host CPU has registered this interrupt request
//!          it set the corresponding /CS in active state.
//
//*****************************************************************************
static void
wifi_irq_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;

  chSysLockFromIsr();

  irq_count++;

  switch (spiState) {
  case SPI_STATE_POWERUP:
    chSemSignalI(&sem_init);
    break;

  case SPI_STATE_IDLE:
    chSemSignalI(&sem_io_ready);
    break;

  default:
    break;
  }

  chSysUnlockFromIsr();
}
