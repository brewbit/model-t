
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "xflash.h"


// Read Commands
#define CMD_READ      0x03
#define CMD_FAST_READ 0x0B
#define CMD_DOR       0x3B
#define CMD_QOR       0x6B
#define CMD_DIOR      0xBB
#define CMD_QIOR      0xEB
#define CMD_RDID      0x9F
#define CMD_READ_ID   0x90

// Write Control Commands
#define CMD_WREN      0x06
#define CMD_WRDI      0x04

// Erase Commands
#define CMD_P4E       0x20
#define CMD_P8E       0x40
#define CMD_SE        0xD8
#define CMD_BE        0x60

// Program Commands
#define CMD_PP        0x02
#define CMD_QPP       0x32

// Status & Config Register Commands
#define CMD_RDSR      0x05
#define CMD_WRR       0x01
#define CMD_RCR       0x35
#define CMD_CLSR      0x30

// Power Save Commands
#define CMD_DP        0xB9
#define CMD_RES       0xAB

// OTP Commands
#define CMD_OTPP      0x42
#define CMD_OTPR      0x4B


#define SECTOR_SIZE 0x10000 // 64K
#define PAGE_SIZE   0x100   // 256


// Status Register Bitmasks
#define SR_WIP   0x01
#define SR_WEL   0x02
#define SR_BP0   0x04
#define SR_BP1   0x08
#define SR_BP2   0x10
#define SR_E_ERR 0x20
#define SR_P_ERR 0x40
#define SR_SRWD  0x80


static void
xflash_txn_begin(void);

static void
xflash_txn_end(void);

static void
send_cmd(uint8_t cmd, uint8_t* cmd_tx_buf, uint32_t cmd_tx_len, uint8_t* cmd_rx_buf, uint32_t cmd_rx_len);

static uint8_t
read_status_reg(void);

static void
write_enable(void);


static const SPIConfig flash_spi_cfg = {
    .end_cb = NULL,
    .ssport = PORT_SFLASH_CS,
    .sspad = PAD_SFLASH_CS,
    .cr1 = SPI_CR1_CPOL | SPI_CR1_CPHA
};

uint8_t rx_buf[256];


static void
xflash_txn_begin()
{
#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(SPI_FLASH);
#endif
  spiStart(SPI_FLASH, &flash_spi_cfg);
  spiSelect(SPI_FLASH);
}

static void
xflash_txn_end()
{
  spiUnselect(SPI_FLASH);
#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(SPI_FLASH);
#endif
}

static void
send_cmd(uint8_t cmd, uint8_t* cmd_tx_buf, uint32_t cmd_tx_len, uint8_t* cmd_rx_buf, uint32_t cmd_rx_len)
{
  xflash_txn_begin();
  spiSend(SPI_FLASH, 1, &cmd);
  if (cmd_tx_len > 0)
    spiSend(SPI_FLASH, cmd_tx_len, cmd_tx_buf);
  if (cmd_rx_len > 0)
    spiReceive(SPI_FLASH, cmd_rx_len, cmd_rx_buf);
  xflash_txn_end();
}

static uint8_t
read_status_reg()
{
  uint8_t sr;
  send_cmd(CMD_RDSR, NULL, 0, &sr, 1);
  return sr;
}

static void
write_enable()
{
  send_cmd(CMD_WREN, NULL, 0, NULL, 0);

  chprintf(SD_STDIO, "write enable: %x\r\n", read_status_reg());
}

static void
sector_erase(uint32_t sector)
{
  uint8_t erase_cmd[3];
  uint32_t erase_addr = sector * SECTOR_SIZE;
  erase_cmd[0] = erase_addr >> 16;
  erase_cmd[1] = erase_addr >> 8;
  erase_cmd[2] = erase_addr;

  write_enable();
  send_cmd(CMD_SE, erase_cmd, sizeof(erase_cmd), NULL, 0);

  chprintf(SD_STDIO, "sector erase: %x\r\n", read_status_reg());

  while (read_status_reg() & SR_WIP)
    chThdSleepMilliseconds(100);

  chprintf(SD_STDIO, "erase complete: %x\r\n", read_status_reg());
}

uint8_t pp_cmd[256+3];
static void
page_program(uint32_t page)
{
  int i;
  uint32_t page_addr = page * PAGE_SIZE;
  pp_cmd[0] = page_addr >> 16;
  pp_cmd[1] = page_addr >> 8;
  pp_cmd[2] = page_addr;
  for (i = 0; i < 256; ++i) {
    pp_cmd[3 + i] = i;
  }

  write_enable();

  send_cmd(CMD_PP, pp_cmd, 256 + 3, NULL, 0);

  chprintf(SD_STDIO, "page program: %x\r\n", read_status_reg());

  while (read_status_reg() & SR_WIP)
    chThdSleepMilliseconds(100);

  chprintf(SD_STDIO, "page program complete: %x\r\n", read_status_reg());
}

static void
page_read(uint32_t page)
{
  int i;
  uint32_t page_addr = page * PAGE_SIZE;
  pp_cmd[0] = page_addr >> 16;
  pp_cmd[1] = page_addr >> 8;
  pp_cmd[2] = page_addr;

  send_cmd(CMD_READ, pp_cmd, 3, rx_buf, 256);

  chprintf(SD_STDIO, "page read:\r\n");

  for (i = 0; i < 256; ++i) {
    unsigned char c = rx_buf[i];
    if (c > 0x0F)
      chprintf(SD_STDIO, "%x ", c);
    else
      chprintf(SD_STDIO, "0%x ", c);
  }
}

void
xflash_write()
{
  int sr = read_status_reg();
  chprintf(SD_STDIO, "STATUS: %x\r\n", sr);

//  sector_erase(0);
//
//  page_program(0);

  page_read(0);

  send_cmd(CMD_RDID, NULL, 0, rx_buf, 80);

  chprintf(SD_STDIO, "RDID:\r\n");
  int i;
  for (i = 0; i < 80; ++i) {
      unsigned char c = rx_buf[i];
      if (c > 0x0F)
        chprintf(SD_STDIO, "%x", c);
      else
        chprintf(SD_STDIO, "0%x", c);
  }
  chprintf(SD_STDIO, "\r\n");
}
