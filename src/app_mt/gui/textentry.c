
#include "ch.h"
#include "hal.h"
#include "gui/textentry.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gfx.h"
#include "gui.h"

#include <string.h>

#define NUM_VISIBLE_ROWS 3
#define NUM_BUTTONS_PER_ROW 5

#define NUM_ROWS_ALL      21
#define NUM_ROWS_NUMERIC  3

#define MAX_TEXT_LEN 256


typedef const char* btn_row_t[NUM_BUTTONS_PER_ROW];


typedef struct {
  text_handler_t text_handler;
  void* user_data;

  widget_t* widget;
  widget_t* buttons[NUM_VISIBLE_ROWS][NUM_BUTTONS_PER_ROW];
  widget_t* text_label;
  int row_idx;
  char text[MAX_TEXT_LEN];

  btn_row_t* btn_layout;
  uint32_t num_rows;
} textentry_screen_t;


static void textentry_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void ok_button_clicked(button_event_t* event);
static void backspace_button_clicked(button_event_t* event);
static void up_button_clicked(button_event_t* event);
static void down_button_clicked(button_event_t* event);
static void char_button_clicked(button_event_t* event);
static void update_input_buttons(textentry_screen_t* screen);

static const widget_class_t textentry_screen_widget_class = {
    .on_destroy = textentry_screen_destroy,
};

static btn_row_t btn_layout_all[NUM_ROWS_ALL] = {
    {"A","B","C","D","E"},
    {"F","G","H","I","J"},
    {"K","L","M","N","O"},

    {"P","Q","R","S","T"},
    {"U","V","W","X","Y"},
    {"Z"," ",NULL,NULL,NULL},

    {"a","b","c","d","e"},
    {"f","g","h","i","j"},
    {"k","l","m","n","o"},

    {"p","q","r","s","t"},
    {"u","v","w","x","y"},
    {"z"," ",NULL,NULL,NULL},

    {"0","1","2","3","4"},
    {"5","6","7","8","9"},
    {"!","@","#","$","%"},

    {"^","&","*","(",")"},
    {"-","_","=","+","["},
    {"]","\\",";","'",NULL},

    {",",".","/","{","}"},
    {"|",":","\"","<",">"},
    {"?","`","~"," ",NULL},
};

static btn_row_t btn_layout_numeric[NUM_ROWS_NUMERIC] = {
    {"0","1","2","3","4"},
    {"5","6","7","8","9"},
    {".",NULL,NULL,NULL,NULL},
};

void
textentry_screen_show(textentry_format_t format, text_handler_t text_handler, void* user_data)
{
  int i;
  textentry_screen_t* screen = calloc(1, sizeof(textentry_screen_t));

  screen->text_handler = text_handler;
  screen->user_data = user_data;

  switch (format) {
    case TXT_FMT_IP:
      screen->btn_layout = btn_layout_numeric;
      screen->num_rows = NUM_ROWS_NUMERIC;
      break;

    default:
    case TXT_FMT_ANY:
      screen->btn_layout = btn_layout_all;
      screen->num_rows = NUM_ROWS_ALL;
      break;
  }

  screen->widget = widget_create(NULL, &textentry_screen_widget_class, screen, display_rect);

  rect_t rect = {
      .x = 7,
      .y = 5,
      .width = 48,
      .height = 48,
  };
  button_create(screen->widget, rect, img_cancel, WHITE, BLACK, back_button_clicked);

  rect.x = 7;
  rect.y = 65;
  for (i = 0; i < NUM_VISIBLE_ROWS; ++i) {
    int j;
    for (j = 0; j < NUM_BUTTONS_PER_ROW; ++j) {
      widget_t* b = button_create(screen->widget, rect, img_circle, WHITE, BLACK, char_button_clicked);
      button_set_font(b, font_opensans_regular_22);
      screen->buttons[i][j] = b;
      rect.x += 52;
    }
    rect.y += 58;
    rect.x = 7;
  }

  rect.x = 268;
  rect.y = 5;
  button_create(screen->widget, rect, img_check, WHITE, BLACK, ok_button_clicked);

  rect.y = 65;
  button_create(screen->widget, rect, img_backspace, WHITE, BLACK, backspace_button_clicked);

  rect.y += 58;
  button_create(screen->widget, rect, img_up, WHITE, BLACK, up_button_clicked);

  rect.y += 58;
  button_create(screen->widget, rect, img_down, WHITE, BLACK, down_button_clicked);

  update_input_buttons(screen);

  rect.x = 60;
  rect.y = 8;
  rect.width = 200;
  screen->text_label = label_create(screen->widget, rect, "", font_opensans_regular_22, WHITE, 2);
  widget_set_background(screen->text_label, LIGHT_GRAY);

  gui_push_screen(screen->widget);
}

static void
update_input_buttons(textentry_screen_t* screen)
{
  int i;
  for (i = 0; i < NUM_VISIBLE_ROWS; ++i) {
    btn_row_t* row = &screen->btn_layout[screen->row_idx + i];

    int j;
    for (j = 0; j < NUM_BUTTONS_PER_ROW; ++j) {
      widget_t* btn = screen->buttons[i][j];
      const char* btn_text = (*row)[j];

      if (btn_text == NULL)
        widget_hide(btn);
      else {
        if (*btn_text == ' ')
          button_set_icon(btn, img_space);
        else
          button_set_icon(btn, img_circle);

        button_set_text(btn, btn_text);
        widget_show(btn);
      }
    }
  }
}

static void
textentry_screen_destroy(widget_t* w)
{
  textentry_screen_t* screen = widget_get_instance_data(w);

  free(screen);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
ok_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);
    if (screen->text_handler != NULL)
      screen->text_handler(screen->text, screen->user_data);
    gui_pop_screen();
  }
}

static void
backspace_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);

    int text_len = strlen(screen->text);
    if (text_len > 0) {
      screen->text[text_len - 1] = '\0';
      label_set_text(screen->text_label, screen->text);
    }
  }
}

static void
up_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);

    if (screen->row_idx >= NUM_VISIBLE_ROWS)
      screen->row_idx -= NUM_VISIBLE_ROWS;
    update_input_buttons(screen);
  }
}

static void
down_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);

    if (screen->row_idx < (int)(screen->num_rows - NUM_VISIBLE_ROWS))
      screen->row_idx += NUM_VISIBLE_ROWS;
    update_input_buttons(screen);
  }
}

static void
char_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);
    const char* btn_text = button_get_text(event->widget);

    strncat(screen->text, btn_text, sizeof(screen->text));
    label_set_text(screen->text_label, screen->text);
  }
}
