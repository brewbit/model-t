
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "common.h"

typedef enum {
  PROBE_1,
  PROBE_2,

  NUM_PROBES
} probe_id_t;

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
  temperature_t setpoint;
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

probe_settings_t
temp_control_get_probe_settings(probe_id_t probe);

output_settings_t
temp_control_get_output_settings(output_id_t output);

#endif
