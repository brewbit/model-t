
#ifndef QUANTITY_WIDGET_H
#define QUANTITY_WIDGET_H

#include "gui/widget.h"

widget_t*
quantity_widget_create(widget_t* parent, rect_t rect);

void
quantity_widget_set_value(widget_t* w, quantity_t quantity);

#endif
