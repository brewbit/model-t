
#ifndef GUI_H
#define GUI_H

#include "gui/widget.h"

void
gui_init(void);

void
gui_set_screen(widget_t* screen);

void
gui_paint(void);

void
gui_acquire_touch_capture(widget_t* widget);

void
gui_release_touch_capture(void);

#endif
