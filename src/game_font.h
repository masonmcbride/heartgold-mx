#pragma once
#include <stdint.h>
typedef struct HGFont HGFont;
HGFont *hg_font_new(void);
void hg_font_free(HGFont *);
/* Original font-data/text glyph functions, with a UTF-8 boundary adapter. */
unsigned hg_font_width(HGFont *,const char *text);
void hg_font_draw(HGFont *,const char *text,uint8_t *rgba,unsigned width,unsigned height,int x,int y,unsigned max_width,uint32_t foreground,uint32_t shadow);
