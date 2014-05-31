
#include "gui/update.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "progressbar.h"
#include "gui.h"
#include "gfx.h"
#include "ota_update.h"

#include <string.h>
#include <stdio.h>


typedef struct {
  widget_t* widget;
  widget_t* button;
  widget_t* header_label;
  widget_t* desc_label;
  widget_t* progress;
} update_screen_t;



static void update_screen_msg(msg_event_t* event);
static void update_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void update_button_clicked(button_event_t* event);
static void dispatch_update_status(update_screen_t* s, const ota_update_status_t* status);


static const widget_class_t update_screen_widget_class = {
    .on_msg     = update_screen_msg,
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

  ota_update_status_t us = ota_update_get_status();
  dispatch_update_status(s, &us);

  gui_msg_subscribe(MSG_OTAU_STATUS, s->widget);

  return s->widget;
}

static void
update_screen_destroy(widget_t* w)
{
  update_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_OTAU_STATUS, s->widget);

  free(s);
}

static void
update_screen_msg(msg_event_t* event)
{
  update_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_OTAU_STATUS:
    dispatch_update_status(s, event->msg_data);
    break;

  default:
    break;
  }
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
    ota_update_status_t us = ota_update_get_status();

    switch (us.state) {
    case OU_IDLE:
    case OU_UPDATE_NOT_AVAILABLE:
    case OU_FAILED:
      msg_post(MSG_OTAU_CHECK, NULL);
      break;

    case OU_UPDATE_AVAILABLE:
      msg_post(MSG_OTAU_START, NULL);
      break;

    default:
      break;
    }
  }
}

static void
dispatch_update_status(update_screen_t* s, const ota_update_status_t* status)
{
  char* header = NULL;
  char* desc = NULL;
  char* formatted_str = NULL;

  switch (status->state) {
  case OU_IDLE:
    header = "Check";
    desc = "Touch here to check for updates";
    break;

  case OU_WAIT_API_CONN:
    header = "Not Connected";
    desc = "Waiting for account connection";
    break;

  case OU_CHECKING:
    header = "Checking";
    desc = "Contacting server to check for updates...";
    break;

  case OU_UPDATE_AVAILABLE:
    header = "Update available!";
    desc = formatted_str = malloc(256);
    snprintf(formatted_str, 256, "Version %s is available. Touch here to download.",
        status->update_ver);
    break;

  case OU_UPDATE_NOT_AVAILABLE:
    header = "You're up to date!";
    desc = "No updates found. Touch to check again";
    break;

  case OU_CHUNK_TIMEOUT:
    header = "Downloading";
    desc = "Request timed out. Retrying.";
    break;

  case OU_DOWNLOADING:
  {
    int percent_complete = (100 * status->update_downloaded) / status->update_size;
    header = "Downloading";
    desc = formatted_str = malloc(256);
    snprintf(formatted_str, 256, "Received %d / %d bytes (%d%% complete)",
        (int)status->update_downloaded,
        (int)status->update_size,
        percent_complete);

    widget_show(s->progress);
    progressbar_set_progress(s->progress, percent_complete);
    break;
  }

  case OU_COMPLETE:
    header = "Installing";
    desc = "Do not remove power during update!";
    widget_hide(s->progress);
    break;

  case OU_FAILED:
    header = "Update failed!";
    desc = formatted_str = malloc(256);
    snprintf(formatted_str, 256, "Error code: %d. Touch to try again.",
        status->error_code);
    break;

  default:
    break;
  }

  label_set_text(s->header_label, header);
  label_set_text(s->desc_label, desc);

  if (formatted_str != NULL)
    free(formatted_str);
}
