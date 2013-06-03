
#include "gui_calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"
#include "image.h"


#define NUM_SAMPLES_PER_POINT 32


static void calib_widget_touch(touch_event_t* event);
static void calib_widget_paint(paint_event_t* event);

static void calib_raw_touch(bool touch_down, point_t raw, point_t calib);
static void calib_touch_down(point_t p);
static void calib_touch_up(point_t p);


static const point_t ref_pts[NUM_CALIB_POINTS] = {
    {  50,  50 },
    { 270, 120 },
    { 170, 190 },
};

static int ref_pt_idx;
static uint8_t calib_complete;
static point_t sampled_pts[NUM_CALIB_POINTS][NUM_SAMPLES_PER_POINT];
static int sample_idx;
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

    if (sample_idx == 0)
      setColor(YELLOW);
    else if (sample_idx < NUM_SAMPLES_PER_POINT)
      setColor(RED);
    else
      setColor(GREEN);
  }
  else {
    marker_pos = &last_touch_pos;
    setColor(BLUE);
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
  sampled_pts[ref_pt_idx][sample_idx % NUM_SAMPLES_PER_POINT] = p;
  sample_idx++;
  widget_invalidate(calib_screen);
}

static void
calib_touch_up(point_t p)
{
  (void)p;

  if (++ref_pt_idx < NUM_CALIB_POINTS) {
    sample_idx = 0;
    widget_invalidate(calib_screen);
  }
  else {
    int i,j;
    calib_complete = 1;
    point_t avg_sample[3];
    for (i = 0; i < 3; ++i) {
      avg_sample[i].x = 0;
      avg_sample[i].y = 0;
      for (j = 0; j < NUM_SAMPLES_PER_POINT; ++j) {
        avg_sample[i].x += sampled_pts[i][j].x;
        avg_sample[i].y += sampled_pts[i][j].y;
      }
      avg_sample[i].x /= NUM_SAMPLES_PER_POINT;
      avg_sample[i].y /= NUM_SAMPLES_PER_POINT;
    }
    touch_calibrate(ref_pts, avg_sample);
  }
}

