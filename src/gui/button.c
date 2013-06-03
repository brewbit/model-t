
#include "button.h"

#include <stdlib.h>


typedef struct {
  char* text;

  void (*on_click)(void);
} button_t;


static void button_touch(touch_event_t* event);


static const widget_class_t button_widget_class = {
    .on_touch = button_touch
};

widget_t*
button_create(rect_t rect, char* text)
{
  button_t* b = calloc(1, sizeof(button_t));

  b->text = text;

  return widget_create(&button_widget_class, b, rect);
}

static void
button_touch(touch_event_t* event)
{

}

