
#ifndef QUANTITY_WIDGET_H
#define QUANTITY_WIDGET_H

#include "gui/widget.h"

widget_t*
quantity_widget_create(widget_t* parent, rect_t rect, unit_t display_unit);

void
quantity_widget_set_value(widget_t* w, quantity_t quantity);

void
quantity_widget_set_unit(widget_t* w, unit_t unit);

#endif
