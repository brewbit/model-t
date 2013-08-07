
#include "gui_home.h"
#include "gfx.h"
#include "gui.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui_history.h"
#include "gui_sensor.h"
#include "gui_output.h"
#include "gui_settings.h"
#include "gui_wifi.h"
#include "temp_widget.h"
#include "chprintf.h"
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
  widget_t* temp_widget;
  widget_t* button;
} sensor_info_t;

typedef struct {
  systime_t temp_timestamp;
  char temp_unit;

  widget_t* screen;
  widget_t* stage_button;
  sensor_info_t sensors[NUM_SENSORS];

  widget_t* output1_button;
  widget_t* output2_button;
  widget_t* conn_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_destroy(widget_t* w);
static void home_screen_msg(msg_event_t* event);

static void click_sensor_button(button_event_t* event);
static void click_output_button(button_event_t* event);
static void click_conn_button(button_event_t* event);
static void click_settings_button(button_event_t* event);
static void click_stage_button(button_event_t* event);

static void dispatch_output_settings(home_screen_t* s, output_settings_msg_t* msg);
static void dispatch_sensor_sample(home_screen_t* s, sensor_msg_t* msg);
static void dispatch_sensor_timeout(home_screen_t* s, sensor_timeout_msg_t* msg);

static void set_output_settings(home_screen_t* s, output_id_t output, output_function_t function);
static void place_temp_widgets(home_screen_t* s);


static const widget_class_t home_widget_class = {
    .on_destroy = home_screen_destroy,
    .on_msg     = home_screen_msg
};

widget_t*
home_screen_create()
{
  home_screen_t* s = calloc(1, sizeof(home_screen_t));
  s->temp_timestamp = chTimeNow();
  s->temp_unit = 'F';

  s->screen = widget_create(NULL, &home_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
  s->stage_button = button_create(s->screen, rect, NULL, GREEN, NULL, NULL, NULL, click_stage_button);

  rect.x = TILE_X(3);
  rect.width = TILE_SPAN(1);
  rect.height = TILE_SPAN(1);
  s->sensors[SENSOR_1].button = button_create(s->screen, rect, img_temp_med, AMBER, NULL, NULL, NULL, click_sensor_button);
  widget_disable(s->sensors[SENSOR_1].button);

  rect.y = TILE_Y(1);
  s->sensors[SENSOR_2].button = button_create(s->screen, rect, img_temp_med, PURPLE, NULL, NULL, NULL, click_sensor_button);
  widget_disable(s->sensors[SENSOR_2].button);

  rect.x = TILE_X(0);
  rect.y = TILE_Y(2);
  s->output1_button = button_create(s->screen, rect, img_plug, ORANGE, NULL, NULL, NULL, click_output_button);

  rect.x = TILE_X(1);
  s->output2_button = button_create(s->screen, rect, img_plug, CYAN, NULL, NULL, NULL, click_output_button);

  rect.x = TILE_X(2);
  s->conn_button = button_create(s->screen, rect, img_signal, STEEL, NULL, NULL, NULL, click_conn_button);

  rect.x = TILE_X(3);
  s->settings_button = button_create(s->screen, rect, img_settings, OLIVE, NULL, NULL, NULL, click_settings_button);

  rect.x = 0;
  rect.width = TILE_SPAN(3);
  s->sensors[SENSOR_1].temp_widget = temp_widget_create(s->stage_button, rect);

  s->sensors[SENSOR_2].temp_widget = temp_widget_create(s->stage_button, rect);

  place_temp_widgets(s);

  set_output_settings(s, OUTPUT_1,
      app_cfg_get_output_settings(OUTPUT_1)->function);
  set_output_settings(s, OUTPUT_2,
      app_cfg_get_output_settings(OUTPUT_2)->function);

  gui_msg_subscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_subscribe(MSG_SENSOR_TIMEOUT, s->screen);
  gui_msg_subscribe(MSG_OUTPUT_SETTINGS, s->screen);

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_unsubscribe(MSG_SENSOR_TIMEOUT, s->screen);
  gui_msg_unsubscribe(MSG_OUTPUT_SETTINGS, s->screen);

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

  case MSG_OUTPUT_SETTINGS:
    dispatch_output_settings(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_sensor_sample(home_screen_t* s, sensor_msg_t* msg)
{
  /* Update the sensor button icons based on the current temp/setpoint */
  widget_t* btn = s->sensors[msg->sensor].button;
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(msg->sensor);
  if (msg->sample.value > sensor_settings->setpoint.value)
    button_set_icon(btn, img_temp_hi);
  else if (msg->sample.value < sensor_settings->setpoint.value)
    button_set_icon(btn, img_temp_low);
  else
    button_set_icon(btn, img_temp_med);

  /* Update the temperature display widget */
  widget_t* w = s->sensors[msg->sensor].temp_widget;
  temp_widget_set_value(w, &msg->sample);

  /* Enable the sensor button and adjust the placement of the temp display widgets */
  if (!widget_is_enabled(s->sensors[msg->sensor].button)) {
    widget_enable(s->sensors[msg->sensor].button, TRUE);
    place_temp_widgets(s);
  }
}

static void
dispatch_sensor_timeout(home_screen_t* s, sensor_timeout_msg_t* msg)
{
  widget_t* w = s->sensors[msg->sensor].temp_widget;

  if (widget_is_enabled(s->sensors[msg->sensor].button)) {
    widget_disable(s->sensors[msg->sensor].button);
    button_set_icon(s->sensors[msg->sensor].button, img_temp_med);
    quantity_t sample = {
        .unit = UNIT_NONE,
        .value = NAN
    };
    temp_widget_set_value(w, &sample);
    place_temp_widgets(s);
  }
}

static void
place_temp_widgets(home_screen_t* s)
{
  int i;
  rect_t rect = widget_get_rect(s->stage_button);

  sensor_info_t* active_sensors[NUM_SENSORS];
  int num_active_sensors = 0;
  for (i = 0; i < NUM_SENSORS; ++i) {
    if (widget_is_enabled(s->sensors[i].button)) {
      widget_show(s->sensors[i].temp_widget);
      active_sensors[num_active_sensors++] = &s->sensors[i];
    }
    else
      widget_hide(s->sensors[i].temp_widget);
  }

  if (num_active_sensors == 0) {
    widget_show(s->sensors[SENSOR_1].temp_widget);
    active_sensors[num_active_sensors++] = &s->sensors[SENSOR_1];
  }

  for (i = 0; i < num_active_sensors; ++i) {
    rect_t wrect = widget_get_rect(active_sensors[i]->temp_widget);

    int spacing = (rect.height - (num_active_sensors * wrect.height)) / (num_active_sensors + 1);
    wrect.y = (spacing * (i + 1)) + (wrect.height * i);

    widget_set_rect(active_sensors[i]->temp_widget, wrect);
  }
}

static void
dispatch_output_settings(home_screen_t* s, output_settings_msg_t* msg)
{
  set_output_settings(s, msg->output, msg->settings.function);
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

    default:
      break;
  }

  button_set_color(btn, color);
}

static void
click_sensor_button(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  home_screen_t* s = widget_get_instance_data(parent);

  sensor_id_t sensor;
  if (event->widget == s->sensors[SENSOR_1].button)
    sensor = SENSOR_1;
  else
    sensor = SENSOR_2;

  widget_t* settings_screen = sensor_settings_screen_create(sensor);
  gui_push_screen(settings_screen);
}

static void
click_output_button(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  home_screen_t* s = widget_get_instance_data(parent);

  output_id_t output;
  if (event->widget == s->output1_button)
    output = OUTPUT_1;
  else
    output = OUTPUT_2;

  widget_t* settings_screen = output_settings_screen_create(output);
  gui_push_screen(settings_screen);
}

static void
click_conn_button(button_event_t* event)
{
  (void)event;

  widget_t* wifi_screen = wifi_screen_create();
  gui_push_screen(wifi_screen);
}

static void
click_settings_button(button_event_t* event)
{
  (void)event;

  widget_t* settings_screen = settings_screen_create();
  gui_push_screen(settings_screen);
}

static void
click_stage_button(button_event_t* event)
{
  (void)event;

  widget_t* history_screen = history_screen_create();
  gui_push_screen(history_screen);
}
