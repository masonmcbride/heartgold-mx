#include "game_font.h"
#include "font_data.h"
#include "compat/resources.h"
#include "charmap.inc"
struct HGFont { struct FontData *data; };
static u32 le32(const u8 *p) { return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24; }
HGFont *hg_font_new(void) {
    /* Validate disk layout before handing the font to original pointer-based code. */
    const u8 *bytes;size_t size;
    if(!native_resource_member(NARC_graphic_font,0,0,&bytes,&size) || size<16)return NULL;
    u32 header=le32(bytes),widths=le32(bytes+4),count=le32(bytes+8);
    if(header<16 || header>size || count<428 || count>65535 || widths>size || count>size-widths || bytes[14]<1 || bytes[14]>2 || bytes[15]<1 || bytes[15]>2 || bytes[13]>16 || bytes[13]==0)return NULL;
    size_t glyph_size=16u*bytes[14]*bytes[15];
    if(count>(size-header)/glyph_size)return NULL;
    for(u32 i=0;i<count;i++)if(bytes[widths+i]>16)return NULL;
    HGFont *f=Heap_Alloc(HEAP_ID_DEFAULT,sizeof(*f));f->data=FontData_New(NARC_graphic_font,0,FONTARC_MODE_DIRECT,FALSE,HEAP_ID_DEFAULT);
    GenerateFontHalfRowLookupTable(1,0,2);return f;
}
void hg_font_free(HGFont *f) { if(f) { FontData_Delete(f->data);Heap_Free(f); } }
static uint16_t next_glyph(const char **text) {
    const uint8_t *p=(const uint8_t *)*text;uint32_t value=*p++;unsigned remaining=0;
    if(value>=0xc2 && value<0xe0) { value&=31;remaining=1; }
    else if(value>=0xe0 && value<0xf0) { value&=15;remaining=2; }
    else if(value>=0xf0 && value<0xf5) { value&=7;remaining=3; }
    else if(value>=128)value='?';
    for(unsigned i=0;i<remaining;i++) { if((*p&0xc0)!=0x80) { value='?';break; }value=(value<<6)|(*p++&63); }
    *text=(const char *)p;if(value=='\n')return CHAR_LF;
    for(unsigned i=0;i<NELEMS(glyph_map);i++)if(glyph_map[i].unicode==value)return glyph_map[i].glyph;
    return 428;
}
unsigned hg_font_width(HGFont *f,const char *text) {
    unsigned width=0,max=0;while(*text) { u16 glyph=next_glyph(&text);if(glyph==CHAR_LF) { if(width>max)max=width;width=0; }else width+=GetGlyphWidth(f->data,glyph); }return width>max?width:max;
}
size_t hg_font_page_end(HGFont *f,const char *text,unsigned max_width,unsigned lines) {
    if(!f||!text||!max_width||!lines)return 0;
    const char *cursor=text;unsigned width=0,line=1;
    while(*cursor) {
        if(*cursor=='\r')return (size_t)(cursor-text)+1;
        const char *next=cursor;u16 glyph=next_glyph(&next);
        if(glyph==CHAR_LF) {
            cursor=next;if(line==lines)break;line++;width=0;continue;
        }
        unsigned advance=GetGlyphWidth(f->data,glyph);
        if(width&&advance>max_width-width) { if(line==lines)break;line++;width=0; }
        /* Match draw's admitted oversized-first-glyph behavior while keeping
         * subtraction bounded on the next iteration. */
        width=advance>max_width-width?max_width:width+advance;cursor=next;
    }
    return (size_t)(cursor-text);
}
static void write_color(uint8_t *dst,uint32_t c) { dst[0]=(uint8_t)(c>>16);dst[1]=(uint8_t)(c>>8);dst[2]=(uint8_t)c;dst[3]=255; }
void hg_font_draw(HGFont *f,const char *text,uint8_t *out,unsigned w,unsigned h,int x,int y,unsigned max_width,uint32_t fg,uint32_t shadow) {
    if(!w || !h || w>4096 || h>4096 || x<0 || y<0 || (unsigned)x>=w || (unsigned)y>=h || !max_width)return;
    unsigned start=(unsigned)x,dx=start,dy=(unsigned)y;
    while(*text && dy<h) {
        u16 id=next_glyph(&text);if(id==CHAR_LF) { dx=start;dy+=16;continue; }
        _Alignas(4) GlyphInfo glyph={0};TryLoadGlyph(f->data,id,&glyph);
        if(dx>start && (dx-start+glyph.width>max_width)) { dx=start;dy+=16; }
        if(dy>=h)break;
        for(unsigned py=0;py<glyph.height && py<16;py++)for(unsigned px=0;px<glyph.width && px<16;px++) {
            unsigned index=((py/8)*2+px/8)*32+(py%8)*4+(px%8)/2;
            unsigned color=(glyph.data[index]>>((px%2)*4))&15;
            if(color && dx+px<w && dy+py<h && dx-start+px<max_width)write_color(out+((size_t)(dy+py)*w+dx+px)*4,color==1?fg:shadow);
        }
        dx+=glyph.width;
    }
}
