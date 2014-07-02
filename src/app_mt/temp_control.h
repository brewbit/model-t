
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "common.h"
#include "sensor.h"
#include "types.h"


typedef enum {
  CONTROLLER_1,
  CONTROLLER_2,
  NUM_CONTROLLERS
} temp_controller_id_t;

#include "temp_profile.h"

typedef enum {
  OUTPUT_NONE = -1,
  OUTPUT_1,
  OUTPUT_2,

  NUM_OUTPUTS
} output_id_t;

typedef enum {
  OUTPUT_FUNC_HEATING,
  OUTPUT_FUNC_COOLING,
  OUTPUT_FUNC_NONE
} output_function_t;

typedef enum {
  ON_OFF,
  PID,
} output_ctrl_t;

typedef enum {
  SP_STATIC,
  SP_TEMP_PROFILE
} setpoint_type_t;

typedef enum {
  STEP_HOLD,
  STEP_RAMP
} temp_profile_step_type_t;

typedef struct {
  bool enabled;
  output_function_t function;
  quantity_t cycle_delay;
} output_settings_t;

typedef struct {
  uint32_t duration;
  quantity_t value;
  temp_profile_step_type_t type;
} temp_profile_step_t;

typedef struct {
  uint32_t id;
  char name[100];
  uint32_t num_steps;
  quantity_t start_value;
  temp_profile_step_t steps[32];
} temp_profile_t;

typedef struct {
  temp_controller_id_t controller;
  setpoint_type_t setpoint_type;
  quantity_t static_setpoint;
  temp_profile_t temp_profile;
  output_settings_t output_settings[NUM_OUTPUTS];
} controller_settings_t;

typedef enum {
  CYCLE_DELAY,
  OUTPUT_CONTROL_ENABLED,
  OUTPUT_CONTROL_DISABLED
} output_state_t;

typedef struct {
  output_id_t output;
  bool enabled;
  output_state_t state;
} output_status_t;


void
temp_control_init(temp_controller_id_t controller);

void
temp_control_start(controller_settings_t* cmd);

void
temp_control_halt(temp_controller_id_t controller);

float
temp_control_get_current_setpoint(temp_controller_id_t controller);

output_ctrl_t
temp_control_get_output_function(output_id_t output);

#endif
