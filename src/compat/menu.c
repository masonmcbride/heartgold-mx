#include "list_menu_cursor.h"
/* Native text/layout is drawn by AppKit. This adapter tracks invalidation and
 * cursor placement for the original selection engine, not DS pixel rasterization. */
struct ListMenuCursor { u32 color; };
struct ListMenuCursor *ListMenuCursorNew(enum HeapID h) { return Heap_Alloc(h,sizeof(struct ListMenuCursor)); }
void ListMenuCursorSetColor(struct ListMenuCursor *p,u32 c) { p->color=c; }
void DestroyListMenuCursorObj(struct ListMenuCursor *p) { Heap_Free(p); }
void ListMenuUpdateCursorObj(struct ListMenuCursor *p,Window *w,u8 x,u8 y) { (void)p;w->cursor_x=x;w->cursor_y=y;w->revision++; }
void FillWindowPixelBuffer(Window *w,u8 c) { (void)c;w->revision++; }
void CopyWindowToVram(Window *w) { w->revision++; }
void FillWindowPixelRect(Window *w,u8 c,u16 x,u16 y,u16 width,u16 height) { (void)c;(void)x;(void)y;(void)width;(void)height;w->revision++; }
void ScrollWindow(Window *w,u8 d,u8 n,u8 c) { (void)d;(void)n;(void)c;w->revision++; }
u16 GetWindowWidth(Window *w) { return w->width; }
u16 GetWindowHeight(Window *w) { return w->height; }
u8 GetFontAttribute(u8 font,int attr) { (void)font;if(attr!=1)abort();return 16; }
/* Full text-printer task and focus-indicator paths remain unsupported. */
u16 *String_cstr(String *s) { (void)s;abort(); }
RenderResult FontID_RenderText(enum Font f,TextPrinter *p) { (void)f;(void)p;abort(); }
void *GfGfxLoader_GetCharData(NarcId a,s32 m,BOOL c,NNSG2dCharacterData **p,enum HeapID h) { (void)a;(void)m;(void)c;(void)p;(void)h;abort(); }
void MI_CpuCopy32(const void *s,void *d,u32 n) { memcpy(d,s,n); }
void BlitBitmapRectToWindow(Window *w,const void *s,u16 a,u16 b,u16 c,u16 d,u16 e,u16 f,u16 g,u16 h) { (void)w;(void)s;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;abort(); }
