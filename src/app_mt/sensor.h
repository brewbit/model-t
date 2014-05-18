#ifndef SENSOR_H
#define SENSOR_H

#include "onewire.h"
#include "common.h"
#include <stdint.h>


#define MAX_NUM_SENSOR_CONFIGS 32

typedef enum {
  SENSOR_NONE = -1,
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

typedef struct {
  uint8_t sensor_sn[6];
  quantity_t offset;
} sensor_config_t;

typedef struct {
  uint8_t sensor_sn[6];
  quantity_t offset;
  sensor_id_t sensor_id;
} sensor_cfg_msg_t;

sensor_port_t*
sensor_init(sensor_id_t sensor, onewire_bus_t* port);

sensor_config_t*
get_sensor_cfg(sensor_id_t sensor_id);

bool
get_sensor_conn_status(sensor_id_t sensor_id);

#endif
