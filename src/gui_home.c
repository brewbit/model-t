
#include "gui_home.h"
#include "gfx.h"
#include "gui.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui_probe.h"
#include "gui_output.h"

#include <string.h>
#include <stdio.h>


#define TILE_SPACE 6
#define TILE_SIZE 72

#define TILE_POS(pos) ((((pos) + 1) * TILE_SPACE) + ((pos) * TILE_SIZE))
#define TILE_SPAN(ntiles) (((ntiles) * TILE_SIZE) + (((ntiles) - 1) * TILE_SPACE))

#define TILE_X(pos) (TILE_POS(pos) + 1)
#define TILE_Y(pos) TILE_POS(pos)

typedef struct {
  systime_t temp_timestamp;
  char temp_unit;
  char temp_str1[8];
  char temp_str2[8];

  widget_t* screen;
  widget_t* stage_button;
  widget_t* probe1_temp_label;
  widget_t* probe2_temp_label;
  widget_t* probe1_button;
  widget_t* probe2_button;
  widget_t* output1_button;
  widget_t* output2_button;
  widget_t* conn_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_destroy(widget_t* w);
static void home_screen_msg(msg_event_t* event);

static void click_probe_button(click_event_t* event);
static void click_output_button(click_event_t* event);
static void click_conn_button(click_event_t* event);
static void click_settings_button(click_event_t* event);


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
  s->stage_button = button_create(s->screen, rect, NULL, NULL, GREEN, NULL);

  rect.x = TILE_X(3);
  rect.width = TILE_SPAN(1);
  rect.height = TILE_SPAN(1);
  s->probe1_button = button_create(s->screen, rect, NULL, img_temp_hi, AMBER, click_probe_button);

  rect.y = TILE_Y(1);
  s->probe2_button = button_create(s->screen, rect, NULL, img_temp_low, PURPLE, click_probe_button);

  rect.x = TILE_X(0);
  rect.y = TILE_Y(2);
  s->output1_button = button_create(s->screen, rect, NULL, img_plug, ORANGE, click_output_button);

  rect.x = TILE_X(1);
  s->output2_button = button_create(s->screen, rect, NULL, img_plug, CYAN, click_output_button);

  rect.x = TILE_X(2);
  s->conn_button = button_create(s->screen, rect, NULL, img_signal, STEEL, click_conn_button);

  rect.x = TILE_X(3);
  s->settings_button = button_create(s->screen, rect, NULL, img_settings, OLIVE, click_settings_button);

  // TODO replace these and the one on the probe screen with standard temperature label widgets that handle placement
  rect.x = 25;
  rect.y = 13;
  rect.width = 150;
  s->probe1_temp_label = label_create(s->stage_button, rect, s->temp_str1, font_opensans_62, WHITE, 1);
  widget_set_background(s->probe1_temp_label, GREEN, FALSE);

  rect.y = 83;
  s->probe2_temp_label = label_create(s->stage_button, rect, s->temp_str2, font_opensans_62, WHITE, 1);
  widget_set_background(s->probe2_temp_label, GREEN, FALSE);

  rect.x = 175;
  rect.y = 13;
  label_create(s->stage_button, rect, "F", font_opensans_22, DARK_GRAY, 1);
  rect.y = 83;
  label_create(s->stage_button, rect, "F", font_opensans_22, DARK_GRAY, 1);

  gui_msg_subscribe(MSG_NEW_TEMP, s->screen);

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_NEW_TEMP, s->screen);

  free(s);
}

static void
home_screen_msg(msg_event_t* event)
{
  home_screen_t* s = widget_get_instance_data(event->widget);
  if (event->msg_id == MSG_NEW_TEMP) {
    float temp = *((float*)event->msg_data);
    sprintf(s->temp_str1, "%0.1f", temp);
    sprintf(s->temp_str2, "%0.1f", temp);
    widget_invalidate(s->probe1_temp_label);
    widget_invalidate(s->probe2_temp_label);
  }
}

static void
click_probe_button(click_event_t* event)
{
//  widget_t* parent = widget_get_parent(event->widget);
//  home_screen_t* s = widget_get_instance_data(parent);

  widget_t* settings_screen = probe_settings_screen_create();
  gui_push_screen(settings_screen);
}

static void
click_output_button(click_event_t* event)
{
//  widget_t* parent = widget_get_parent(event->widget);
//  home_screen_t* s = widget_get_instance_data(parent);

  widget_t* settings_screen = output_settings_screen_create();
  gui_push_screen(settings_screen);
}

static void
click_conn_button(click_event_t* event)
{
  (void)event;
}

static void
click_settings_button(click_event_t* event)
{
  (void)event;
}
