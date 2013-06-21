#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"
#include "image.h"


typedef struct {
  event_id_t id;
  widget_t* widget;
  point_t pos;
} button_event_t;


typedef void (*button_event_handler_t)(button_event_t* event);

widget_t*
button_create(
    widget_t* parent,
    rect_t rect,
    const Image_t* icon,
    uint16_t color,
    button_event_handler_t down_handler,
    button_event_handler_t repeat_handler,
    button_event_handler_t up_handler,
    button_event_handler_t click_handler);

#endif
