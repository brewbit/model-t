#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"
#include "image.h"


typedef void (*click_handler_t)(click_event_t* event);

widget_t*
button_create(rect_t rect, const char* text, const Image_t* icon, click_handler_t click_handler);

#endif
