
#include "ch.h"
#include "widget.h"
#include "common.h"

#include <string.h>


#define CALL_WC(w, m)   if ((w)->widget_class != NULL && (w)->widget_class->m != NULL) (w)->widget_class->m


typedef struct widget_s {
  const widget_class_t* widget_class;
  void* instance_data;

  struct widget_s* parent;

  struct widget_s* first_child;
  struct widget_s* last_child;

  struct widget_s* next_sibling;
  struct widget_s* prev_sibling;

  rect_t rect;
  bool invalid;
} widget_t;


static void
widget_invalidate_predicate(widget_t* w);

static void
widget_paint_predicate(widget_t* w);

static void
widget_destroy_predicate(widget_t* w);

static void
dispatch_touch(widget_t* w, touch_event_t* event);


widget_t*
widget_create(const widget_class_t* widget_class, void* instance_data, rect_t rect)
{
  widget_t* w = calloc(1, sizeof(widget_t));

  w->widget_class = widget_class;
  w->instance_data = instance_data;

  w->rect = rect;
  w->invalid = true;

  return w;
}

void
widget_destroy(widget_t* w)
{
  widget_for_each(w, widget_destroy_predicate, WIDGET_TRAVERSAL_BOTTOM_UP);
}

static void
widget_destroy_predicate(widget_t* w)
{
  CALL_WC(w, on_destroy)(w);
  free(w);
}

rect_t
widget_get_rect(widget_t* w)
{
  return w->rect;
}

void*
widget_get_instance_data(widget_t* w)
{
  return w->instance_data;
}

void
widget_add_child(widget_t* parent, widget_t* child)
{
  if (parent->last_child != NULL) {
    parent->last_child->next_sibling = child;
    child->prev_sibling = parent->last_child;
    parent->last_child = child;
  }
  else {
    chDbgAssert(parent->first_child == NULL, "widget_add_child(), #1", "");

    parent->first_child = parent->last_child = child;
  }

  child->parent = parent;
}

void
widget_unparent(widget_t* w)
{
  if (w->prev_sibling != NULL)
    w->prev_sibling->next_sibling = w->next_sibling;
  if (w->next_sibling != NULL)
    w->next_sibling->prev_sibling = w->prev_sibling;

  w->next_sibling = NULL;
  w->prev_sibling = NULL;
  w->parent = NULL;
}

void
widget_for_each(widget_t* w, widget_predicate_t pred, widget_traversal_dir_t dir)
{
  widget_t* child;
  widget_t* next_child = NULL;

  if (dir == WIDGET_TRAVERSAL_TOP_DOWN)
    pred(w);

  for (child = w->first_child; child != NULL; child = next_child) {
    /*  Save the next child locally because this child might be destroyed
     *  inside the predicate...
     */
    next_child = child->next_sibling;
    widget_for_each(child, pred, dir);
  }

  if (dir == WIDGET_TRAVERSAL_BOTTOM_UP)
    pred(w);
}

widget_t*
widget_hit_test(widget_t* root, point_t p)
{
  widget_t* w;

  for (w = root->first_child; w != NULL; w = w->next_sibling) {
    widget_t* w_hit = widget_hit_test(w, p);
    if (w_hit != NULL)
      return w_hit;
  }

  if (rect_inside(root->rect, p))
    return root;

  return NULL;
}

void
widget_dispatch_event(widget_t* w, event_t* event)
{
  switch (event->id) {
  case EVT_PAINT:
    break;

  case EVT_TOUCH_UP:
  case EVT_TOUCH_DOWN:
    dispatch_touch(w, (touch_event_t*)event);
    break;

  default:
    break;
  }
}

static void
dispatch_touch(widget_t* w, touch_event_t* event)
{
  // TODO bubble touch up to parent if this widget doesn't handle touches
  CALL_WC(w, on_touch)(event);
}

void
widget_paint(widget_t* w)
{
  widget_for_each(w, widget_paint_predicate, WIDGET_TRAVERSAL_TOP_DOWN);
}

static void
widget_paint_predicate(widget_t* w)
{
  if (w->invalid) {
    paint_event_t event = {
        .id = EVT_PAINT,
        .widget = w,
    };

    CALL_WC(w, on_paint)(&event);

    w->invalid = false;
  }
}

void
widget_invalidate(widget_t* w)
{
  widget_for_each(w, widget_invalidate_predicate, WIDGET_TRAVERSAL_TOP_DOWN);
}

static void
widget_invalidate_predicate(widget_t* w)
{
  w->invalid = true;
}
