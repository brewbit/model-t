
#include "gui/calib.h"
#include "gui.h"
#include "touch_calib.h"
#include "touch.h"
#include "gfx.h"
#include "image.h"
#include "button.h"
#include "label.h"

#include <string.h>


#define MARKER_SIZE           50
#define NUM_SAMPLES_PER_POINT 32


typedef struct {
  int ref_pt_idx;
  uint8_t calib_complete;
  point_t sampled_pts[NUM_CALIB_POINTS][NUM_SAMPLES_PER_POINT];
  uint32_t sample_count[NUM_CALIB_POINTS];
  int sample_idx;
  point_t last_touch_pos;

  widget_t* widget;
  widget_t* recal_button;
  widget_t* complete_button;
  widget_t* lbl_instructions;
} calib_screen_t;


static void calib_widget_touch(touch_event_t* event);
static void calib_widget_paint(paint_event_t* event);
static void calib_widget_msg(msg_event_t* event);
static void calib_widget_destroy(widget_t* w);

static void restart_calib(button_event_t* event);
static void complete_calib(button_event_t* event);

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
calib_screen_create()
{
  calib_screen_t* s = calloc(1, sizeof(calib_screen_t));

  s->widget = widget_create(NULL, &calib_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->complete_button = button_create(s->widget, rect, img_left, WHITE, BLACK, complete_calib);
  widget_hide(s->complete_button);

  rect.x = 320 - 56 - 15;
  rect.y = 240 - 56 - 15;
  s->recal_button = button_create(s->widget, rect, img_update, WHITE, BLACK, restart_calib);
  widget_hide(s->recal_button);

  rect.x = 50;
  rect.y = 100;
  rect.width = 175;
  s->lbl_instructions = label_create(s->widget, rect, "Touch and hold the marker until it turns green", font_opensans_regular_18, WHITE, 3);

  gui_msg_subscribe(MSG_TOUCH_INPUT, s->widget);

  return s->widget;
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
restart_calib(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen_widget = widget_get_parent(event->widget);
    calib_screen_t* s = widget_get_instance_data(screen_widget);
    s->calib_complete = false;
    s->sample_idx = 0;
    s->ref_pt_idx = 0;
    memset(s->sample_count, 0, sizeof(s->sample_count));

    widget_show(s->lbl_instructions);
    widget_hide(s->recal_button);
    widget_hide(s->complete_button);
    widget_invalidate(screen_widget);
  }
}

static void
complete_calib(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    touch_save_calib();
    gui_pop_screen();
  }
}

static void
calib_widget_paint(paint_event_t* event)
{
  calib_screen_t* s = widget_get_instance_data(event->widget);
  const point_t* marker_pos;

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
  rect_t r = {
      marker_pos->x - (MARKER_SIZE / 2),
      marker_pos->y - (MARKER_SIZE / 2),
      MARKER_SIZE,
      MARKER_SIZE};
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
  if (s->sample_count[s->ref_pt_idx] < NUM_SAMPLES_PER_POINT)
    s->sample_count[s->ref_pt_idx] += 1;
  s->sample_idx++;

  if ((s->sample_idx == 1) ||
      (s->sample_idx == NUM_SAMPLES_PER_POINT))
    widget_invalidate(s->widget);
}

static void
calib_touch_up(calib_screen_t* s, point_t p)
{
  (void)p;

  if (s->sample_count[s->ref_pt_idx] < NUM_SAMPLES_PER_POINT)
    return;

  if (++s->ref_pt_idx < NUM_CALIB_POINTS) {
    s->sample_idx = 0;
    widget_invalidate(s->widget);
  }
  else {
    int i,j;
    s->calib_complete = 1;
    point_t avg_sample[NUM_CALIB_POINTS];
    for (i = 0; i < NUM_CALIB_POINTS; ++i) {
      avg_sample[i].x = 0;
      avg_sample[i].y = 0;
      for (j = 0; j < (int)s->sample_count[i]; ++j) {
        avg_sample[i].x += s->sampled_pts[i][j].x;
        avg_sample[i].y += s->sampled_pts[i][j].y;
      }
      avg_sample[i].x /= s->sample_count[i];
      avg_sample[i].y /= s->sample_count[i];
    }
    touch_set_calib(ref_pts, avg_sample);

    widget_hide(s->lbl_instructions);
    widget_show(s->recal_button);
    widget_show(s->complete_button);
  }
}

