#ifndef WIDGET_H
#define WIDGET_H

#include "types.h"
#include "event.h"


typedef enum {
  WIDGET_TRAVERSAL_BEFORE_CHILDREN,
  WIDGET_TRAVERSAL_AFTER_CHILDREN,
} widget_traversal_event_t;

typedef struct {
  void (*on_layout)(widget_t* w);
  void (*on_paint)(paint_event_t* event);
  void (*on_touch)(touch_event_t* event);
  void (*on_msg)(msg_event_t* event);
  void (*on_enable)(enable_event_t* event);
  void (*on_destroy)(widget_t* w);
} widget_class_t;

typedef void (*widget_predicate_t)(widget_t* w, widget_traversal_event_t event, void* data);


widget_t*
widget_create(widget_t* parent, const widget_class_t* widget_class, void* instance_data, rect_t rect);

void
widget_destroy(widget_t* w);

widget_t*
widget_get_parent(widget_t* w);

rect_t
widget_get_rect(widget_t* w);

void
widget_set_rect(widget_t* w, rect_t rect);

void*
widget_get_instance_data(widget_t* w);

void
widget_add_child(widget_t* parent, widget_t* child);

int
widget_num_children(widget_t* w);

widget_t*
widget_get_child(widget_t* parent, int idx);

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
widget_hide(widget_t* w);

void
widget_show(widget_t* w);

bool
widget_is_visible(widget_t* w);

void
widget_enable(widget_t* w, bool enabled);

void
widget_disable(widget_t* w);

bool
widget_is_enabled(widget_t* w);

void
widget_set_background(widget_t* w, color_t color, bool transparent);

void
widget_for_each(widget_t* w, widget_predicate_t pred, void* data);

#endif
