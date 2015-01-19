
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


#define MAX_TEMP_C (510)
#define MIN_TEMP_C (-50)

#define MAX_TEMP_F (950)
#define MIN_TEMP_F (-58)


typedef enum {
  SELECT_NONE,
  SELECT_1,
  SELECT_2,
  SELECT_1_2,

  NUM_OUTPUT_SELECTIONS
} output_selection_t;


typedef struct {
  widget_t* screen;
  widget_t* button_list;

  temp_controller_id_t controller;
  controller_settings_t settings;

  uint8_t output_selection;
  output_selection_t output_selection_valid[NUM_OUTPUT_SELECTIONS];
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


static const widget_class_t controller_settings_widget_class = {
    .on_destroy = controller_settings_screen_destroy,
};


widget_t*
controller_settings_screen_create(temp_controller_id_t controller)
{
  controller_settings_screen_t* s = calloc(1, sizeof(controller_settings_screen_t));

  s->screen = widget_create(NULL, &controller_settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK);

  char* title;
  temp_controller_id_t other_controller;
  if (controller == CONTROLLER_1) {
    title = "Controller 1 Setup";
    other_controller = CONTROLLER_2;
  }
  else {
    title = "Controller 2 Setup";
    other_controller = CONTROLLER_1;
  }
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  s->controller = controller;
  s->settings = *app_cfg_get_controller_settings(controller);

  /* Convert enabled flags to selection enum */
  if (s->settings.output_settings[OUTPUT_1].enabled &&
      s->settings.output_settings[OUTPUT_2].enabled)
    s->output_selection = SELECT_1_2;
  else if (s->settings.output_settings[OUTPUT_1].enabled)
    s->output_selection = SELECT_1;
  else if (s->settings.output_settings[OUTPUT_2].enabled)
    s->output_selection = SELECT_2;
  else
    s->output_selection = SELECT_NONE;

  /* Figure out which output selections are allowed by checking which outputs are currently
   * being controlled by the other controller.
   */
  const controller_settings_t* other_controller_settings = app_cfg_get_controller_settings(other_controller);
  s->output_selection_valid[SELECT_NONE] = true;
  if (!other_controller_settings->output_settings[OUTPUT_1].enabled)
    s->output_selection_valid[SELECT_1] = true;
  if (!other_controller_settings->output_settings[OUTPUT_2].enabled)
    s->output_selection_valid[SELECT_2] = true;
  if (!other_controller_settings->output_settings[OUTPUT_1].enabled &&
      !other_controller_settings->output_settings[OUTPUT_2].enabled)
    s->output_selection_valid[SELECT_1_2] = true;

  /* If the current output selection is not valid, set it to none */
  if (!s->output_selection_valid[s->output_selection])
    s->output_selection = SELECT_NONE;

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
  add_button_spec(buttons, &num_buttons, setpoint_type_button_clicked, img_hysteresis, VIOLET,
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

      add_button_spec(buttons, &num_buttons, static_setpoint_button_clicked, img_temp_hi_small, TEAL,
          "Static Setpoint", setpoint_subtext, s);
      break;
    }

    case SP_TEMP_PROFILE:
    {
      const temp_profile_t* tp = &s->settings.temp_profile;
      if (strlen(tp->name) > 0)
        snprintf(setpoint_subtext, 128, "Selected profile: '%s'", tp->name);
      else
        snprintf(setpoint_subtext, 128, "Selected profile: No profiles uploaded from the web!");

      add_button_spec(buttons, &num_buttons, temp_profile_button_clicked, img_temp_hi_small, INDIGO,
          "Temp Profile", setpoint_subtext, s);
      break;
    }

    default:
      break;
  }

  switch (s->output_selection) {
    case SELECT_1:
      subtext = "Use Output #1";
      break;

    case SELECT_2:
      subtext = "Use Output #2";
      break;

    case SELECT_1_2:
      subtext = "Use Output #1 & #2";
      break;

    default:
    case SELECT_NONE:
      subtext = "No output";
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

    /* Find the next valid output selection */
    do {
      if (++s->output_selection >= NUM_OUTPUT_SELECTIONS)
        s->output_selection = 0;
    } while (!s->output_selection_valid[s->output_selection]);

    /* Set output enabled flags accordingly */
    switch (s->output_selection) {
      case SELECT_1:
        s->settings.output_settings[OUTPUT_1].enabled = true;
        s->settings.output_settings[OUTPUT_2].enabled = false;
        break;

      case SELECT_2:
        s->settings.output_settings[OUTPUT_1].enabled = false;
        s->settings.output_settings[OUTPUT_2].enabled = true;
        break;

      case SELECT_1_2:
        s->settings.output_settings[OUTPUT_1].enabled = true;
        s->settings.output_settings[OUTPUT_2].enabled = true;
        break;

      default:
      case SELECT_NONE:
        s->settings.output_settings[OUTPUT_1].enabled = false;
        s->settings.output_settings[OUTPUT_2].enabled = false;
        break;
    }

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
}

static void
static_setpoint_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  controller_settings_screen_t* s = widget_get_user_data(event->widget);
  unit_t temp_units = app_cfg_get_temp_unit();
  quantity_t setpoint;
  float min;
  float max;

  char* title;
  if (s->controller == CONTROLLER_1)
    title = "Controller 1 Setpoint";
  else
    title = "Controller 2 Setpoint";

  float velocity_steps[] = {
      0.1f, 0.5f, 1.0f
  };

  if (temp_units == UNIT_TEMP_DEG_F) {
    min = MIN_TEMP_F;
    max = MAX_TEMP_F;
  }
  else{
    min = MIN_TEMP_C;
    max = MAX_TEMP_C;
  }
  setpoint = quantity_convert(s->settings.static_setpoint, temp_units);

  widget_t* static_setpoint_screen = quantity_select_screen_create(
      title, setpoint, min, max,
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

    app_cfg_set_controller_settings(s->controller, SS_DEVICE, &s->settings);

    gui_pop_screen();
  }
}
