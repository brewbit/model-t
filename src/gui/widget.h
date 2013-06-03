#ifndef WIDGET_H
#define WIDGET_H

#include "types.h"
#include "event.h"


typedef enum {
  WIDGET_TRAVERSAL_TOP_DOWN,
  WIDGET_TRAVERSAL_BOTTOM_UP,
} widget_traversal_dir_t;

typedef struct {
  void (*on_paint)(paint_event_t* event);
  void (*on_touch)(touch_event_t* event);
  void (*on_destroy)(widget_t* w);
} widget_class_t;

typedef void (*widget_predicate_t)(widget_t* w);


widget_t*
widget_create(const widget_class_t* widget_class, void* instance_data, rect_t rect);

void
widget_destroy(widget_t* w);

widget_t*
widget_get_parent(widget_t* w);

rect_t
widget_get_rect(widget_t* w);

void*
widget_get_instance_data(widget_t* w);

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
widget_for_each(widget_t* w, widget_predicate_t pred, widget_traversal_dir_t dir);

#endif
