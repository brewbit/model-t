
#ifndef TOUCH_H
#define TOUCH_H

#include "common.h"

typedef void (*touch_cb)(bool touch_down, point_t raw, point_t calib);

typedef struct touch_handler_s {
  touch_cb on_touch;

  struct touch_handler_s* next;
} touch_handler_t;


void
touch_init(void);

void
touch_handler_register(touch_handler_t* handler);

void
touch_handler_unregister(touch_handler_t* handler);

void
touch_calibrate(
    const point_t* ref_pts,
    const point_t* sampled_pts);

#endif
