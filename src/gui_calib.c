
#include "gui_calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"


static void
calib_paint(void);

static void
calib_raw_touch(uint16_t x, uint16_t y);

static void
calib_touch(uint16_t x, uint16_t y);

static void
calib_touch_up(uint16_t x, uint16_t y);


static int ref_pt_idx;
static uint8_t calib_complete;

static const point_t ref_pts[MAX_SAMPLES] = {
    {  50,  50 },
    { 270, 120 },
    { 170, 190 },
};
static point_t sampled_pts[MAX_SAMPLES];

screen_t calib_gui = {
    .on_paint          = calib_paint,
    .on_raw_touch_down = calib_raw_touch,
    .on_touch_down     = calib_touch,
    .on_touch_up       = calib_touch_up,
};

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
}

static void
calib_touch(uint16_t x, uint16_t y)
{
  if (calib_complete) {
    terminal_write("\n\ncalibrated touch\n");
    terminal_write("x: ");
    terminal_write_int(x);
    terminal_write("\ny: ");
    terminal_write_int(y);

    fillCircle(x, y, 25);
  }
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
    touch_calibrate(ref_pts, sampled_pts);
  }
}

