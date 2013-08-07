
#include "gui_sensor.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"
#include "quantity_widget.h"
#include "app_cfg.h"

#define SETPOINT_STEPS_PER_VELOCITY 30
#define MAX_TEMP_C (110)
#define MIN_TEMP_C (-10)

#define MAX_TEMP_F (212)
#define MIN_TEMP_F (-10)

#define MAX_HUMIDITY (100)
#define MIN_HUMIDITY (0)


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
  widget_t* quantity_widget;
  sensor_id_t sensor;
  sensor_settings_t settings;
  float setpoint_delta;
  uint8_t setpoint_steps;
} sensor_screen_t;


static void sensor_settings_screen_destroy(widget_t* w);

static void back_button_clicked(button_event_t* event);
static void up_button_clicked(button_event_t* event);
static void down_button_clicked(button_event_t* event);
static void up_or_down_released(button_event_t* event);
static void adjust_setpoint_velocity(sensor_screen_t* s);
static void set_setpoint(sensor_screen_t* s, quantity_t* setpoint);


widget_class_t sensor_settings_widget_class = {
    .on_destroy = sensor_settings_screen_destroy
};

widget_t*
sensor_settings_screen_create(sensor_id_t sensor)
{
  sensor_screen_t* s = calloc(1, sizeof(sensor_screen_t));
  s->widget = widget_create(NULL, &sensor_settings_widget_class, s, display_rect);

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
  char* title = (sensor == SENSOR_1) ? "Sensor 1 Setup" : "Sensor 2 Setup";
  label_create(s->widget, rect, title, font_opensans_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 56;
  rect.height = 56;
  s->up_button = button_create(s->widget, rect, img_up, RED, up_button_clicked, up_button_clicked, up_or_down_released, NULL);

  rect.y = 165;
  s->down_button = button_create(s->widget, rect, img_down, CYAN, down_button_clicked, down_button_clicked, up_or_down_released, NULL);

  rect.x = 66;
  rect.y = 130;
  rect.width = 254;
  s->quantity_widget = quantity_widget_create(s->widget, rect);

  s->sensor = sensor;
  s->settings = *app_cfg_get_sensor_settings(sensor);
  set_setpoint(s, &s->settings.setpoint);

  s->setpoint_delta = 0.1;
  s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;

  return s->widget;
}

static void
sensor_settings_screen_destroy(widget_t* w)
{
  sensor_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  widget_t* w = widget_get_parent(event->widget);
  sensor_screen_t* s = widget_get_instance_data(w);

  app_cfg_set_sensor_settings(s->sensor, &s->settings);

  gui_pop_screen();
}

static void
up_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  sensor_screen_t* s = widget_get_instance_data(screen);
  quantity_t new_sp = {
      .unit = s->settings.setpoint.unit,
      .value = s->settings.setpoint.value + s->setpoint_delta
  };
  set_setpoint(s, &new_sp);
  adjust_setpoint_velocity(s);
}

static void
down_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  sensor_screen_t* s = widget_get_instance_data(screen);
  quantity_t new_sp = {
      .unit = s->settings.setpoint.unit,
      .value = s->settings.setpoint.value - s->setpoint_delta
  };
  set_setpoint(s, &new_sp);
  adjust_setpoint_velocity(s);
}

static void
up_or_down_released(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  sensor_screen_t* s = widget_get_instance_data(screen);
  s->setpoint_delta = 0.1;
  s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;
}

static void
adjust_setpoint_velocity(sensor_screen_t* s)
{
  if (s->setpoint_steps == 0)
    return;

  if (--s->setpoint_steps <= 0 && s->setpoint_delta < 1) {
    if (s->setpoint_delta == 0.1)
      s->setpoint_delta = 0.5;
    else if (s->setpoint_delta == 0.5)
      s->setpoint_delta = 1;

    s->setpoint_steps = SETPOINT_STEPS_PER_VELOCITY;
    s->settings.setpoint.value += s->setpoint_delta;
  }
}

static void
set_setpoint(sensor_screen_t* s, quantity_t* setpoint)
{
  if (setpoint->unit == UNIT_TEMP_DEG_F) {
    setpoint->value = LIMIT(setpoint->value, MIN_TEMP_F, MAX_TEMP_F);
  }
  if (setpoint->unit == UNIT_TEMP_DEG_C) {
    setpoint->value = LIMIT(setpoint->value, MIN_TEMP_C, MAX_TEMP_C);
  }
  else if (setpoint->unit == UNIT_HUMIDITY_PCT) {
    setpoint->value = LIMIT(setpoint->value, MIN_HUMIDITY, MAX_HUMIDITY);
  }

  s->settings.setpoint = *setpoint;
  quantity_widget_set_value(s->quantity_widget, setpoint);
}
