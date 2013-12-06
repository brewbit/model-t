
#include "gui_output.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "sensor.h"

#include <string.h>

typedef struct {
  widget_t* widget;
  widget_t* back_button;

  widget_t* trigger_sensor1_button;
  widget_t* trigger_sensor1_icon;
  widget_t* trigger_sensor1_header_label;
  widget_t* trigger_sensor1_desc_label;

  widget_t* trigger_sensor2_button;
  widget_t* trigger_sensor2_icon;
  widget_t* trigger_sensor2_header_label;
  widget_t* trigger_sensor2_desc_label;

  output_id_t output;
} output_screen_t;


static void output_trigger_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void trigger_sensor1_button_clicked(button_event_t* event);
static void trigger_sensor2_button_clicked(button_event_t* event);
static void dispatch_sensor_sample(output_screen_t* s, sensor_msg_t* msg);
static void dispatch_sensor_timeout(output_screen_t* s, sensor_timeout_msg_t* msg);

static void output_trigger_screen_msg(msg_event_t* event);


widget_class_t output_trigger_widget_class = {
    .on_destroy = output_trigger_screen_destroy,
    .on_msg     = output_trigger_screen_msg
};


widget_t*
output_trigger_screen_create(output_id_t output)
{
  char* header;
  char* desc;
  output_screen_t* s = chHeapAlloc(NULL, sizeof(output_screen_t));
  memset(s, 0, sizeof(output_screen_t));
  bool sensor1_is_connected = sensor_is_connected(SENSOR_1);
  bool sensor2_is_connected = sensor_is_connected(SENSOR_2);

  s->widget = widget_create(NULL, &output_trigger_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, img_left, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (output == OUTPUT_1) ? "Output 1 Trig" : "Output 2 Trig";
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 300;
  rect.height = 66;
  s->trigger_sensor1_button = button_create(s->widget, rect, NULL, BLACK, trigger_sensor1_button_clicked);

  rect.y = 165;
  s->trigger_sensor2_button = button_create(s->widget, rect, NULL, BLACK, trigger_sensor2_button_clicked);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->trigger_sensor1_icon = icon_create(s->trigger_sensor1_button, rect, img_temp_38, AMBER);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->trigger_sensor1_header_label = label_create(s->trigger_sensor1_button, rect, NULL, font_opensans_regular_16, WHITE, 1);

  rect.y = 30;
  s->trigger_sensor1_desc_label = label_create(s->trigger_sensor1_button, rect, NULL, font_opensans_regular_8, WHITE, 2);
  header = "Trigger: Sensor 1";
  desc = "The temperature will be read from Sensor 1.";
  label_set_text(s->trigger_sensor1_header_label, header);
  label_set_text(s->trigger_sensor1_desc_label, desc);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->trigger_sensor2_icon = icon_create(s->trigger_sensor2_button, rect, img_temp_38, PURPLE);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->trigger_sensor2_header_label = label_create(s->trigger_sensor2_button, rect, NULL, font_opensans_regular_16, WHITE, 1);

  rect.y = 30;
  s->trigger_sensor2_desc_label = label_create(s->trigger_sensor2_button, rect, NULL, font_opensans_regular_8, WHITE, 2);
  header = "Trigger: Sensor 2";
  desc = "The temperature will be read from Sensor 2.";
  label_set_text(s->trigger_sensor2_header_label, header);
  label_set_text(s->trigger_sensor2_desc_label, desc);

  if (!sensor1_is_connected)
    widget_disable(s->trigger_sensor1_button);

  if (!sensor2_is_connected)
    widget_disable(s->trigger_sensor2_button);

  s->output = output;

  gui_msg_subscribe(MSG_SENSOR_TIMEOUT, s->widget);
  gui_msg_subscribe(MSG_SENSOR_SAMPLE, s->widget);

  return s->widget;
}

static void
output_trigger_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  gui_msg_unsubscribe(MSG_SENSOR_SAMPLE, s->widget);
  gui_msg_unsubscribe(MSG_SENSOR_TIMEOUT, s->widget);
  chHeapFree(s);
}

static void
output_trigger_screen_msg(msg_event_t* event)
{
  output_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
    case MSG_SENSOR_SAMPLE:
      dispatch_sensor_sample(s, event->msg_data);
      break;

    case MSG_SENSOR_TIMEOUT:
      dispatch_sensor_timeout(s, event->msg_data);
      break;

    default:
      break;
  }
}

static void
dispatch_sensor_sample(output_screen_t* s, sensor_msg_t* msg)
{
  if (msg->sensor == SENSOR_1)
    widget_enable(s->trigger_sensor1_button, true);
  else if (msg->sensor == SENSOR_2)
    widget_enable(s->trigger_sensor2_button, true);
}

static void
dispatch_sensor_timeout(output_screen_t* s, sensor_timeout_msg_t* msg)
{
  if (msg->sensor == SENSOR_1)
    widget_disable(s->trigger_sensor1_button);
  else if (msg->sensor == SENSOR_2)
    widget_disable(s->trigger_sensor2_button);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
trigger_sensor1_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.trigger = SENSOR_1;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}

static void
trigger_sensor2_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.trigger = SENSOR_2;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}
