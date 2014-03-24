
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "common.h"
#include "sensor.h"
#include "types.h"
#include "temp_profile.h"


typedef enum {
  OUTPUT_1,
  OUTPUT_2,

  NUM_OUTPUTS
} output_id_t;

typedef enum {
  OUTPUT_FUNC_HEATING,
  OUTPUT_FUNC_COOLING,
  OUTPUT_FUNC_MANUAL
} output_function_t;

typedef enum {
  ON_OFF,
  PID,
} output_ctrl_t;

typedef enum {
  SP_STATIC,
  SP_TEMP_PROFILE
} setpoint_type_t;

typedef struct {
  setpoint_type_t setpoint_type;
  quantity_t static_setpoint;
  uint32_t temp_profile_id;
} sensor_settings_t;

typedef struct {
  output_function_t function;
  sensor_id_t trigger;
  quantity_t compressor_delay;
  output_ctrl_t output_mode;
} output_settings_t;

typedef struct {
  sensor_id_t sensor;
  sensor_settings_t settings;
} sensor_settings_msg_t;

typedef struct {
  output_id_t output;
  output_settings_t settings;
} output_settings_msg_t;

typedef struct {
  output_id_t output;
  bool enabled;
} output_status_t;


void
temp_control_init(void);

void
temp_control_start_temp_profile(sensor_id_t sensor, uint32_t temp_profile_id);

float
temp_control_get_current_setpoint(sensor_id_t sensor);

#endif
