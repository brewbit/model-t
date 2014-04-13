
#include "gui/setup.h"
#include "label.h"
#include "gui.h"
#include "gfx.h"

#include <string.h>


widget_t*
setup_screen_create()
{
  widget_t* widget = widget_create(NULL, NULL, NULL, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(widget, rect, "Configuring...", font_opensans_regular_22, WHITE, 1);

  rect.y = 80;
  rect.x = 40;
  rect.width = 240;
  label_create(widget, rect, "Please wait while your device is configured for the first time. This may take a few minutes.", font_opensans_regular_18, WHITE, 5);

  return widget;
}
