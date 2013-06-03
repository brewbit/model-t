#ifndef WIDGET_H
#define WIDGET_H

#include "types.h"
#include "event.h"

typedef struct widget_s {
  struct widget_s* parent;

  struct widget_s* first_child;
  struct widget_s* last_child;

  struct widget_s* next_sibling;
  struct widget_s* prev_sibling;

  rect_t rect;
  bool invalid;

  void (*on_paint)(paint_event_t* event);
  void (*on_touch_down)(touch_event_t* event);
  void (*on_touch_up)(touch_event_t* event);
} widget_t;

typedef void (*widget_predicate_t)(widget_t* w);

void
widget_init(widget_t* w, rect_t rect);

void
widget_add_child(widget_t* parent, widget_t* child);

void
widget_unparent(widget_t* w);

widget_t*
widget_hit_test(widget_t* root, point_t p);

void
widget_dispatch_event(widget_t* w, event_t* event);

void
widget_paint(widget_t* screen);

void
widget_invalidate(widget_t* screen);

void
widget_for_each(widget_t* w, widget_predicate_t pred);

#endif
