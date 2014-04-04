
#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

typedef enum {
  UNIT_NONE,
  UNIT_TEMP_DEG_F,
  UNIT_TEMP_DEG_C,
  UNIT_TIME_SEC,
  UNIT_TIME_MIN,
  UNIT_TIME_HOUR,
  UNIT_TIME_DAY
} unit_t;

typedef struct {
  float value;
  unit_t unit;
} quantity_t;


static inline quantity_t
quantity_convert(quantity_t q, unit_t unit)
{
  if (q.unit == unit)
    return q;

  switch (unit) {
  case UNIT_TEMP_DEG_C:
    if (q.unit == UNIT_TEMP_DEG_F)
      q.value = (5.0f / 9.0f) * (q.value - 32);
    break;

  case UNIT_TEMP_DEG_F:
    if (q.unit == UNIT_TEMP_DEG_C)
      q.value = ((9.0f / 5.0f) * q.value) + 32;
    break;

    /* Can't convert any other quantities */
  default:
    break;
  }

  q.unit = unit;

  return q;
}


typedef uint16_t color_t;


typedef struct {
  int32_t width;
  int32_t height;
} Extents_t;

typedef struct {
  int32_t x;
  int32_t y;
} point_t;

//inline float
//point_dist_sq(point_t p1, point_t p2)
//{
//
//}
//
//inline float
//point_dist(point_t p1, point_t p2)
//{
//  sqrt(point_dist_sq());
//}


typedef struct {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} rect_t;

static inline bool
rect_inside(rect_t r, point_t p)
{
  return ((p.x >= r.x) &&
          (p.x <= (r.x + r.width)) &&
          (p.y >= r.y) &&
          (p.y <= (r.y + r.height)));
}

static inline point_t
rect_center(rect_t r)
{
  point_t center = {
      .x = r.x + (r.width/2),
      .y = r.y + (r.height/2),
  };
  return center;
}

#endif
