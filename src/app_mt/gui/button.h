#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"
#include "image.h"
#include "font.h"


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
    uint16_t icon_color,
    uint16_t btn_color,
    button_event_handler_t evt_handler);

void
button_set_icon(widget_t* w, const Image_t* icon);

void
button_set_color(widget_t* w, uint16_t color);

void
button_set_icon_color(widget_t* w, uint16_t color);

void
button_set_text(widget_t* w, const char* text);

const char*
button_get_text(widget_t* w);

void
button_set_font(widget_t* w, const font_t* font);

#endif
