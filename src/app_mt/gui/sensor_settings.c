
#include "gui/sensor_settings.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui/quantity_select.h"
#include "gui/button_list.h"

#include <string.h>
#include <stdio.h>

typedef struct {
  widget_t* screen;
  widget_t* button_list;

  sensor_id_t sensor;
  sensor_settings_t settings;
} sensor_settings_screen_t;


static void sensor_settings_screen_destroy(widget_t* w);
static void set_sensor_settings(sensor_settings_screen_t* s, setpoint_type_t setpoint_type, quantity_t static_setpoint, uint32_t temp_profile_id);
static void temp_profile_button_clicked(button_event_t* event);
static void setpooint_type_button_clicked(button_event_t* event);
static void static_setpoint_button_clicked(button_event_t* event);
static void update_static_setpoint(quantity_t delay, void* user_data);
static void back_button_clicked(button_event_t* event);


widget_class_t sensor_settings_widget_class = {
    .on_destroy = sensor_settings_screen_destroy,
};


widget_t*
sensor_settings_screen_create(sensor_id_t sensor)
{
  sensor_settings_screen_t* s = calloc(1, sizeof(sensor_settings_screen_t));

  s->screen = widget_create(NULL, &sensor_settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  char* title = (sensor == SENSOR_1) ? "Sensor 1 Setup" : "Sensor 2 Setup";
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  s->sensor = sensor;
  s->settings = *app_cfg_get_sensor_settings(sensor);

  set_sensor_settings(s, s->settings.setpoint_type, s->settings.static_setpoint, s->settings.temp_profile_id);

  return s->screen;
}

static void
sensor_settings_screen_destroy(widget_t* w)
{
  sensor_settings_screen_t* s = widget_get_instance_data(w);
  free(s);
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

  char* subtext;
  switch (setpoint_type) {
    case SP_STATIC:
      subtext = "Static Setpoint - Temp is held at fixed value";
      break;

    case SP_TEMP_PROFILE:
      subtext = "Temp Profile - Temp follows customized profile";
      break;

    default:
      subtext = "Invalid setpoint type!";
      break;
  }
  add_button_spec(buttons, &num_buttons, setpooint_type_button_clicked, img_snowflake, CYAN,
      "Setpoint Type", subtext, s);

  subtext = malloc(128);
  switch (setpoint_type) {
    case SP_STATIC:
      snprintf(subtext, 128, "Setpoint value: %d.%d %c",
          (int)(s->settings.static_setpoint.value),
          ((int)(s->settings.static_setpoint.value * 10.0f)) % 10, 'F');
      add_button_spec(buttons, &num_buttons, static_setpoint_button_clicked, img_snowflake, CYAN,
          "Static Setpoint", subtext, s);
      break;

    case SP_TEMP_PROFILE:
    {
      const temp_profile_t* tp = app_cfg_get_temp_profile(s->settings.temp_profile_id);
      if (tp != NULL)
        snprintf(subtext, 128, "Selected profile: '%s'", tp->name);
      else
        snprintf(subtext, 128, "Selected profile: id=%u", (unsigned int)s->settings.temp_profile_id);

      add_button_spec(buttons, &num_buttons, temp_profile_button_clicked, img_snowflake, CYAN,
          "Temp Profile", subtext, s);
      break;
    }

    default:
      break;
  }

  button_list_set_buttons(s->button_list, buttons, num_buttons);
  free(subtext);
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
  (void)event;
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
  if (s->sensor == SENSOR_1)
    title = "Sensor 1 Setpoint";
  else
    title = "Sensor 2 Setpoint";

  float velocity_steps[] = {
      0.1f, 0.5f, 1.0f
  };
  widget_t* static_setpoint_screen = quantity_select_screen_create(
      title, s->settings.static_setpoint, velocity_steps, 3, update_static_setpoint, s);
  gui_push_screen(static_setpoint_screen);
}

static void
update_static_setpoint(quantity_t setpoint, void* user_data)
{
  sensor_settings_screen_t* s = user_data;
  set_sensor_settings(s, s->settings.setpoint_type, setpoint, s->settings.temp_profile_id);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    sensor_settings_screen_t* s = widget_get_user_data(event->widget);

    app_cfg_set_sensor_settings(s->sensor, &s->settings);

    gui_pop_screen();
  }
}
