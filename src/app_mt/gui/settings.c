
#include "gui/settings.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "gui.h"
#include "app_cfg.h"
#include "gui/update.h"
#include "gui/calib.h"
#include "ota_update.h"
#include "gui/info.h"
#include "gui/button_list.h"
#include "quantity_select.h"

#include <string.h>
#include <stdio.h>


#define MIN_HYSTERESIS 0.0
#define MAX_HYSTERESIS 10.0


typedef struct {
  widget_t* screen;
  widget_t* button_list;
} settings_screen_t;


static void settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void unit_button_clicked(button_event_t* event);
static void update_button_clicked(button_event_t* event);
static void calibrate_button_clicked(button_event_t* event);
static void info_button_clicked(button_event_t* event);
static void control_mode_button_clicked(button_event_t* event);
static void rebuild_settings_screen(settings_screen_t* s);
static void hysteresis_button_clicked(button_event_t* event);
static void update_hysteresis(quantity_t hysteresis, void* user_data);


widget_class_t settings_widget_class = {
    .on_destroy = settings_screen_destroy
};

widget_t*
settings_screen_create()
{
  settings_screen_t* s = calloc(1, sizeof(settings_screen_t));

  s->screen = widget_create(NULL, &settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  char* title = "Model-T Settings";
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  rebuild_settings_screen(s);

  return s->screen;
}

static void
settings_screen_destroy(widget_t* w)
{
  settings_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
unit_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);

    if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_C)
      app_cfg_set_temp_unit(UNIT_TEMP_DEG_F);
    else
      app_cfg_set_temp_unit(UNIT_TEMP_DEG_C);

    rebuild_settings_screen(s);
  }
}

static void
update_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* update_screen = update_screen_create();

    gui_push_screen(update_screen);

    rebuild_settings_screen(s);

  }
}

static void
info_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* info_screen = info_screen_create();

    gui_push_screen(info_screen);

    rebuild_settings_screen(s);
  }
}

static void
calibrate_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* calib_screen = calib_screen_create();

    gui_push_screen(calib_screen);

    rebuild_settings_screen(s);
  }
}

static void
control_mode_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);

    output_ctrl_t control_mode = app_cfg_get_control_mode();
    if (control_mode == ON_OFF)
      app_cfg_set_control_mode(PID);
    else
      app_cfg_set_control_mode(ON_OFF);

    rebuild_settings_screen(s);
  }
}

static void
hysteresis_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
      return;

  settings_screen_t* s = widget_get_user_data(event->widget);

  float velocity_steps[] = {
      0.1f
  };
  quantity_t hysteresis = app_cfg_get_hysteresis();
  if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_C) {
    hysteresis.value *= (5.0f / 9.0f);
    hysteresis.unit = UNIT_TEMP_DEG_C;
  }
  widget_t* hysteresis_delay_screen = quantity_select_screen_create(
      "Hysteresis", hysteresis, MIN_HYSTERESIS, MAX_HYSTERESIS, velocity_steps, 1,
      update_hysteresis, s);
  gui_push_screen(hysteresis_delay_screen);
}

static void
update_hysteresis(quantity_t hysteresis, void* user_data)
{
  settings_screen_t* s = user_data;

  app_cfg_set_hysteresis(hysteresis);

  rebuild_settings_screen(s);
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
rebuild_settings_screen(settings_screen_t* s)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[8];
  color_t color = DARK_GRAY;

  char* subtext;
  char* hysteresis_subtext = NULL;
  char* text;
  const Image_t* img;

  text = "Display Units";
  if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_F) {
    img = img_deg_f;
    subtext = "Display units in degrees Fahrenheit";
  }
  else {
    img = img_deg_c;
    subtext = "Display units in degrees Celsius";
  }
  add_button_spec(buttons, &num_buttons, unit_button_clicked, img, ORANGE,
      text, subtext, s);

  text = "Control Mode";
  switch (app_cfg_get_control_mode()) {
    case ON_OFF:
      color = RED;
      subtext = "ON/OFF - Enable/Disable output at setpoint";
      break;
    case PID:
      color = STEEL;
      subtext = "PID - Minimize output error by adjusting control inputs";
      break;
  }
  add_button_spec(buttons, &num_buttons, control_mode_button_clicked, img_graph_signal, color,
      text, subtext, s);

  if (app_cfg_get_control_mode() == ON_OFF) {
    hysteresis_subtext = malloc(128);
    quantity_t hysteresis = app_cfg_get_hysteresis();

    if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_F)
      subtext = "F";
    else {
      /* We don't use quantity_convert() here because this is a relative temperature value */
      hysteresis.value *= (5.0f / 9.0f);
      subtext = "C";
    }

    snprintf(hysteresis_subtext, 128, "Hysteresis: %d.%d %s",
      (int)(hysteresis.value),
      ((int)(fabs(hysteresis.value) * 10.0f)) % 10,
      subtext);

    add_button_spec(buttons, &num_buttons, hysteresis_button_clicked, img_hysteresis, MAGENTA,
        "Hysteresis", hysteresis_subtext, s);
  }

  text = "Model-T Updates";
  subtext = "Check for Model-T software updates";
  add_button_spec(buttons, &num_buttons, update_button_clicked, img_update, GREEN,
      text, subtext, s);

  text = "Screen Calibration";
  subtext = "Perform 3 point screen calibration";
  add_button_spec(buttons, &num_buttons, calibrate_button_clicked, img_hand, AMBER,
      text, subtext, s);

  text = "Model-T Info";
  subtext = "Display detailed device information";
  add_button_spec(buttons, &num_buttons, info_button_clicked, img_info, CYAN,
      text, subtext, s);

  button_list_set_buttons(s->button_list, buttons, num_buttons);

  if (hysteresis_subtext != NULL)
    free(hysteresis_subtext);
}
