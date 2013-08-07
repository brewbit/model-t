
#ifndef GUI_H
#define GUI_H

#include "gui/widget.h"

void
gui_init(void);

void
gui_idle(void);

void
gui_push_screen(widget_t* screen);

void
gui_pop_screen(void);

void
gui_acquire_touch_capture(widget_t* widget);

void
gui_release_touch_capture(void);

void
gui_msg_subscribe(msg_id_t id, widget_t* w);

void
gui_msg_unsubscribe(msg_id_t id, widget_t* w);

#endif
