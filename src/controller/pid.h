#ifndef PID_H
#define PID_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "ch.h"
#include "types.h"
#include "sensor.h"


#define MANUAL    0
#define AUTOMATIC 1

#define DIRECT  0
#define REVERSE 1

typedef struct {
  float kp;
  float ki;
  float kd;

  float integral;
  float last_sample;
  float pid_output;

  bool   in_auto;
  float  out_min;
  float  out_max;
  int8_t controller_direction;

  /* Time is in system ticks */
  uint16_t sample_time;
  uint32_t last_time;
  uint32_t window_size;
  uint32_t window_start_time;

} pid_t;



void pid_init(pid_t* pid, quantity_t sample);
void pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample);
void set_gains(pid_t* pid, float Kp, float Ki, float Kd);
void set_mode(pid_t* pid, quantity_t sample, uint8_t Mode);
void pid_reinit(pid_t* pid, quantity_t sample);
void set_controller_direction(pid_t* pid, uint8_t direction);
void set_output_limits(pid_t* pid, float Min, float Max);

#endif
