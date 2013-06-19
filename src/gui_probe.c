
#include "gui_probe.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"

#define SETPOINT_STEPS_PER_VELOCITY 30


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
  widget_t* temp_label;
  widget_t* temp_unit_label;
  char temp_str[10];
  probe_id_t probe;
  probe_settings_t settings;
  uint8_t setpoint_delta;
  uint8_t setpoint_steps;
} probe_screen_t;


static void probe_settings_screen_destroy(widget_t* w);

static void back_button_clicked(button_event_t* event);
static void up_button_clicked(button_event_t* event);
static void down_button_clicked(button_event_t* event);
static void up_or_down_released(button_event_t* event);
static void adjust_setpoint_velocity(probe_screen_t* s);
static void set_setpoint(probe_screen_t* s, int setpoint);


widget_class_t probe_settings_widget_class = {
    .on_destroy = probe_settings_screen_destroy
};

widget_t*
probe_settings_screen_create(probe_id_t probe)
{
  probe_screen_t* s = calloc(1, sizeof(probe_screen_t));
  s->widget = widget_create(NULL, &probe_settings_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, NULL, img_left, BLACK, NULL, NULL, NULL, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (probe == PROBE_1) ? "Probe 1 Setup" : "Probe 2 Setup";
  label_create(s->widget, rect, title, font_opensans_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 56;
  rect.height = 56;
  s->up_button = button_create(s->widget, rect, NULL, img_up, RED, up_button_clicked, up_button_clicked, up_or_down_released, NULL);

  rect.y = 165;
  s->down_button = button_create(s->widget, rect, NULL, img_down, CYAN, down_button_clicked, down_button_clicked, up_or_down_released, NULL);

  rect.x = 100;
  rect.y = 130;
  rect.width = 160;
  rect.height = 100;
  s->temp_label = label_create(s->widget, rect, s->temp_str, font_opensans_62, WHITE, 1);
  widget_set_background(s->temp_label, BLACK, FALSE);

  rect.x = 275;
  rect.y = 130;
  rect.width = 20;
  s->temp_unit_label = label_create(s->widget, rect, "F", font_opensans_22, LIGHT_GRAY, 1);

  s->probe = probe;
  s->settings = temp_control_get_probe_settings(probe);
  set_setpoint(s, s->settings.setpoint);

  s->setpoint_delta = 1;
  s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;

  return s->widget;
}

static void
probe_settings_screen_destroy(widget_t* w)
{
  probe_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  widget_t* w = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(w);

  probe_settings_msg_t msg = {
      .probe = s->probe,
      .settings = s->settings
  };
  msg_broadcast(MSG_PROBE_SETTINGS, &msg);

  gui_pop_screen();
}

static void
up_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(screen);
  set_setpoint(s, s->settings.setpoint + s->setpoint_delta);
  adjust_setpoint_velocity(s);
}

static void
down_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(screen);
  set_setpoint(s, s->settings.setpoint - s->setpoint_delta);
  adjust_setpoint_velocity(s);
}

static void
up_or_down_released(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  probe_screen_t* s = widget_get_instance_data(screen);
  s->setpoint_delta = 1;
  s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;
}

static void
adjust_setpoint_velocity(probe_screen_t* s)
{
  if (s->setpoint_steps == 0) {
    chprintf(stdout, "steps == 0\r\n");
    return;
  }

  if (--s->setpoint_steps <= 0 && s->setpoint_delta < 50) {
    if (s->setpoint_delta == 1)
      s->setpoint_delta = 10;
    else if (s->setpoint_delta == 10)
      s->setpoint_delta = 50;

    s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;
    s->settings.setpoint = ((s->settings.setpoint / s->setpoint_delta) + 1) * s->setpoint_delta;
  }
}

static void
set_setpoint(probe_screen_t* s, int setpoint)
{
  s->settings.setpoint = setpoint;
  sprintf(s->temp_str, "%0.1f", setpoint / 10.0f);
  widget_invalidate(s->temp_label);
}
