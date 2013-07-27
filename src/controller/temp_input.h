#ifndef TEMP_INPUT_H
#define TEMP_INPUT_H

#include "onewire.h"
#include "common.h"
#include <stdint.h>


typedef enum {
  PROBE_1,
  PROBE_2,

  NUM_PROBES
} probe_id_t;


struct temp_port_s;
typedef struct temp_port_s temp_port_t;

typedef struct {
  probe_id_t probe;
  sensor_sample_t sample;
} sensor_msg_t;

typedef struct {
  probe_id_t probe;
} probe_timeout_msg_t;


temp_port_t*
temp_input_init(probe_id_t probe, onewire_bus_t* port);

#endif
