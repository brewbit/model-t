#include "font.h"
#include "common.h"
#include <stdlib.h>

const Glyph_t*
font_find_glyph(const Font_t* font, char ch)
{
  const Glyph_t* g = font->glyphs[(uint8_t) ch];
  if (g == NULL)
    g = font->glyphs[0];

  return g;
}

Extents_t
font_text_extents(const Font_t* font, const char* str)
{
  Extents_t e;
  e.width = 0;
  e.height = 0;

  int min_y = 0;
  int max_y = 0;

  while (*str) {
    const Glyph_t* g = font_find_glyph(font, *str++);
    e.width += g->advance;

    min_y = MIN(g->yoffset, min_y);
    max_y = MAX(g->height + g->yoffset, max_y);
  }

  e.height = max_y - min_y;

  return e;
}
