
#include "ch.h"
#include "temp_control.h"
#include "sensor.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "pid.h"

#include <stdlib.h>

typedef struct {
  sensor_port_t* port;
  quantity_t last_sample;
} temp_input_t;

typedef struct {
  uint32_t gpio;
  uint32_t delay_startTime;
  pid_t    pid_control;
} relay_output_t;


static msg_t temp_control_thread(void* arg);
static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data);
static void dispatch_output_settings(output_settings_msg_t* msg);
static void dispatch_sensor_settings(sensor_settings_msg_t* msg);
static void dispatch_sensor_sample(sensor_msg_t* msg);
static void dispatch_sensor_timeout(sensor_timeout_msg_t* msg);
static void trigger_output(sensor_id_t sensor, output_function_t function);
static void evaluate_setpoint(sensor_id_t sensor, quantity_t setpoint, quantity_t sample);
static void start_compressor_delay(uint32_t delay_startTime);
static bool compressor_delay_has_expired(uint32_t compressor_delay, uint32_t delay_startTime);


static temp_input_t inputs[NUM_SENSORS];
static relay_output_t outputs[NUM_OUTPUTS];
static Thread* thread;


void
temp_control_init()
{
  thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, temp_control_thread, NULL);

  inputs[SENSOR_1].port = sensor_init(SENSOR_1, SD_OW1);
  inputs[SENSOR_2].port = sensor_init(SENSOR_2, SD_OW2);

  outputs[OUTPUT_1].gpio = GPIOB_RELAY1;
  outputs[OUTPUT_2].gpio = GPIOB_RELAY2;

  pid_init(&outputs[SENSOR_1].pid_control);
  pid_init(&outputs[SENSOR_2].pid_control);

  msg_subscribe(MSG_SENSOR_SAMPLE, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_SENSOR_TIMEOUT, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_SENSOR_SETTINGS, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_OUTPUT_SETTINGS, thread, dispatch_temp_input_msg, NULL);
}

static msg_t
temp_control_thread(void* arg)
{
  (void)arg;

  while (1) {
    Thread* tp = chMsgWait();
    thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);
    dispatch_temp_input_msg(msg->id, msg->msg_data, msg->user_data);
    chMsgRelease(tp, 0);
  }

  return 0;
}

static void
dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data)
{
  (void)user_data;

  switch (id) {
  case MSG_SENSOR_SAMPLE:
    dispatch_sensor_sample(msg_data);
    break;

  case MSG_SENSOR_TIMEOUT:
    dispatch_sensor_timeout(msg_data);
    break;

  case MSG_SENSOR_SETTINGS:
    dispatch_sensor_settings(msg_data);
    break;

  case MSG_OUTPUT_SETTINGS:
    dispatch_output_settings(msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_sensor_sample(sensor_msg_t* msg)
{
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(msg->sensor);

  evaluate_setpoint(
      msg->sensor,
      sensor_settings->setpoint,
      msg->sample);
}

static void
dispatch_sensor_timeout(sensor_timeout_msg_t* msg)
{
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(msg->sensor);

  /* Set the last temp to the setpoint to disable any active outputs */
  evaluate_setpoint(
      msg->sensor,
      sensor_settings->setpoint,
      sensor_settings->setpoint);
}

static void
dispatch_output_settings(output_settings_msg_t* msg)
{
  if (msg->output >= NUM_OUTPUTS)
    return;

  /* Re-evaluate the last temp from both sensors with the new output settings */
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(SENSOR_1);

  evaluate_setpoint(
      SENSOR_1,
      sensor_settings->setpoint,
      inputs[SENSOR_1].last_sample);

  sensor_settings = app_cfg_get_sensor_settings(SENSOR_2);
  evaluate_setpoint(
      SENSOR_2,
      sensor_settings->setpoint,
      inputs[SENSOR_2].last_sample);
}

static void
dispatch_sensor_settings(sensor_settings_msg_t* msg)
{
  if (msg->sensor >= NUM_SENSORS)
    return;

  /* Re-evaluate the last temp reading with the new sensor settings */
  evaluate_setpoint(
      msg->sensor,
      msg->settings.setpoint,
      inputs[msg->sensor].last_sample);
}

static void
evaluate_setpoint(sensor_id_t sensor, quantity_t setpoint, quantity_t sample)
{
  inputs[sensor].last_sample = sample;

  if ((sample.value) > setpoint.value) {
    trigger_output(sensor, OUTPUT_FUNC_COOLING);
  }
  else if ((sample.value) < setpoint.value) {
    trigger_output(sensor, OUTPUT_FUNC_HEATING);
  }
}

static void
trigger_output(sensor_id_t sensor, output_function_t function)
{
  int i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);
    bool time_expired = compressor_delay_has_expired(output_settings->compressor_delay,
                                                     outputs[i].delay_startTime);

     if (output_settings->trigger == sensor) {
      if (output_settings->function == function) {
    	if(time_expired) {
    	  palSetPad(GPIOB, outputs[i].gpio);
    	}
      }
      else {
        palClearPad(GPIOB, outputs[i].gpio);
        start_compressor_delay(outputs[i].delay_startTime);
      }
    }
  }
}

static bool
compressor_delay_has_expired(uint32_t compressor_delay, uint32_t delay_startTime)
{
  uint32_t current_time = chTimeNow();

  if ((current_time - delay_startTime) > compressor_delay)
    return TRUE;
  else
    return FALSE;
}

static void
start_compressor_delay(uint32_t delay_startTime)
{
  (void)delay_startTime;
//  delay_startTime = chTimeNow();
}

