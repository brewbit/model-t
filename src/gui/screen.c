
#include "screen.h"
#include "lcd.h"

#include <stdlib.h>


static void screen_paint(paint_event_t* event);


screen_t*
screen_create()
{
  screen_t* s = calloc(1, sizeof(screen_t));

  rect_t rect = {
      .x = 0,
      .y = 0,
      .width = DISP_WIDTH,
      .height = DISP_HEIGHT
  };
  widget_init(&s->widget, rect);
  s->widget.on_paint = screen_paint;

  return s;
}

static void
screen_paint(paint_event_t* event)
{
  screen_t* s = (screen_t*)(event->widget);

  if (s->bg_type == BG_IMAGE)
    tile_bitmap(s->bg_img, 0, 0, DISP_WIDTH, DISP_HEIGHT);
  else
    fillScr(s->bg_color);
}

void
screen_set_bg_img(screen_t* s, const Image_t* bg_img)
{
  s->bg_type = BG_IMAGE;
  s->bg_img = bg_img;
}

void
screen_set_bg_color(screen_t* s, color_t bg_color)
{
  s->bg_type = BG_COLOR;
  s->bg_color = bg_color;
}
