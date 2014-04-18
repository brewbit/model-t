
#include "gui/output_settings.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "gui/quantity_select.h"
#include "gui/button_list.h"

#include <string.h>
#include <stdio.h>

typedef struct {
  widget_t* screen;
  widget_t* button_list;

  output_id_t output;
  output_settings_t settings;
} output_screen_t;


static void output_settings_screen_destroy(widget_t* w);
static void output_settings_screen_msg(msg_event_t* event);
static void dispatch_output_settings(output_screen_t* s, output_settings_msg_t* msg);
static void set_output_settings(output_screen_t* s);
static void back_button_clicked(button_event_t* event);
static void cycle_delay_button_clicked(button_event_t* event);
static void output_mode_button_clicked(button_event_t* event);
static void function_button_clicked(button_event_t* event);
static void update_cycle_delay(quantity_t delay, void* user_data);


widget_class_t output_settings_widget_class = {
    .on_destroy = output_settings_screen_destroy,
    .on_msg     = output_settings_screen_msg
};


widget_t*
output_settings_screen_create(output_id_t output)
{
  output_screen_t* s = calloc(1, sizeof(output_screen_t));

  s->screen = widget_create(NULL, &output_settings_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  char* title = (output == OUTPUT_1) ? "Output 1 Setup" : "Output 2 Setup";
  s->button_list = button_list_screen_create(s->screen, title, back_button_clicked, s);

  s->output = output;
  s->settings = *app_cfg_get_output_settings(output);

  set_output_settings(s);

  return s->screen;
}

static void
output_settings_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  gui_msg_unsubscribe(MSG_OUTPUT_SETTINGS, w);
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
set_output_settings(output_screen_t* s)
{
  uint32_t num_buttons = 0;
  button_spec_t buttons[4];

  char* subtext;
  char* output_delay_subtext;
  char* text;
  color_t color;
  const Image_t* img;

  switch (s->settings.function) {
    default:
    case OUTPUT_FUNC_HEATING:
      text = "Heating Mode";
      subtext = "Enable output when temp is less than the setpoint";
      color = ORANGE;
      img = img_flame;
      break;

    case OUTPUT_FUNC_COOLING:
      text = "Cooling Mode";
      subtext = "Enable output when temp is greater than the setpoint";
      color = CYAN;
      img = img_snowflake;
      break;
  }
  add_button_spec(buttons, &num_buttons, function_button_clicked, img, color,
      text, subtext, s);


  switch (s->settings.output_mode) {
    case ON_OFF:
      color = RED;
      text = "ON/OFF Mode";
      subtext = "Enable/Disable output at setpoint";
      break;
    case PID:
      color = STEEL;
      text = "PID Mode";
      subtext = "Minimize output error by adjusting control inputs";
      break;
  }
  add_button_spec(buttons, &num_buttons, output_mode_button_clicked, img_graph_signal, color,
      text, subtext, s);

  output_delay_subtext = malloc(128);
  snprintf(output_delay_subtext, 128, "Delay value: %d Min",
    (int)(s->settings.cycle_delay.value));
  add_button_spec(buttons, &num_buttons, cycle_delay_button_clicked, img_stopwatch, GREEN,
      "Compressor Delay", output_delay_subtext, s);

  button_list_set_buttons(s->button_list, buttons, num_buttons);
  free(output_delay_subtext);
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
    set_output_settings(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    output_screen_t* s = widget_get_user_data(event->widget);

    app_cfg_set_output_settings(s->output, &s->settings);

    gui_pop_screen();
  }
}

static void
function_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    output_screen_t* s = widget_get_user_data(event->widget);

    if (s->settings.function == OUTPUT_FUNC_HEATING)
      s->settings.function = OUTPUT_FUNC_COOLING;
    else
      s->settings.function = OUTPUT_FUNC_HEATING;

    set_output_settings(s);
  }
}

static void
cycle_delay_button_clicked(button_event_t* event)
{
  if (event->id != EVT_BUTTON_CLICK)
    return;

  output_screen_t* s = widget_get_user_data(event->widget);

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
      title, s->settings.cycle_delay, velocity_steps, 1, update_cycle_delay, s);
  gui_push_screen(output_delay_screen);
}

static void
output_mode_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    output_screen_t* s = widget_get_user_data(event->widget);

    if (s->settings.output_mode == ON_OFF)
      s->settings.output_mode = PID;
    else
      s->settings.output_mode = ON_OFF;

    set_output_settings(s);
  }
}

static void
update_cycle_delay(quantity_t delay, void* user_data)
{
  output_screen_t* s = user_data;
  s->settings.cycle_delay = delay;

  set_output_settings(s);
}

