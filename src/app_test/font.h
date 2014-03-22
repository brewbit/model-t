#ifndef __FONT_H__
#define __FONT_H__

#include "font_resources.h"
#include "common.h"

const glyph_t*
font_find_glyph(const font_t* font, char ch);

Extents_t
font_text_extents(const font_t* font, const char* str);

#endif
