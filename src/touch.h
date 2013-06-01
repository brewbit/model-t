
#ifndef TOUCH_H
#define TOUCH_H

#include "common.h"


void
touch_init(void);

void
touch_calibrate(
    const point_t* ref_pts,
    const point_t* sampled_pts);

#endif
