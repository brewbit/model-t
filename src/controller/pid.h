#ifndef PID_H
#define PID_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "ch.h"
#include "types.h"
#include "sensor.h"


int16_t pid_exec(sensor_id_t sensor, quantity_t setpoint, quantity_t sample);

void set_gains(sensor_id_t sensor);

void set_sample_time(sensor_id_t sensor, uint16_t newSampleTime);

void set_mode(uint8_t Mode, sensor_id_t sensor, quantity_t sample);

void pid_reinit(sensor_id_t sensor, quantity_t sample);

void SetControllerDirection(uint8_t Direction);

#endif
