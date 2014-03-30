
#ifndef __CC3000_SPI_H__
#define __CC3000_SPI_H__

#include <string.h>
#include <stdlib.h>
#include "types.h"


void spi_open(void);
void spi_close(void);
uint8_t* spi_get_buffer(void);
void spi_write(uint16_t usLength);

#endif

