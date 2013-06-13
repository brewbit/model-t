
#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

typedef enum {
  OUTPUT_FUNC_HEATING,
  OUTPUT_FUNC_COOLING,
  OUTPUT_FUNC_MANUAL
} output_function_t;

typedef enum {
  OUTPUT_TRIG_PROBE1,
  OUTPUT_TRIG_PROBE2
} output_trigger_t;

#endif
