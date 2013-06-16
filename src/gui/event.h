
#ifndef EVENT_H
#define EVENT_H

#include "message.h"

struct widget_s;
typedef struct widget_s widget_t;

typedef enum {
  EVT_PAINT,
  EVT_TOUCH_DOWN,
  EVT_TOUCH_UP,
  EVT_CLICK,
  EVT_MSG
} event_id_t;

typedef struct {
  event_id_t id;
  widget_t* widget;
} event_t;

typedef struct {
  event_id_t id;
  widget_t* widget;
} paint_event_t;

typedef struct {
  event_id_t id;
  widget_t* widget;
  point_t pos;
} touch_event_t;

typedef struct {
  event_id_t id;
  widget_t* widget;
  point_t pos;
} click_event_t;

typedef struct {
  event_id_t id;
  widget_t* widget;
  msg_id_t msg_id;
  void* msg_data;
} msg_event_t;


typedef void (*widget_event_handler_t)(event_t* event);

#endif
