
#include "ch.h"
#include "temp_control.h"
#include "sensor.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "temp_profile.h"
#include "pid.h"
#include "board.h"

#include <stdlib.h>


typedef enum {
  TC_IDLE,
  TC_ACTIVE,
  TC_SENSOR_TIMED_OUT
} temp_controller_state_t;

struct temp_controller_s;

typedef struct {
  output_id_t id;
  pid_controller_t pid_control;
  output_status_t status;
  bool temp_ovrd;
  bool output_ovrd;
  systime_t cycle_delay_start_time;
  struct temp_controller_s* controller;
  Thread* thread;
} relay_output_t;

typedef struct temp_controller_s {
  sensor_id_t sensor;
  temp_controller_id_t controller;
  temp_controller_state_t state;
  quantity_t last_sample;
  temp_profile_run_t temp_profile_run;
  relay_output_t outputs[NUM_OUTPUTS];
} temp_controller_t;


static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);
static void dispatch_controller_settings(temp_controller_t* tc, const controller_settings_t* msg, bool resume_profile);
static void dispatch_init(temp_controller_t* tc);
static void dispatch_sensor_sample(temp_controller_t* tc, sensor_msg_t* msg);
static void dispatch_sensor_timeout(temp_controller_t* tc, sensor_timeout_msg_t* msg);
static void dispatch_output_ovrd(temp_controller_t* tc, output_ovrd_msg_t* msg);
static void output_init(temp_controller_t* tc, output_id_t id);
static msg_t output_thread(void* arg);
static void start_cycle_delay(relay_output_t* output);
static void set_output_state(relay_output_t* output, output_state_t output_state);
static void relay_control(relay_output_t* output);
static void enable_relay(relay_output_t* output, bool enabled);
static float get_sp(temp_controller_t* tc);
static const output_settings_t* get_output_settings(temp_controller_t* tc, output_id_t output);
static void internal_temp_ovrd_check(relay_output_t* output);

static temp_controller_t* controllers[NUM_CONTROLLERS];

static const uint32_t out_gpio[NUM_OUTPUTS] = {
    [OUTPUT_1] = PAD_RELAY1,
    [OUTPUT_2] = PAD_RELAY2
};


void
temp_control_init(temp_controller_id_t controller)
{
  temp_controller_t* tc = calloc(1, sizeof(temp_controller_t));
  controllers[controller] = tc;
  tc->controller = controller;
  if (controller == CONTROLLER_1)
    tc->sensor = SENSOR_1;
  else
    tc->sensor = SENSOR_2;

  tc->state = TC_SENSOR_TIMED_OUT;

  msg_listener_t* l = msg_listener_create("temp_ctrl", 1024, dispatch_temp_input_msg, tc);

  msg_subscribe(l, MSG_SENSOR_SAMPLE,   NULL);
  msg_subscribe(l, MSG_SENSOR_TIMEOUT,  NULL);
  msg_subscribe(l, MSG_API_CONTROLLER_SETTINGS, NULL);
  msg_subscribe(l, MSG_CONTROLLER_SETTINGS, NULL);
  msg_subscribe(l, MSG_OUTPUT_OVRD, NULL);
}

float
temp_control_get_current_setpoint(temp_controller_id_t controller)
{
  if (controller >= NUM_CONTROLLERS)
    return NAN;

  temp_controller_t* tc = controllers[controller];

  return get_sp(tc);
}

temp_control_status_t
temp_control_get_status(temp_controller_id_t controller, output_id_t output)
{
  temp_control_status_t status;
  temp_controller_t* tc = controllers[controller];
  const output_settings_t* output_settings = get_output_settings(tc, output);

  status.function = output_settings->function;
  status.output_enabled = tc->outputs[output].status.enabled;
  status.kp = tc->outputs[output].pid_control.kp;
  status.ki = tc->outputs[output].pid_control.ki;
  status.kd = tc->outputs[output].pid_control.kd;

  return (status);
}

temp_controller_t*
temp_control_get_controller_for(output_id_t output)
{
  int i;

  if (output >= NUM_OUTPUTS)
    return NULL;

  for (i = 0; i < NUM_CONTROLLERS; ++i) {
    const controller_settings_t* controller_settings =
        app_cfg_get_controller_settings(i);

    if (controller_settings->output_settings[output].enabled)
      return controllers[i];
  }

  return NULL;
}

void
temp_control_enable_output(output_id_t output, bool enable)
{
  temp_controller_t* tc = temp_control_get_controller_for(output);
  if (tc != NULL)
    enable_relay(&tc->outputs[output], enable);
}

output_function_t
temp_control_get_output_function(output_id_t output)
{
  temp_controller_t* tc = temp_control_get_controller_for(output);
  if (tc != NULL)
    return get_output_settings(tc, output)->function;

  return OUTPUT_FUNC_NONE;
}

static const output_settings_t*
get_output_settings(temp_controller_t* tc, output_id_t output)
{
  const controller_settings_t* settings = app_cfg_get_controller_settings(tc->controller);
  return &settings->output_settings[output];
}

static void
output_init(temp_controller_t* tc, output_id_t output)
{
  relay_output_t* out = &tc->outputs[output];
  const output_settings_t* settings = get_output_settings(tc, output);

  out->id = output;
  out->controller = tc;

  if (settings->function == OUTPUT_FUNC_MANUAL)
    return;

  pid_init(&out->pid_control);
  pid_set_output_limits(&out->pid_control, -20, 20);

  if (settings->function == OUTPUT_FUNC_COOLING)
    pid_set_output_sign(&out->pid_control, NEGATIVE);
  else
    pid_set_output_sign(&out->pid_control, POSITIVE);

  out->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, output_thread, out);
}

static void
internal_temp_ovrd_check(relay_output_t* output)
{
  /* Check internal unit temperature and disable outputs if temperature exceeds 85C.
   * Do no allow outputs to re-enable until temperature is below 70 C.
   */
  if (board_get_core_temp() < 70.0) {
    output->temp_ovrd = false;
  }
  else if (board_get_core_temp() > 85.0) {
    output->temp_ovrd = true;
  }
}

static msg_t
output_thread(void* arg)
{
  relay_output_t* output = arg;
  chRegSetThreadName("output");

  const output_settings_t* output_settings =
      get_output_settings(output->controller, output->id);
  systime_t cycle_delay = S2ST(60 * output_settings->cycle_delay.value);

  if (output_settings->function == OUTPUT_FUNC_COOLING)
    pid_set_output_sign(&output->pid_control, NEGATIVE);
  else if (output_settings->function == OUTPUT_FUNC_HEATING)
    pid_set_output_sign(&output->pid_control, POSITIVE);

  pid_reinit(&output->pid_control,
      output->controller->last_sample.value);

  /* Wait 1 cycle delay before starting window and PID */
  start_cycle_delay(output);

  output->status.output = output->id;

  while (!chThdShouldTerminate()) {

    /* Check internal unit temperature and disable outputs if temperature exceeds 85C.
     * Do no allow outputs to re-enable until temperature is below 70 C.
     */
	internal_temp_ovrd_check(output);

	/* If the probe associated with this output is not active or if the output is set
     * to disabled turn OFF the output
     */
    if (output->controller->state != TC_ACTIVE ||
        !output_settings->enabled ||
        output->temp_ovrd)
      set_output_state(output, OUTPUT_CONTROL_DISABLED);

    switch (output->status.state) {
      case OUTPUT_CONTROL_DISABLED:
        enable_relay(output, false);

        if (output->controller->state == TC_ACTIVE &&
            output_settings->enabled)
          start_cycle_delay(output);
        break;

      case OUTPUT_CONTROL_ENABLED:
        relay_control(output);
        break;

      case CYCLE_DELAY:
        if (cycle_delay < 1) {
          set_output_state(output, OUTPUT_CONTROL_ENABLED);
          break;
        }

        if ((chTimeNow() - output->cycle_delay_start_time) > cycle_delay) {
          /* Restart PID after cycle delay */
          if (output->pid_control.enabled == false)
            output->pid_control.enabled = true;

          set_output_state(output, OUTPUT_CONTROL_ENABLED);
        }
        break;
    }

    chThdSleepSeconds(1);
  }

  enable_relay(output, false);

  return 0;
}

static void
relay_control(relay_output_t* output)
{
  const output_settings_t* output_settings = get_output_settings(output->controller, output->id);
  float sample = output->controller->last_sample.value;
  float setpoint = get_sp(output->controller);
  float hysteresis = app_cfg_get_hysteresis().value;

  output->status.output = output->id;

  if (output->output_ovrd) {
    enable_relay(output, false);
    output->pid_control.enabled = false;
    return;
  }

  switch (app_cfg_get_control_mode()) {
  case ON_OFF:
    if (output_settings->function == OUTPUT_FUNC_HEATING) {
      if (sample <= setpoint - hysteresis)
        enable_relay(output, true);
      else if (sample >= setpoint)
        enable_relay(output, false);
    }
    else {
      if (sample >= setpoint + hysteresis)
        enable_relay(output, true);
      else if (sample <= setpoint)
        enable_relay(output, false);
    }
    break;

  case PID:
    if (output->pid_control.enabled == false)
      output->pid_control.enabled = true;

    if (output_settings->function == OUTPUT_FUNC_HEATING) {
      if (sample < (setpoint + output->pid_control.out) - hysteresis)
        enable_relay(output, true);
      else
        enable_relay(output, false);
    }
    else {
      if (sample > (setpoint - output->pid_control.out) + hysteresis)
        enable_relay(output, true);
      else
        enable_relay(output, false);
    }
    break;

  default:
    break;
  }
}

static void
enable_relay(relay_output_t* output, bool enable)
{
  /* If the output was just disabled, start the cycle delay */
  if (output->status.enabled && !enable)
    start_cycle_delay(output);

  palWritePad(GPIOC, out_gpio[output->id], enable);
  output->status.enabled = enable;
  msg_send(MSG_OUTPUT_STATUS, &output->status);
}

static void
start_cycle_delay(relay_output_t* output)
{
  output->pid_control.enabled = false;
  output->cycle_delay_start_time = chTimeNow();
  set_output_state(output, CYCLE_DELAY);
}

static void
set_output_state(relay_output_t* output, output_state_t output_state)
{
  if (output->status.state != output_state) {
    output->status.state = output_state;
    msg_send(MSG_OUTPUT_STATUS, &output->status);
  }
}

static void
dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)listener_data;
  (void)sub_data;

  switch (id) {
  case MSG_INIT:
    dispatch_init(listener_data);
    break;

  case MSG_SENSOR_SAMPLE:
    dispatch_sensor_sample(listener_data, msg_data);
    break;

  case MSG_SENSOR_TIMEOUT:
    dispatch_sensor_timeout(listener_data, msg_data);
    break;

  case MSG_CONTROLLER_SETTINGS:
  case MSG_API_CONTROLLER_SETTINGS:
    dispatch_controller_settings(listener_data, msg_data, false);
    break;

  case MSG_OUTPUT_OVRD:
    dispatch_output_ovrd(listener_data, msg_data);
    break;

  default:
    break;
  }
}

static float
get_sp(temp_controller_t* tc)
{
  float sp;
  const controller_settings_t* settings = app_cfg_get_controller_settings(tc->controller);

  if (settings->setpoint_type == SP_STATIC)
    return settings->static_setpoint.value;
  else if (temp_profile_get_current_setpoint(&tc->temp_profile_run, &sp))
    return sp;

  return NAN;
}

static void
dispatch_init(temp_controller_t* tc)
{
  const controller_settings_t* cs = app_cfg_get_controller_settings(tc->controller);
  dispatch_controller_settings(tc, cs, true);
}

static void
dispatch_sensor_sample(temp_controller_t* tc, sensor_msg_t* msg)
{
  int i;

  if (msg->sensor != tc->sensor)
    return;

  tc->last_sample = msg->sample;
  if (tc->state == TC_SENSOR_TIMED_OUT)
    tc->state = TC_ACTIVE;

  const controller_settings_t* cs = app_cfg_get_controller_settings(tc->controller);
  if (cs->setpoint_type == SP_TEMP_PROFILE)
    temp_profile_update(&tc->temp_profile_run, msg->sample);

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = get_output_settings(tc, tc->outputs[i].id);
      if (app_cfg_get_control_mode() == PID &&
          output_settings->enabled == true) {
        pid_exec(&tc->outputs[i].pid_control,
            get_sp(tc),
            msg->sample.value);
      }
  }
}

static void
dispatch_sensor_timeout(temp_controller_t* tc, sensor_timeout_msg_t* msg)
{
  if (msg->sensor != tc->sensor)
    return;

  if (tc->state == TC_ACTIVE)
    tc->state = TC_SENSOR_TIMED_OUT;
}

static void
dispatch_controller_settings(temp_controller_t* tc, const controller_settings_t* settings, bool resume_profile)
{
  int i;
  if (tc->controller != settings->controller)
    return;

  /* Stop output threads */
  for (i = 0; i < NUM_OUTPUTS; ++i) {
    relay_output_t* out = &tc->outputs[i];
    if (out->thread != NULL) {
      chThdTerminate(out->thread);
      chThdWait(out->thread);
      out->thread = NULL;
    }
  }

  tc->state = TC_IDLE;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    if (settings->output_settings[i].enabled)
      output_init(tc, i);
  }

  if (settings->setpoint_type == SP_TEMP_PROFILE) {
    temp_profile_run_t* tpr = &tc->temp_profile_run;
    if (resume_profile)
      temp_profile_resume(tpr, tc->controller);
    else
      temp_profile_start(tpr, tc->controller, settings->temp_profile.id, settings->temp_profile.start_point);
  }

  tc->state = TC_SENSOR_TIMED_OUT;
}

static void
dispatch_output_ovrd(temp_controller_t* tc, output_ovrd_msg_t* msg)
{
  if (tc->outputs[msg->output].output_ovrd == false)
    tc->outputs[msg->output].output_ovrd = true;
  else
    tc->outputs[msg->output].output_ovrd = false;
}
