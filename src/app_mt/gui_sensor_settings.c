
#include "gui_sensor_settings.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui_quantity_select.h"

#include <string.h>

typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* setpooint_type_button;
  widget_t* static_setpoint_button;
  widget_t* temp_profile_button;

  sensor_id_t sensor;
  sensor_settings_t settings;
} sensor_settings_screen_t;


static void sensor_settings_screen_destroy(widget_t* w);
static void sensor_settings_screen_msg(msg_event_t* event);
static void dispatch_sensor_settings(sensor_settings_screen_t* s, sensor_settings_msg_t* msg);
static void set_sensor_settings(sensor_settings_screen_t* s, setpoint_type_t setpoint_type, quantity_t static_setpoint, uint32_t temp_profile_id);
static void back_button_clicked(button_event_t* event);
static void temp_profile_button_clicked(button_event_t* event);
static void setpooint_type_button_clicked(button_event_t* event);
static void static_setpoint_button_clicked(button_event_t* event);
static void update_static_setpoint(quantity_t delay, void* user_data);


widget_class_t sensor_settings_widget_class = {
    .on_destroy = sensor_settings_screen_destroy,
    .on_msg     = sensor_settings_screen_msg
};


widget_t*
sensor_settings_screen_create(sensor_id_t sensor)
{
  sensor_settings_screen_t* s = chHeapAlloc(NULL, sizeof(sensor_settings_screen_t));
  memset(s, 0, sizeof(sensor_settings_screen_t));

  s->widget = widget_create(NULL, &sensor_settings_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (sensor == SENSOR_1) ? "Sensor 1 Setup" : "Sensor 2 Setup";
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 48;
  rect.y = 86;
  rect.width = 56;
  rect.height = 56;
  s->setpooint_type_button = button_create(s->widget, rect, img_flame, WHITE, ORANGE, setpooint_type_button_clicked);

  rect.x += 84;
  s->static_setpoint_button = button_create(s->widget, rect, img_temp_38, WHITE, AMBER, static_setpoint_button_clicked);

  s->temp_profile_button = button_create(s->widget, rect, img_stopwatch, WHITE, GREEN, temp_profile_button_clicked);

  s->sensor = sensor;
  s->settings = *app_cfg_get_sensor_settings(sensor);

  gui_msg_subscribe(MSG_SENSOR_SETTINGS, s->widget);
  set_sensor_settings(s, s->settings.setpoint_type, s->settings.static_setpoint, s->settings.temp_profile_id);

  return s->widget;
}

static void
sensor_settings_screen_destroy(widget_t* w)
{
  sensor_settings_screen_t* s = widget_get_instance_data(w);
  gui_msg_unsubscribe(MSG_SENSOR_SETTINGS, w);
  chHeapFree(s);
}

static void
sensor_settings_screen_msg(msg_event_t* event)
{
  sensor_settings_screen_t* s = widget_get_instance_data(event->widget);

  if (event->msg_id == MSG_SENSOR_SETTINGS)
    dispatch_sensor_settings(s, event->msg_data);
}

static void
dispatch_sensor_settings(sensor_settings_screen_t* s, sensor_settings_msg_t* msg)
{
  if (msg->sensor == s->sensor)
    set_sensor_settings(s, msg->settings.setpoint_type, msg->settings.static_setpoint, msg->settings.temp_profile_id);
}

static void
set_sensor_settings(sensor_settings_screen_t* s, setpoint_type_t setpoint_type, quantity_t static_setpoint, uint32_t temp_profile_id)
{
  color_t color = 0;
  const Image_t* img = img_snowflake;
  switch (setpoint_type) {
    case SP_STATIC:
      color = CYAN;
      img = img_snowflake;
      widget_show(s->static_setpoint_button);
      widget_hide(s->temp_profile_button);
      break;

    case SP_TEMP_PROFILE:
      color = ORANGE;
      img = img_flame;
      widget_hide(s->static_setpoint_button);
      widget_show(s->temp_profile_button);
      break;

    default:
      break;
  }

  widget_set_background(s->setpooint_type_button, color, FALSE);
  button_set_icon(s->setpooint_type_button, img);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
setpooint_type_button_clicked(button_event_t* event)
{
//  if (event->id == EVT_BUTTON_CLICK) {
//    widget_t* screen = widget_get_parent(event->widget);
//    sensor_settings_screen_t* s = widget_get_instance_data(screen);
//
//    widget_t* setpooint_type_screen = setpooint_type_screen_create(s->sensor);
//    gui_push_screen(setpooint_type_screen);
//  }
}

static void
temp_profile_button_clicked(button_event_t* event)
{
//  if (event->id == EVT_BUTTON_CLICK) {
//    widget_t* screen = widget_get_parent(event->widget);
//    sensor_settings_screen_t* s = widget_get_instance_data(screen);
//
//    widget_t* temp_profile_screen = temp_profile_screen_create(s->sensor);
//    gui_push_screen(temp_profile_screen);
//  }
}

static void
static_setpoint_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  widget_t* screen = widget_get_parent(event->widget);
  sensor_settings_screen_t* s = widget_get_instance_data(screen);

  char* title;
  if (s->sensor == SENSOR_1) {
    title = "Sensor 1 Setpoint";
  }
  else {
    title = "Sensor 2 Setpoint";
  }

  float velocity_steps[] = {
      0.1f, 0.5f, 1.0f
  };
  widget_t* static_setpoint_screen = quantity_select_screen_create(
      title, s->settings.static_setpoint, velocity_steps, 1, update_static_setpoint, s);
  gui_push_screen(static_setpoint_screen);
}

static void
update_static_setpoint(quantity_t setpoint, void* user_data)
{
  sensor_settings_screen_t* s = user_data;
  s->settings.static_setpoint = setpoint;
  app_cfg_set_sensor_settings(s->sensor, &s->settings);
}

