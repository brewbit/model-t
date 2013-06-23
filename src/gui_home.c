
#include "gui_home.h"
#include "gfx.h"
#include "gui.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui_probe.h"
#include "gui_output.h"
#include "gui_settings.h"
#include "temp_widget.h"
#include "chprintf.h"

#include <string.h>
#include <stdio.h>


#define TILE_SPACE 6
#define TILE_SIZE 72

#define TILE_POS(pos) ((((pos) + 1) * TILE_SPACE) + ((pos) * TILE_SIZE))
#define TILE_SPAN(ntiles) (((ntiles) * TILE_SIZE) + (((ntiles) - 1) * TILE_SPACE))

#define TILE_X(pos) (TILE_POS(pos) + 1)
#define TILE_Y(pos) TILE_POS(pos)

typedef struct {
  widget_t* temp_widget;
  widget_t* button;
} probe_info_t;

typedef struct {
  systime_t temp_timestamp;
  char temp_unit;

  widget_t* screen;
  widget_t* stage_button;
  probe_info_t probes[NUM_PROBES];

  widget_t* output1_button;
  widget_t* output2_button;
  widget_t* conn_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_destroy(widget_t* w);
static void home_screen_msg(msg_event_t* event);

static void click_probe_button(button_event_t* event);
static void click_output_button(button_event_t* event);
static void click_conn_button(button_event_t* event);
static void click_settings_button(button_event_t* event);

static void dispatch_output_settings(home_screen_t* s, output_settings_msg_t* msg);
static void dispatch_new_temp(home_screen_t* s, temp_msg_t* msg);
static void dispatch_probe_timeout(home_screen_t* s, probe_timeout_msg_t* msg);

static void place_temp_widgets(home_screen_t* s);


static const widget_class_t home_widget_class = {
    .on_destroy = home_screen_destroy,
    .on_msg     = home_screen_msg
};

widget_t*
home_screen_create()
{
  home_screen_t* s = calloc(1, sizeof(home_screen_t));
  s->temp_timestamp = chTimeNow();
  s->temp_unit = 'F';

  s->screen = widget_create(NULL, &home_widget_class, s, display_rect);
  widget_set_background(s->screen, BLACK, FALSE);

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
  s->stage_button = button_create(s->screen, rect, NULL, GREEN, NULL, NULL, NULL, NULL);

  rect.x = TILE_X(3);
  rect.width = TILE_SPAN(1);
  rect.height = TILE_SPAN(1);
  s->probes[PROBE_1].button = button_create(s->screen, rect, img_temp_med, AMBER, NULL, NULL, NULL, click_probe_button);
  widget_disable(s->probes[PROBE_1].button);

  rect.y = TILE_Y(1);
  s->probes[PROBE_2].button = button_create(s->screen, rect, img_temp_med, PURPLE, NULL, NULL, NULL, click_probe_button);
  widget_disable(s->probes[PROBE_2].button);

  rect.x = TILE_X(0);
  rect.y = TILE_Y(2);
  s->output1_button = button_create(s->screen, rect, img_plug, ORANGE, NULL, NULL, NULL, click_output_button);

  rect.x = TILE_X(1);
  s->output2_button = button_create(s->screen, rect, img_plug, CYAN, NULL, NULL, NULL, click_output_button);

  rect.x = TILE_X(2);
  s->conn_button = button_create(s->screen, rect, img_signal, STEEL, NULL, NULL, NULL, click_conn_button);

  rect.x = TILE_X(3);
  s->settings_button = button_create(s->screen, rect, img_settings, OLIVE, NULL, NULL, NULL, click_settings_button);

  rect.x = 0;
  rect.width = TILE_SPAN(3);
  s->probes[PROBE_1].temp_widget = temp_widget_create(s->stage_button, rect);

  s->probes[PROBE_2].temp_widget = temp_widget_create(s->stage_button, rect);

  place_temp_widgets(s);

  gui_msg_subscribe(MSG_NEW_TEMP, s->screen);
  gui_msg_subscribe(MSG_PROBE_TIMEOUT, s->screen);
  gui_msg_subscribe(MSG_OUTPUT_SETTINGS, s->screen);

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_NEW_TEMP, s->screen);
  gui_msg_unsubscribe(MSG_PROBE_TIMEOUT, s->screen);
  gui_msg_unsubscribe(MSG_OUTPUT_SETTINGS, s->screen);

  free(s);
}

static void
home_screen_msg(msg_event_t* event)
{
  home_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_NEW_TEMP:
    dispatch_new_temp(s, event->msg_data);
    break;

  case MSG_PROBE_TIMEOUT:
    dispatch_probe_timeout(s, event->msg_data);
    break;

  case MSG_OUTPUT_SETTINGS:
    dispatch_output_settings(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_new_temp(home_screen_t* s, temp_msg_t* msg)
{
  widget_t* w = s->probes[msg->probe].temp_widget;

  temp_widget_set_value(w, msg->temp);

  if (!widget_is_enabled(s->probes[msg->probe].button)) {
    widget_enable(s->probes[msg->probe].button, true);
    place_temp_widgets(s);
  }
}

static void
dispatch_probe_timeout(home_screen_t* s, probe_timeout_msg_t* msg)
{
  widget_t* w = s->probes[msg->probe].temp_widget;

  if (widget_is_enabled(s->probes[msg->probe].button)) {
    widget_disable(s->probes[msg->probe].button);
    temp_widget_set_value(w, INVALID_TEMP);
    place_temp_widgets(s);
  }
}

static void
place_temp_widgets(home_screen_t* s)
{
  int i;
  rect_t rect = widget_get_rect(s->stage_button);

  probe_info_t* active_probes[NUM_PROBES];
  int num_active_probes = 0;
  for (i = 0; i < NUM_PROBES; ++i) {
    if (widget_is_enabled(s->probes[i].button)) {
      widget_show(s->probes[i].temp_widget);
      active_probes[num_active_probes++] = &s->probes[i];
    }
    else
      widget_hide(s->probes[i].temp_widget);
  }

  if (num_active_probes == 0) {
    widget_show(s->probes[PROBE_1].temp_widget);
    active_probes[num_active_probes++] = &s->probes[PROBE_1];
  }

  for (i = 0; i < num_active_probes; ++i) {
    rect_t wrect = widget_get_rect(active_probes[i]->temp_widget);

    int spacing = (rect.height - (num_active_probes * wrect.height)) / (num_active_probes + 1);
    wrect.y = (spacing * (i + 1)) + (wrect.height * i);

    widget_set_rect(active_probes[i]->temp_widget, wrect);
  }
}

static void
dispatch_output_settings(home_screen_t* s, output_settings_msg_t* msg)
{
  widget_t* btn;
  if (msg->output == OUTPUT_1)
    btn = s->output1_button;
  else
    btn = s->output2_button;

  color_t color = 0;
  switch (msg->settings.function) {
  case OUTPUT_FUNC_COOLING:
    color = CYAN;
    break;

  case OUTPUT_FUNC_HEATING:
    color = ORANGE;
    break;

  case OUTPUT_FUNC_MANUAL:
    color = MAGENTA;
    break;
  }

  widget_set_background(btn, color, FALSE);
}

static void
click_probe_button(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  home_screen_t* s = widget_get_instance_data(parent);

  probe_id_t probe;
  if (event->widget == s->probes[PROBE_1].button)
    probe = PROBE_1;
  else
    probe = PROBE_2;

  widget_t* settings_screen = probe_settings_screen_create(probe);
  gui_push_screen(settings_screen);
}

static void
click_output_button(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  home_screen_t* s = widget_get_instance_data(parent);

  output_id_t output;
  if (event->widget == s->output1_button)
    output = OUTPUT_1;
  else
    output = OUTPUT_2;

  widget_t* settings_screen = output_settings_screen_create(output);
  gui_push_screen(settings_screen);
}

static void
click_conn_button(button_event_t* event)
{
  (void)event;
}

static void
click_settings_button(button_event_t* event)
{
  (void)event;

  widget_t* settings_screen = settings_screen_create();
  gui_push_screen(settings_screen);
}
