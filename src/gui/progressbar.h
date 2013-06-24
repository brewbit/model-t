#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "widget.h"


widget_t*
progressbar_create(widget_t* parent, rect_t rect, color_t bg_color, color_t fg_color);

uint8_t
progressbar_get_progress(widget_t* w);

void
progressbar_set_progress(widget_t* w, uint8_t progress);

#endif
