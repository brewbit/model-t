
#include "gui_output.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui_output_function.h"
#include "gui_output_trigger.h"
#include "gui_quantity_select.h"

#include <string.h>

typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* function_button;
  widget_t* trigger_button;
  widget_t* compressor_delay_button;

  output_id_t output;
  output_settings_t settings;
} output_screen_t;


static void output_settings_screen_destroy(widget_t* w);
static void output_settings_screen_msg(msg_event_t* event);
static void dispatch_output_settings(output_screen_t* s, output_settings_msg_t* msg);
static void set_output_settings(output_screen_t* s, output_function_t function, sensor_id_t trigger);
static void back_button_clicked(button_event_t* event);
static void compressor_delay_button_clicked(button_event_t* event);
static void function_button_clicked(button_event_t* event);
static void trigger_button_clicked(button_event_t* event);
static void update_compressor_delay(quantity_t delay, void* user_data);


widget_class_t output_settings_widget_class = {
    .on_destroy = output_settings_screen_destroy,
    .on_msg     = output_settings_screen_msg
};


widget_t*
output_settings_screen_create(output_id_t output)
{
  output_screen_t* s = chHeapAlloc(NULL, sizeof(output_screen_t));
  memset(s, 0, sizeof(output_screen_t));

  s->widget = widget_create(NULL, &output_settings_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

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
  char* title = (output == OUTPUT_1) ? "Output 1 Setup" : "Output 2 Setup";
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 48;
  rect.y = 120;
  rect.width = 56;
  rect.height = 56;
  s->function_button = button_create(s->widget, rect, img_flame, WHITE, ORANGE, function_button_clicked);

  rect.x += 84;
  s->trigger_button = button_create(s->widget, rect, img_temp_38, WHITE, AMBER, trigger_button_clicked);

  rect.x += 84;
  s->compressor_delay_button = button_create(s->widget, rect, img_stopwatch, WHITE, GREEN, compressor_delay_button_clicked);

  s->output = output;
  s->settings = *app_cfg_get_output_settings(output);

  gui_msg_subscribe(MSG_OUTPUT_SETTINGS, s->widget);
  set_output_settings(s, s->settings.function, s->settings.trigger);

  return s->widget;
}

static void
output_settings_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  gui_msg_unsubscribe(MSG_OUTPUT_SETTINGS, w);
  chHeapFree(s);
}

static void
output_settings_screen_msg(msg_event_t* event)
{
  output_screen_t* s = widget_get_instance_data(event->widget);

  if (event->msg_id == MSG_OUTPUT_SETTINGS)
    dispatch_output_settings(s, event->msg_data);
}

static void
dispatch_output_settings(output_screen_t* s, output_settings_msg_t* msg)
{
  if (msg->output == s->output)
    set_output_settings(s, msg->settings.function, msg->settings.trigger);
}

static void
set_output_settings(output_screen_t* s, output_function_t function, sensor_id_t trigger)
{
  widget_t* btn1 = s->function_button;
  widget_t* btn2 = s->trigger_button;

  color_t color = 0;
  const Image_t* img = img_snowflake;
  switch (function) {
    case OUTPUT_FUNC_COOLING:
      color = CYAN;
      img = img_snowflake;
      break;

    case OUTPUT_FUNC_HEATING:
      color = ORANGE;
      img = img_flame;
      break;

    default:
      break;
  }

  widget_set_background(btn1, color, FALSE);
  button_set_icon(btn1, img);

  widget_set_background(btn2, (trigger == SENSOR_1) ? AMBER : PURPLE, FALSE);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
function_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    widget_t* output_function_screen = output_function_screen_create(s->output);
    gui_push_screen(output_function_screen);
  }
}

static void
trigger_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    widget_t* output_trigger_screen = output_trigger_screen_create(s->output);
    gui_push_screen(output_trigger_screen);
  }
}

static void
compressor_delay_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  widget_t* screen = widget_get_parent(event->widget);
  output_screen_t* s = widget_get_instance_data(screen);

  char* title;
  if (s->output == OUTPUT_1) {
    title = "Output 1 Delay";
  }
  else {
    title = "Output 2 Delay";
  }

  float velocity_steps[] = {
      1.0f
  };
  widget_t* output_delay_screen = quantity_select_screen_create(
      title, s->settings.compressor_delay, velocity_steps, 1, update_compressor_delay, s);
  gui_push_screen(output_delay_screen);
}

static void
update_compressor_delay(quantity_t delay, void* user_data)
{
  output_screen_t* s = user_data;
  s->settings.compressor_delay = delay;
  app_cfg_set_output_settings(s->output, &s->settings);
}

