
#include "ch.h"
#include "hal.h"

#include "hci.h"
#include "cc3000_spi.h"


#define ASSERT_CS()    do { \
                         spiAcquireBus(&SPID2); \
                         palClearPad(PORT_WIFI_CS, PAD_WIFI_CS); \
                       } while (0)


#define DEASSERT_CS()  do { \
                         palSetPad(PORT_WIFI_CS, PAD_WIFI_CS); \
                         spiReleaseBus(&SPID2); \
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
  SPI_STATE_INITIALIZED,
  SPI_STATE_IDLE,
  SPI_STATE_WRITE_IRQ,
  SPI_STATE_WRITE_FIRST_PORTION,
  SPI_STATE_WRITE_EOT,
  SPI_STATE_READ_IRQ,
  SPI_STATE_READ_FIRST_PORTION,
  SPI_STATE_READ_EOT,
} spi_state_t;


static void
wifi_irq_cb(EXTDriver *extp, expchannel_t channel);

static void
SSIContReadOperation(void);

static long
SpiReadDataCont(void);

static void
SpiTriggerRxProcessing(void);

static msg_t
SpiReadThread(void* arg);




static spi_state_t spiState;
static gcSpiHandleRx SPIRxHandler;
static unsigned char* txPacket;
static unsigned short txPacketLength;
static unsigned char* rxPacket;
static unsigned short rxPacketLength;
static Semaphore irq_sem;

unsigned char wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

static const SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad = 0,
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

  //      Disable Interrupt
  tSLInformation.WlanInterruptDisable();
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
  palSetPad(PORT_WIFI_CS, PAD_WIFI_CS);

  extStart(&EXTD1, &extcfg);

  spiStart(&SPID2, &spi_cfg);

  chSemInit(&irq_sem, 0);



  spiState = SPI_STATE_POWERUP;
  SPIRxHandler = pfRxHandler;
  txPacketLength = 0;
  txPacket = NULL;
  rxPacket = wlan_rx_buffer;
  rxPacketLength = 0;
  wlan_rx_buffer[CC3000_RX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;
  wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;

  // Enable interrupt on WLAN IRQ pin
  tSLInformation.WlanInterruptEnable();

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, SpiReadThread, NULL);
}

static msg_t
SpiReadThread(void* arg)
{
  (void)arg;

  while (TRUE) {
    chprintf(SD_STDIO, "Waiting for IRQ %d\r\n", chTimeNow());
    chSemWait(&irq_sem);
    chprintf(SD_STDIO, "got it %d\r\n", chTimeNow());

    if (spiState == SPI_STATE_POWERUP) {
      //This means IRQ line was low call a callback of HCI Layer to inform
      //on event
      spiState = SPI_STATE_INITIALIZED;
    }
    else if (spiState == SPI_STATE_IDLE) {
      spiState = SPI_STATE_READ_IRQ;

      /* IRQ line goes down - we are start reception */
      ASSERT_CS();

      // Wait for TX/RX Compete which will come as DMA interrupt
      spiExchange(&SPID2, 10, tSpiReadHeader, rxPacket);

      spiState = SPI_STATE_READ_EOT;

      SSIContReadOperation();
    }
    else if (spiState == SPI_STATE_WRITE_IRQ) {
      spiSend(&SPID2, txPacketLength, txPacket);

      spiState = SPI_STATE_IDLE;

      DEASSERT_CS();
    }
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
long
SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength)
{
  // workaround for first transaction
  ASSERT_CS();

  chThdSleepMicroseconds(50);

  // SPI writes first 4 bytes of data
  spiSend(&SPID2, 4, ucBuf);

  chThdSleepMicroseconds(50);

  spiSend(&SPID2, usLength - 4, ucBuf + 4);

  // From this point on - operate in a regular way
  spiState = SPI_STATE_IDLE;

  DEASSERT_CS();

  return 0;
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
long
SpiWrite(unsigned char *pUserBuffer, unsigned short usLength)
{
  chprintf(SD_STDIO, "SpiWrite(%d)\r\n", usLength);

  if((usLength & 1) == 0)
    usLength++;

  pUserBuffer[0] = SPI_WRITE_OP;
  pUserBuffer[1] = (usLength >> 8) & 0xFF;
  pUserBuffer[2] = (usLength) & 0xFF;
  pUserBuffer[3] = 0;
  pUserBuffer[4] = 0;

  usLength += SPI_HEADER_SIZE;

  if (spiState == SPI_STATE_POWERUP) {
    while (spiState != SPI_STATE_INITIALIZED);
  }

  if (spiState == SPI_STATE_INITIALIZED) {
    // This is time for first TX/RX transactions over SPI: the IRQ is down -
    // so need to send read buffer size command
    SpiFirstWrite(pUserBuffer, usLength);
  }
  else {
    // We need to prevent here race that can occur in case 2 back to back
    // packets are sent to the  device, so the state will move to IDLE and once
    //again to not IDLE due to IRQ
    tSLInformation.WlanInterruptDisable();

    while (spiState != SPI_STATE_IDLE);

    spiState = SPI_STATE_WRITE_IRQ;
    txPacket = pUserBuffer;
    txPacketLength = usLength;

    // Assert the CS line and wait till SSI IRQ line is active and then
    // initialize write operation
    ASSERT_CS();

    // Re-enable IRQ - if it was not disabled - this is not a problem...
    tSLInformation.WlanInterruptEnable();

    // check for a missing interrupt between the CS assertion and enabling back the interrupts
    if (tSLInformation.ReadWlanInterruptPin() == 0) {
      spiSend(&SPID2, usLength, pUserBuffer);

      spiState = SPI_STATE_IDLE;

      DEASSERT_CS();
    }
  }

  // Due to the fact that we are currently implementing a blocking situation
  // here we will wait till end of transaction
  while (SPI_STATE_IDLE != spiState);

  return 0;
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
long
SpiReadDataCont(void)
{
  long data_to_recv;
  unsigned char *evnt_buff, type;

  //determine what type of packet we have
  evnt_buff =  rxPacket;
  data_to_recv = 0;
  STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_PACKET_TYPE_OFFSET,
      type);

  switch(type)
  {
  case HCI_TYPE_DATA:
    // We need to read the rest of data..
    STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE),
        HCI_DATA_LENGTH_OFFSET, data_to_recv);
    if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
      data_to_recv++;

    if (data_to_recv)
      spiReceive(&SPID2, data_to_recv, evnt_buff + 10);
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
      spiReceive(&SPID2, data_to_recv, evnt_buff + 10);

    spiState = SPI_STATE_READ_EOT;
    break;
  }

  return 0;
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
  //SPI_IRQ_IE &= ~SPI_IRQ_PIN;
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
  chSemSignalI(&irq_sem);
  chSysUnlockFromIsr();
}

//*****************************************************************************
//
//! SSIContReadOperation
//!
//!  @param  none
//!
//!  @return none
//!
//!  @brief  SPI read operation
//
//*****************************************************************************
static void
SSIContReadOperation()
{
  // The header was read - continue with  the payload read
  if (!SpiReadDataCont()) {
    // All the data was read - finalize handling by switching to the task
    //      and calling from task Event Handler
    chprintf(SD_STDIO, "processing data\r\n");
    SpiTriggerRxProcessing();
  }
}
