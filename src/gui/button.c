
#include "button.h"

#include <stdlib.h>


static void
button_touch_down()
{

}

button_t*
button_create(rect_t rect, char* text)
{
  button_t* b = calloc(1, sizeof(button_t));

  widget_init(&b->widget, rect);
  b->widget.on_touch_down = button_touch_down;

  b->text = text;

  return b;
}

void
button_destroy(button_t* button)
{
  free(button);
}
