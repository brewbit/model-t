
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "common.h"
#include "temp_input.h"
#include "types.h"

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


typedef struct {
  sensor_sample_t setpoint;
} probe_settings_t;

typedef struct {
  output_function_t function;
  probe_id_t trigger;
} output_settings_t;

typedef struct {
  probe_id_t probe;
  probe_settings_t settings;
} probe_settings_msg_t;

typedef struct {
  output_id_t output;
  output_settings_t settings;
} output_settings_msg_t;


void
temp_control_init(void);

#endif
