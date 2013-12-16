
#include "listbox.h"
#include "gui.h"
#include "gfx.h"
#include "gui/button.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  int pos;
  int row_height;
  widget_t* item_container;
  widget_t* up_button;
  widget_t* dn_button;
} listbox_t;


static void listbox_layout(widget_t* w);
static void listbox_destroy(widget_t* w);

static void up_button_event(button_event_t* event);
static void down_button_event(button_event_t* event);


static const widget_class_t listbox_widget_class = {
    .on_layout  = listbox_layout,
    .on_destroy = listbox_destroy,
};

widget_t*
listbox_create(widget_t* parent, rect_t rect, int row_height)
{
  listbox_t* l = calloc(1, sizeof(listbox_t));

  widget_t* lb = widget_create(parent, NULL, l, rect);

  l->pos = 0;
  l->row_height = row_height;

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
  button_rect.y += 52 + button_pad;
  l->dn_button = button_create(lb, button_rect, img_down, WHITE, BLACK, down_button_event);

  return lb;
}

static void
listbox_destroy(widget_t* w)
{
  listbox_t* l = widget_get_instance_data(w);
  free(l);
}

void
listbox_add_row(widget_t* lb, widget_t* row)
{
  listbox_t* l = widget_get_instance_data(lb);
  widget_add_child(l->item_container, row);

  rect_t container_rect = widget_get_rect(l->item_container);
  rect_t rect = widget_get_rect(row);
  rect.x = 0;
  rect.y = 0;
  rect.width = MIN(rect.width, container_rect.width);
  rect.height = MIN(rect.height, l->row_height);
  widget_set_rect(row, rect);
}

static void
listbox_layout(widget_t* w)
{
  int i;
  listbox_t* l = widget_get_instance_data(w);
  rect_t rect = widget_get_rect(w);

  int num_visible_rows = rect.height / l->row_height;
  for (i = 0; i < widget_num_children(w); ++i) {
    int visible_index = i - l->pos;

    widget_t* child = widget_get_child(w, i);
    if (visible_index >= 0 && visible_index < num_visible_rows) {
      rect_t child_rect = widget_get_rect(child);
      child_rect.y = (visible_index * l->row_height) + ((l->row_height - child_rect.height) / 2);
      widget_set_rect(child, child_rect);
      widget_show(child);
    }
    else {
      widget_hide(child);
    }
  }

  widget_enable(l->up_button, (l->pos > 0));
  widget_enable(l->dn_button, (l->pos < (widget_num_children(w) - num_visible_rows)));
}

static void
up_button_event(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  listbox_t* l = widget_get_instance_data(parent);

  if (event->id == EVT_BUTTON_CLICK) {
    l->pos--;
  }
}

static void
down_button_event(button_event_t* event)
{
  widget_t* parent = widget_get_parent(event->widget);
  listbox_t* l = widget_get_instance_data(parent);

  if (event->id == EVT_BUTTON_CLICK) {
    l->pos++;
  }
}
