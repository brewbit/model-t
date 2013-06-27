
#ifndef TEMP_WIDGET
#define TEMP_WIDGET

#include "gui/widget.h"

widget_t*
temp_widget_create(widget_t* parent, rect_t rect);

void
temp_widget_set_value(widget_t* w, temperature_t temp);

#endif
