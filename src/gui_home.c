
#include "gui_home.h"
#include "gfx.h"
#include "gui.h"
#include "gui/button.h"
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
  float cur_temp;
  systime_t temp_timestamp;
  char temp_unit;

  widget_t* screen;
  widget_t* probe1_button;
  widget_t* probe2_button;
  widget_t* output1_button;
  widget_t* output2_button;
  widget_t* conn_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_paint(paint_event_t* event);
static void home_screen_destroy(widget_t* w);

static void click_probe_button(click_event_t* event);
static void click_output_button(click_event_t* event);
static void click_conn_button(click_event_t* event);
static void click_settings_button(click_event_t* event);


static const widget_class_t home_widget_class = {
    .on_paint   = home_screen_paint,
    .on_destroy = home_screen_destroy,
};

widget_t*
home_screen_create()
{
  home_screen_t* s = calloc(1, sizeof(home_screen_t));
  s->cur_temp = 73.2;
  s->temp_timestamp = chTimeNow();
  s->temp_unit = 'F';

  s->screen = widget_create(NULL, &home_widget_class, s, display_rect);

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
//  button_create(s->screen, rect, NULL, NULL, GREEN, NULL);

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

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
home_screen_paint(paint_event_t* event)
{
  char temp_str[16];
  home_screen_t* s = widget_get_instance_data(event->widget);

  gfx_set_bg_color(BLACK);
  gfx_clear_screen();

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
  gfx_set_fg_color(GREEN);
  gfx_fill_rect(rect);

  gfx_set_bg_color(GREEN);
  gfx_set_fg_color(WHITE);
  gfx_set_font(font_opensans_62);
//  if (chTimeNow() - s->temp_timestamp < MS2ST(5))
    sprintf(temp_str, "%0.1f", s->cur_temp);
//  else
//    sprintf(temp_str, "--.- %c", s->temp_unit);
    gfx_draw_str(temp_str, 25, 25);
//    gfx_draw_str("F");
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
