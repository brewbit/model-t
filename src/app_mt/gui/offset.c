#include "offset.h"
#include "quantity_select.h"
#include "gui.h"
#include "app_cfg.h"
#include "button.h"
#include "lcd.h"
#include "gfx.h"
#include "gui/button_list.h"

#include <string.h>
#include <stdio.h>

#define MIN_PROBE_OFFSET -30
#define MAX_PROBE_OFFSET 30

typedef struct {
  sensor_id_t sensor_id;
  bool sensor1_enabled;
  bool sensor2_enabled;

  widget_t* screen;
  widget_t* button_list;
} offset_screen_t;


static void back_button_clicked(button_event_t* event);
static void rebuild_offset_screen(offset_screen_t* s);
static void offset_screen_destroy(widget_t* w);
static void offset_widget_msg(msg_event_t* event);
static void update_probe_offset(quantity_t probe_offset, void* user_data);
static void build_offset_screen(offset_screen_t* s, char* title);


static const widget_class_t offset_widget_class = {
    .on_msg     = offset_widget_msg,
    .on_destroy = offset_screen_destroy
};

widget_t*
offset_screen_create()
{
  offset_screen_t* s = calloc(1, sizeof(offset_screen_t));

  s->screen = widget_create(NULL, &offset_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK);

  char* title = "Set Probe Offset";
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  gui_msg_subscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_subscribe(MSG_SENSOR_TIMEOUT, s->screen);

  rebuild_offset_screen(s);

  return s->screen;
}

static void offset_widget_msg(msg_event_t* event)
{
  offset_screen_t* s = widget_get_instance_data(event->widget);

  if (event->msg_id == MSG_SENSOR_SAMPLE) {
    sensor_msg_t* msg = event->msg_data;
    if (msg->sensor == SENSOR_1 && s->sensor1_enabled == false) {
      s->sensor1_enabled = true;
      rebuild_offset_screen(s);
    }
    else if (msg->sensor == SENSOR_2 && s->sensor2_enabled == false) {
      s->sensor2_enabled = true;
      rebuild_offset_screen(s);
    }
  }
  else if (event->msg_id == MSG_SENSOR_TIMEOUT) {
    sensor_timeout_msg_t* msg = event->msg_data;
    if (msg->sensor == SENSOR_1 && s->sensor1_enabled == true) {
      s->sensor1_enabled = false;
      rebuild_offset_screen(s);
    }
    else if (msg->sensor == SENSOR_2 && s->sensor2_enabled == true) {
      s->sensor2_enabled = false;
      rebuild_offset_screen(s);
    }
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
offset_screen_destroy(widget_t* w)
{
  offset_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_SENSOR_SAMPLE, s->screen);
  gui_msg_unsubscribe(MSG_SENSOR_TIMEOUT, s->screen);

  free(s);
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
probe1_offset_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
      return;

  offset_screen_t* s = widget_get_user_data(event->widget);
  s->sensor_id = SENSOR_1;

  build_offset_screen(s, "Probe 1 Offset");
}

static void
probe2_offset_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
      return;

  offset_screen_t* s = widget_get_user_data(event->widget);
  s->sensor_id = SENSOR_2;

  build_offset_screen(s, "Probe 2 Offset");
}

static void
build_offset_screen(offset_screen_t* s, char* title)
{
  float velocity_steps[] = {
      0.1f
  };
  sensor_config_t* sensor_cfg = get_sensor_cfg(s->sensor_id);
  quantity_t probe_offset = app_cfg_get_probe_offset(sensor_cfg->sensor_serial);
  if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_C) {
    probe_offset.value *= (5.0f / 9.0f);
    probe_offset.unit = UNIT_TEMP_DEG_C;
  }

  widget_t* probe_offset_screen = quantity_select_screen_create(
      title, probe_offset, MIN_PROBE_OFFSET, MAX_PROBE_OFFSET, velocity_steps, 1,
      update_probe_offset, s);
  gui_push_screen(probe_offset_screen);
}

static void
update_probe_offset(quantity_t probe_offset, void* user_data)
{
  offset_screen_t* s = user_data;

  sensor_config_t* sensor_cfg = get_sensor_cfg(s->sensor_id);
  app_cfg_set_probe_offset(probe_offset, sensor_cfg->sensor_serial);

  rebuild_offset_screen(s);
}

static void
rebuild_offset_screen(offset_screen_t* s)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[3];
  char* text;
  char* units_subtext;
  char* probe1_subtext = NULL;
  char* probe2_subtext = NULL;

  bool sensor1_connected = get_sensor_conn_status(SENSOR_1);
  bool sensor2_connected = get_sensor_conn_status(SENSOR_2);

  sensor_config_t* sensor1_cfg = get_sensor_cfg(SENSOR_1);
  sensor_config_t* sensor2_cfg = get_sensor_cfg(SENSOR_2);

  quantity_t probe1_offset = app_cfg_get_probe_offset(sensor1_cfg->sensor_serial);
  quantity_t probe2_offset = app_cfg_get_probe_offset(sensor2_cfg->sensor_serial);

  if (app_cfg_get_temp_unit() == UNIT_TEMP_DEG_F) {
    units_subtext = "F";
  }
  else {
    probe1_offset.value *= (5.0f / 9.0f);
    probe2_offset.value *= (5.0f / 9.0f);
    units_subtext = "C";
  }

  if (sensor1_connected == true) {
    text = "Probe 1 Offset";
    probe1_subtext = malloc(128);
    snprintf(probe1_subtext, 128, "Probe 1 Offset: %d.%d %s",
         (int)(probe1_offset.value),
         ((int)(fabs(probe1_offset.value) * 10.0f)) % 10,
         units_subtext);
    add_button_spec(buttons, &num_buttons, probe1_offset_button_clicked, img_temp_med, AMBER,
        text, probe1_subtext, s);
  }

  if (sensor2_connected == true) {
    text = "Probe 2 Offset";
    probe2_subtext = malloc(128);
    snprintf(probe2_subtext, 128, "Probe 2 Offset: %d.%d %s",
         (int)(probe2_offset.value),
         ((int)(fabs(probe2_offset.value) * 10.0f)) % 10,
         units_subtext);

    add_button_spec(buttons, &num_buttons, probe2_offset_button_clicked, img_temp_med, MAGENTA,
        text, probe2_subtext, s);
  }

  button_list_set_buttons(s->button_list, buttons, num_buttons);

  if (probe1_subtext != NULL)
    free(probe1_subtext);

  if (probe2_subtext != NULL)
    free(probe2_subtext);
}

