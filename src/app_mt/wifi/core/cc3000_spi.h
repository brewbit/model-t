
#ifndef __CC3000_SPI_H__
#define __CC3000_SPI_H__

#include <string.h>
#include <stdlib.h>
#include "types.h"


extern uint8_t wlan_tx_buffer[];
extern uint8_t wlan_rx_buffer[];


void spi_open(void);
void spi_close(void);
void spi_write(uint8_t *pUserBuffer, uint16_t usLength);

#endif

