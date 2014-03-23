#ifndef ICON_H
#define ICON_H

#include "widget.h"
#include "image.h"


widget_t*
icon_create(widget_t* parent, rect_t rect, const Image_t* icon, uint16_t icon_color, uint16_t bg_color);

void
icon_set_image(widget_t* w, const Image_t* image);

#endif
