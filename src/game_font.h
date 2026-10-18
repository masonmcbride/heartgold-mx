#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct HGFont HGFont;
HGFont *hg_font_new(void);
void hg_font_free(HGFont *);
/* Original font-data/text glyph functions, with a UTF-8 boundary adapter. */
unsigned hg_font_width(HGFont *,const char *text);
void hg_font_draw(HGFont *,const char *text,uint8_t *rgba,unsigned width,unsigned height,int x,int y,unsigned max_width,uint32_t foreground,uint32_t shadow);
/* Bytes consumed by a native page, using original glyph widths. Includes a
 * consumed newline/CR page boundary; never splits a UTF-8 glyph. */
size_t hg_font_page_end(HGFont *,const char *,unsigned max_width,unsigned lines);
