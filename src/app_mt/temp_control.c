
#include "ch.h"
#include "temp_control.h"
#include "sensor.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "pid.h"
#include "sensor_settings.h"

#include <stdlib.h>

typedef struct {
  sensor_port_t* port;
  quantity_t last_sample;
} temp_input_t;

typedef struct {
  output_id_t id;
  uint32_t gpio;
  pid_t    pid_control;
  output_ctrl_t output_mode;

  bool sensor_active;

  systime_t window_start_time;
  systime_t window_time;
  output_status_t status;
} relay_output_t;


static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);
static void dispatch_output_settings(output_settings_msg_t* msg);
static void dispatch_sensor_settings(sensor_settings_msg_t* msg);
static void dispatch_sensor_sample(sensor_msg_t* msg);
static void dispatch_sensor_timeout(sensor_timeout_msg_t* msg);
static void output_init(relay_output_t* out, output_id_t id, uint32_t gpio);
static msg_t output_thread(void* arg);
static void cycle_delay(output_id_t output);
static void relay_control(relay_output_t* output);
static void enable_relay(relay_output_t* output, bool enabled);


static temp_input_t inputs[NUM_SENSORS];
static relay_output_t outputs[NUM_OUTPUTS];


void
temp_control_init()
{
  inputs[SENSOR_1].port = sensor_init(SENSOR_1, SD_OW1);
  inputs[SENSOR_2].port = sensor_init(SENSOR_2, SD_OW2);

  output_init(&outputs[OUTPUT_1], OUTPUT_1, PAD_RELAY1);
  output_init(&outputs[OUTPUT_2], OUTPUT_2, PAD_RELAY2);

  msg_listener_t* l = msg_listener_create("temp_ctrl", 1024, dispatch_temp_input_msg, NULL);

  msg_subscribe(l, MSG_SENSOR_SAMPLE,   NULL);
  msg_subscribe(l, MSG_SENSOR_TIMEOUT,  NULL);
  msg_subscribe(l, MSG_SENSOR_SETTINGS, NULL);
  msg_subscribe(l, MSG_OUTPUT_SETTINGS, NULL);
}

static void
output_init(relay_output_t* out, output_id_t id, uint32_t gpio)
{
  const output_settings_t* settings = app_cfg_get_output_settings(id);

  out->id = id;
  out->gpio = gpio;
  out->output_mode = settings->output_mode;
  out->window_time = settings->compressor_delay.value * S2ST(60) * 4;

  pid_init(&out->pid_control);
  pid_set_output_limits(&out->pid_control, 0, out->window_time);

  if (settings->function == OUTPUT_FUNC_COOLING)
    pid_set_output_sign(&out->pid_control, NEGATIVE);
  else
    pid_set_output_sign(&out->pid_control, POSITIVE);

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, output_thread, out);
}

static msg_t
output_thread(void* arg)
{
  relay_output_t* output = arg;
  chRegSetThreadName("output");

  /* Wait 1 compressor delay before starting window */
  cycle_delay(output->id);

  output->status.output = output->id;

  output->window_start_time = chTimeNow();
  palSetPad(GPIOC, output->gpio);

  while (1) {
    /* If the probe associated with this output is not active disable the output */
    if (!output->sensor_active)
      enable_relay(output, false);
    else
      relay_control(output);

    chThdSleepSeconds(1);
  }

  return 0;
}

static void
relay_control(relay_output_t* output)
{
  const output_settings_t* output_settings = app_cfg_get_output_settings(output->id);
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(output_settings->trigger);

  output->status.output = output->id;

  switch(output->output_mode) {
  case ON_OFF:
    if (output_settings->function == OUTPUT_FUNC_HEATING) {
      if (inputs[output->id].last_sample.value < sensor_settings_get_current_setpoint(sensor_settings))
        enable_relay(output, true);
      else
        enable_relay(output, false);
    }
    else {
      if (inputs[output->id].last_sample.value > sensor_settings_get_current_setpoint(sensor_settings))
        enable_relay(output, true);
      else
        enable_relay(output, false);
    }

    cycle_delay(output->id);
    break;

  case PID:
    if ((chTimeNow() - output->window_start_time) >= output->pid_control.out) {
      systime_t sleepTime;
      int32_t   windowDelay;

      enable_relay(output, false);

      /* Make sure thread sleep time is > 0  or else chThdSleep will crap itself */
      windowDelay = output->window_time - output->pid_control.out;
      sleepTime =  windowDelay > 0 ? windowDelay : 0;
      if (sleepTime > 0)
        chThdSleep(sleepTime);

      /* Setup next on window */
      output->window_start_time = chTimeNow();
      enable_relay(output, true);
    }
    break;

  default:
    break;
  }
}

static void
enable_relay(relay_output_t* output, bool enabled)
{
  palWritePad(GPIOC, output->gpio, enabled);
  output->status.enabled = enabled;
  msg_send(MSG_OUTPUT_STATUS, &output->status);
}

static void
cycle_delay(output_id_t output)
{
  const output_settings_t* settings = app_cfg_get_output_settings(output);
  if (settings->compressor_delay.value > 0)
    chThdSleepSeconds(60 * settings->compressor_delay.value);
}

static void
dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)listener_data;
  (void)sub_data;

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
  int i;

  inputs[msg->sensor].last_sample = msg->sample;

  for (i = 0; i < NUM_OUTPUTS; i++) {
    const output_settings_t* settings = app_cfg_get_output_settings(i);
    if (msg->sensor == settings->trigger)
      outputs[i].sensor_active = true;
  }

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);
    if ((msg->sensor == output_settings->trigger) &&
        (output_settings->output_mode == PID)) {
      const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(output_settings->trigger);

      pid_exec(&outputs[i].pid_control,
          sensor_settings_get_current_setpoint(sensor_settings),
          msg->sample.value);
    }
  }
}

static void
dispatch_sensor_timeout(sensor_timeout_msg_t* msg)
{
  int i;
  for (i = 0; i < NUM_OUTPUTS; i++) {
    const output_settings_t* settings = app_cfg_get_output_settings(i);
    if (msg->sensor == settings->trigger)
      outputs[i].sensor_active = false;
  }
}

static void
dispatch_output_settings(output_settings_msg_t* msg)
{
  if (msg->output >= NUM_OUTPUTS)
    return;

  uint8_t i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    relay_output_t* output = &outputs[i];

    const output_settings_t* output_settings = app_cfg_get_output_settings(i);
    if (output_settings->function == OUTPUT_FUNC_COOLING)
      pid_set_output_sign(&output->pid_control, NEGATIVE);
    else if (output_settings->function == OUTPUT_FUNC_HEATING)
      pid_set_output_sign(&output->pid_control, POSITIVE);

      pid_set_output_limits(
          &output->pid_control,
          0,
          output->window_time - (msg->settings.compressor_delay.value * S2ST(60)));

      if (output_settings->trigger == SENSOR_1)
        pid_reinit(&output->pid_control, inputs[SENSOR_1].last_sample.value);
      else if (output_settings->trigger == SENSOR_2)
        pid_reinit(&output->pid_control, inputs[SENSOR_2].last_sample.value);
  }
}

static void
dispatch_sensor_settings(sensor_settings_msg_t* msg)
{
  if (msg->sensor >= NUM_SENSORS)
    return;

  uint8_t i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
      const output_settings_t* output_settings = app_cfg_get_output_settings(i);
      if (output_settings->trigger == SENSOR_1)
        pid_reinit(&outputs[i].pid_control, inputs[SENSOR_1].last_sample.value);
      else if (output_settings->trigger == SENSOR_2)
        pid_reinit(&outputs[i].pid_control, inputs[SENSOR_2].last_sample.value);
  }
}
