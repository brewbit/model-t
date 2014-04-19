
#include "gui/controller_settings.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui/quantity_select.h"
#include "gui/button_list.h"
#include "gui/output_settings.h"

#include <string.h>
#include <stdio.h>


#define MAX_TEMP_C (110)
#define MIN_TEMP_C (-10)

#define MAX_TEMP_F (212)
#define MIN_TEMP_F (-10)


typedef struct {
  widget_t* screen;
  widget_t* button_list;

  sensor_id_t sensor;
  controller_settings_t settings;
} controller_settings_screen_t;


static void controller_settings_screen_destroy(widget_t* w);
static void set_controller_settings(controller_settings_screen_t* s);
static void output_selection_button_clicked(button_event_t* event);
static void temp_profile_button_clicked(button_event_t* event);
static void setpoint_type_button_clicked(button_event_t* event);
static void static_setpoint_button_clicked(button_event_t* event);
static void output_settings_button_clicked(button_event_t* event);
static void update_static_setpoint(quantity_t delay, void* user_data);
static void back_button_clicked(button_event_t* event);


widget_class_t controller_settings_widget_class = {
    .on_destroy = controller_settings_screen_destroy,
};


widget_t*
controller_settings_screen_create(sensor_id_t sensor)
{
  controller_settings_screen_t* s = calloc(1, sizeof(controller_settings_screen_t));

  s->screen = widget_create(NULL, &controller_settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  char* title = (sensor == SENSOR_1) ? "Controller 1 Setup" : "Controller 2 Setup";
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  s->sensor = sensor;
  s->settings = *app_cfg_get_controller_settings(sensor);

  set_controller_settings(s);

  return s->screen;
}

static void
controller_settings_screen_destroy(widget_t* w)
{
  controller_settings_screen_t* s = widget_get_instance_data(w);
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
set_controller_settings(controller_settings_screen_t* s)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[16];

  char* subtext;
  char* setpoint_subtext;

  switch (s->settings.setpoint_type) {
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
  add_button_spec(buttons, &num_buttons, setpoint_type_button_clicked, img_snowflake, CYAN,
      "Setpoint Type", subtext, s);

  setpoint_subtext = malloc(128);
  switch (s->settings.setpoint_type) {
    case SP_STATIC:
    {
      quantity_t setpoint = quantity_convert(s->settings.static_setpoint, app_cfg_get_temp_unit());

      if (setpoint.unit == UNIT_TEMP_DEG_F)
        subtext = "F";
      else
        subtext = "C";

      snprintf(setpoint_subtext, 128, "Setpoint value: %d.%d %s",
          (int)(setpoint.value),
          ((int)(fabs(setpoint.value) * 10.0f)) % 10, subtext);

      add_button_spec(buttons, &num_buttons, static_setpoint_button_clicked, img_snowflake, CYAN,
          "Static Setpoint", setpoint_subtext, s);
      break;
    }

    case SP_TEMP_PROFILE:
    {
      const temp_profile_t* tp = app_cfg_get_temp_profile(s->settings.temp_profile_id);
      if (tp != NULL)
        snprintf(setpoint_subtext, 128, "Selected profile: '%s'", tp->name);
      else
        snprintf(setpoint_subtext, 128, "Selected profile: id=%u", (unsigned int)s->settings.temp_profile_id);

      add_button_spec(buttons, &num_buttons, temp_profile_button_clicked, img_snowflake, CYAN,
          "Temp Profile", setpoint_subtext, s);
      break;
    }

    default:
      break;
  }

  const controller_settings_t* other_sensor;

  switch (s->sensor) {
    case SENSOR_1:
      other_sensor = app_cfg_get_controller_settings(SENSOR_2);
      break;

    case SENSOR_2:
      other_sensor = app_cfg_get_controller_settings(SENSOR_1);
      break;

    default:
      s->settings.output_selection = SELECT_NONE;
      other_sensor = app_cfg_get_controller_settings(s->sensor);
      break;
  }

  switch (s->settings.output_selection) {
    case SELECT_1:
      if (other_sensor->output_selection != SELECT_1 &&
          other_sensor->output_selection != SELECT_1_2) {
        subtext = "Use Output #1";
        s->settings.output_settings[OUTPUT_1].enabled = true;
        s->settings.output_settings[OUTPUT_2].enabled = false;

        s->settings.output_selection = SELECT_1;
        break;
      }
      /* Intentional fall-through */

    case SELECT_2:
      if (other_sensor->output_selection != SELECT_2 &&
          other_sensor->output_selection != SELECT_1_2) {
        subtext = "Use Output #2";
        s->settings.output_settings[OUTPUT_1].enabled = false;
        s->settings.output_settings[OUTPUT_2].enabled = true;

        s->settings.output_selection = SELECT_2;
        break;
      }
      /* Intentional fall-through */

    case SELECT_1_2:
      if (other_sensor->output_selection != SELECT_1 &&
          other_sensor->output_selection != SELECT_2 &&
          other_sensor->output_selection != SELECT_1_2) {
        subtext = "Use Output #1 & #2";
        s->settings.output_settings[OUTPUT_1].enabled = true;
        s->settings.output_settings[OUTPUT_2].enabled = true;

        s->settings.output_selection = SELECT_1_2;
        break;
      }
      /* Intentional fall-through */

    default:
    case SELECT_NONE:
      s->settings.output_settings[OUTPUT_1].enabled = false;
      s->settings.output_settings[OUTPUT_2].enabled = false;
      subtext = "No output";
      s->settings.output_selection = SELECT_NONE;
      break;
  }

  add_button_spec(buttons, &num_buttons, output_selection_button_clicked, img_plug, CYAN,
      "Output Selection", subtext, s);

  if (s->settings.output_settings[OUTPUT_1].enabled)
    add_button_spec(buttons, &num_buttons, output_settings_button_clicked, img_plug, CYAN,
        "Output 1 Settings", "Set settings for output 1", &s->settings.output_settings[OUTPUT_1]);

  if (s->settings.output_settings[OUTPUT_2].enabled)
    add_button_spec(buttons, &num_buttons, output_settings_button_clicked, img_plug, CYAN,
        "Output 2 Settings", "Set settings for output 2", &s->settings.output_settings[OUTPUT_2]);

  button_list_set_buttons(s->button_list, buttons, num_buttons);
  free(setpoint_subtext);
}

static void
output_selection_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    controller_settings_screen_t* s = widget_get_user_data(event->widget);

    s->settings.output_selection += 1;
    if (s->settings.output_selection == NUM_OUTPUT_SELECTIONS)
      s->settings.output_selection = 0;

    set_controller_settings(s);
  }
}

static void
setpoint_type_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    controller_settings_screen_t* s = widget_get_user_data(event->widget);

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

    s->settings.setpoint_type = new_sp_type;
    set_controller_settings(s);
  }
}

static void
temp_profile_button_clicked(button_event_t* event)
{
  (void)event;
//  if (event->id == EVT_BUTTON_CLICK) {
//  controller_settings_screen_t* s = widget_get_user_data(event->widget);
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

  controller_settings_screen_t* s = widget_get_user_data(event->widget);

  char* title;
  if (s->sensor == SENSOR_1)
    title = "Controller 1 Setpoint";
  else
    title = "Controller 2 Setpoint";

  float velocity_steps[] = {
      0.1f, 0.5f, 1.0f
  };
  widget_t* static_setpoint_screen = quantity_select_screen_create(
      title, s->settings.static_setpoint, MIN_TEMP_F, MAX_TEMP_F,
      velocity_steps, 3, update_static_setpoint, s);
  gui_push_screen(static_setpoint_screen);
}

static void
output_settings_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  output_settings_t* output_settings = widget_get_user_data(event->widget);

  widget_t* settings_screen = output_settings_screen_create(output_settings);
  gui_push_screen(settings_screen);
}

static void
update_static_setpoint(quantity_t setpoint, void* user_data)
{
  controller_settings_screen_t* s = user_data;

  s->settings.static_setpoint = quantity_convert(setpoint, UNIT_TEMP_DEG_F);
  set_controller_settings(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    controller_settings_screen_t* s = widget_get_user_data(event->widget);

    app_cfg_set_controller_settings(s->sensor, &s->settings);

    gui_pop_screen();
  }
}
