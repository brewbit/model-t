
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

  /* draw border */
  setColor(COLOR(0x88, 0x88, 0x88));
  drawRect(rect);

  /* draw background */
  rect.x += 1;
  rect.y += 1;
  rect.width -= 2;
  rect.height -= 2;
  if (b->is_down) {
    setColor(COLOR(0xAA, 0xAA, 0xAA));
    set_bg_color(COLOR(0xAA, 0xAA, 0xAA));
    fillRect(rect);
  }
  else {
    point_t bg_anchor = { .x = rect.x, .y = rect.y };
    set_bg_img(img_button_bg, bg_anchor);
    tile_bitmap(img_button_bg, rect);
  }

  /* draw text */
  setColor(BLACK);
  setFont(font_terminal);
  point_t center = rect_center(rect);
  print(b->text, rect.x + 10, center.y);

  /* draw icon */
}
