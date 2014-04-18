
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "common.h"
#include "sensor.h"
#include "types.h"
#include "temp_profile.h"


typedef enum {
  OUTPUT_NONE = -1,
  OUTPUT_1,
  OUTPUT_2,

  NUM_OUTPUTS
} output_id_t;

typedef enum {
  SELECT_NONE,
  SELECT_1,
  SELECT_2,
  SELECT_1_2,

  NUM_OUTPUT_SELECTIONS
} output_selection_t;

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
  bool enabled;
  output_function_t function;
  quantity_t cycle_delay;
  output_ctrl_t output_mode;
  quantity_t hysteresis;
  sensor_id_t trigger;

} output_settings_t;

typedef struct {
  setpoint_type_t setpoint_type;
  quantity_t static_setpoint;
  uint32_t temp_profile_id;

  uint8_t output_selection;
} controller_settings_t;

typedef struct {
  sensor_id_t sensor;
  controller_settings_t settings;
} controller_settings_msg_t;

typedef struct {
  output_id_t output;
  output_settings_t settings;
} output_settings_msg_t;

typedef struct {
  output_id_t output;
  bool enabled;
} output_status_t;

typedef struct {
  controller_settings_t controller_settings[NUM_SENSORS];
  output_settings_t output_settings[NUM_OUTPUTS];
} temp_control_cmd_t;


void
temp_control_init(void);

void
temp_control_start(temp_control_cmd_t* cmd);

void
temp_control_halt(void);

float
temp_control_get_current_setpoint(sensor_id_t sensor);

#endif
