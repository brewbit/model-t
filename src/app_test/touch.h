
#ifndef TOUCH_H
#define TOUCH_H

#include "common.h"

/* This arrangement of values facilitates
 *  calculations within getDisplayPoint()
 */
typedef struct {
  int32_t An;     /* A = An/Divider */
  int32_t Bn;     /* B = Bn/Divider */
  int32_t Cn;     /* C = Cn/Divider */
  int32_t Dn;     /* D = Dn/Divider */
  int32_t En;     /* E = En/Divider */
  int32_t Fn;     /* F = Fn/Divider */
  int32_t Divider;
} matrix_t;

typedef struct {
  bool touch_down;
  point_t raw;
  point_t calib;
} touch_msg_t;


void
touch_init(void);

void
touch_calibrate(
    const point_t* ref_pts,
    const point_t* sampled_pts);

void
touch_calib_reset(void);

#endif
