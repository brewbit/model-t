
#include "gui_settings.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"
#include "settings.h"


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* unit_button;

  device_settings_t settings;
} settings_screen_t;


static void settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void unit_button_clicked(button_event_t* event);

static void set_unit(settings_screen_t* s, temperature_unit_t unit);


widget_class_t settings_widget_class = {
    .on_destroy = settings_screen_destroy
};

widget_t*
settings_screen_create()
{
  settings_screen_t* s = calloc(1, sizeof(settings_screen_t));
  s->widget = widget_create(NULL, &settings_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, img_left, BLACK, NULL, NULL, NULL, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Settings", font_opensans_22, WHITE, 1);

  rect.x = 48;
  rect.y = 86;
  rect.width = 56;
  rect.height = 56;
  s->unit_button = button_create(s->widget, rect, img_deg_f, ORANGE, NULL, NULL, NULL, unit_button_clicked);

  rect.x += 84;
  button_create(s->widget, rect, NULL, CYAN, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, NULL, STEEL, NULL, NULL, NULL, NULL);

  rect.x = 48;
  rect.y += 84;
  button_create(s->widget, rect, NULL, OLIVE, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, NULL, PURPLE, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, NULL, GREEN, NULL, NULL, NULL, NULL);

  s->settings = *settings_get();
  set_unit(s, s->settings.unit);

  return s->widget;
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
  widget_t* w = widget_get_parent(event->widget);
  settings_screen_t* s = widget_get_instance_data(w);

  settings_set(&s->settings);

  gui_pop_screen();
}

static void
unit_button_clicked(button_event_t* event)
{
  widget_t* w = widget_get_parent(event->widget);
  settings_screen_t* s = widget_get_instance_data(w);

  if (s->settings.unit == TEMP_C)
    set_unit(s, TEMP_F);
  else
    set_unit(s, TEMP_C);
}

static void
set_unit(settings_screen_t* s, temperature_unit_t unit)
{
  s->settings.unit = unit;
  if (unit == TEMP_F) {
    button_set_icon(s->unit_button, img_deg_f);
  }
  else {
    button_set_icon(s->unit_button, img_deg_c);
  }
}
