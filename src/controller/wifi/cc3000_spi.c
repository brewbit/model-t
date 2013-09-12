
#include "ch.h"
#include "hal.h"

#include "hci.h"
#include "cc3000_spi.h"


#define ASSERT_CS()    do { \
                         palClearPad(PORT_WIFI_CS, PAD_WIFI_CS); \
                       } while (0)

//                         spiAcquireBus(&SPID2);

#define DEASSERT_CS()  do { \
                         palSetPad(PORT_WIFI_CS, PAD_WIFI_CS); \
                       } while (0)

//                         spiReleaseBus(&SPID2);

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
SpiWriteDataSynchronous(unsigned char* pUserBuffer, unsigned short usLength);

static void
SSIContReadOperation(void);

static long
SpiReadDataCont(void);

static void
SpiTriggerRxProcessing(void);

static void
SpiIntHandler(SPIDriver *spip);




static spi_state_t spiState;
static gcSpiHandleRx SPIRxHandler;
static unsigned char* txPacket;
static unsigned short txPacketLength;
static unsigned char* rxPacket;
static unsigned short rxPacketLength;

unsigned char wlan_rx_buffer[CC3000_RX_BUFFER_SIZE];
unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

static const SPIConfig spi_cfg = {
    .end_cb = SpiIntHandler,
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
}

void SpiPoll()
{
  chprintf(SD_STDIO, "spiState = %d\r\n", spiState);
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
  SpiWriteDataSynchronous(ucBuf, 4);

  chThdSleepMicroseconds(50);

  SpiWriteDataSynchronous(ucBuf + 4, usLength - 4);

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
      SpiWriteDataSynchronous(pUserBuffer, usLength);

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
//!  SpiWriteDataSynchronous
//!
//!  @param  data  buffer to write
//!  @param  size  buffer's size
//!
//!  @return none
//!
//!  @brief  Spi write operation
//
//*****************************************************************************
static void
SpiWriteDataSynchronous(unsigned char* pUserBuffer, unsigned short usLength)
{
  chSysLock();
  spiStartSendI(&SPID2, usLength, pUserBuffer);
  _spi_wait_s(&SPID2);
  chSysUnlock();
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
static long
SpiReadDataCont()
{
  long data_to_recv;
  unsigned char *evnt_buff, type;

  //determine what type of packet we have
  evnt_buff =  rxPacket;
  data_to_recv = 0;
  STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_PACKET_TYPE_OFFSET, type);

  switch(type) {
  case HCI_TYPE_DATA:
    STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_DATA_LENGTH_OFFSET, data_to_recv);

    // We need to read the rest of data..
    if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
      data_to_recv++;

    if (data_to_recv)
      spiStartReceiveI(&SPID2, data_to_recv, evnt_buff + 10);

    spiState = SPI_STATE_READ_EOT;
    break;

  case HCI_TYPE_EVNT:
    // Calculate the rest length of the data
    STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_EVENT_LENGTH_OFFSET, data_to_recv);
    data_to_recv -= 1;

    // Add padding byte if needed
    if ((HEADERS_SIZE_EVNT + data_to_recv) & 1)
      data_to_recv++;

    if (data_to_recv)
      spiStartReceiveI(&SPID2, data_to_recv, evnt_buff + 10);

    spiState = SPI_STATE_READ_EOT;
    break;
  }

  return data_to_recv;
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

  if (spiState == SPI_STATE_POWERUP) {
    //This means IRQ line was low call a callback of HCI Layer to inform
    //on event
    spiState = SPI_STATE_INITIALIZED;
  }
  else if (spiState == SPI_STATE_IDLE) {
    spiState = SPI_STATE_READ_IRQ;

    /* IRQ line goes down - we are start reception */
    ASSERT_CS();

    // Start header read
    chSysLockFromIsr();
    spiStartExchangeI(&SPID2, 10, tSpiReadHeader, rxPacket);
    chSysUnlockFromIsr();
  }
  else if (spiState == SPI_STATE_WRITE_IRQ) {
    spiState = SPI_STATE_WRITE_EOT;

    chSysLockFromIsr();
    spiStartSendI(&SPID2, txPacketLength, txPacket);
    chSysUnlockFromIsr();
  }
}

//*****************************************************************************
//
//! SpiIntHandler
//!
//! @param  none
//!
//! @return none
//!
//! @brief  SSI interrupt handler. Get command/data packet from the external
//!         WLAN device and pass it through the UART to the PC
//
//*****************************************************************************
static void
SpiIntHandler(SPIDriver *spip)
{
  (void)spip;

  unsigned short data_to_recv;
  unsigned char *evnt_buff;
  evnt_buff =  rxPacket;
  data_to_recv = 0;
  if (spiState == SPI_STATE_READ_IRQ) {
    SSIContReadOperation();
  }
  else if (spiState == SPI_STATE_READ_FIRST_PORTION) {
    STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE), HCI_DATA_LENGTH_OFFSET, data_to_recv);

    if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1)) {
      data_to_recv++;
    }

    chSysLockFromIsr();
    spiStartExchangeI(&SPID2, data_to_recv, tSpiReadHeader, rxPacket + 10);
    chSysUnlockFromIsr();

    spiState = SPI_STATE_READ_EOT;
  }
  else if (spiState == SPI_STATE_READ_EOT) {
    SpiTriggerRxProcessing();
  }
  else if (spiState == SPI_STATE_WRITE_EOT) {
    DEASSERT_CS();
    spiState = SPI_STATE_IDLE;
  }
  else if (spiState == SPI_STATE_WRITE_FIRST_PORTION) {
    spiState = SPI_STATE_WRITE_EOT;

    chSysLockFromIsr();
    spiStartSendI(&SPID2, txPacketLength, txPacket);
    chSysUnlockFromIsr();
  }
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
    SpiTriggerRxProcessing();
  }
}
