
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
static const point_t ref_pts[MAX_SAMPLES] = {
    {0, 0},
    {0, 0},
    {0, 0}
};
static int ref_pt_idx;

screen_t calib_gui = {
    .on_paint          = calib_paint,
    .on_raw_touch_down = calib_raw_touch,
    .on_touch_down     = calib_touch,
    .on_touch_up       = calib_touch_up,
};

static void
calib_paint()
{
  point_t* ref_pt = &ref_pts[ref_pt_idx];
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
}

static void
calib_touch(uint16_t x, uint16_t y)
{
  terminal_write("\n\ncalibrated touch\n");
  terminal_write("x: ");
  terminal_write_int(x);
  terminal_write("\ny: ");
  terminal_write_int(y);

  fillCircle(x, y, 25);
}

static void
calib_touch_up(uint16_t x, uint16_t y)
{
  terminal_write("\ntouch up\n");
}

