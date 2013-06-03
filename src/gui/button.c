
#include "button.h"
#include "gui.h"
#include "lcd.h"

#include <stdlib.h>


typedef struct {
  bool is_down;
  char* text;

  click_handler_t on_click;
} button_t;


static void button_touch(touch_event_t* event);
static void button_paint(paint_event_t* event);
static void button_destroy(widget_t* w);


static const widget_class_t button_widget_class = {
    .on_touch = button_touch,
    .on_paint = button_paint,
    .on_destroy = button_destroy,
};

widget_t*
button_create(rect_t rect, char* text, click_handler_t click_handler)
{
  button_t* b = calloc(1, sizeof(button_t));

  b->text = text;
  b->on_click = click_handler;

  return widget_create(&button_widget_class, b, rect);
}

static void
button_destroy(widget_t* w)
{
  button_t* b = widget_get_instance_data(w);
  free(b);
}

static void
button_touch(touch_event_t* event)
{
  button_t* b = widget_get_instance_data(event->widget);

  if (event->id == EVT_TOUCH_DOWN) {
    if (!b->is_down) {
      b->is_down = true;
      gui_acquire_touch_capture(event->widget);
      widget_invalidate(event->widget);
    }
  }
  else {
    if (b->is_down) {
      b->is_down = false;
      gui_release_touch_capture();
      if (b->on_click) {
        click_event_t ce = {
            .id = EVT_CLICK,
            .widget = event->widget,
            .pos = event->pos,
        };
        b->on_click(&ce);
      }
      widget_invalidate(event->widget);
    }
  }
}

static void
button_paint(paint_event_t* event)
{
  button_t* b = widget_get_instance_data(event->widget);

  rect_t rect = widget_get_rect(event->widget);

  if (b->is_down)
    setColor(RED);
  else
    setColor(BLUE);

  fillRect(rect);

  setFont(font_test);
  point_t center = rect_center(rect);
  print(b->text, rect.x + 10, center.y);
}
