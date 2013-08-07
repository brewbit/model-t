#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint8_t
crc8(uint8_t *addr, uint8_t len);

uint16_t
crc16(uint16_t crc, uint8_t data);

#endif
