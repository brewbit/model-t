#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"

typedef struct {
  widget_t widget;
  char* text;

  void (*on_click)(void);
} button_t;

button_t*
button_create(rect_t rect, char* text);

void
button_destroy(button_t* button);

#endif
