
#include "gui/info.h"
#include "button.h"
#include "label.h"
#include "listbox.h"
#include "gui.h"
#include "gfx.h"
#include "net.h"
#include "bootloader_api.h"

#include <string.h>


typedef struct {
  widget_t* widget;
} info_screen_t;



static void
info_screen_destroy(widget_t* w);

static void
back_button_clicked(button_event_t* event);

static void
add_info(widget_t* lb, const char* title, const char* version);


widget_class_t info_screen_widget_class = {
    .on_destroy = info_screen_destroy
};


widget_t*
info_screen_create()
{
  info_screen_t* s = calloc(1, sizeof(info_screen_t));

  s->widget = widget_create(NULL, &info_screen_widget_class, s, display_rect);

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
  label_create(s->widget, rect, "Device Info", font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 80;
  rect.width = 300;
  rect.height = 150;
  widget_t* lb = listbox_create(s->widget, rect, 20);
  add_info(lb, "Device SW", VERSION_STR);
  add_info(lb, "Boot SW", _bootloader_api.get_version());
  add_info(lb, "Device ID", device_id);
  const net_status_t* ns = net_get_status();
  add_info(lb, "WiFi SP", ns->sp_ver);
  if (ns->dhcp_resolved) {
    add_info(lb, "IP Addr", ns->ip_addr);
    add_info(lb, "Subnet", ns->subnet_mask);
    add_info(lb, "Gateway", ns->default_gateway);
    add_info(lb, "DNS", ns->dns_server);
  }
  else {
    add_info(lb, "IP Addr", "Not assigned");
    add_info(lb, "Subnet", "Not assigned");
    add_info(lb, "Gateway", "Not assigned");
    add_info(lb, "DNS", "Not assigned");
  }
  add_info(lb, "MAC Addr", ns->mac_addr);

  return s->widget;
}

static void
info_screen_destroy(widget_t* w)
{
  info_screen_t* s = widget_get_instance_data(w);

  free(s);
}


static void
add_info(widget_t* lb, const char* name, const char* value)
{
  rect_t rect = {
      .x = 0,
      .y = 0,
      .width = 200,
      .height = 20
  };
  widget_t* info_panel = widget_create(NULL, NULL, NULL, rect);

  rect.width = 65;
  label_create(info_panel, rect, name, font_opensans_regular_12, WHITE, 1);

  rect.x += 70;
  rect.width = 175;
  label_create(info_panel, rect, value, font_opensans_regular_12, WHITE, 1);

  listbox_add_item(lb, info_panel);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}
