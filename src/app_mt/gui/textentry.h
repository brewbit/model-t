
#ifndef GUI_TEXTENTRY_H
#define GUI_TEXTENTRY_H

#include "widget.h"

typedef void (*text_handler_t)(const char* text, void* user_data);

void
textentry_screen_show(text_handler_t text_handler, void* user_data);

#endif
