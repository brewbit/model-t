
#include "gui_settings.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"
#include "app_cfg.h"
#include "gui_update.h"
#include "gui_calib.h"
#include "gui_activation.h"

#include <string.h>


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* unit_button;

  unit_t temp_unit;
} settings_screen_t;


static void settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void unit_button_clicked(button_event_t* event);
static void update_button_clicked(button_event_t* event);
static void calibrate_button_clicked(button_event_t* event);
static void activation_button_clicked(button_event_t* event);

static void set_unit(settings_screen_t* s, unit_t unit);


widget_class_t settings_widget_class = {
    .on_destroy = settings_screen_destroy
};

widget_t*
settings_screen_create()
{
  settings_screen_t* s = chHeapAlloc(NULL, sizeof(settings_screen_t));
  memset(s, 0, sizeof(settings_screen_t));

  s->widget = widget_create(NULL, &settings_widget_class, s, display_rect);

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
  label_create(s->widget, rect, "Settings", font_opensans_regular_22, WHITE, 1);

  rect.x = 48;
  rect.y = 86;
  rect.width = 56;
  rect.height = 56;
  s->unit_button = button_create(s->widget, rect, img_deg_f, WHITE, ORANGE, unit_button_clicked);

  rect.x += 84;
  button_create(s->widget, rect, img_hand, WHITE, STEEL, calibrate_button_clicked);

  rect.x += 84;
  button_create(s->widget, rect, img_update, WHITE, CYAN, update_button_clicked);

  rect.x = 48;
  rect.y = 150;
  rect.width = 56 ;
  rect.width = 56 ;
  button_create(s->widget, rect, img_activation, WHITE, EMERALD, activation_button_clicked);

  s->temp_unit = app_cfg_get_temp_unit();
  set_unit(s, s->temp_unit);

  return s->widget;
}

static void
settings_screen_destroy(widget_t* w)
{
  settings_screen_t* s = widget_get_instance_data(w);
  chHeapFree(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    settings_screen_t* s = widget_get_instance_data(w);

    app_cfg_set_temp_unit(s->temp_unit);

    gui_pop_screen();
  }
}

static void
unit_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    settings_screen_t* s = widget_get_instance_data(w);

    if (s->temp_unit == UNIT_TEMP_DEG_C)
      set_unit(s, UNIT_TEMP_DEG_F);
    else
      set_unit(s, UNIT_TEMP_DEG_C);
  }
}

static void
activation_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* activation_screen = activation_screen_create();
    gui_push_screen(activation_screen);
  }
}

static void
update_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
//  widget_t* update_screen = update_screen_create();
//  gui_push_screen(update_screen);
  }
}

static void
calibrate_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
//  widget_t* calib_screen = calib_screen_create();
//  gui_push_screen(calib_screen);
  }
}

static void
set_unit(settings_screen_t* s, unit_t unit)
{
  s->temp_unit = unit;
  if (unit == UNIT_TEMP_DEG_F) {
    button_set_icon(s->unit_button, img_deg_f);
  }
  else {
    button_set_icon(s->unit_button, img_deg_c);
  }
}
