
#ifndef GUI_CALIB
#define GUI_CALIB

#include "gui/widget.h"

typedef void (*calib_complete_handler_t)(widget_t* w);

widget_t*
calib_screen_create(calib_complete_handler_t completion_handler);

#endif
