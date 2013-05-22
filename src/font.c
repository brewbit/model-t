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
font_text_extents(const Font_t* font, char* str)
{
  Extents_t e;
  e.width = 0;
  e.height = 0;

  while (*str) {
    Extents_t ce = font_char_extents(font, *str++);
    e.width += ce.width;
    e.height = MAX(e.height, ce.height);
  }

  return e;
}

Extents_t
font_char_extents(const Font_t* font, char ch)
{
  Extents_t e;
  const Glyph_t* g = font_find_glyph(font, ch);

  e.width = g->width;
  e.height = g->height;

  return e;
}
