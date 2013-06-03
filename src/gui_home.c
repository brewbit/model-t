
#include "gui_home.h"
#include "lcd.h"
#include "gui/button.h"


static void
home_screen_paint(paint_event_t* event);


static const widget_class_t home_widget_class = {
    .on_paint = home_screen_paint
};

void
calib_complete(void)
{
  widget_t* home_screen = home_screen_create();
  gui_set_screen(home_screen);
}

void
start_calib(click_event_t* event)
{
  widget_t* calib_screen = calib_screen_create(calib_complete);
  gui_set_screen(calib_screen);
}

widget_t*
home_screen_create()
{
  widget_t* w = widget_create(&home_widget_class, NULL, display_rect);

  rect_t rect = {
      .x = 0,
      .y = 240-43,
      .width = 159,
      .height = 43,
  };
  widget_t* calib_button = button_create(rect, "Calibrate", img_hand, start_calib);
  widget_add_child(w, calib_button);

  return w;
}

static void
home_screen_paint(paint_event_t* event)
{
  (void)event;

  tile_bitmap(img_background, display_rect);

  point_t anchor = {0, 0};
  set_bg_img(img_background, anchor);
  setColor(COLOR(0xD0, 0x25, 0x41));
  setFont(font_test_large);
  Extents_t extents = font_text_extents(font_test_large, "76.3 C");
  print("76.3 C",
      (DISP_WIDTH / 2) - (extents.width / 2),
      15);
}
