#pragma once
#include "ds_video.h"
DSVideo *hg_native_video(void);
void hg_native_video_blend_main(void);
void hg_native_video_blend_sub(void);
void hg_native_video_windows_reset(unsigned);
int hg_native_video_reset_BG(void);
int hg_native_video_set_BG(int);
int hg_native_video_reset_BGExtPltt(void);
int hg_native_video_set_BGExtPltt(int);
int hg_native_video_reset_SubBG(void);
int hg_native_video_set_SubBG(int);
int hg_native_video_reset_SubBGExtPltt(void);
int hg_native_video_set_SubBGExtPltt(int);
int hg_native_video_reset_OBJ(void);
int hg_native_video_set_OBJ(int);
int hg_native_video_reset_OBJExtPltt(void);
int hg_native_video_set_OBJExtPltt(int);
int hg_native_video_reset_SubOBJ(void);
int hg_native_video_set_SubOBJ(int);
int hg_native_video_reset_SubOBJExtPltt(void);
int hg_native_video_set_SubOBJExtPltt(int);
int hg_native_video_reset_Tex(void);
int hg_native_video_set_Tex(int);
int hg_native_video_reset_TexPltt(void);
int hg_native_video_set_TexPltt(int);

void hg_native_geometry_init(void);
void hg_native_matrix_init(void);
void hg_native_swap_buffers(int,int);
void hg_native_bg0_priority(unsigned);
void hg_native_visible_planes(unsigned);
void hg_native_shading(int);
void hg_native_antialias(int);
void hg_native_alpha_test(int,unsigned);
void hg_native_alpha_blend(int);
void hg_native_edge_marking(int);
void hg_native_fog(int,int,int,unsigned);
void hg_native_clear_color(unsigned,unsigned,unsigned,unsigned,int);
void hg_native_viewport(unsigned,unsigned,unsigned,unsigned);
int hg_native_obj_bank(void);
int hg_native_sub_obj_bank(void);
int hg_native_obj_palette_bank(void);
int hg_native_sub_obj_palette_bank(void);
void hg_native_obj_mode(int);
void hg_native_sub_obj_mode(int);
void hg_native_oam_load_main(const void *,unsigned,unsigned);
void hg_native_oam_load_sub(const void *,unsigned,unsigned);
void hg_native_cache_publish(const void *,unsigned);
void hg_native_display_select(int);

void hg_native_bg_control(unsigned,unsigned,int,int,int,int,int);
void hg_native_bg_priority(unsigned,unsigned,unsigned);
void hg_native_bg_mosaic(unsigned,unsigned,int);
void hg_native_bg_scroll(unsigned,unsigned,unsigned,unsigned);
void hg_native_bg_load(unsigned,unsigned,int,const void *,unsigned,unsigned);
void hg_native_graphics_mode(int,int,int);
void hg_native_sub_graphics_mode(int);
void hg_native_bg_char_offset(int);
void hg_native_bg_screen_offset(int);
void hg_native_visible_sub_planes(unsigned);
void hg_native_bg_palette_main(const void *,unsigned,unsigned);
static inline void hg_native_bg0_0_control(int size,int color,int screen,int chars,int ext){hg_native_bg_control(0,0,size,color,screen,chars,ext);}
static inline void hg_native_bg0_0_priority(unsigned p){hg_native_bg_priority(0,0,p);}
static inline void hg_native_bg0_0_mosaic(int on){hg_native_bg_mosaic(0,0,on);}
static inline void hg_native_bg0_0_scroll(unsigned x,unsigned y){hg_native_bg_scroll(0,0,x,y);}
static inline void hg_native_bg0_0_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,0,0,p,offset,n);}
static inline void hg_native_bg0_0_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,0,1,p,offset,n);}
static inline void hg_native_bg0_1_control(int size,int color,int screen,int chars,int ext){hg_native_bg_control(0,1,size,color,screen,chars,ext);}
static inline void hg_native_bg0_1_priority(unsigned p){hg_native_bg_priority(0,1,p);}
static inline void hg_native_bg0_1_mosaic(int on){hg_native_bg_mosaic(0,1,on);}
static inline void hg_native_bg0_1_scroll(unsigned x,unsigned y){hg_native_bg_scroll(0,1,x,y);}
static inline void hg_native_bg0_1_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,1,0,p,offset,n);}
static inline void hg_native_bg0_1_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,1,1,p,offset,n);}
static inline void hg_native_bg0_2_control(int size,int color,int screen,int chars){hg_native_bg_control(0,2,size,color,screen,chars,0);}
static inline void hg_native_bg0_2_priority(unsigned p){hg_native_bg_priority(0,2,p);}
static inline void hg_native_bg0_2_mosaic(int on){hg_native_bg_mosaic(0,2,on);}
static inline void hg_native_bg0_2_scroll(unsigned x,unsigned y){hg_native_bg_scroll(0,2,x,y);}
static inline void hg_native_bg0_2_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,2,0,p,offset,n);}
static inline void hg_native_bg0_2_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,2,1,p,offset,n);}
static inline void hg_native_bg0_3_control(int size,int color,int screen,int chars){hg_native_bg_control(0,3,size,color,screen,chars,0);}
static inline void hg_native_bg0_3_priority(unsigned p){hg_native_bg_priority(0,3,p);}
static inline void hg_native_bg0_3_mosaic(int on){hg_native_bg_mosaic(0,3,on);}
static inline void hg_native_bg0_3_scroll(unsigned x,unsigned y){hg_native_bg_scroll(0,3,x,y);}
static inline void hg_native_bg0_3_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,3,0,p,offset,n);}
static inline void hg_native_bg0_3_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(0,3,1,p,offset,n);}
static inline void hg_native_bg1_0_control(int size,int color,int screen,int chars,int ext){hg_native_bg_control(1,0,size,color,screen,chars,ext);}
static inline void hg_native_bg1_0_priority(unsigned p){hg_native_bg_priority(1,0,p);}
static inline void hg_native_bg1_0_mosaic(int on){hg_native_bg_mosaic(1,0,on);}
static inline void hg_native_bg1_0_scroll(unsigned x,unsigned y){hg_native_bg_scroll(1,0,x,y);}
static inline void hg_native_bg1_0_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,0,0,p,offset,n);}
static inline void hg_native_bg1_0_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,0,1,p,offset,n);}
static inline void hg_native_bg1_1_control(int size,int color,int screen,int chars,int ext){hg_native_bg_control(1,1,size,color,screen,chars,ext);}
static inline void hg_native_bg1_1_priority(unsigned p){hg_native_bg_priority(1,1,p);}
static inline void hg_native_bg1_1_mosaic(int on){hg_native_bg_mosaic(1,1,on);}
static inline void hg_native_bg1_1_scroll(unsigned x,unsigned y){hg_native_bg_scroll(1,1,x,y);}
static inline void hg_native_bg1_1_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,1,0,p,offset,n);}
static inline void hg_native_bg1_1_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,1,1,p,offset,n);}
static inline void hg_native_bg1_2_control(int size,int color,int screen,int chars){hg_native_bg_control(1,2,size,color,screen,chars,0);}
static inline void hg_native_bg1_2_priority(unsigned p){hg_native_bg_priority(1,2,p);}
static inline void hg_native_bg1_2_mosaic(int on){hg_native_bg_mosaic(1,2,on);}
static inline void hg_native_bg1_2_scroll(unsigned x,unsigned y){hg_native_bg_scroll(1,2,x,y);}
static inline void hg_native_bg1_2_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,2,0,p,offset,n);}
static inline void hg_native_bg1_2_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,2,1,p,offset,n);}
static inline void hg_native_bg1_3_control(int size,int color,int screen,int chars){hg_native_bg_control(1,3,size,color,screen,chars,0);}
static inline void hg_native_bg1_3_priority(unsigned p){hg_native_bg_priority(1,3,p);}
static inline void hg_native_bg1_3_mosaic(int on){hg_native_bg_mosaic(1,3,on);}
static inline void hg_native_bg1_3_scroll(unsigned x,unsigned y){hg_native_bg_scroll(1,3,x,y);}
static inline void hg_native_bg1_3_screen(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,3,0,p,offset,n);}
static inline void hg_native_bg1_3_char(const void *p,unsigned offset,unsigned n){hg_native_bg_load(1,3,1,p,offset,n);}
void hg_native_obj_palette_main(const void *,unsigned,unsigned);
void hg_native_bg_palette_sub(const void *,unsigned,unsigned);
void hg_native_obj_palette_sub(const void *,unsigned,unsigned);

/* Opening graphics-mode admission; other display restore modes remain open. */
void hg_native_display_main_on(void);
void hg_native_display_sub_on(void);
void hg_native_global_polygon_attr(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
