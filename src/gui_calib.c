
#include "gui_calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"
#include "image.h"


static void calib_widget_touch(touch_event_t* event);
static void calib_widget_paint(paint_event_t* event);

static void calib_raw_touch(bool touch_down, point_t raw, point_t calib);
static void calib_touch_down(point_t p);
static void calib_touch_up(point_t p);


static const point_t ref_pts[MAX_SAMPLES] = {
    {  50,  50 },
    { 270, 120 },
    { 170, 190 },
};

static int ref_pt_idx;
static uint8_t calib_complete;
static point_t sampled_pts[MAX_SAMPLES];
static widget_t* calib_screen;
static widget_t* calib_widget;
static touch_handler_t touch_handler = {
    .on_touch = calib_raw_touch
};
static point_t last_touch_pos;
static const widget_class_t calib_widget_class = {
    .on_paint = calib_widget_paint,
    .on_touch = calib_widget_touch,
};

void
calib_start()
{
  calib_screen = screen_create();
  screen_set_bg_img(calib_screen, img_background);

  calib_widget = widget_create(&calib_widget_class, NULL, widget_get_rect(calib_screen));

  widget_add_child(calib_screen, calib_widget);

  gui_set_screen(calib_screen);

  touch_handler_register(&touch_handler);
}

static void
calib_widget_paint(paint_event_t* p)
{
  const point_t* marker_pos;

  (void)p;

  if (!calib_complete) {
    marker_pos = &ref_pts[ref_pt_idx];
  }
  else {
    marker_pos = &last_touch_pos;
  }
  fillCircle(marker_pos->x, marker_pos->y, 25);
}

static void
calib_widget_touch(touch_event_t* event)
{
  if (calib_complete) {
    last_touch_pos = event->pos;
    widget_invalidate(calib_screen);
  }
}

static void
calib_raw_touch(bool touch_down, point_t raw, point_t calib)
{
  (void)calib;

  if (!calib_complete) {
    if (touch_down)
      calib_touch_down(raw);
    else
      calib_touch_up(raw);
  }
  else {
    last_touch_pos = calib;
    widget_invalidate(calib_screen);
  }
}

static void
calib_touch_down(point_t p)
{
  sampled_pts[ref_pt_idx] = p;
}

static void
calib_touch_up(point_t p)
{
  (void)p;

  if (++ref_pt_idx < MAX_SAMPLES) {
    widget_invalidate(calib_screen);
  }
  else {
    calib_complete = 1;
    touch_calibrate(ref_pts, sampled_pts);
  }
}

