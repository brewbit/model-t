#ifndef LABEL_H
#define LABEL_H

#include "widget.h"
#include "image.h"
#include "font.h"


widget_t*
label_create(widget_t* parent, rect_t rect, const char* text, const Font_t* font, color_t color);

void
label_set_text(widget_t* w, char* text);

#endif
