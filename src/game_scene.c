#include "game_scene.h"
#include "constants/items.h"
#include "item.h"
#include "game_font.h"
#include "ds_scene_graphics.h"
#include "compat/resources.h"
struct HGScene { HGFont *font;uint8_t top[256*192*4],bottom[256*192*4]; };
static DSBlob resource(int archive,int member) { DSBlob b={0};native_resource_member(archive,0,member,&b.data,&b.size);return b; }
static DSBlob bag(unsigned member) { return resource(NARC_native_bag,(int)member); }
static void rect(uint8_t *p,unsigned x,unsigned y,unsigned w,unsigned h,uint32_t color) {
    for(unsigned yy=y;yy<y+h && yy<192;yy++)for(unsigned xx=x;xx<x+w && xx<256;xx++) { uint8_t *d=p+(yy*256+xx)*4;d[0]=(uint8_t)(color>>16);d[1]=(uint8_t)(color>>8);d[2]=(uint8_t)color;d[3]=255; }
}
HGScene *hg_scene_new(void) { HGScene *s=calloc(1,sizeof(*s));if(!s)abort();s->font=hg_font_new();if(!s->font) { free(s);return NULL; }return s; }
void hg_scene_free(HGScene *s) { if(s) { hg_font_free(s->font);free(s); } }
int hg_scene_render(HGScene *s,const HGInventory *v,uint64_t tick,uint8_t out[HG_SCENE_BYTES]) {
    if(!ds_draw_tilemap(bag(7),bag(8),bag(94),s->top,256,192) || !ds_draw_tilemap(bag(46),bag(38),bag(39),s->bottom,256,192))return 0;
    unsigned pocket=(unsigned)hg_inventory_pocket(v),selected=hg_inventory_selected(v),scroll=hg_inventory_scroll(v),count=hg_inventory_count(v);
    const uint32_t ink=0x303848,shadow=0xc0c8d0;
    hg_font_draw(s->font,hg_inventory_pocket_name(pocket),s->top,256,192,12,5,200,ink,shadow);
    rect(s->top,8,24,240,130,0x788898);rect(s->top,9,25,238,128,0xf8f8f0);
    for(unsigned row=0;row<8 && scroll+row<count;row++) {
        unsigned index=scroll+row,y=26+row*16;HGItem item;if(!hg_inventory_item(v,index,&item))return 0;
        if(index==selected) { rect(s->top,10,y,236,16,0xd8e8f8);rect(s->top,10,y,3,16,0xc85830); }
        hg_font_draw(s->font,item.name,s->top,256,192,20,(int)y,176,ink,shadow);
        char qty[16];snprintf(qty,sizeof(qty),"x%u",item.quantity);unsigned width=hg_font_width(s->font,qty);
        hg_font_draw(s->font,qty,s->top,256,192,(int)(238-width),(int)y,40,ink,shadow);
    }
    if(!count)hg_font_draw(s->font,"Empty pocket",s->top,256,192,24,72,210,ink,shadow);
    HGItem item;
    if(hg_inventory_item(v,selected,&item)) {
        if(!ds_draw_animation(resource(NARC_itemtool_itemdata_item_icon,GetItemIndexMapping(item.id,ITEMNARC_NCGR)),resource(NARC_itemtool_itemdata_item_icon,GetItemIndexMapping(item.id,ITEMNARC_NCLR)),resource(NARC_itemtool_itemdata_item_icon,GetItemIconCell()),resource(NARC_itemtool_itemdata_item_icon,GetItemIconAnim()),0,tick,s->top,256,192,28,174))return 0;
        hg_font_draw(s->font,item.name,s->top,256,192,50,158,194,ink,shadow);
        char position[32];snprintf(position,sizeof(position),"%u / %u",selected+1,count);hg_font_draw(s->font,position,s->top,256,192,50,174,194,ink,shadow);
    }
    rect(s->bottom,4,3,248,51,0xf8f8f0);
    hg_font_draw(s->font,count && *item.description?item.description:"Choose a pocket. Add one to refill it.",s->bottom,256,192,8,4,240,ink,shadow);
    static const char *short_names[]={"Items","Medicine","Balls","TM/HM","Berries","Mail","Battle","Key"};
    for(unsigned p=0;p<8;p++) {
        unsigned x=(p%4)*64,y=56+(p/4)*66;
        if(p==pocket) { rect(s->bottom,x+1,y+1,62,64,0xf0b878);rect(s->bottom,x+3,y+3,58,60,0xffe8b0); }
        unsigned sequence=p==pocket?8+p:p;
        if(!ds_draw_animation(bag(2),bag(3),bag(1),bag(0),sequence,p==pocket?tick:0,s->bottom,256,192,(int)x+32,(int)y+29))return 0;
        unsigned width=hg_font_width(s->font,short_names[p]);
        hg_font_draw(s->font,short_names[p],s->bottom,256,192,(int)x+(int)((64-width)/2),(int)y+48,62,ink,shadow);
    }
    memset(out,0,HG_SCENE_BYTES);
    for(unsigned y=0;y<192;y++) { memcpy(out+(y*HG_SCENE_WIDTH)*4,s->top+y*256*4,256*4);memcpy(out+(y*HG_SCENE_WIDTH+272)*4,s->bottom+y*256*4,256*4); }
    return 1;
}
int hg_scene_click(HGInventory *v,unsigned x,unsigned y) {
    if(x<256 && x>=8 && x<248 && y>=26 && y<154) { unsigned row=hg_inventory_scroll(v)+(y-26)/16;if(row<hg_inventory_count(v)) { hg_inventory_select(v,row);return 1; } }
    if(x>=272 && x<528 && y>=56 && y<188) {
        unsigned pocket=(x-272)/64+((y-56)/66)*4;if(pocket>=8)return 0;
        while((unsigned)hg_inventory_pocket(v)!=pocket)hg_inventory_action(v,HG_NEXT_POCKET);return 1;
    }
    return 0;
}
