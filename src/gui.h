
#ifndef GUI_H
#define GUI_H

#include "gui/widget.h"

void
gui_init(widget_t* root);

void
gui_push_screen(widget_t* screen);

void
gui_pop_screen(void);

void
gui_paint(void);

void
gui_acquire_touch_capture(widget_t* widget);

void
gui_release_touch_capture(void);

#endif
