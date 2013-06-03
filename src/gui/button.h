#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"


typedef void (*click_handler_t)(click_event_t* event);

widget_t*
button_create(rect_t rect, char* text, click_handler_t click_handler);

#endif
