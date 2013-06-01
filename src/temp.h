#ifndef TEMP_H__
#define TEMP_H__

#include "onewire.h"
#include <stdint.h>


typedef struct {
  onewire_bus_t ob;
  uint8_t wa_thread[512];
  Thread* thread;
} temp_port_t;

void
temp_init(temp_port_t* tp);

#endif
