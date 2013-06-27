
#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define INVALID_TEMP INT_MIN

typedef int32_t temperature_t; // temperature in 0.01 C per count

typedef enum {
  TEMP_C,
  TEMP_F
} temperature_unit_t;

#define DEGF(f) (temperature_t)((((f) - 32) * 100 * 5) / 9)
#define DEGC(c) (temperature_t)((c) * 100)

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
