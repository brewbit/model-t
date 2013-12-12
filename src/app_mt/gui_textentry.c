
#include "ch.h"
#include "hal.h"
#include "gui_textentry.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gfx.h"
#include "gui.h"

#include <string.h>


#define NUM_ROWS 21
#define MAX_TEXT_LEN 256


typedef struct {
  widget_t* widget;
  widget_t* buttons[3][5];
  widget_t* text_label;
  int row_idx;
  char text[MAX_TEXT_LEN];
} textentry_screen_t;


static void textentry_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void backspace_button_clicked(button_event_t* event);
static void up_button_clicked(button_event_t* event);
static void down_button_clicked(button_event_t* event);
static void char_button_clicked(button_event_t* event);
static void update_input_buttons(textentry_screen_t* screen);

static const widget_class_t textentry_screen_widget_class = {
    .on_destroy = textentry_screen_destroy,
};

static const char* btn_layout[NUM_ROWS][5] = {
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

widget_t*
textentry_screen_create()
{
  int i;
  textentry_screen_t* screen = calloc(1, sizeof(textentry_screen_t));

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
  for (i = 0; i < 3; ++i) {
    int j;
    for (j = 0; j < 5; ++j) {
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
  button_create(screen->widget, rect, img_check, WHITE, BLACK, back_button_clicked);

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
  widget_set_background(screen->text_label, LIGHT_GRAY, false);

  return screen->widget;
}

static void
update_input_buttons(textentry_screen_t* screen)
{
  int i;
  for (i = 0; i < 3; ++i) {
    int j;
    for (j = 0; j < 5; ++j) {
      widget_t* btn = screen->buttons[i][j];
      const char* btn_text = btn_layout[screen->row_idx + i][j];

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

    if (screen->row_idx >= 3)
      screen->row_idx -= 3;
    update_input_buttons(screen);
  }
}

static void
down_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    textentry_screen_t* screen = widget_get_instance_data(w);

    if (screen->row_idx < NUM_ROWS - 3)
      screen->row_idx += 3;
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
