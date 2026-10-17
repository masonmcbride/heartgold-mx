#include "ds_scene_graphics.h"
#include <math.h>
#include <string.h>
static uint16_t rd16(const uint8_t *p) { return p[0]|(uint16_t)p[1]<<8; }
static uint32_t rd32(const uint8_t *p) { return rd16(p)|(uint32_t)rd16(p+2)<<16; }
static bool span(size_t size,size_t offset,size_t length) { return offset<=size && length<=size-offset; }
static DSBlob block(DSBlob file,const char *magic,const char *tag) {
    DSBlob found={0};const uint8_t *p=file.data;size_t n=file.size;
    if(n<16 || memcmp(p,magic,4) || rd16(p+4)!=0xfeff || rd32(p+8)!=n || rd16(p+12)!=16)return found;
    size_t off=16;
    for(unsigned i=0;i<rd16(p+14);i++) {
        if(!span(n,off,8))return (DSBlob){0};size_t length=rd32(p+off+4);
        if(length<8 || !span(n,off,length))return (DSBlob){0};
        if(!memcmp(p+off,tag,4)) { if(found.data)return (DSBlob){0};found=(DSBlob){p+off,length}; }
        off+=length;
    }
    return off==n?found:(DSBlob){0};
}
static DSBlob pixels(DSBlob file,bool palette) {
    DSBlob b=block(file,palette?"RLCN":"RGCN",palette?"TTLP":"RAHC");
    if(!b.data || b.size<(palette?24:32) || rd32(b.data+(palette?8:12))!=3)return (DSBlob){0};
    size_t offset=8+(size_t)rd32(b.data+(palette?20:28)),length=rd32(b.data+(palette?16:24));
    if(!span(b.size,offset,length))return (DSBlob){0};return (DSBlob){b.data+offset,length};
}
static void color(uint8_t *out,const uint8_t *p) {
    uint16_t c=rd16(p);for(unsigned i=0;i<3;i++) { unsigned v=(c>>(i*5))&31;out[i]=(v<<3)|(v>>2); }out[3]=255;
}
bool ds_draw_tilemap(DSBlob chars,DSBlob pal,DSBlob screen,uint8_t *out,unsigned w,unsigned h) {
    if(!w || !h || w>256 || h>192)return false;
    DSBlob g=pixels(chars,false),p=pixels(pal,true),s=block(screen,"RCSN","NRCS");
    if(!g.data || !p.data || !s.data || s.size<20 || rd32(s.data+12)!=0)return false;
    unsigned sw=rd16(s.data+8),sh=rd16(s.data+10);size_t length=rd32(s.data+16);
    if((sw!=256 && sw!=512) || (sh!=256 && sh!=512) || length!=(size_t)sw*sh/32 || !span(s.size,20,length))return false;
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
        unsigned tx=x/8,ty=y/8;size_t map=((ty/32)*(sw/256)+tx/32)*1024+(ty%32)*32+tx%32;
        uint16_t entry=rd16(s.data+20+map*2);unsigned px=x%8,py=y%8;
        if(entry&0x400)px=7-px;if(entry&0x800)py=7-py;
        size_t index=(size_t)(entry&1023)*32+py*4+px/2;
        if(index>=g.size)return false;unsigned value=(g.data[index]>>((px%2)*4))&15;
        size_t pc=((entry>>12)*16+value)*2;if(!span(p.size,pc,2))return false;
        color(out+(y*w+x)*4,p.data+pc); /* Background palette zero is opaque. */
    }
    return true;
}
bool ds_animation_frame(DSBlob animation,unsigned sequence,uint64_t tick,DSAnimFrame *out) {
    DSBlob b=block(animation,"RNAN","KNBA");if(!b.data || b.size<32 || sequence>=rd16(b.data+8))return false;
    size_t seq=8+(size_t)rd32(b.data+12),frames=8+(size_t)rd32(b.data+16),content=8+(size_t)rd32(b.data+20);
    if(!span(b.size,seq,(size_t)rd16(b.data+8)*16) || !span(b.size,frames,(size_t)rd16(b.data+10)*8) || content>b.size)return false;
    const uint8_t *s=b.data+seq+sequence*16;unsigned count=rd16(s),loop=rd16(s+2),element=rd16(s+4),mode=rd32(s+8);
    if(!count || loop>=count || element>2 || rd16(s+6)!=1 || (mode!=1 && mode!=2))return false;
    size_t offset=rd32(s+12);if(offset%8 || offset>(size_t)rd16(b.data+10)*8 || count>((size_t)rd16(b.data+10)*8-offset)/8)return false;
    frames+=offset;uint64_t duration=0,prefix=0;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *f=b.data+frames+i*8;unsigned d=rd16(f+4);if(!d)return false;
        size_t co=rd32(f);unsigned need=element==0?2:element==1?16:8;
        if(co>b.size-content || !span(b.size,content+co,need))return false;
        if(i<loop)prefix+=d;duration+=d;
    }
    if(tick>=duration)tick=mode==2?prefix+(tick-prefix)%(duration-prefix):duration-1;
    unsigned chosen=0;
    for(;chosen<count-1;chosen++) { unsigned d=rd16(b.data+frames+chosen*8+4);if(tick<d)break;tick-=d; }
    const uint8_t *v=b.data+content+rd32(b.data+frames+chosen*8);
    *out=(DSAnimFrame){.cell=rd16(v),.scale_x=4096,.scale_y=4096};
    if(element==1) { out->rotation=rd16(v+2);out->scale_x=(int32_t)rd32(v+4);out->scale_y=(int32_t)rd32(v+8);out->x=(int16_t)rd16(v+12);out->y=(int16_t)rd16(v+14); }
    else if(element==2) { out->x=(int16_t)rd16(v+4);out->y=(int16_t)rd16(v+6); }
    return true;
}
bool ds_draw_cell(DSBlob chars,DSBlob pal,DSBlob cells,unsigned cell,uint8_t *out,unsigned w,unsigned h,int ox,int oy) {
    if(!w || !h || w>256 || h>192 || ox< -1024 || ox>1024 || oy< -1024 || oy>1024)return false;
    DSBlob g=pixels(chars,false),p=pixels(pal,true),b=block(cells,"RECN","KBEC");
    if(!g.data || !p.data || !b.data || b.size<32 || cell>=rd16(b.data+8) || rd16(b.data+10)>1 || rd32(b.data+20))return false;
    unsigned stride=rd16(b.data+10)?16:8,mapping=rd32(b.data+16);if(mapping>3)return false;
    size_t table=8+(size_t)rd32(b.data+12),table_length=(size_t)rd16(b.data+8)*stride;
    if(!span(b.size,table,table_length))return false;
    size_t oams=table+table_length;const uint8_t *c=b.data+table+cell*stride;unsigned count=rd16(c);size_t rel=rd32(c+4);
    if(count>128 || rel>b.size-oams || !span(b.size,oams+rel,(size_t)count*6))return false;
    static const unsigned widths[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}};
    static const unsigned heights[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};
    for(unsigned i=count;i>0;i--) {
        const uint8_t *o=b.data+oams+rel+(i-1)*6;uint16_t a=rd16(o),d=rd16(o+2),e=rd16(o+4);
        if(a&0x100)return false; /* Per-object affine matrices need a separate runtime. */
        if(a&0x200)continue;if(a&0x3c00 || (a>>14)==3)return false;
        unsigned shape=a>>14,size=d>>14,sw=widths[shape][size],sh=heights[shape][size];
        int x=(d&511)>=256?(int)(d&511)-512:(d&511),y=(a&255)>=128?(int)(a&255)-256:(a&255);
        size_t base=(size_t)(e&1023)*(32u<<mapping);
        if(!span(g.size,base,(size_t)sw*sh/2))return false;
        for(unsigned py=0;py<sh;py++)for(unsigned px=0;px<sw;px++) {
            unsigned sx=d&0x1000?sw-1-px:px,sy=d&0x2000?sh-1-py:py;
            size_t tile=(sy/8)*(sw/8)+sx/8,index=base+tile*32+(sy%8)*4+(sx%8)/2;
            unsigned value=(g.data[index]>>((sx%2)*4))&15;if(!value)continue;
            size_t pc=((e>>12)*16+value)*2;if(!span(p.size,pc,2))return false;
            int dx=ox+x+(int)px,dy=oy+y+(int)py;
            if(dx>=0 && dy>=0 && (unsigned)dx<w && (unsigned)dy<h)color(out+((unsigned)dy*w+(unsigned)dx)*4,p.data+pc);
        }
    }
    return true;
}
bool ds_draw_animation(DSBlob chars,DSBlob pal,DSBlob cells,DSBlob anim,unsigned sequence,uint64_t tick,uint8_t *out,unsigned w,unsigned h,int ox,int oy) {
    if(ox< -1024 || ox>1024 || oy< -1024 || oy>1024)return false;
    DSAnimFrame f;if(!ds_animation_frame(anim,sequence,tick,&f))return false;
    if(f.scale_x==4096 && f.scale_y==4096 && !f.rotation)return ds_draw_cell(chars,pal,cells,f.cell,out,w,h,ox+f.x,oy+f.y);
    if(!w || !h || w>256 || h>192 || !f.scale_x || !f.scale_y || f.scale_x< -32768 || f.scale_x>32768 || f.scale_y< -32768 || f.scale_y>32768)return false;
    uint8_t sprite[128*128*4]={0};
    if(!ds_draw_cell(chars,pal,cells,f.cell,sprite,128,128,64,64))return false;
    double angle=f.rotation*(6.283185307179586/65536.0),cs=cos(angle),sn=sin(angle),sx=f.scale_x/4096.0,sy=f.scale_y/4096.0;
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
        double dx=(double)x-ox-f.x,dy=(double)y-oy-f.y;
        int px=(int)floor((cs*dx+sn*dy)/sx+64.5),py=(int)floor((-sn*dx+cs*dy)/sy+64.5);
        if(px>=0 && py>=0 && px<128 && py<128) { const uint8_t *p=sprite+((unsigned)py*128+(unsigned)px)*4;if(p[3])memcpy(out+(y*w+x)*4,p,4); }
    }
    return true;
}
