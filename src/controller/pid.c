
#include "pid.h"

/* proportional gain */
float kp[NUM_SENSORS];
/* integral gain */
float ki[NUM_SENSORS];
/* derivative gain */
float kd[NUM_SENSORS];

/* 1 second */
uint16_t sampleTime = MS2ST(1000);
int32_t  lastSample[NUM_SENSORS];
int16_t  output[NUM_SENSORS];
int16_t  iTerm[NUM_SENSORS];
uint16_t  outMin = -200;
uint16_t  outMax = 200;
bool      inAuto = FALSE;

#define MANUAL    0
#define AUTOMATIC 1

#define DIRECT  0
#define REVERSE 1
int8_t controllerDirection = DIRECT;


int16_t pid_exec(sensor_id_t sensor, quantity_t setpoint, quantity_t sample)
{
  static uint32_t lastTime[NUM_SENSORS];
  static float  lastSample[NUM_SENSORS];
  int32_t error;
  int32_t dSample;

  if (!inAuto)
    return output[sensor];

  /*How long since we last calculated*/
  uint32_t currentTime = chTimeNow();
  uint32_t timeChange = (uint32_t)(currentTime - lastTime[sensor]);

  if(timeChange >= sampleTime) {
    /* P roportional */
    error = setpoint.value - sample.value;

    /* I ntegral */
    iTerm[sensor] += (int16_t)(ki[sensor] * error);
    if (iTerm[sensor] > outMax)
      iTerm[sensor] = outMax;
    else if (iTerm[sensor] < outMin)
      iTerm[sensor] = outMin;

    /* D erivative */
    dSample = (uint32_t)(sample.value - lastSample[sensor]);

    /*Compute PID Output*/
    output[sensor] = (int16_t)(kp[sensor] * error) + iTerm[sensor] - (kd[sensor] * dSample);

    /*Remember some variables for next time*/
    lastSample[sensor] = sample.value;
    lastTime[sensor] = currentTime;

    /* return output */
    return output[sensor];
  }
  return output[sensor];
}


// void set_gains(sensor_id_t sensor, int16_t Kp, int16_t Ki, int16_t Kd)
void set_gains(sensor_id_t sensor)
{
  //if (Kp[sensor]<0 || Ki[sensor]<0|| Kd[sensor]<0) return;

  float sampleTimeInSec = ((float)sampleTime)/1000;

  inAuto = AUTOMATIC;

  kp[sensor] = .5;
  ki[sensor] = 0 * sampleTimeInSec;
  kd[sensor] = 0 / sampleTimeInSec;

  if(controllerDirection == REVERSE) {
    kp[sensor] = (0 - kp[sensor]);
    ki[sensor] = (0 - ki[sensor]);
    kd[sensor] = (0 - kd[sensor]);
  }
}


void set_sample_time(sensor_id_t sensor, uint16_t newSampleTime)
{
   if (newSampleTime > 0) {
      float ratio  = (double)newSampleTime
                      / (double)sampleTime;
      ki[sensor] *= ratio;
      kd[sensor] /= ratio;
      sampleTime = (uint16_t)newSampleTime;
   }
}

void SetOutputLimits(sensor_id_t sensor, double Min, double Max)
{
   if(Min > Max) return;
   outMin = Min;
   outMax = Max;

   if(output[sensor] > outMax)
     output[sensor] = outMax;
   else if(output[sensor] < outMin)
     output[sensor] = outMin;

   if(iTerm[sensor] > outMax)
     iTerm[sensor] = outMax;
   else if(iTerm[sensor] < outMin)
     iTerm[sensor] = outMin;
}

void set_mode(uint8_t Mode, sensor_id_t sensor, quantity_t sample)
{
  bool newAuto = (Mode == AUTOMATIC);
  if(newAuto && !inAuto) {
    /*we just went from manual to auto*/
    pid_reinit(sensor, sample);
  }
  inAuto = newAuto;
}

void pid_reinit(sensor_id_t sensor, quantity_t sample)
{
  lastSample[sensor] = sample.value;
  iTerm[sensor] = output[sensor];

  if(iTerm[sensor] > outMax)
    iTerm[sensor] = outMax;
  else if(iTerm[sensor] < outMin)
    iTerm[sensor]= outMin;
}

void SetControllerDirection(uint8_t Direction)
{
   controllerDirection = Direction;
}
