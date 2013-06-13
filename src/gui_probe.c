
#include "gui_probe.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"

#include <stdio.h>

typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
  widget_t* temp_label;
  widget_t* temp_unit_label;
  char temp_str[10];
  int setpoint;
} probe_screen_t;


static void probe_settings_screen_destroy(widget_t* w);

static void back_button_clicked(click_event_t* event);
static void up_button_clicked(click_event_t* event);
static void down_button_clicked(click_event_t* event);

static void set_setpoint(probe_screen_t* s, int setpoint);

widget_class_t probe_settings_widget_class = {
    .on_destroy = probe_settings_screen_destroy
};

widget_t*
probe_settings_screen_create()
{
  probe_screen_t* s = calloc(1, sizeof(probe_screen_t));
  s->widget = widget_create(NULL, &probe_settings_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, NULL, img_left, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 20;
  rect.width = 220;
  rect.height = -1;
  label_create(s->widget, rect, "Probe 1 Setup", font_opensans_22, WHITE);

  rect.x = 15;
  rect.y = 99;
  rect.width = 56;
  rect.height = 56;
  s->up_button = button_create(s->widget, rect, NULL, img_up, RED, up_button_clicked);

  rect.y = 169;
  s->down_button = button_create(s->widget, rect, NULL, img_down, CYAN, down_button_clicked);

  rect.x = 100;
  rect.y = 100;
  rect.width = 160;
  rect.height = 100;
  s->temp_label = label_create(s->widget, rect, s->temp_str, font_opensans_62, WHITE);
  widget_set_background(s->temp_label, BLACK, FALSE);

  rect.x = 275;
  rect.y = 120;
  rect.width = 20;
  s->temp_unit_label = label_create(s->widget, rect, "F", font_opensans_22, LIGHT_GRAY);

  set_setpoint(s, 732);

  return s->widget;
}

static void
probe_settings_screen_destroy(widget_t* w)
{
  probe_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(click_event_t* event)
{
  (void)event;

  gui_pop_screen();
}

static void
up_button_clicked(click_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(screen);
  set_setpoint(s, s->setpoint + 1);
}

static void
down_button_clicked(click_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(screen);
  set_setpoint(s, s->setpoint - 1);
}

static void
set_setpoint(probe_screen_t* s, int setpoint)
{
  if (s->setpoint != setpoint) {
    s->setpoint = setpoint;
    sprintf(s->temp_str, "%0.1f", setpoint / 10.0f);
    widget_invalidate(s->temp_label);
  }
}
