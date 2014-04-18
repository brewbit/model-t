
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

#include <string.h>
#include <stdio.h>


typedef struct {
  widget_t* screen;
  widget_t* button_list;

  unit_t temp_unit;
} settings_screen_t;


static void settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void unit_button_clicked(button_event_t* event);
static void update_button_clicked(button_event_t* event);
static void calibrate_button_clicked(button_event_t* event);
static void info_button_clicked(button_event_t* event);

static void set_unit_settings(settings_screen_t* s);


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

  s->temp_unit = app_cfg_get_temp_unit();
  set_unit_settings(s);

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
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);

    app_cfg_set_temp_unit(s->temp_unit);

    gui_pop_screen();
  }
}

static void
unit_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);

    if (s->temp_unit == UNIT_TEMP_DEG_C)
      s->temp_unit = UNIT_TEMP_DEG_F;
    else
      s->temp_unit = UNIT_TEMP_DEG_C;

    set_unit_settings(s);
  }
}

static void
update_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* update_screen = update_screen_create();

    gui_push_screen(update_screen);

    set_unit_settings(s);

  }
}

static void
info_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* info_screen = info_screen_create();

    gui_push_screen(info_screen);

    set_unit_settings(s);
  }
}

static void
calibrate_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    settings_screen_t* s = widget_get_user_data(event->widget);
    widget_t* calib_screen = calib_screen_create();

    gui_push_screen(calib_screen);

    set_unit_settings(s);
  }
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
set_unit_settings(settings_screen_t* s)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[5];

  char* subtext;
  char* text;
  const Image_t* img;

  text = "Display Units";
  if (s->temp_unit == UNIT_TEMP_DEG_F) {
    img = img_deg_f;
    subtext = "Display units in degrees Fahrenheit";
  }
  else {
    img = img_deg_c;
    subtext = "Display units in degrees Celsius";
  }
  add_button_spec(buttons, &num_buttons, unit_button_clicked, img, ORANGE,
      text, subtext, s);

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
}
