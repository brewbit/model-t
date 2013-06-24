
#include "gui_update.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"
#include "gfx.h"


typedef struct {
  widget_t* widget;
} update_screen_t;


static void update_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);


widget_class_t update_screen_widget_class = {
    .on_destroy = update_screen_destroy
};


widget_t*
update_screen_create()
{
  update_screen_t* s = calloc(1, sizeof(update_screen_t));
  s->widget = widget_create(NULL, &update_screen_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, BLACK, NULL, NULL, NULL, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Update", font_opensans_22, WHITE, 1);

  return s->widget;
}

static void
update_screen_destroy(widget_t* w)
{
  update_screen_t* s = widget_get_instance_data(w);

  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  (void)event;

  gui_pop_screen();
}
