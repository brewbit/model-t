
#include "gui_calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "touch.h"
#include "gfx.h"
#include "image.h"
#include "gui/button.h"


#define NUM_SAMPLES_PER_POINT 128


typedef struct {
  int ref_pt_idx;
  uint8_t calib_complete;
  point_t sampled_pts[NUM_CALIB_POINTS][NUM_SAMPLES_PER_POINT];
  int sample_idx;
  point_t last_touch_pos;
  calib_complete_handler_t completion_handler;

  widget_t* widget;
  widget_t* recal_button;
  widget_t* complete_button;
} calib_screen_t;


static void calib_widget_touch(touch_event_t* event);
static void calib_widget_paint(paint_event_t* event);
static void calib_widget_msg(msg_event_t* event);
static void calib_widget_destroy(widget_t* w);

static void restart_calib(click_event_t* event);
static void complete_calib(click_event_t* event);

static void calib_raw_touch(calib_screen_t* s, bool touch_down, point_t raw);
static void calib_touch_down(calib_screen_t* s, point_t p);
static void calib_touch_up(calib_screen_t* s, point_t p);


static const point_t ref_pts[NUM_CALIB_POINTS] = {
    {  50,  50 },
    { 270, 120 },
    { 170, 190 },
};

static const widget_class_t calib_widget_class = {
    .on_paint   = calib_widget_paint,
    .on_touch   = calib_widget_touch,
    .on_msg     = calib_widget_msg,
    .on_destroy = calib_widget_destroy,
};

widget_t*
calib_screen_create(calib_complete_handler_t completion_handler)
{
  calib_screen_t* calib_screen = calloc(1, sizeof(calib_screen_t));
  calib_screen->completion_handler = completion_handler;
  calib_screen->widget = widget_create(NULL, &calib_widget_class, calib_screen, display_rect);

  rect_t rect = {
      .x = 0,
      .y = 240-43,
      .width = 160,
      .height = 43,
  };
  calib_screen->recal_button = button_create(calib_screen->widget, rect, "Retry", img_thumbs_down, WHITE, restart_calib);
  widget_hide(calib_screen->recal_button);
  rect.x = 160;
  calib_screen->complete_button = button_create(calib_screen->widget, rect, "Accept", img_thumbs_up, WHITE, complete_calib);
  widget_hide(calib_screen->complete_button);

  gui_msg_subscribe(MSG_TOUCH_INPUT, calib_screen->widget);

  return calib_screen->widget;
}

static void
calib_widget_destroy(widget_t* w)
{
  calib_screen_t* s = widget_get_instance_data(w);
  gui_msg_unsubscribe(MSG_TOUCH_INPUT, s->widget);
  free(s);
}

static void
calib_widget_msg(msg_event_t* event)
{
  calib_screen_t* s = widget_get_instance_data(event->widget);

  if (event->msg_id == MSG_TOUCH_INPUT) {
    touch_msg_t* msg = event->msg_data;
    calib_raw_touch(s, msg->touch_down, msg->raw);
  }
}

static void
restart_calib(click_event_t* event)
{
  widget_t* screen_widget = widget_get_parent(event->widget);
  calib_screen_t* s = widget_get_instance_data(screen_widget);
  s->calib_complete = false;
  s->sample_idx = 0;
  s->ref_pt_idx = 0;

  widget_hide(s->recal_button);
  widget_hide(s->complete_button);
  widget_invalidate(screen_widget);
}

static void
complete_calib(click_event_t* event)
{
  widget_t* screen_widget = widget_get_parent(event->widget);
  calib_screen_t* s = widget_get_instance_data(screen_widget);
  if (s->completion_handler)
    s->completion_handler(screen_widget);
}

static void
calib_widget_paint(paint_event_t* event)
{
  calib_screen_t* s = widget_get_instance_data(event->widget);
  const point_t* marker_pos;

  gfx_tile_bitmap(img_squares, display_rect);

  if (!s->calib_complete) {
    marker_pos = &ref_pts[s->ref_pt_idx];

    if (s->sample_idx == 0)
      gfx_set_fg_color(YELLOW);
    else if (s->sample_idx < NUM_SAMPLES_PER_POINT)
      gfx_set_fg_color(RED);
    else
      gfx_set_fg_color(GREEN);
  }
  else {
    marker_pos = &s->last_touch_pos;
    gfx_set_fg_color(COBALT);
  }
  rect_t r = {marker_pos->x, marker_pos->y, 25, 25};
  gfx_fill_rect(r);
}

static void
calib_widget_touch(touch_event_t* event)
{
  calib_screen_t* s = widget_get_instance_data(event->widget);

  if (s->calib_complete) {
    s->last_touch_pos = event->pos;
    widget_invalidate(event->widget);
  }
}

static void
calib_raw_touch(calib_screen_t* s, bool touch_down, point_t raw)
{
  if (!s->calib_complete) {
    if (touch_down)
      calib_touch_down(s, raw);
    else
      calib_touch_up(s, raw);
  }
}

static void
calib_touch_down(calib_screen_t* s, point_t p)
{
  s->sampled_pts[s->ref_pt_idx][s->sample_idx % NUM_SAMPLES_PER_POINT] = p;
  s->sample_idx++;

  if ((s->sample_idx == 1) ||
      (s->sample_idx == NUM_SAMPLES_PER_POINT))
    widget_invalidate(s->widget);
}

static void
calib_touch_up(calib_screen_t* s, point_t p)
{
  (void)p;

  if (++s->ref_pt_idx < NUM_CALIB_POINTS) {
    s->sample_idx = 0;
    widget_invalidate(s->widget);
  }
  else {
    int i,j;
    s->calib_complete = 1;
    point_t avg_sample[3];
    for (i = 0; i < 3; ++i) {
      avg_sample[i].x = 0;
      avg_sample[i].y = 0;
      for (j = 0; j < NUM_SAMPLES_PER_POINT; ++j) {
        avg_sample[i].x += s->sampled_pts[i][j].x;
        avg_sample[i].y += s->sampled_pts[i][j].y;
      }
      avg_sample[i].x /= NUM_SAMPLES_PER_POINT;
      avg_sample[i].y /= NUM_SAMPLES_PER_POINT;
    }
    touch_calibrate(ref_pts, avg_sample);

    widget_show(s->recal_button);
    widget_show(s->complete_button);
  }
}

