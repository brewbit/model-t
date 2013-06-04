
#include "gui_home.h"
#include "lcd.h"
#include "gui.h"
#include "gui/button.h"

#include <string.h>
#include <stdio.h>

typedef struct {
  float cur_temp;
  systime_t temp_timestamp;
  char temp_unit;

  widget_t* screen;
  widget_t* calib_button;
  widget_t* settings_button;
} home_screen_t;


static void home_screen_touch(touch_event_t* event);
static void home_screen_paint(paint_event_t* event);
static void home_screen_destroy(widget_t* w);

static void start_calib(click_event_t* event);
static void calib_complete(void);


static const widget_class_t home_widget_class = {
    .on_touch   = home_screen_touch,
    .on_paint   = home_screen_paint,
    .on_destroy = home_screen_destroy,
};

widget_t*
home_screen_create()
{
  home_screen_t* s = calloc(1, sizeof(home_screen_t));
  s->cur_temp = 73.2;
  s->temp_timestamp = chTimeNow();
  s->temp_unit = 'F';

  s->screen = widget_create(&home_widget_class, s, display_rect);

  rect_t rect = {
      .x = 0,
      .y = 240-43,
      .width = 160,
      .height = 43,
  };
  s->calib_button = button_create(rect, "Calibrate", img_hand, start_calib);
  rect.x = 160;
  s->settings_button = button_create(rect, "Settings", img_sliders, start_calib);

  widget_add_child(s->screen, s->calib_button);
  widget_add_child(s->screen, s->settings_button);

  widget_hide(s->calib_button);
  widget_hide(s->settings_button);

  return s->screen;
}

void
home_screen_destroy(widget_t* w)
{
  home_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
home_screen_touch(touch_event_t* event)
{
  home_screen_t* s = widget_get_instance_data(event->widget);

  widget_show(s->calib_button);
  widget_show(s->settings_button);
  widget_invalidate(s->screen);
}

static void
home_screen_paint(paint_event_t* event)
{
  char temp_str[16];
  home_screen_t* s = widget_get_instance_data(event->widget);

  if (chTimeNow() - s->temp_timestamp < MS2ST(5))
    sprintf(temp_str, "%0.1f %c", s->cur_temp, s->temp_unit);
  else
    sprintf(temp_str, "--.- %c", s->temp_unit);

  tile_bitmap(img_background, display_rect);

  point_t anchor = {0, 0};
  set_bg_img(img_background, anchor);
  setColor(COLOR(0xD0, 0x25, 0x41));
  setFont(font_test_large);
  Extents_t extents = font_text_extents(font_test_large, temp_str);
  print(temp_str,
      (DISP_WIDTH / 2) - (extents.width / 2),
      15);
}

static void
start_calib(click_event_t* event)
{
  (void)event;

  widget_t* calib_screen = calib_screen_create(calib_complete);
  gui_set_screen(calib_screen);
}

static void
calib_complete(void)
{
  widget_t* home_screen = home_screen_create();
  gui_set_screen(home_screen);
}
