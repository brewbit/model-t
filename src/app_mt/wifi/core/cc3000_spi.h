
#ifndef __CC3000_SPI_H__
#define __CC3000_SPI_H__

#include <string.h>
#include <stdlib.h>



typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);


extern unsigned char wlan_tx_buffer[];
extern unsigned char wlan_rx_buffer[];


void spi_open(gcSpiHandleRx pfRxHandler);
void spi_close(void);
void spi_write(unsigned char *pUserBuffer, unsigned short usLength);

#endif

