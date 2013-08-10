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

  float    integral;
  uint32_t last_time;
  int32_t  last_sample;
  int32_t  pid_output;

  uint16_t sample_time;          // MS2ST(1000);
  bool     in_auto;              //TRUE;
  float    out_min;              //-1000;
  float    out_max;              // 100;
  int8_t   controller_direction; //DIRECT;

  int32_t window_start_time;
  int8_t  pid_turn_relay_on;
} pid_t;


void pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample);
void pid_init(pid_t* pid);

void set_gains(pid_t* pid, float Kp, float Ki, float Kd);

void set_sample_time(pid_t* pid, uint16_t new_sample_time);

void set_mode(pid_t* pid, uint8_t Mode, quantity_t sample);

void pid_reinit(pid_t* pid, quantity_t sample);

void set_controller_direction(pid_t* pid, uint8_t direction);

void set_output_limits(pid_t* pid, float Min, float Max);

#endif
