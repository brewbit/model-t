#ifndef ONEWIRE_H
#define ONEWIRE_H

#include "ch.h"
#include "hal.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  SerialDriver* port;
} onewire_bus_t;

#define READ_ROM            0x33 // Identification
#define SKIP_ROM            0xCC // Skip addressing
#define MATCH_ROM           0x55 // Address specific device
#define SEARCH_ROM          0xF0 // Obtain IDs of all devices on the bus
#define OVERDRIVE_SKIP_ROM  0x3C // Overdrive version of SKIP ROM
#define OVERDRIVE_MATCH_ROM 0x69 // Overdriver version of MATCH ROM

void
onewire_init(onewire_bus_t* ob);

bool
onewire_reset(onewire_bus_t* ob);

bool
onewire_read_rom(onewire_bus_t* ob, uint8_t* addr);

void
onewire_send_bit(onewire_bus_t* ob, uint8_t b);

uint8_t
onewire_recv_bit(onewire_bus_t* ob);

void
onewire_send_byte(onewire_bus_t* ob, uint8_t b);

uint8_t
onewire_recv_byte(onewire_bus_t* ob);

#endif
