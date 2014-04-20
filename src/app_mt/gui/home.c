
#include "gui/home.h"
#include "gfx.h"
#include "gui.h"
#include "button.h"
#include "label.h"
#include "gui/history.h"
#include "gui/output_settings.h"
#include "gui/settings.h"
#include "gui/controller_settings.h"
#include "gui/wifi.h"
#include "gui/textentry.h"
#include "gui/quantity_select.h"
#include "quantity_widget.h"
#include "app_cfg.h"

#include <string.h>
#include <stdio.h>


#define TILE_SPACE 6
#define TILE_SIZE 72

#define TILE_POS(pos) ((((pos) + 1) * TILE_SPACE) + ((pos) * TILE_SIZE))
#define TILE_SPAN(ntiles) (((ntiles) * TILE_SIZE) + (((ntiles) - 1) * TILE_SPACE))

#define TILE_X(pos) (TILE_POS(pos) + 1)
#define TILE_Y(pos) TILE_POS(pos)

typedef struct {
  widget_t* quantity_widget;
  widget_t* button;
  bool enabled;
} sensor_info_t;

typedef struct {
  systime_t sample_timestamp;

  widget_t* screen;
  widget_t* stage_button;
  sensor_info_t sensors[NUM_SENSORS];
  net_state_t net_state;
  api_state_t api_state;

  widget_t* output1_button;
  widget_t* output2_button;
  widget_t* conn_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_destroy(widget_t* w);
static void home_screen_msg(msg_event_t* event);

static void click_sensor_button(button_event_t* event);
static void click_conn_button(button_event_t* event);
static void click_settings_button(button_event_t* event);
static void click_stage_button(button_event_t* event);

static void dispatch_output_status(home_screen_t* s, output_status_t* msg);
static void dispatch_temp_unit(home_screen_t* s, unit_t unit);
static void dispatch_sensor_sample(home_screen_t* s, sensor_msg_t* msg);
static void dispatch_sensor_timeout(home_screen_t* s, sensor_timeout_msg_t* msg);
static void dispatch_net_status(home_screen_t* s, net_status_t* msg);
static void dispatch_api_status(home_screen_t* s, api_status_t* msg);
static void dispatch_controller_settings(home_screen_t* s, controller_settings_t* msg);

static void set_output_settings(home_screen_t* s, output_id_t output, output_function_t function);
static void set_conn_status(home_screen_t* s);
static void place_quantity_widgets(home_screen_t* s);


static const widget_class_t home_widget_class = {
    .on_destroy = home_screen_destroy,
    .on_msg     = home_screen_msg
};

widget_t*
home_screen_create()
{
  home_screen_t* s = calloc(1, sizeof(home_screen_t));

  s->sample_timestamp = chTimeNow();

  s->screen = widget_create(NULL, &home_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
  s->stage_button = button_create(s->screen, rect, NULL, WHITE, GREEN, click_stage_button);

  rect.x = TILE_X(3);
  rect.width = TILE_SPAN(1);
  rect.height = TILE_SPAN(1);
  s->sensors[SENSOR_1].button = button_create(s->screen, rect, img_temp_med, WHITE, STEEL, click_sensor_button);

  rect.y = TILE_Y(1);
  s->sensors[SENSOR_2].button = button_create(s->screen, rect, img_temp_med, WHITE, STEEL, click_sensor_button);

  rect.x = TILE_X(0);
  rect.y = TILE_Y(2);
  s->output1_button = button_create(s->screen, rect, img_plug, WHITE, ORANGE, NULL);

  rect.x = TILE_X(1);
  s->output2_button = button_create(s->screen, rect, img_plug, WHITE, CYAN, NULL);

  rect.x = TILE_X(2);
  s->conn_button = button_create(s->screen, rect, img_signal, RED, STEEL, click_conn_button);

  rect.x = TILE_X(3);
  s->settings_button = button_create(s->screen, rect, img_settings, WHITE, COBALT, click_settings_button);

  rect.x = 0;
  rect.width = TILE_SPAN(3);
  s->sensors[SENSOR_1].quantity_widget = quantity_widget_create(s->stage_button, rect, app_cfg_get_temp_unit());

  s->sensors[SENSOR_2].quantity_widget = quantity_widget_create(s->stage_button, rect, app_cfg_get_temp_unit());

  place_quantity_widgets(s);

  set_output_settings(s, OUTPUT_1,
      temp_control_get_output_function(OUTPUT_1));
  set_output_settings(s, OUTPUT_2,
      temp_control_get_output_function(OUTPUT_2));

  gui_msg_subscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_subscribe(MSG_SENSOR_TIMEOUT, s->screen);
  gui_msg_subscribe(MSG_OUTPUT_STATUS, s->screen);
  gui_msg_subscribe(MSG_TEMP_UNIT, s->screen);
  gui_msg_subscribe(MSG_NET_STATUS, s->screen);
  gui_msg_subscribe(MSG_API_STATUS, s->screen);
  gui_msg_subscribe(MSG_CONTROLLER_SETTINGS, s->screen);

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_unsubscribe(MSG_SENSOR_TIMEOUT, s->screen);
  gui_msg_unsubscribe(MSG_OUTPUT_STATUS, s->screen);
  gui_msg_unsubscribe(MSG_TEMP_UNIT, s->screen);
  gui_msg_unsubscribe(MSG_NET_STATUS, s->screen);
  gui_msg_unsubscribe(MSG_API_STATUS, s->screen);
  gui_msg_unsubscribe(MSG_CONTROLLER_SETTINGS, s->screen);

  free(s);
}

static void
home_screen_msg(msg_event_t* event)
{
  home_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_SENSOR_SAMPLE:
    dispatch_sensor_sample(s, event->msg_data);
    break;

  case MSG_SENSOR_TIMEOUT:
    dispatch_sensor_timeout(s, event->msg_data);
    break;

  case MSG_OUTPUT_STATUS:
    dispatch_output_status(s, event->msg_data);
    break;

  case MSG_TEMP_UNIT:
    dispatch_temp_unit(s, *((unit_t*)event->msg_data));
    break;

  case MSG_NET_STATUS:
    dispatch_net_status(s, event->msg_data);
    break;

  case MSG_API_STATUS:
    dispatch_api_status(s, event->msg_data);
    break;

  case MSG_CONTROLLER_SETTINGS:
    dispatch_controller_settings(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_sensor_sample(home_screen_t* s, sensor_msg_t* msg)
{
  /* Update the sensor button icons based on the current sample/setpoint */
  widget_t* btn = s->sensors[msg->sensor].button;
  float setpoint = temp_control_get_current_setpoint(msg->sensor);
  if (msg->sample.value > setpoint)
    button_set_icon(btn, img_temp_hi);
  else if (msg->sample.value < setpoint)
    button_set_icon(btn, img_temp_low);
  else
    button_set_icon(btn, img_temp_med);

  /* Update the quantity display widget */
  widget_t* w = s->sensors[msg->sensor].quantity_widget;
  if (msg->sample.value > 999.9)
    msg->sample.value = 999.9;
  else if (msg->sample.value < -99.9)
    msg->sample.value = -99.9;
  quantity_widget_set_value(w, msg->sample);

  /* Enable the sensor button and adjust the placement of the quantity display widgets */
  s->sensors[msg->sensor].enabled = true;
  if (s->sensors[msg->sensor].enabled) {
    widget_enable(s->sensors[msg->sensor].button, TRUE);

    if (msg->sensor == SENSOR_1)
      button_set_color(s->sensors[msg->sensor].button, AMBER);
    else
      button_set_color(s->sensors[msg->sensor].button, PURPLE);

    place_quantity_widgets(s);
  }
}

static void
dispatch_sensor_timeout(home_screen_t* s, sensor_timeout_msg_t* msg)
{
  widget_t* w = s->sensors[msg->sensor].quantity_widget;

  s->sensors[msg->sensor].enabled = false;

  if (widget_is_enabled(s->sensors[msg->sensor].button)) {
    button_set_icon(s->sensors[msg->sensor].button, img_temp_low);
    button_set_color(s->sensors[msg->sensor].button, STEEL);
    quantity_t sample = {
        .unit = UNIT_NONE,
        .value = NAN
    };
    quantity_widget_set_value(w, sample);
    place_quantity_widgets(s);
  }
}

static void
dispatch_net_status(home_screen_t* s, net_status_t* msg)
{
  s->net_state = msg->net_state;
  set_conn_status(s);
}

static void
dispatch_api_status(home_screen_t* s, api_status_t* msg)
{
  s->api_state = msg->state;
  set_conn_status(s);
}

static void
set_conn_status(home_screen_t* s)
{
  if (s->net_state == NS_CONNECTED &&
      s->api_state == AS_CONNECTED) {
      button_set_color(s->conn_button, EMERALD);
      button_set_icon_color(s->conn_button, WHITE);
  }
  else if (s->net_state == NS_CONNECTED ||
           s->api_state == AS_CONNECTED) {
    button_set_color(s->conn_button, STEEL);
    button_set_icon_color(s->conn_button, YELLOW);
  }
  else {
    button_set_color(s->conn_button, STEEL);
    button_set_icon_color(s->conn_button, RED);
  }
}

static void
place_quantity_widgets(home_screen_t* s)
{
  int i;
  rect_t rect = widget_get_rect(s->stage_button);

  sensor_info_t* active_sensors[NUM_SENSORS];
  int num_active_sensors = 0;
  for (i = 0; i < NUM_SENSORS; ++i) {
    if (s->sensors[i].enabled) {
      widget_show(s->sensors[i].quantity_widget);
      active_sensors[num_active_sensors++] = &s->sensors[i];
    }
    else
      widget_hide(s->sensors[i].quantity_widget);
  }

  if (num_active_sensors == 0) {
    widget_show(s->sensors[SENSOR_1].quantity_widget);
    active_sensors[num_active_sensors++] = &s->sensors[SENSOR_1];
  }

  for (i = 0; i < num_active_sensors; ++i) {
    rect_t wrect = widget_get_rect(active_sensors[i]->quantity_widget);

    int spacing = (rect.height - (num_active_sensors * wrect.height)) / (num_active_sensors + 1);
    wrect.y = (spacing * (i + 1)) + (wrect.height * i);

    widget_set_rect(active_sensors[i]->quantity_widget, wrect);
  }
}

static void
dispatch_controller_settings(home_screen_t* s, controller_settings_t* settings)
{
  (void)settings;

  int i;
  for (i = 0; i < NUM_OUTPUTS; ++i) {
    set_output_settings(s, i,
        temp_control_get_output_function(i));
  }
}

static void
dispatch_output_status(home_screen_t* s, output_status_t* msg)
{
  widget_t* btn;

  if (msg->output == OUTPUT_1)
    btn = s->output1_button;
  else
    btn = s->output2_button;

  if (msg->enabled)
    button_set_icon_color(btn, LIME);
  else
    button_set_icon_color(btn, WHITE);
}

static void
dispatch_temp_unit(home_screen_t* s, unit_t unit)
{
  int i;
  for (i = 0; i < NUM_SENSORS; ++i) {
    quantity_widget_set_unit(s->sensors[i].quantity_widget, unit);
  }
}

static void
set_output_settings(home_screen_t* s, output_id_t output, output_function_t function)
{
  widget_t* btn;

  if (output == OUTPUT_1)
    btn = s->output1_button;
  else
    btn = s->output2_button;

  color_t color = 0;
  switch (function) {
    case OUTPUT_FUNC_COOLING:
      color = CYAN;
      break;

    case OUTPUT_FUNC_HEATING:
      color = ORANGE;
      break;

    case OUTPUT_FUNC_NONE:
      color = STEEL;
      break;

    default:
      break;
  }

  button_set_color(btn, color);
}

static void
click_sensor_button(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  widget_t* parent = widget_get_parent(event->widget);
  home_screen_t* s = widget_get_instance_data(parent);

  sensor_id_t sensor;
  if (event->widget == s->sensors[SENSOR_1].button)
    sensor = SENSOR_1;
  else
    sensor = SENSOR_2;

  widget_t* settings_screen = controller_settings_screen_create(sensor);
  gui_push_screen(settings_screen);
}

static void
click_conn_button(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* wifi_screen = wifi_screen_create();
    gui_push_screen(wifi_screen);
  }
}

static void
click_settings_button(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* settings_screen = settings_screen_create();
    gui_push_screen(settings_screen);
  }
}

static void
click_stage_button(button_event_t* event)
{
  (void)event;
  if (event->id == EVT_BUTTON_CLICK) {
//    widget_t* history_screen = textentry_screen_create();
//    widget_t* history_screen = history_screen_create();
//    gui_push_screen(history_screen);
  }
}
