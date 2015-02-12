
#include "ch.h"
#include "hal.h"
#include "gui/button_list.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "listbox.h"
#include "gfx.h"
#include "gui.h"
#include "net.h"
#include "web_api.h"
#include "gui/activation.h"
#include "app_cfg.h"

#include <string.h>


widget_t*
button_list_screen_create(
    widget_t* screen,
    char* title,
    button_event_handler_t back_handler,
    void* user_data)
{
  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  widget_t* b = button_create(screen, rect, img_left, WHITE, BLACK, back_handler);
  widget_set_user_data(b, user_data);
  button_set_up_bg_color(b, BLACK);
  button_set_up_icon_color(b, WHITE);
  button_set_down_bg_color(b, BLACK);
  button_set_down_icon_color(b, LIGHT_GRAY);
  button_set_disabled_bg_color(b, BLACK);
  button_set_disabled_icon_color(b, DARK_GRAY);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(screen, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 5;
  rect.y = 85;
  rect.width = 300;
  rect.height = 160;
  widget_t* lb = listbox_create(screen, rect, 76);

  return lb;
}

widget_t*
button_list_prompt_screen_create(
    widget_t* screen,
    char* title,
    button_event_handler_t back_handler,
    void* user_data)
{
  rect_t rect = {
      .x = 15,
      .y = 20,
      .width = 30,
      .height = 30,
  };
  widget_t* b = button_create(screen, rect, img_left, WHITE, BLACK, back_handler);
  widget_set_user_data(b, user_data);
  button_set_up_bg_color(b, BLACK);
  button_set_up_icon_color(b, WHITE);
  button_set_down_bg_color(b, BLACK);
  button_set_down_icon_color(b, LIGHT_GRAY);
  button_set_disabled_bg_color(b, BLACK);
  button_set_disabled_icon_color(b, DARK_GRAY);

  rect.x = 70;
  rect.y = 15;
  rect.width = 220;
  rect.height = 30;
  label_create(screen, rect, title, font_opensans_regular_18, WHITE, 3);

  rect.x = 5;
  rect.y = 75;
  rect.width = 300;
  rect.height = 160;
  widget_t* lb = listbox_create(screen, rect, 52);

  return lb;
}

void
button_list_set_buttons(widget_t* button_list, button_spec_t* buttons, uint32_t num_buttons)
{
  int i;

  listbox_clear(button_list);

  for (i = 0; i < (int)num_buttons; ++i) {
    rect_t rect;
    rect.x = 10;
    rect.y = 85;
    rect.width = 240;
    rect.height = 66;
    widget_t* button = button_create(NULL, rect, NULL, WHITE, BLACK, buttons[i].btn_event_handler);
    widget_set_user_data(button, buttons[i].user_data);

    rect.x = 5;
    rect.y = 5;
    rect.width = 56;
    rect.height = 56;
    icon_create(button, rect, buttons[i].img, WHITE, buttons[i].color);

    rect.x = 70;
    rect.y = 5;
    rect.width = 160;
    label_create(button, rect, buttons[i].text, font_opensans_regular_18, WHITE, 1);

    rect.y = 30;
    label_create(button, rect, buttons[i].subtext, font_opensans_regular_12, WHITE, 2);

    listbox_add_item(button_list, button);
  }
}

void
button_list_set_small_buttons(widget_t* button_list, button_spec_t* buttons, uint32_t num_buttons)
{
  int i;

  listbox_clear(button_list);

  for (i = 0; i < (int)num_buttons; ++i) {
    rect_t rect;
    rect.x = 10;
    rect.y = 0;
    rect.width = 240;
    rect.height = 55;
    widget_t* button = button_create(NULL, rect, NULL, WHITE, BLACK, buttons[i].btn_event_handler);
    widget_set_user_data(button, buttons[i].user_data);

    rect.x = 5;
    rect.y = 5;
    rect.width = 45;
    rect.height = 45;
    icon_create(button, rect, buttons[i].img, WHITE, buttons[i].color);

    rect.x = 70;
    rect.y = 10;
    rect.width = 160;
    label_create(button, rect, buttons[i].text, font_opensans_regular_18, WHITE, 1);

    rect.y = 30;
    label_create(button, rect, buttons[i].subtext, font_opensans_regular_12, WHITE, 2);

    listbox_add_item(button_list, button);
  }
}
