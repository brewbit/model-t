
#include "gui_sensor_settings.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui_quantity_select.h"
#include "gui_button_list.h"

#include <string.h>

typedef struct {
  widget_t* screen;
  widget_t* button_list;

  sensor_id_t sensor;
  sensor_settings_t settings;
} sensor_settings_screen_t;


static void sensor_settings_screen_destroy(widget_t* w);
static void sensor_settings_screen_msg(msg_event_t* event);
static void dispatch_sensor_settings(sensor_settings_screen_t* s, sensor_settings_msg_t* msg);
static void set_sensor_settings(sensor_settings_screen_t* s, setpoint_type_t setpoint_type, quantity_t static_setpoint, uint32_t temp_profile_id);
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

  s->screen = widget_create(NULL, &sensor_settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  char* title = (sensor == SENSOR_1) ? "Sensor 1 Setup" : "Sensor 2 Setup";
  s->button_list = button_list_screen_create(s->screen, title, NULL, 0);

  s->sensor = sensor;
  s->settings = *app_cfg_get_sensor_settings(sensor);

  gui_msg_subscribe(MSG_SENSOR_SETTINGS, s->screen);
  set_sensor_settings(s, s->settings.setpoint_type, s->settings.static_setpoint, s->settings.temp_profile_id);

  return s->screen;
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
add_button_spec(
    button_spec_t* buttons,
    uint32_t* num_buttons,
    button_event_handler_t btn_event_handler,
    const Image_t* img,
    color_t color,
    const char* text,
    const char* subtext,
    void* user_data)
{
  buttons[*num_buttons].btn_event_handler = btn_event_handler;
  buttons[*num_buttons].img = img;
  buttons[*num_buttons].color = color;
  buttons[*num_buttons].text = text;
  buttons[*num_buttons].subtext = subtext;
  buttons[*num_buttons].user_data = user_data;
  (*num_buttons)++;
}

static void
set_sensor_settings(sensor_settings_screen_t* s, setpoint_type_t setpoint_type, quantity_t static_setpoint, uint32_t temp_profile_id)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[4];

  s->settings.setpoint_type = setpoint_type;
  s->settings.static_setpoint = static_setpoint;
  s->settings.temp_profile_id = temp_profile_id;

  add_button_spec(buttons, &num_buttons, setpooint_type_button_clicked, img_snowflake, CYAN, "Setpoint Type", "blah", s);
  switch (setpoint_type) {
    case SP_STATIC:
      add_button_spec(buttons, &num_buttons, static_setpoint_button_clicked, img_snowflake, CYAN, "Static Setpoint", "blah", s);
      break;

    case SP_TEMP_PROFILE:
      add_button_spec(buttons, &num_buttons, temp_profile_button_clicked, img_snowflake, CYAN, "Temp Profile", "blah", s);
      break;

    default:
      break;
  }

  button_list_set_buttons(s->button_list, buttons, num_buttons);
}

static void
setpooint_type_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    sensor_settings_screen_t* s = widget_get_user_data(event->widget);

    setpoint_type_t new_sp_type = SP_STATIC;
    switch (s->settings.setpoint_type) {
      case SP_STATIC:
        new_sp_type = SP_TEMP_PROFILE;
        break;

      case SP_TEMP_PROFILE:
        new_sp_type = SP_STATIC;
        break;

      default:
        break;
    }

    set_sensor_settings(s, new_sp_type, s->settings.static_setpoint, s->settings.temp_profile_id);
  }
}

static void
temp_profile_button_clicked(button_event_t* event)
{
//  if (event->id == EVT_BUTTON_CLICK) {
//  sensor_settings_screen_t* s = widget_get_user_data(event->widget);
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

  sensor_settings_screen_t* s = widget_get_user_data(event->widget);

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

