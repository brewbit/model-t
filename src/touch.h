
#ifndef TOUCH_H
#define TOUCH_H

#include "common.h"

typedef void (*touch_handler_t)(bool touch_down, point_t raw, point_t calib, void* user_data);


void
touch_init(void);

void
touch_handler_register(touch_handler_t handler, void* user_data);

void
touch_handler_unregister(touch_handler_t handler);

void
touch_calibrate(
    const point_t* ref_pts,
    const point_t* sampled_pts);

#endif
