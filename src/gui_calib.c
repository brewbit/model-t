
#include "gui_calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "terminal.h"

typedef enum {
  CALIB_
} calib_state_t;


static void
calib_paint(void);

static void
calib_raw_touch(uint16_t x, uint16_t y);

static void
calib_touch(uint16_t x, uint16_t y);

static void
calib_touch_up(uint16_t x, uint16_t y);


static calib_state_t calib_state;
static int ref_pt_idx;
static uint8_t calib_complete;

static const point_t ref_pts[MAX_SAMPLES] = {
    {  50,  50 },
    { 270, 120 },
    { 170, 190 },
};
static point_t sampled_pts[MAX_SAMPLES];
static matrix_t calib_matrix;

//static const point_t perfect_screen_pts[MAX_SAMPLES] = {
//    { 650, 370 },
//    { 290, 160 },
//    { 430, 120 },
//};

screen_t calib_gui = {
    .on_paint          = calib_paint,
    .on_raw_touch_down = calib_raw_touch,
    .on_touch_down     = calib_touch,
    .on_touch_up       = calib_touch_up,
};

//void
//calib_init()
//{
//
//}

static void
calib_paint()
{
  const point_t* ref_pt = &ref_pts[ref_pt_idx];
  fillCircle(ref_pt->x, ref_pt->y, 25);
}

static void
calib_raw_touch(uint16_t x, uint16_t y)
{
  terminal_clear();
  terminal_write("raw touch\n");
  terminal_write("x: ");
  terminal_write_int(x);
  terminal_write("\ny: ");
  terminal_write_int(y);

  if (!calib_complete) {
    sampled_pts[ref_pt_idx].x = x;
    sampled_pts[ref_pt_idx].y = y;
  }
  else {
    point_t calib;
    point_t raw = {
        .x = x,
        .y = y,
    };
    getDisplayPoint(
        &calib,
        &raw,
        &calib_matrix);

    terminal_write("\n\ncalibrated touch\n");
    terminal_write("x: ");
    terminal_write_int(calib.x);
    terminal_write("\ny: ");
    terminal_write_int(calib.y);

    fillCircle(calib.x, calib.y, 25);
  }
}

static void
calib_touch(uint16_t x, uint16_t y)
{

}

static void
calib_touch_up(uint16_t x, uint16_t y)
{
  terminal_write("\ntouch up\n");
  if (++ref_pt_idx < MAX_SAMPLES) {
    calib_paint();
  }
  else {
    calib_complete = 1;
    setCalibrationMatrix(ref_pts, sampled_pts, &calib_matrix);
  }
}

