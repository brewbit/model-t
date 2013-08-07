
#include "gui_output_delay.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"
#include "quantity_widget.h"
#include "app_cfg.h"

/* Delays are in minutes */
#define MIN_DELAY 0
#define MAX_DELAY 30


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
  widget_t* quantity_widget;
  output_id_t output;
  output_settings_t settings;
} output_delay_screen_t;


static void output_delay_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void up_button_clicked(button_event_t* event);
static void down_button_clicked(button_event_t* event);
static void up_or_down_released(button_event_t* event);
static void set_delay(output_delay_screen_t* s, quantity_t setpoint);


widget_class_t output_delay_widget_class = {
    .on_destroy = output_delay_screen_destroy
};

widget_t*
output_delay_screen_create(output_id_t output)
{
  output_delay_screen_t* s = calloc(1, sizeof(output_delay_screen_t));
  s->widget = widget_create(NULL, &output_delay_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

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
  char* title = (output == OUTPUT_1) ? "Output 1 Delay" : "Output 2 Delay";
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

  s->output = output;
  s->settings = *app_cfg_get_output_settings(output);
  set_delay(s, s->settings.setpoint);

  return s->widget;
}

static void
output_delay_screen_destroy(widget_t* w)
{
  output_delay_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  widget_t* w = widget_get_parent(event->widget);
  output_delay_screen_t* s = widget_get_instance_data(w);

  app_cfg_set_output_settings(s->output, &s->settings);

  gui_pop_screen();
}

static void
up_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  output_delay_screen_t* s = widget_get_instance_data(screen);
  quantity_t new_sp = {
      .unit = s->settings.setpoint.unit,
      .value = s->settings.setpoint.value += 1
  };

  set_delay(s, new_sp);
}

static void
down_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  output_delay_screen_t* s = widget_get_instance_data(screen);
  quantity_t new_sp = {
      .unit = s->settings.setpoint.unit,
      .value = s->settings.setpoint.value -= 1
  };

  set_delay(s, new_sp);
}

static void
up_or_down_released(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  output_delay_screen_t* s = widget_get_instance_data(screen);
}

static void
set_delay(output_delay_screen_t* s, quantity_t setpoint)
{
  if (setpoint.value < MIN_DELAY)
    setpoint.value = MIN_DELAY;
  else if (setpoint.value > MAX_DELAY)
    setpoint.value = MAX_DELAY;

  s->settings.compressor_delay = S2ST(setpoint.value);
  s->settings.setpoint = setpoint;
  quantity_widget_set_value(s->quantity_widget, setpoint);
}
