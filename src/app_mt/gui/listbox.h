#ifndef LISTBOX_H
#define LISTBOX_H

#include "widget.h"
#include "image.h"
#include "font.h"


widget_t*
listbox_create(widget_t* parent, rect_t rect, int item_height);

void
listbox_add_item(widget_t* lb, widget_t* item);

int
listbox_num_items(widget_t* lb);

widget_t*
listbox_get_item(widget_t* lb, int i);

void
listbox_delete_item(widget_t* item);

void
listbox_clear(widget_t* lb);

#endif
