
#include "ch.h"
#include "hal.h"

#include "hci.h"
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

// The magic number that resides at the end of the TX/RX buffer (1 byte after
// the allocated size) for the purpose of detection of the overrun. The location
// of the memory where the magic number resides shall never be written. In case
// it is written - the overrun occurred and either receive function or send
// function will stuck forever.
#define CC3000_BUFFER_MAGIC_NUMBER (0xDE)


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

static void
spi_process_rx(void);

static msg_t
spi_io_thread(void* arg);

static void
spi_first_write(unsigned char *ucBuf, unsigned short usLength);




static spi_state_t spiState;
static gcSpiHandleRx SPIRxHandler;
static unsigned char* txPacket;
static unsigned short txPacketLength;
static unsigned char* rxPacket;
static unsigned short rxPacketLength;
static Semaphore sem_init;
Semaphore sem_io_ready;
Semaphore sem_write_complete;

int irq_count, missed_irq_count, irq_timeout_count, io_thread_loc;

unsigned char wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

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
static const unsigned char tSpiReadHeader[] = {SPI_READ_OP, 0, 0, 0, 0};

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
  rxPacket = NULL;

  // Disable Interrupt
  if (EXTD1.state == EXT_ACTIVE)
    extChannelDisable(&EXTD1, 12);
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
spi_open(gcSpiHandleRx pfRxHandler)
{
  spiStart(SPI_WLAN, &wlan_spi_cfg);
  extStart(&EXTD1, &extcfg);

  chSemInit(&sem_init, 0);
  chSemInit(&sem_io_ready, 0);
  chSemInit(&sem_write_complete, 0);

  spiState = SPI_STATE_POWERUP;
  SPIRxHandler = pfRxHandler;
  txPacketLength = 0;
  txPacket = NULL;
  rxPacket = wlan_rx_buffer;
  rxPacketLength = 0;
  wlan_rx_buffer[CC3000_RX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;
  wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;

  // Enable interrupt on WLAN IRQ pin
  extChannelEnable(&EXTD1, 12);

  chThdCreateFromHeap(NULL, 1024, HIGHPRIO, spi_io_thread, NULL);
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
      /* IRQ line goes down - we are start reception */
      ASSERT_CS();

      /* Read header */
      spiExchange(SPI_WLAN, 10, tSpiReadHeader, rxPacket);

      /* Read payload */
      spi_read_payload();

      DEASSERT_CS();

      spi_process_rx();
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
spi_first_write(unsigned char *ucBuf, unsigned short usLength)
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
spi_write(unsigned char *pUserBuffer, unsigned short usLength)
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
  long data_to_recv;
  unsigned char *evnt_buff, type;

  //determine what type of packet we have
  evnt_buff =  rxPacket;
  data_to_recv = 0;
  STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_PACKET_TYPE_OFFSET,
      type);

  switch(type) {
  case HCI_TYPE_DATA:
    // We need to read the rest of data..
    STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE),
        HCI_DATA_LENGTH_OFFSET, data_to_recv);
    if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
      data_to_recv++;

    if (data_to_recv)
      spiReceive(SPI_WLAN, data_to_recv, evnt_buff + 10);
    break;

  case HCI_TYPE_EVNT:
    // Calculate the rest length of the data
    STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE),
        HCI_EVENT_LENGTH_OFFSET, data_to_recv);
    data_to_recv -= 1;

    // Add padding byte if needed
    if ((HEADERS_SIZE_EVNT + data_to_recv) & 1)
      data_to_recv++;

    if (data_to_recv)
      spiReceive(SPI_WLAN, data_to_recv, evnt_buff + 10);
    break;
  }
}

//*****************************************************************************
//
//! spi_process_rx
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Spi RX processing
//
//*****************************************************************************
static void
spi_process_rx(void)
{
  // The magic number that resides at the end of the TX/RX buffer (1 byte after
  // the allocated size) for the purpose of detection of the overrun. If the
  // magic number is overwritten - buffer overrun occurred - and we will stuck
  // here forever!
  if (rxPacket[CC3000_RX_BUFFER_SIZE - 1] != CC3000_BUFFER_MAGIC_NUMBER) {
    printf("Buffer overflow detected in SPI RX handler!\r\n");
    return;
  }

  SPIRxHandler(rxPacket + SPI_HEADER_SIZE);
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
