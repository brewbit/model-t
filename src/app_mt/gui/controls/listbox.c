
#include "listbox.h"
#include "gui.h"
#include "gfx.h"
#include "button.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  int pos;
  int item_height;
  widget_t* item_container;
  widget_t* up_button;
  widget_t* dn_button;
} listbox_t;


static void listbox_layout(widget_t* w);
static void listbox_destroy(widget_t* w);

static void up_button_event(button_event_t* event);
static void down_button_event(button_event_t* event);

static int num_visible_items(widget_t* lb);


static const widget_class_t listbox_widget_class = {
    .on_layout  = listbox_layout,
    .on_destroy = listbox_destroy,
};

widget_t*
listbox_create(widget_t* parent, rect_t rect, int item_height)
{
  listbox_t* l = calloc(1, sizeof(listbox_t));

  widget_t* lb = widget_create(parent, NULL, l, rect);

  l->pos = 0;
  l->item_height = item_height;

  rect_t container_rect = {
      .x = 0,
      .y = 0,
      .width = rect.width - 52,
      .height = rect.height
  };
  l->item_container = widget_create(lb, &listbox_widget_class, l, container_rect);

  int button_pad = (rect.height - (2 * 52)) / 3;
  rect_t button_rect = {
      .x = rect.width - 52,
      .y = button_pad,
      .width = 52,
      .height = 52
  };
  l->up_button = button_create(lb, button_rect, img_up, WHITE, BLACK, up_button_event);
  button_set_up_bg_color(l->up_button, BLACK);
  button_set_up_icon_color(l->up_button, WHITE);
  button_set_down_bg_color(l->up_button, BLACK);
  button_set_down_icon_color(l->up_button, LIGHT_GRAY);
  button_set_disabled_bg_color(l->up_button, BLACK);
  button_set_disabled_icon_color(l->up_button, DARK_GRAY);

  button_rect.y += 52 + button_pad;
  l->dn_button = button_create(lb, button_rect, img_down, WHITE, BLACK, down_button_event);
  button_set_up_bg_color(l->dn_button, BLACK);
  button_set_up_icon_color(l->dn_button, WHITE);
  button_set_down_bg_color(l->dn_button, BLACK);
  button_set_down_icon_color(l->dn_button, LIGHT_GRAY);
  button_set_disabled_bg_color(l->dn_button, BLACK);
  button_set_disabled_icon_color(l->dn_button, DARK_GRAY);

  return lb;
}

void
listbox_clear(widget_t* lb)
{
  listbox_t* l = widget_get_instance_data(lb);
  while (1) {
    widget_t* w = widget_get_child(l->item_container, 0);
    if (w == NULL)
      break;

    widget_unparent(w);
    widget_destroy(w);
  }
  widget_invalidate(lb);
}

static void
listbox_destroy(widget_t* w)
{
  listbox_t* l = widget_get_instance_data(w);
  free(l);
}

void
listbox_add_item(widget_t* lb, widget_t* item)
{
  listbox_t* l = widget_get_instance_data(lb);
  widget_add_child(l->item_container, item);

  rect_t container_rect = widget_get_rect(l->item_container);
  rect_t rect = widget_get_rect(item);
  rect.x = 0;
  rect.y = 0;
  rect.width = MIN(rect.width, container_rect.width);
  rect.height = MIN(rect.height, l->item_height);
  widget_set_rect(item, rect);
  widget_invalidate(lb);
}

int
listbox_num_items(widget_t* lb)
{
  listbox_t* l = widget_get_instance_data(lb);
  return widget_num_children(l->item_container);
}

widget_t*
listbox_get_item(widget_t* lb, int i)
{
  listbox_t* l = widget_get_instance_data(lb);
  return widget_get_child(l->item_container, i);
}

void
listbox_delete_item(widget_t* item)
{
  // TODO check if this item is actually owned by the listbox?
  return widget_unparent(item);
}

static void
listbox_layout(widget_t* w)
{
  int i;
  listbox_t* l = widget_get_instance_data(w);

  int visible_items = num_visible_items(w);
  for (i = 0; i < widget_num_children(w); ++i) {
    int visible_index = i - l->pos;

    widget_t* child = widget_get_child(w, i);
    if (visible_index >= 0 && visible_index < visible_items) {
      rect_t child_rect = widget_get_rect(child);
      child_rect.y = (visible_index * l->item_height) + ((l->item_height - child_rect.height) / 2);
      widget_set_rect(child, child_rect);
      widget_show(child);
    }
    else {
      widget_hide(child);
    }
  }

  widget_enable(l->up_button, (l->pos > 0));
  widget_enable(l->dn_button, (l->pos < (widget_num_children(w) - visible_items)));
}

static int
num_visible_items(widget_t* lb)
{
  listbox_t* l = widget_get_instance_data(lb);
  rect_t rect = widget_get_rect(lb);

  return rect.height / l->item_height;
}

static void
up_button_event(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  listbox_t* l = widget_get_instance_data(parent);

  if (event->id == EVT_BUTTON_CLICK ||
      event->id == EVT_BUTTON_REPEAT) {
    if (l->pos > 0) {
      l->pos--;
      widget_invalidate(l->item_container);
    }
  }
}

static void
down_button_event(button_event_t* event)
{
  widget_t* lb = widget_get_parent(event->widget);
  listbox_t* l = widget_get_instance_data(lb);

  if (event->id == EVT_BUTTON_CLICK ||
      event->id == EVT_BUTTON_REPEAT) {
    if (l->pos < widget_num_children(l->item_container) - num_visible_items(lb)) {
      l->pos++;
      widget_invalidate(l->item_container);
    }
  }
}
