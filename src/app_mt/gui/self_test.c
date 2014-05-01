
#include <ch.h>
#include <hal.h>

#include "gui/self_test.h"
#include "label.h"
#include "gui.h"
#include "gfx.h"
#include "temp_control.h"
#include "button.h"
#include "touch.h"
#include "wlan.h"
#include "net.h"
#include "app_cfg.h"

#include <stdio.h>
#include <string.h>


typedef struct {
  widget_t* sensor_test_status[NUM_SENSORS];
  widget_t* relay_test_status[NUM_OUTPUTS];
  widget_t* wifi_test_status;
  widget_t* recovery_img_status;
  widget_t* touch_status;
  Thread* thd_test;
} self_test_screen_t;


static void
self_test_screen_destroy(widget_t* w);

static void
self_test_screen_msg(msg_event_t* event);

static void
back_button_clicked(button_event_t* event);

static void
dispatch_touch(self_test_screen_t* s, touch_msg_t* msg);

static void
dispatch_sensor_sample(self_test_screen_t* s, sensor_msg_t* msg);

static void
dispatch_recovery_img_status(self_test_screen_t* s, recovery_img_load_state_t* state);

static void
dispatch_net_status(self_test_screen_t* s, net_status_t* status);

static void
dispatch_ping_report(self_test_screen_t* s, netapp_pingreport_args_t* ping_report);

static void
relay_test(widget_t* label, uint32_t write_pad, uint32_t read_pad);

static msg_t
test_thread(void* arg);


static const widget_class_t self_test_widget_class = {
    .on_destroy = self_test_screen_destroy,
    .on_msg = self_test_screen_msg
};


widget_t*
self_test_screen_create()
{
  self_test_screen_t* s = calloc(1, sizeof(self_test_screen_t));
  widget_t* widget = widget_create(NULL, &self_test_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 35;
  rect.width = 220;
  label_create(widget, rect, "Self Test", font_opensans_regular_22, WHITE, 1);

  rect.y = 90;
  rect.x = 30;
  rect.width = 120;
  label_create(widget, rect, "WiFi Test:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->wifi_test_status = label_create(widget, rect, "NOT STARTED", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Recovery Image:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->recovery_img_status = label_create(widget, rect, "NOT CHECKED", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Sensor 1:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->sensor_test_status[SENSOR_1] = label_create(widget, rect, "NO DATA", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Sensor 2:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->sensor_test_status[SENSOR_2] = label_create(widget, rect, "NO DATA", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Relay 1:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->relay_test_status[OUTPUT_1] = label_create(widget, rect, "NOT STARTED", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Relay 2:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->relay_test_status[OUTPUT_2] = label_create(widget, rect, "NOT STARTED", font_opensans_regular_12, WHITE, 1);

  rect.x = 30;
  rect.y += 20;
  label_create(widget, rect, "Touchscreen:", font_opensans_regular_12, WHITE, 1);
  rect.x = 160;
  s->touch_status = label_create(widget, rect, "NO DATA", font_opensans_regular_12, WHITE, 1);

  gui_msg_subscribe(MSG_TOUCH_INPUT, widget);
  gui_msg_subscribe(MSG_SENSOR_SAMPLE, widget);
  gui_msg_subscribe(MSG_RECOVERY_IMG_STATUS, widget);
  gui_msg_subscribe(MSG_NET_STATUS, widget);
  gui_msg_subscribe(MSG_WLAN_PING_REPORT, widget);

  s->thd_test = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, test_thread, s);

  return widget;
}

static msg_t
test_thread(void* arg)
{
  self_test_screen_t* s = arg;

  // Wait for temp control threads to start up and stop messing
  // with the relays...
  chThdSleepSeconds(2);

  net_connect("internets", WLAN_SEC_WPA2, "password");

  while (!chThdShouldTerminate()) {
    relay_test(s->relay_test_status[OUTPUT_1], PAD_RELAY1, PAD_RELAY1_TEST);
    chThdSleepSeconds(1);
    relay_test(s->relay_test_status[OUTPUT_2], PAD_RELAY2, PAD_RELAY2_TEST);
    chThdSleepSeconds(1);
  }

  return 0;
}

static void
relay_test(widget_t* label, uint32_t write_pad, uint32_t read_pad)
{
  color_t color = GREEN;
  char* result = "PASS";

  palSetPad(GPIOC, write_pad);
  chThdSleepMilliseconds(100);
  if (palReadPad(GPIOE, read_pad) != 1) {
    result = "FAIL";
    color = RED;
  }

  palClearPad(GPIOC, write_pad);
  chThdSleepMilliseconds(100);
  if (palReadPad(GPIOE, read_pad) != 0) {
    result = "FAIL";
    color = RED;
  }

  label_set_text(label, result);
  label_set_color(label, color);
}

static void
self_test_screen_destroy(widget_t* w)
{
  self_test_screen_t* s = widget_get_instance_data(w);

  chThdTerminate(s->thd_test);
  chThdWait(s->thd_test);

  gui_msg_unsubscribe(MSG_TOUCH_INPUT, w);
  gui_msg_unsubscribe(MSG_SENSOR_SAMPLE, w);
  gui_msg_unsubscribe(MSG_RECOVERY_IMG_STATUS, w);
  gui_msg_unsubscribe(MSG_NET_STATUS, w);
  gui_msg_unsubscribe(MSG_WLAN_PING_REPORT, w);

  free(s);
}

static void
self_test_screen_msg(msg_event_t* event)
{
  self_test_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
    case MSG_TOUCH_INPUT:
      dispatch_touch(s, event->msg_data);
      break;

    case MSG_SENSOR_SAMPLE:
      dispatch_sensor_sample(s, event->msg_data);
      break;

    case MSG_RECOVERY_IMG_STATUS:
      dispatch_recovery_img_status(s, event->msg_data);
      break;

    case MSG_NET_STATUS:
      dispatch_net_status(s, event->msg_data);
      break;

    case MSG_WLAN_PING_REPORT:
      dispatch_ping_report(s, event->msg_data);
      break;

    default:
      break;
  }
}

static void
dispatch_touch(self_test_screen_t* s, touch_msg_t* msg)
{
  char str[32];
  snprintf(str, 32, "(%d, %d)", (int)msg->calib.x, (int)msg->calib.y);
  label_set_text(s->touch_status, str);
  label_set_color(s->touch_status, GREEN);
}

static void
dispatch_sensor_sample(self_test_screen_t* s, sensor_msg_t* msg)
{
  char str[32];
  snprintf(str, 32, "%f", msg->sample.value);
  label_set_text(s->sensor_test_status[msg->sensor], str);
  label_set_color(s->sensor_test_status[msg->sensor], GREEN);
}

static void
dispatch_recovery_img_status(self_test_screen_t* s, recovery_img_load_state_t* state)
{
  switch (*state) {
    case RECOVERY_IMG_CHECKING:
      label_set_text(s->recovery_img_status, "CHECKING");
      break;

    case RECOVERY_IMG_LOADING:
      label_set_text(s->recovery_img_status, "LOADING");
      break;

    case RECOVERY_IMG_LOADED:
      label_set_text(s->recovery_img_status, "LOADED");
      label_set_color(s->recovery_img_status, GREEN);
      break;

    case RECOVERY_IMG_FAILED:
      label_set_text(s->recovery_img_status, "FAILED");
      label_set_color(s->recovery_img_status, RED);
      break;

    default:
      break;
  }
}

static void
dispatch_net_status(self_test_screen_t* s, net_status_t* status)
{
  switch (status->net_state) {
    case NS_DISCONNECTED:
      label_set_text(s->wifi_test_status, "DISCONNECTED");
      break;

    case NS_CONNECT:
    case NS_CONNECTING:
      label_set_text(s->wifi_test_status, "CONNECTING");
      break;

    case NS_CONNECTED:
      label_set_text(s->wifi_test_status, "CONNECTED");
      break;

    case NS_CONNECT_FAILED:
      label_set_text(s->wifi_test_status, "FAILED");
      break;

    default:
      break;
  }
}

static void
dispatch_ping_report(self_test_screen_t* s, netapp_pingreport_args_t* ping_report)
{
  if ((ping_report->packets_sent > 0) &&
      (ping_report->packets_received > 0)) {
    label_set_text(s->wifi_test_status, "PASS");
    label_set_color(s->wifi_test_status, GREEN);

    // Clear out the test net settings
    net_settings_t settings;
    memset(&settings, 0, sizeof(settings));
    app_cfg_set_net_settings(&settings);
    app_cfg_flush();
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}
