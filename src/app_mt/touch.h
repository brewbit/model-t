
#ifndef TOUCH_H
#define TOUCH_H

#include "types.h"


typedef struct {
  bool touch_down;
  point_t raw;
  point_t calib;
} touch_msg_t;


void
touch_init(void);

void
touch_set_calib(
    const point_t* ref_pts,
    const point_t* sampled_pts);

void
touch_save_calib(void);

void
touch_calib_reset(void);

#endif
