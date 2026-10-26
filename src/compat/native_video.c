#include "compat/native.h"
#include "compat/native_video.h"
static DSVideo video;
DSVideo *hg_native_video(void){return &video;}
void hg_native_video_blend_main(void){ds_video_blend_none(&video,0);}
void hg_native_video_blend_sub(void){ds_video_blend_none(&video,1);}
void hg_native_video_windows_reset(unsigned engine){GF_ASSERT(engine<2);ds_video_windows(&video,engine,0);}
int hg_native_video_reset_BG(void){unsigned previous;GF_ASSERT(ds_video_route(&video,0,0,&previous));return previous;}
int hg_native_video_set_BG(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,0,mask,&previous));return previous;}
int hg_native_video_reset_BGExtPltt(void){unsigned previous;GF_ASSERT(ds_video_route(&video,1,0,&previous));return previous;}
int hg_native_video_set_BGExtPltt(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,1,mask,&previous));return previous;}
int hg_native_video_reset_SubBG(void){unsigned previous;GF_ASSERT(ds_video_route(&video,2,0,&previous));return previous;}
int hg_native_video_set_SubBG(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,2,mask,&previous));return previous;}
int hg_native_video_reset_SubBGExtPltt(void){unsigned previous;GF_ASSERT(ds_video_route(&video,3,0,&previous));return previous;}
int hg_native_video_set_SubBGExtPltt(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,3,mask,&previous));return previous;}
int hg_native_video_reset_OBJ(void){unsigned previous;GF_ASSERT(ds_video_route(&video,4,0,&previous));return previous;}
int hg_native_video_set_OBJ(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,4,mask,&previous));return previous;}
int hg_native_video_reset_OBJExtPltt(void){unsigned previous;GF_ASSERT(ds_video_route(&video,5,0,&previous));return previous;}
int hg_native_video_set_OBJExtPltt(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,5,mask,&previous));return previous;}
int hg_native_video_reset_SubOBJ(void){unsigned previous;GF_ASSERT(ds_video_route(&video,6,0,&previous));return previous;}
int hg_native_video_set_SubOBJ(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,6,mask,&previous));return previous;}
int hg_native_video_reset_SubOBJExtPltt(void){unsigned previous;GF_ASSERT(ds_video_route(&video,7,0,&previous));return previous;}
int hg_native_video_set_SubOBJExtPltt(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,7,mask,&previous));return previous;}
int hg_native_video_reset_Tex(void){unsigned previous;GF_ASSERT(ds_video_route(&video,8,0,&previous));return previous;}
int hg_native_video_set_Tex(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,8,mask,&previous));return previous;}
int hg_native_video_reset_TexPltt(void){unsigned previous;GF_ASSERT(ds_video_route(&video,9,0,&previous));return previous;}
int hg_native_video_set_TexPltt(int mask){unsigned previous;GF_ASSERT(ds_video_route(&video,9,mask,&previous));return previous;}

static void control(unsigned mask,unsigned value){unsigned old=video.engine[0][0x60]|(unsigned)video.engine[0][0x61]<<8;old=(old&~mask)|value;video.engine[0][0x60]=old;video.engine[0][0x61]=old>>8;}
void hg_native_geometry_init(void){video.geometry_initialized=1;video.viewport[0]=video.viewport[1]=0;video.viewport[2]=255;video.viewport[3]=191;}
void hg_native_matrix_init(void){GF_ASSERT(video.geometry_initialized);video.matrix_epoch++;}
void hg_native_swap_buffers(int sort,int buffer){GF_ASSERT((sort==0||sort==1)&&(buffer==0||buffer==1));video.sort_mode=sort;video.buffer_mode=buffer;}
void hg_native_bg0_priority(unsigned priority){GF_ASSERT(priority<4);video.engine[0][8]=(video.engine[0][8]&~3u)|priority;}
void hg_native_visible_planes(unsigned planes){GF_ASSERT(planes<32);ds_video_set_display(&video,0,(ds_video_display(&video,0)&~0x1f00u)|(planes<<8));}
void hg_native_shading(int mode){GF_ASSERT(mode==0||mode==1);control(2,mode<<1);}
void hg_native_antialias(int enable){control(16,enable?16:0);}
void hg_native_alpha_test(int enable,unsigned ref){GF_ASSERT(ref<32);video.alpha_reference=ref;control(4,enable?4:0);}
void hg_native_alpha_blend(int enable){control(8,enable?8:0);}
void hg_native_edge_marking(int enable){control(32,enable?32:0);}
void hg_native_fog(int enable,int mode,int slope,unsigned offset){GF_ASSERT((mode==0||mode==1)&&slope<11&&slope>=0&&offset<0x8000);video.fog_offset=offset;control(0x0fc0,(enable?128:0)|(mode<<6)|(slope<<8));}
void hg_native_clear_color(unsigned color,unsigned alpha,unsigned depth,unsigned id,int fog){GF_ASSERT(color<0x8000&&alpha<32&&depth<0x8000&&id<64);video.clear_color=color|(fog?0x8000:0)|(alpha<<16)|(id<<24);video.clear_depth=depth;}
void hg_native_viewport(unsigned x1,unsigned y1,unsigned x2,unsigned y2){GF_ASSERT(x1<=x2&&y1<=y2&&x2<256&&y2<192);video.viewport[0]=x1;video.viewport[1]=y1;video.viewport[2]=x2;video.viewport[3]=y2;}
int hg_native_obj_bank(void){return video.banks[4];}
int hg_native_sub_obj_bank(void){return video.banks[6];}
int hg_native_obj_palette_bank(void){return video.banks[5];}
int hg_native_sub_obj_palette_bank(void){return video.banks[7];}
static void obj_mode(unsigned engine,int mode){GF_ASSERT(mode==0||mode==0x10||mode==0x100010||mode==0x200010||mode==0x300010);ds_video_set_display(&video,engine,(ds_video_display(&video,engine)&~0x300010u)|mode);}
void hg_native_obj_mode(int mode){obj_mode(0,mode);}
void hg_native_sub_obj_mode(int mode){obj_mode(1,mode);}
static void oam_load(unsigned engine,const void *bytes,unsigned offset,unsigned size){GF_ASSERT(engine<2&&bytes&&offset<=0x400&&size<=0x400-offset&&!(offset&1)&&!(size&1));memcpy(video.oam[engine]+offset,bytes,size);}
void hg_native_oam_load_main(const void *bytes,unsigned offset,unsigned size){oam_load(0,bytes,offset,size);}
void hg_native_oam_load_sub(const void *bytes,unsigned offset,unsigned size){oam_load(1,bytes,offset,size);}
void hg_native_cache_publish(const void *bytes,unsigned size){(void)bytes;(void)size;__atomic_thread_fence(__ATOMIC_RELEASE);}
void hg_native_display_select(int select){GF_ASSERT(select==0||select==1);video.display_select=select;}

static unsigned read_half(const u8 *p){return p[0]|(unsigned)p[1]<<8;}
static void write_half(u8 *p,unsigned value){p[0]=value;p[1]=value>>8;}
void hg_native_bg_control(unsigned e,unsigned layer,int size,int color,int screen,int chars,int ext){
    GF_ASSERT(e<2&&layer<4&&size>=0&&size<4&&(color==0||color==1)&&screen>=0&&screen<32&&chars>=0&&chars<16&&(ext==0||ext==1));
    u8 *p=video.engine[e]+8+layer*2;unsigned old=read_half(p);
    write_half(p,(old&0x43)|(chars<<2)|(color<<7)|(screen<<8)|(ext<<13)|(size<<14));
}
void hg_native_bg_priority(unsigned e,unsigned layer,unsigned priority){GF_ASSERT(e<2&&layer<4&&priority<4);u8 *p=video.engine[e]+8+layer*2;write_half(p,(read_half(p)&~3u)|priority);}
void hg_native_bg_mosaic(unsigned e,unsigned layer,int enable){GF_ASSERT(e<2&&layer<4);u8 *p=video.engine[e]+8+layer*2;write_half(p,(read_half(p)&~64u)|(enable?64:0));}
void hg_native_bg_scroll(unsigned e,unsigned layer,unsigned x,unsigned y){GF_ASSERT(e<2&&layer<4);u8 *p=video.engine[e]+0x10+layer*4;write_half(p,x&511);write_half(p+2,y&511);}
void hg_native_bg_load(unsigned e,unsigned layer,int chars,const void *p,unsigned offset,unsigned size){
    GF_ASSERT(e<2&&layer<4);unsigned control=read_half(video.engine[e]+8+layer*2);
    unsigned global=e?0:ds_video_display(&video,0);
    size_t base=chars?(((control>>2)&15)*0x4000u+((global>>24)&7)*0x10000u):(((control>>8)&31)*0x800u+((global>>27)&7)*0x10000u);
    GF_ASSERT(offset<=SIZE_MAX-base&&ds_video_transfer(&video,e?2:0,base+offset,p,size));
}
void hg_native_graphics_mode(int display,int bg,int mode){GF_ASSERT((display==0||display==0x10000)&&bg>=0&&bg<=6&&(mode==0||mode==8));ds_video_set_display(&video,0,(ds_video_display(&video,0)&~0xf000fu)|display|bg|mode);}
void hg_native_sub_graphics_mode(int bg){GF_ASSERT(bg>=0&&bg<=6);ds_video_set_display(&video,1,(ds_video_display(&video,1)&~7u)|bg);}
void hg_native_bg_char_offset(int offset){GF_ASSERT(offset==0);ds_video_set_display(&video,0,ds_video_display(&video,0)&~0x07000000u);}
void hg_native_bg_screen_offset(int offset){GF_ASSERT(offset==0);ds_video_set_display(&video,0,ds_video_display(&video,0)&~0x38000000u);}
void hg_native_visible_sub_planes(unsigned planes){GF_ASSERT(planes<32);ds_video_set_display(&video,1,(ds_video_display(&video,1)&~0x1f00u)|(planes<<8));}
void hg_native_bg_palette_main(const void *bytes,unsigned offset,unsigned size){GF_ASSERT(bytes&&offset<=512&&size<=512-offset);memcpy(video.palette[0]+offset,bytes,size);}
static void palette_load(unsigned engine,unsigned object,const void *bytes,unsigned offset,unsigned size){GF_ASSERT(engine<2&&object<2&&bytes&&offset<=512&&size<=512-offset);memcpy(video.palette[engine]+object*512+offset,bytes,size);}
void hg_native_obj_palette_main(const void *bytes,unsigned offset,unsigned size){palette_load(0,1,bytes,offset,size);}
void hg_native_bg_palette_sub(const void *bytes,unsigned offset,unsigned size){palette_load(1,0,bytes,offset,size);}
void hg_native_obj_palette_sub(const void *bytes,unsigned offset,unsigned size){palette_load(1,1,bytes,offset,size);}

void hg_native_display_main_on(void){
    unsigned old=ds_video_display(&video,0);
    GF_ASSERT((old&0x30000u)==0x10000u);
    ds_video_set_display(&video,0,old&~0x80u);
}
void hg_native_display_sub_on(void){
    unsigned old=ds_video_display(&video,1);
    ds_video_set_display(&video,1,(old&~0x80u)|0x10000u);
}
void hg_native_global_polygon_attr(unsigned light,unsigned mode,unsigned cull,unsigned id,unsigned alpha,unsigned misc){
    GF_ASSERT(light<16&&mode<4&&cull<4&&id<64&&alpha<32&&!(misc&~0xf800u));
    hg_native_video()->global_polygon_attr=light|(mode<<4)|(cull<<6)|(alpha<<16)|(id<<24)|misc;
}
