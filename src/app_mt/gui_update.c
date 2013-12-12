
#include "gui_update.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui/progressbar.h"
#include "gui.h"
#include "gfx.h"

#include <string.h>


typedef enum {
  UPDATE_CHECKING_VERSION,
  UPDATE_AVAILABLE,
  UPDATE_NOT_AVAILABLE,
  UPDATE_DOWNLOADING,
  UPDATE_APPLYING
} update_state_t;


typedef struct {
  widget_t* widget;
  widget_t* button;
  widget_t* header_label;
  widget_t* desc_label;
  widget_t* progress;

  update_state_t state;
  VirtualTimer timer;
} update_screen_t;


static void update_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void update_button_clicked(button_event_t* event);
static void update_ui(update_screen_t* s);
static void update_state(void* arg);


widget_class_t update_screen_widget_class = {
    .on_destroy = update_screen_destroy
};


widget_t*
update_screen_create()
{
  update_screen_t* s = chHeapAlloc(NULL, sizeof(update_screen_t));
  memset(s, 0, sizeof(update_screen_t));

  s->state = UPDATE_CHECKING_VERSION;

  s->widget = widget_create(NULL, &update_screen_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "OTA Update", font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 85;
  rect.width = 300;
  rect.height = 66;
  s->button = button_create(s->widget, rect, NULL, WHITE, BLACK, update_button_clicked);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  icon_create(s->button, rect, img_update, WHITE, CYAN);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->header_label = label_create(s->button, rect, "", font_opensans_regular_18, WHITE, 1);

  rect.y = 30;
  s->desc_label = label_create(s->button, rect, "", font_opensans_regular_12, WHITE, 2);

  rect.x = 50;
  rect.y = 170;
  rect.width = 220;
  rect.height = 50;
  s->progress = progressbar_create(s->widget, rect, CYAN, ORANGE);
  widget_hide(s->progress);

  update_ui(s);

  return s->widget;
}

static void
update_screen_destroy(widget_t* w)
{
  update_screen_t* s = widget_get_instance_data(w);

  chHeapFree(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
update_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    update_screen_t* s = widget_get_instance_data(w);

    if (s->state == UPDATE_AVAILABLE) {
      s->state = UPDATE_DOWNLOADING;
      update_ui(s);
    }
  }
}

static void
update_ui(update_screen_t* s)
{
  char* header = NULL;
  char* desc = NULL;
  int delay = 0;

  switch (s->state) {
  case UPDATE_CHECKING_VERSION:
    header = "Checking for updates";
    desc = "Contacting server...";
    delay = 4000;
    s->state = UPDATE_NOT_AVAILABLE;
    break;

  case UPDATE_AVAILABLE:
    header = "Update available!";
    desc = "Touch here to apply the update.";
    break;

  case UPDATE_NOT_AVAILABLE:
    header = "You're up to date!";
    desc = "There are currently no updates.";
    break;

  case UPDATE_DOWNLOADING:
    header = "Downloading update";
    desc = "Do not remove power during update!";

    widget_show(s->progress);
    if (progressbar_get_progress(s->progress) < 100) {
      progressbar_set_progress(s->progress, progressbar_get_progress(s->progress) + 1);
    }
    else {
      progressbar_set_progress(s->progress, 0);
      s->state = UPDATE_APPLYING;
    }
    delay = 50;
    break;

  case UPDATE_APPLYING:
    header = "Applying update";
    desc = "Do not remove power during update!";

    widget_show(s->progress);
    if (progressbar_get_progress(s->progress) < 100) {
      progressbar_set_progress(s->progress, progressbar_get_progress(s->progress) + 1);
      delay = 100;
    }
    else {
      gui_pop_screen();
      gui_pop_screen();
    }
    break;

  default:
    break;
  }

  label_set_text(s->header_label, header);
  label_set_text(s->desc_label, desc);

  if (delay != 0)
    chVTSetI(&s->timer, MS2ST(delay), update_state, s);
}

static void
update_state(void* arg)
{
  update_screen_t* s = arg;
  update_ui(s);
}
