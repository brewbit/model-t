
#include "ch.h"
#include "widget.h"
#include "common.h"
#include "gfx.h"

#include <string.h>


#define CALL_WC(w, m)   if ((w)->widget_class != NULL && (w)->widget_class->m != NULL) (w)->widget_class->m


typedef struct widget_s {
  const widget_class_t* widget_class;
  void* instance_data;
  void* user_data;

  struct widget_s* parent;

  struct widget_s* first_child;
  struct widget_s* last_child;

  struct widget_s* next_sibling;
  struct widget_s* prev_sibling;

  rect_t rect;
  bool invalid;
  bool visible;
  bool enabled;

  bool bg_transparent;
  color_t bg_color;
} widget_t;


static void
widget_invalidate_predicate(widget_t* w, widget_traversal_event_t event, void* data);

static void
widget_layout_predicate(widget_t* w, widget_traversal_event_t event, void* data);

static void
widget_paint_predicate(widget_t* w, widget_traversal_event_t event, void* data);

static void
widget_destroy_predicate(widget_t* w, widget_traversal_event_t event, void* data);

static void
widget_enable_predicate(widget_t* w, widget_traversal_event_t event, void* data);

static void
dispatch_touch(widget_t* w, touch_event_t* event);

static void
dispatch_msg(widget_t* w, msg_event_t* event);


widget_t*
widget_create(widget_t* parent, const widget_class_t* widget_class, void* instance_data, rect_t rect)
{
  widget_t* w = calloc(1, sizeof(widget_t));

  w->widget_class = widget_class;
  w->instance_data = instance_data;

  w->rect = rect;
  w->invalid = true;
  w->visible = true;
  w->enabled = true;
  w->bg_color = BLACK;
  w->bg_transparent = (parent != NULL);

  if (parent != NULL)
    widget_add_child(parent, w);

  return w;
}

void
widget_destroy(widget_t* w)
{
  widget_for_each(w, widget_destroy_predicate, NULL);
}

static void
widget_destroy_predicate(widget_t* w, widget_traversal_event_t event, void* data)
{
  (void)data;

  if (event == WIDGET_TRAVERSAL_AFTER_CHILDREN) {
    CALL_WC(w, on_destroy)(w);
    chHeapFree(w);
  }
}

widget_t*
widget_get_parent(widget_t* w)
{
  return w->parent;
}

rect_t
widget_get_rect(widget_t* w)
{
  return w->rect;
}

void
widget_set_rect(widget_t* w, rect_t rect)
{
  if (memcmp(&rect, &w->rect, sizeof(rect_t)) != 0) {
    w->rect = rect;
    widget_invalidate(w->parent);
  }
}

void*
widget_get_instance_data(widget_t* w)
{
  return w->instance_data;
}

void
widget_set_user_data(widget_t* w, void* user_data)
{
  w->user_data = user_data;
}

void*
widget_get_user_data(widget_t* w)
{
  return w->user_data;
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

int
widget_num_children(widget_t* w)
{
  widget_t* child;
  int i = 0;
  for (child = w->first_child; child != NULL; child = child->next_sibling) {
    i++;
  }

  return i;
}

// TODO use an arraylist to speed this up...
widget_t*
widget_get_child(widget_t* w, int idx)
{
  widget_t* child;
  int i = 0;
  for (child = w->first_child; child != NULL; child = child->next_sibling) {
    if (i++ == idx)
      return child;
  }

  return NULL;
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
widget_for_each(widget_t* w, widget_predicate_t pred, void* data)
{
  widget_t* child;
  widget_t* next_child = NULL;

  pred(w, WIDGET_TRAVERSAL_BEFORE_CHILDREN, data);

  for (child = w->first_child; child != NULL; child = next_child) {
    /*  Save the next child locally because this child might be destroyed
     *  inside the predicate...
     */
    next_child = child->next_sibling;
    widget_for_each(child, pred, data);
  }

  pred(w, WIDGET_TRAVERSAL_AFTER_CHILDREN, data);
}

widget_t*
widget_hit_test(widget_t* root, point_t p)
{
  widget_t* w;

  for (w = root->first_child; w != NULL; w = w->next_sibling) {
    point_t translated_p = {
        .x = p.x - root->rect.x,
        .y = p.y - root->rect.y,
    };
    widget_t* w_hit = widget_hit_test(w, translated_p);
    if (w_hit != NULL)
      return w_hit;
  }

  if (widget_is_visible(root) && rect_inside(root->rect, p))
    return root;

  return NULL;
}

point_t
widget_rel_pos(widget_t* w, point_t abs_pos)
{
  widget_t* parent;
  point_t rel_pos = abs_pos;

  for (parent = w->parent; parent != NULL; parent = parent->parent) {
    rel_pos.x -= parent->rect.x;
    rel_pos.y -= parent->rect.y;
  }

  return rel_pos;
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

  case EVT_MSG:
    dispatch_msg(w, (msg_event_t*)event);
    break;

  default:
    break;
  }
}

static void
dispatch_msg(widget_t* w, msg_event_t* event)
{
  CALL_WC(w, on_msg)(event);
}

static void
dispatch_touch(widget_t* w, touch_event_t* event)
{
  while (w != NULL) {
    if ((w->widget_class != NULL) && (w->widget_class->on_touch != NULL)) {
      event->widget = w;
      w->widget_class->on_touch(event);
      return;
    }
    else {
      w = w->parent;
    }
  }
}

void
widget_paint(widget_t* w)
{
  widget_for_each(w, widget_layout_predicate, NULL);
  widget_for_each(w, widget_paint_predicate, NULL);
}

static void
widget_layout_predicate(widget_t* w, widget_traversal_event_t event, void* data)
{
  (void)data;

  if (event == WIDGET_TRAVERSAL_BEFORE_CHILDREN) {
    if (widget_is_visible(w)) {
      CALL_WC(w, on_layout)(w);
    }
  }
}

static void
widget_paint_predicate(widget_t* w, widget_traversal_event_t event, void* data)
{
  (void)data;

  if (event == WIDGET_TRAVERSAL_BEFORE_CHILDREN) {
    gfx_ctx_push();

    if (!w->bg_transparent)
      gfx_set_bg_color(w->bg_color);

    if (w->invalid && widget_is_visible(w)) {
      paint_event_t event = {
          .id = EVT_PAINT,
          .widget = w,
      };

      gfx_clear_rect(w->rect);

      CALL_WC(w, on_paint)(&event);

      w->invalid = false;
    }

    gfx_push_translation(w->rect.x, w->rect.y);
  }
  else {
    gfx_ctx_pop();
  }
}

void
widget_invalidate(widget_t* w)
{
  if (w == NULL)
    return;

  widget_for_each(w, widget_invalidate_predicate, NULL);
}

static void
widget_invalidate_predicate(widget_t* w, widget_traversal_event_t event, void* data)
{
  (void)data;

  if (event == WIDGET_TRAVERSAL_BEFORE_CHILDREN)
    w->invalid = true;
}

void
widget_hide(widget_t* w)
{
  if (w->visible) {
    w->visible = false;
    widget_invalidate(w->parent);
  }
}

void
widget_show(widget_t* w)
{
  if (!w->visible) {
    w->visible = true;
    widget_invalidate(w);
  }
}

bool
widget_is_visible(widget_t* w)
{
  return w->visible && (w->parent == NULL || widget_is_visible(w->parent));
}

void widget_enable(widget_t* w, bool enabled)
{
  widget_for_each(w, widget_enable_predicate, &enabled);
}

static void
widget_enable_predicate(widget_t* w, widget_traversal_event_t event, void* data)
{
  if (event == WIDGET_TRAVERSAL_BEFORE_CHILDREN) {
    bool enabled = *(bool*)data;
    if (w->enabled != enabled) {
      w->enabled = enabled;
      widget_invalidate(w);

      enable_event_t event = {
          .id = EVT_ENABLE,
          .widget = w,
          .enabled = enabled
      };
      CALL_WC(w, on_enable)(&event);
    }
  }
}

void
widget_disable(widget_t* w)
{
  widget_enable(w, false);
}

bool
widget_is_enabled(widget_t* w)
{
  return w->enabled;
}

void
widget_set_background(widget_t* w, color_t color, bool transparent)
{
  w->bg_color = color;
  w->bg_transparent = transparent;
  widget_invalidate(w);
}
