
#include "pid.h"

int32_t window_size = 1000;

void pid_init(pid_t* pid)
{
  pid->kp = 5;
  pid->ki = .1;
  pid->kd = 1;

  pid->sample_time = MS2ST(1000);
  pid->in_auto = TRUE;
  pid->out_min = -1000;
  pid->out_max = 1000;
  pid->controller_direction = DIRECT;

  pid->window_start_time = chTimeNow();
  pid->pid_turn_relay_on = FALSE;
}

void pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample)
{
  if (!pid->in_auto)
    return;

  int32_t error;
  float   dSample;

  int32_t current_time = chTimeNow();
  uint32_t time_change = (uint32_t)(current_time - pid->last_time);

  if(time_change >= pid->sample_time) {
    error = setpoint.value - sample.value;

    pid->integral += (float)(pid->ki * error);
    if (pid->integral > pid->out_max)
      pid->integral = pid->out_max;
    else if (pid->integral < pid->out_min)
      pid->integral = pid->out_min;

    dSample = (uint32_t)(sample.value - pid->last_sample);

    /*Compute PID Output*/
    pid->pid_output = (int32_t)((pid->kp * (float)error) + pid->integral - (pid->kd * dSample));

    if(current_time - pid->window_start_time > window_size) {
      pid->window_start_time += window_size;
    }

    if((current_time - pid->window_start_time) < pid->pid_output)
      pid->pid_turn_relay_on = TRUE;
    else
      pid->pid_turn_relay_on = FALSE;

    /*Remember some variables for next time*/
    pid->last_sample = sample.value;
    pid->last_time = current_time;
  }
}


void set_gains(pid_t* pid, float Kp, float Ki, float Kd)
{
  float sample_time_in_sec = ((float)pid->sample_time)/1000;

  pid->in_auto = AUTOMATIC;

  pid->kp = Kp;
  pid->ki = Ki * sample_time_in_sec;
  pid->kd = Kd / sample_time_in_sec;

  if(pid->controller_direction == REVERSE) {
    pid->kp = (0 - pid->kp);
    pid->ki = (0 - pid->ki);
    pid->kd = (0 - pid->kd);
  }
}


void set_sample_time(pid_t* pid, uint16_t new_sample_time)
{
   if (new_sample_time > 0) {
      float ratio  = (double)new_sample_time / (double)pid->sample_time;
      pid->ki *= ratio;
      pid->kd /= ratio;
      pid->sample_time = new_sample_time;
   }
}

void set_output_limits(pid_t* pid, float Min, float Max)
{
   if(Min > Max)
     return;

   pid->out_min = Min;
   pid->out_max = Max;
}

void set_mode(pid_t* pid, uint8_t Mode, quantity_t sample)
{
  bool new_auto = (Mode == AUTOMATIC);
  if(new_auto && !pid->in_auto) {
    /*we just went from manual to auto*/
    pid_reinit(pid, sample);
  }
  pid->in_auto = new_auto;
}

void pid_reinit(pid_t* pid, quantity_t sample)
{
  pid->last_sample = sample.value;
  pid->integral = pid->pid_output;

  if(pid->integral > pid->out_max)
    pid->integral = pid->out_max;
  else if(pid->integral < pid->out_min)
    pid->integral = pid->out_min;
}

void set_controller_direction(pid_t* pid, uint8_t direction)
{
   pid->controller_direction = direction;
}
