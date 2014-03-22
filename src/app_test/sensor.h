#ifndef SENSOR_H
#define SENSOR_H

#include "onewire.h"
#include "common.h"
#include <stdint.h>


typedef enum {
  SENSOR_1,
  SENSOR_2,

  NUM_SENSORS
} sensor_id_t;


struct sensor_port_s;
typedef struct sensor_port_s sensor_port_t;

typedef struct {
  sensor_id_t sensor;
  quantity_t sample;
} sensor_msg_t;

typedef struct {
  sensor_id_t sensor;
} sensor_timeout_msg_t;


sensor_port_t*
sensor_init(sensor_id_t sensor, onewire_bus_t* port);

bool
sensor_is_connected(sensor_id_t sensor);

#endif
