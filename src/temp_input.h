#ifndef TEMP_INPUT_H
#define TEMP_INPUT_H

#include "onewire.h"
#include <stdint.h>


typedef struct {
  onewire_bus_t bus;
  Thread* thread;
  systime_t last_temp_time;
  bool connected;
} temp_port_t;


temp_port_t*
temp_input_init(SerialDriver* port);

#endif
