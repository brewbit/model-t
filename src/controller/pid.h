#ifndef PID_H
#define PID_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "ch.h"
#include "types.h"
#include "temp_input.h"


int16_t pid_exec(probe_id_t probe, quantity_t setpoint, quantity_t sample);

void set_gains(probe_id_t probe);

void set_sample_time(probe_id_t probe, uint16_t newSampleTime);

void set_mode(uint8_t Mode, probe_id_t probe, quantity_t sample);

void pid_reinit(probe_id_t probe, quantity_t sample);

void SetControllerDirection(uint8_t Direction);

#endif
