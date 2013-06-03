
#include "ch.h"
#include "widget.h"

#include <string.h>


static void
widget_invalidate_predicate(widget_t* w);

static void
widget_paint_predicate(widget_t* w);


void
widget_init(widget_t* w, rect_t rect)
{
  memset(w, 0, sizeof(widget_t));
  w->rect = rect;
  w->invalid = true;
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
widget_for_each(widget_t* w, widget_predicate_t pred)
{
  widget_t* child;

  pred(w);

  for (child = w->first_child; child != NULL; child = child->next_sibling) {
    pred(child);
  }
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

  case EVT_TOUCH_DOWN:
    break;

  case EVT_TOUCH_UP:
    break;

  default:
    break;
  }
}

void
widget_paint(widget_t* w)
{
  widget_for_each(w, widget_paint_predicate);
}

static void
widget_paint_predicate(widget_t* w)
{
  if (w->invalid) {
    paint_event_t event = {
        .id = EVT_PAINT,
        .widget = w,
    };

    if (w->on_paint)
      w->on_paint(&event);

    w->invalid = false;
  }
}

void
widget_invalidate(widget_t* w)
{
  widget_for_each(w, widget_invalidate_predicate);
}

static void
widget_invalidate_predicate(widget_t* w)
{
  w->invalid = true;
}
