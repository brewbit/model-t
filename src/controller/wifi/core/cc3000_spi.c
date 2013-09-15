
#include "ch.h"
#include "hal.h"

#include "hci.h"
#include "cc3000_spi.h"


#define ASSERT_CS()    do { \
                         spiAcquireBus(SPI_WLAN); \
                         spiStart(SPI_WLAN, &wlan_spi_cfg); \
                         spiSelect(SPI_WLAN); \
                       } while (0)


#define DEASSERT_CS()  do { \
                         spiUnselect(SPI_WLAN); \
                         spiReleaseBus(SPI_WLAN); \
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
  SPI_STATE_WRITE,
} spi_state_t;


static void
wifi_irq_cb(EXTDriver *extp, expchannel_t channel);

static void
SpiReadDataCont(void);

static void
SpiTriggerRxProcessing(void);

static msg_t
SpiReadThread(void* arg);

static void
SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength);




static spi_state_t spiState;
static gcSpiHandleRx SPIRxHandler;
static unsigned char* txPacket;
static unsigned short txPacketLength;
static unsigned char* rxPacket;
static unsigned short rxPacketLength;
static Semaphore sem_init;
static Semaphore sem_read;
static Semaphore sem_write;

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
//!  SpiClose
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Close Spi interface
//
//*****************************************************************************
void
SpiClose(void)
{
  rxPacket = NULL;

  // Disable Interrupt
  if (EXTD1.state == EXT_ACTIVE)
    extChannelDisable(&EXTD1, 12);
}

//*****************************************************************************
//
//!  SpiOpen
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Open Spi interface
//
//*****************************************************************************
void
SpiOpen(gcSpiHandleRx pfRxHandler)
{
  extStart(&EXTD1, &extcfg);

  chSemInit(&sem_init, 0);
  chSemInit(&sem_read, 0);
  chSemInit(&sem_write, 0);

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

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, SpiReadThread, NULL);
}

static msg_t
SpiReadThread(void* arg)
{
  (void)arg;

  while (TRUE) {
    chSemWait(&sem_read);

    /* IRQ line goes down - we are start reception */
    ASSERT_CS();

    spiExchange(SPI_WLAN, 10, tSpiReadHeader, rxPacket);

    // The header was read - continue with  the payload read
    SpiReadDataCont();

    // All the data was read - finalize handling by switching to the task
    // and calling from task Event Handler
    SpiTriggerRxProcessing();
  }

  return 0;
}

//*****************************************************************************
//
//! SpiFirstWrite
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
SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength)
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
//!  SpiWrite
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
SpiWrite(unsigned char *pUserBuffer, unsigned short usLength)
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
    SpiFirstWrite(pUserBuffer, usLength);
  }
  else {
    spiState = SPI_STATE_WRITE;
    txPacket = pUserBuffer;
    txPacketLength = usLength;

    // Assert the CS line
    ASSERT_CS();

    // Wait for the CC3000 to respond by asserting the IRQ line
    chSemWait(&sem_write);

    // Commence write operation
    spiSend(SPI_WLAN, usLength, pUserBuffer);

    spiState = SPI_STATE_IDLE;

    DEASSERT_CS();
  }
}

//*****************************************************************************
//
//!  SpiReadDataCont
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
SpiReadDataCont(void)
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
//! SpiPauseSpi
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Spi pause operation
//
//*****************************************************************************
void
SpiPauseSpi(void)
{
}

//*****************************************************************************
//
//! SpiResumeSpi
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Spi resume operation
//
//*****************************************************************************
void
SpiResumeSpi(void)
{
}

//*****************************************************************************
//
//! SpiTriggerRxProcessing
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  Spi RX processing
//
//*****************************************************************************
static void
SpiTriggerRxProcessing(void)
{
  // Trigger Rx processing
  SpiPauseSpi();
  DEASSERT_CS();

  // The magic number that resides at the end of the TX/RX buffer (1 byte after
  // the allocated size) for the purpose of detection of the overrun. If the
  // magic number is overwritten - buffer overrun occurred - and we will stuck
  // here forever!
  if (rxPacket[CC3000_RX_BUFFER_SIZE - 1] != CC3000_BUFFER_MAGIC_NUMBER) {
    chSysHalt();
  }

  spiState = SPI_STATE_IDLE;
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

  switch (spiState) {
  case SPI_STATE_POWERUP:
    chSemSignalI(&sem_init);
    break;

  case SPI_STATE_IDLE:
    chSemSignalI(&sem_read);
    break;

  case SPI_STATE_WRITE:
    chSemSignalI(&sem_write);
    break;

  default:
    break;
  }

  chSysUnlockFromIsr();
}
