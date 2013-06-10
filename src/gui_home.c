
#include "gui_home.h"
#include "lcd.h"
#include "gui.h"
#include "gui/button.h"

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
  widget_t* calib_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_paint(paint_event_t* event);
static void home_screen_destroy(widget_t* w);

static void start_calib(click_event_t* event);
static void calib_complete(void);


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
  button_create(s->screen, rect, NULL, img_temp_hi, AMBER, NULL);

  rect.y = TILE_Y(1);
  button_create(s->screen, rect, NULL, img_temp_low, PURPLE, NULL);

  rect.x = TILE_X(0);
  rect.y = TILE_Y(2);
  button_create(s->screen, rect, NULL, img_plug, ORANGE, NULL);

  rect.x = TILE_X(1);
  button_create(s->screen, rect, NULL, img_plug, CYAN, NULL);

  rect.x = TILE_X(2);
  button_create(s->screen, rect, NULL, img_signal, STEEL, NULL);

  rect.x = TILE_X(3);
  button_create(s->screen, rect, NULL, img_settings, OLIVE, NULL);

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

  fillScr(COLOR(32,32,32));

  rect_t rect = {
      .x      = TILE_X(0),
      .y      = TILE_Y(0),
      .width  = TILE_SPAN(3),
      .height = TILE_SPAN(2),
  };
  setColor(GREEN);
  fillRect(rect);

  set_bg_color(GREEN);
  setColor(WHITE);
  setFont(font_temp_large);
//  if (chTimeNow() - s->temp_timestamp < MS2ST(5))
    sprintf(temp_str, "%0.1f", s->cur_temp);
//  else
//    sprintf(temp_str, "--.- %c", s->temp_unit);
    print(temp_str, 25, 25);
//    print("F");
}

static void
start_calib(click_event_t* event)
{
  (void)event;

  widget_t* calib_screen = calib_screen_create(calib_complete);
  gui_set_screen(calib_screen);
}

static void
calib_complete(void)
{
  widget_t* home_screen = home_screen_create();
  gui_set_screen(home_screen);
}
