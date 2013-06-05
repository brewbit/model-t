
#include "screen.h"
#include "lcd.h"

#include <stdlib.h>


typedef enum {
  BG_IMAGE,
  BG_COLOR
} bg_type_t;

typedef struct {
  bg_type_t bg_type;
  const Image_t* bg_img;
  color_t bg_color;
} screen_t;


static void screen_paint(paint_event_t* event);


static const widget_class_t screen_widget_class = {
    .on_paint = screen_paint
};


widget_t*
screen_create()
{
  screen_t* s = calloc(1, sizeof(screen_t));
  rect_t rect = {
      .x = 0,
      .y = 0,
      .width = DISP_WIDTH,
      .height = DISP_HEIGHT
  };
  return widget_create(NULL, &screen_widget_class, s, rect);
}

static void
screen_paint(paint_event_t* event)
{
  screen_t* s = widget_get_instance_data(event->widget);

  if (s->bg_type == BG_IMAGE)
    tile_bitmap(s->bg_img, display_rect);
  else
    fillScr(s->bg_color);
}

void
screen_set_bg_img(widget_t* w, const Image_t* bg_img)
{
  screen_t* s = widget_get_instance_data(w);

  s->bg_type = BG_IMAGE;
  s->bg_img = bg_img;
}

void
screen_set_bg_color(widget_t* w, color_t bg_color)
{
  screen_t* s = widget_get_instance_data(w);

  s->bg_type = BG_COLOR;
  s->bg_color = bg_color;
}
