
#ifndef GUI_BUTTON_LIST_H
#define GUI_BUTTON_LIST_H

#include "widget.h"
#include "button.h"

typedef struct {
  button_event_handler_t btn_event_handler;
  const Image_t* img;
  color_t color;
  const char* text;
  const char* subtext;
  void* user_data;
} button_spec_t;

widget_t*
button_list_screen_create(
    widget_t* screen,
    char* title,
    button_event_handler_t back_handler,
    void* user_data);

void
button_list_set_buttons(widget_t* button_list, button_spec_t* buttons, uint32_t num_buttons);

#endif
