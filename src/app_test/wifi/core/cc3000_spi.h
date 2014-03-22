
#ifndef __CC3000_SPI_H__
#define __CC3000_SPI_H__

#include <string.h>
#include <stdlib.h>



typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);


extern unsigned char wlan_tx_buffer[];
extern unsigned char wlan_rx_buffer[];


void SpiOpen(gcSpiHandleRx pfRxHandler);
void SpiClose(void);
void SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);
void SpiResumeSpi(void);

#endif

