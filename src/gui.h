
#ifndef DISP_MGR_H
#define DISP_MGR_H

typedef enum {
  GUI_PAINT,
  GUI_TOUCH,
  GUI_SET_SCREEN,
} gui_msg_id_t;

typedef struct {
  void (*on_paint)(void);
  void (*on_touch)(uint16_t x, uint16_t y);
} screen_t;

typedef struct {
  gui_msg_id_t id;
} gui_any_msg_t;

typedef struct {
  gui_msg_id_t id;
} gui_paint_msg_t;

typedef struct {
  gui_msg_id_t id;
  uint16_t x;
  uint16_t y;
  uint16_t raw_x;
  uint16_t raw_y;
} gui_touch_msg_t;

typedef struct {
  gui_msg_id_t id;
  screen_t* screen;
} gui_set_screen_msg_t;

typedef struct {
  union {
    gui_any_msg_t any;
    gui_paint_msg_t paint;
    gui_touch_msg_t touch;
    gui_set_screen_msg_t set_screen;
  } u;
} gui_msg_t;

void
gui_init(screen_t* main_screen);

void
gui_set_screen(screen_t* screen);

void
gui_send_msg(gui_msg_t* msg);

#endif
