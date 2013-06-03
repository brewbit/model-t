
#include "gui_home.h"
#include "lcd.h"


static void
home_screen_paint(paint_event_t* event);


static const widget_class_t home_widget_class = {
    .on_paint = home_screen_paint
};

widget_t*
home_screen_create()
{
  widget_t* w = widget_create(NULL, NULL, display_rect);

  return w;
}

static void
home_screen_paint(paint_event_t* event)
{
  tile_bitmap(img_background, display_rect);
}
