#ifndef LISTBOX_H
#define LISTBOX_H

#include "widget.h"
#include "image.h"
#include "font.h"


widget_t*
listbox_create(widget_t* parent, rect_t rect, int row_height);

void
listbox_add_row(widget_t* lb, widget_t* row);

#endif
