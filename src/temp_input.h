#ifndef TEMP_INPUT_H
#define TEMP_INPUT_H

#include "onewire.h"
#include <stdint.h>


typedef struct {
  onewire_bus_t bus;
  Thread* thread;
  Thread* handler;
  systime_t last_temp_time;
  bool connected;
} temp_port_t;

typedef enum {
  MSG_NEW_TEMP,
  MSG_PROBE_TIMEOUT,
} temp_input_msg_id_t;

typedef struct {
  temp_input_msg_id_t id;
  float temp;
} temp_input_msg_t;


temp_port_t*
temp_input_init(SerialDriver* port, Thread* handler);

#endif
