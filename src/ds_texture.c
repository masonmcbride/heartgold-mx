#include "ds_texture.h"
#include <string.h>
#include <math.h>
static unsigned r16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t r32(const uint8_t *p) { return r16(p)|(uint32_t)r16(p+2)<<16; }
static int span(size_t n,size_t o,size_t z) { return o<=n && z<=n-o; }
static const uint8_t *lookup(const uint8_t *p,size_t n,size_t dict,unsigned stride,const char *name) {
    if(!span(n,dict,8))return NULL;const uint8_t *d=p+dict;size_t size=r16(d+2),table=r16(d+6);unsigned count=d[1];
    if(size>n-dict || !span(size,table,4) || r16(d+table)!=stride)return NULL;
    size_t names=table+r16(d+table+2);
    if(names<table+4+stride*count || !span(size,table+4,stride*count) || !span(size,names,16*count))return NULL;
    char key[16]={0};size_t len=strlen(name);if(len>16)return NULL;memcpy(key,name,len);
    for(unsigned i=0;i<count;i++)if(!memcmp(d+names+16*i,key,16))return d+table+4+stride*i;
    return NULL;
}
int ds_texture_bank(const uint8_t *p,size_t n,DSTextureBank *out) {
    if(!p || !out || n<20 || r16(p+4)!=0xfeff || r32(p+8)!=n || r16(p+12)!=16)return 0;
    unsigned blocks=r16(p+14);int btx=!memcmp(p,"BTX0",4),bmd=!memcmp(p,"BMD0",4);
    if((!btx && !bmd) || r16(p+6)!=(btx?1:2) || blocks!=(btx?1:2) || n<16+4*blocks)return 0;
    const uint8_t *texblock=NULL;size_t texsize=0,previous=16+4*blocks;
    for(unsigned i=0;i<blocks;i++) {
        size_t off=r32(p+16+4*i);if(off<previous || !span(n,off,8))return 0;size_t len=r32(p+off+4);
        if(len<8 || !span(n,off,len) || memcmp(p+off,(bmd && !i)?"MDL0":"TEX0",4))return 0;previous=off+len;
        if(!memcmp(p+off,"TEX0",4)) { if(texblock)return 0;texblock=p+off;texsize=len; }
    }
    if(previous!=n || !texblock || texsize<60)return 0;p=texblock;n=texsize;
    size_t tex=r16(p+14),pal=r32(p+52);
    if(!span(n,tex,8) || !span(n,pal,8) || !span(n,r32(p+20),(size_t)r16(p+12)*8) || !span(n,r32(p+36),(size_t)r16(p+24)*8) || !span(n,r32(p+40),(size_t)r16(p+24)*4) || !span(n,r32(p+56),(size_t)r16(p+48)*8))return 0;
    DSTextureBank v={p,n,p[tex+1],p[pal+1]};*out=v;return 1;
}
int ds_texture_find(const DSTextureBank *b,const char *name,const char *pal,DSTexture *out) {
    if(!b || !b->bytes || b->size<60 || !name || !pal || !out)return 0;const uint8_t *p=b->bytes;
    const uint8_t *e=lookup(p,b->size,r16(p+14),8,name);if(!e)return 0;uint32_t params=r32(e);
    unsigned format=(params>>26)&7,width=8u<<((params>>20)&7),height=8u<<((params>>23)&7);
    const unsigned bits[]={0,8,2,4,8,0,8,16};if(!bits[format])return 0;
    size_t length=(size_t)width*height*bits[format]/8,offset=(params&65535)*8u;
    size_t block=r32(p+20),block_size=(size_t)r16(p+12)*8;
    if(!span(block_size,offset,length) || !span(b->size,block,block_size))return 0;
    DSTexture v={p+block+offset,NULL,length,0,width,height,format,(params>>29)&1};
    if(format!=7) {
        e=lookup(p,b->size,r32(p+52),4,pal);if(!e)return 0;
        offset=r16(e)*8u;block=r32(p+56);block_size=(size_t)r16(p+48)*8;
        const unsigned colors[]={0,32,4,16,256,0,8,0};length=colors[format]*2;
        if(!span(block_size,offset,2) || !span(b->size,block,block_size))return 0;
        if(length>block_size-offset)length=block_size-offset;
        unsigned max_index=0;
        for(size_t i=0;i<v.texel_size;i++) {
            unsigned value=v.texels[i];
            if(format==1)value&=31;else if(format==6)value&=7;
            if(format==2) { for(unsigned k=0;k<4;k++)if(((value>>(2*k))&3)>max_index)max_index=(value>>(2*k))&3; }
            else if(format==3) { if((value&15)>max_index)max_index=value&15;if((value>>4)>max_index)max_index=value>>4; }
            else if(value>max_index)max_index=value;
        }
        if(max_index>=length/2)return 0;
        v.palette=p+block+offset;v.palette_size=length;
    }
    *out=v;return 1;
}
static int coord(double value,unsigned size,int repeat,int mirror) {
    if(!repeat)return value<0?0:value>=size?(int)size-1:(int)floor(value);
    int period=(int)size*(mirror?2:1);int v=(int)floor(fmod(value,period));if(v<0)v+=period;
    return mirror && v>=(int)size?period-1-v:v;
}
int ds_texture_sample(const DSTexture *t,double u,double v,uint32_t params,uint32_t *out) {
    if(!t || !t->texels || !out || !t->width || !t->height || t->width>1024 || t->height>1024 || !isfinite(u) || !isfinite(v))return 0;
    int x=coord(u,t->width,params&(1<<16),params&(1<<18)),y=coord(v,t->height,params&(1<<17),params&(1<<19));
    size_t i=(size_t)y*t->width+x;unsigned index,alpha=255,c;
    if(t->format==7) { if(i>=t->texel_size/2)return 0;c=r16(t->texels+2*i);alpha=(c&0x8000)?255:0; }
    else {
        switch(t->format) {
        case 1:case 4:case 6:if(i>=t->texel_size)return 0;index=t->texels[i];break;
        case 2:if(i/4>=t->texel_size)return 0;index=(t->texels[i/4]>>(2*(i%4)))&3;break;
        case 3:if(i/2>=t->texel_size)return 0;index=(t->texels[i/2]>>(4*(i%2)))&15;break;
        default:return 0;
        }
        if(t->format==1) { alpha=(index>>5)*255/7;index&=31; }
        else if(t->format==6) { alpha=(index>>3)*255/31;index&=7; }
        else if(t->transparent_zero && !index)alpha=0;
        if(!t->palette || index>=t->palette_size/2)return 0;c=r16(t->palette+2*index);
    }
    unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
    *out=(r*255/31)|((g*255/31)<<8)|((b*255/31)<<16)|(alpha<<24);return 1;
}
int ds_texture_name(const DSTextureBank *b,unsigned index,int palette,char name[17]) {
    if(!b || !b->bytes || b->size<60 || !name)return 0;
    size_t dict=palette?r32(b->bytes+52):r16(b->bytes+14);unsigned stride=palette?4:8;
    if(!span(b->size,dict,8))return 0;const uint8_t *d=b->bytes+dict;size_t size=r16(d+2),table=r16(d+6);
    if(size>b->size-dict || index>=d[1] || !span(size,table,4) || r16(d+table)!=stride)return 0;
    size_t names=table+r16(d+table+2);
    if(names<table+4+stride*d[1] || !span(size,names,16*d[1]))return 0;
    memcpy(name,d+names+16*index,16);name[16]=0;return 1;
}
