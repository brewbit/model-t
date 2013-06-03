
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
static screen_t* calib_screen;
static touch_handler_t touch_handler = {
    .on_touch = calib_raw_touch
};
static widget_t calib_widget;
static point_t last_touch_pos;

void
calib_start()
{
  calib_screen = screen_create();
  screen_set_bg_img(calib_screen, img_background);


  widget_init(&calib_widget, calib_screen->widget.rect);
  calib_widget.on_paint = calib_widget_paint;
  calib_widget.on_touch_down = calib_widget_touch;

  widget_add_child((widget_t*)calib_screen, (widget_t*)&calib_widget);

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
    last_touch_pos.x = event->x;
    last_touch_pos.y = event->y;
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
    widget_invalidate((widget_t*)calib_screen);
  }
  else {
    calib_complete = 1;
    touch_calibrate(ref_pts, sampled_pts);
  }
}

