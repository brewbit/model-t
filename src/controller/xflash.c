
#include "ch.h"
#include "hal.h"

#include "xflash.h"


#define ASSERT_CS()    do { \
                         spiAcquireBus(SPI_FLASH); \
                         spiStart(SPI_FLASH, &flash_spi_cfg); \
                         spiSelect(SPI_FLASH); \
                       } while (0)


#define DEASSERT_CS()  do { \
                         spiUnselect(SPI_FLASH); \
                         spiReleaseBus(SPI_FLASH); \
                       } while (0)

static const SPIConfig flash_spi_cfg = {
    .end_cb = NULL,
    .ssport = PORT_SFLASH_CS,
    .sspad = PAD_SFLASH_CS,
    .cr1 = SPI_CR1_CPOL | SPI_CR1_CPHA
};

void
xflash_write()
{
  ASSERT_CS();
//  spiSend(SPI_FLASH);
  DEASSERT_CS();
}
