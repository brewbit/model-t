
#ifndef SCREEN_H
#define SCREEN_H

#include "widget.h"
#include "image.h"


widget_t*
screen_create(void);

void
screen_set_bg_img(widget_t* s, const Image_t* bg_img);

void
screen_set_bg_color(widget_t* s, color_t bg_color);

#endif
