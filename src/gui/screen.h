
#ifndef SCREEN_H
#define SCREEN_H

#include "widget.h"
#include "image.h"

typedef enum {
  BG_IMAGE,
  BG_COLOR
} bg_type_t;

typedef struct {
  widget_t widget;

  bg_type_t bg_type;
  const Image_t* bg_img;
  color_t bg_color;
} screen_t;

screen_t*
screen_create(void);

void
screen_set_bg_img(screen_t* s, const Image_t* bg_img);

void
screen_set_bg_color(screen_t* s, color_t bg_color);

#endif
