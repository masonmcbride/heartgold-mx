#include "ds_video.h"
#include <assert.h>
#include <string.h>
uint32_t ds_video_display(const DSVideo *v,unsigned engine){assert(engine<2);const uint8_t *p=v->engine[engine];return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
void ds_video_set_display(DSVideo *v,unsigned engine,uint32_t value){assert(engine<2);for(unsigned i=0;i<4;i++)v->engine[engine][i]=value>>(8*i);}
void ds_video_blend_none(DSVideo *v,unsigned engine){assert(engine<2);v->engine[engine][0x50]=v->engine[engine][0x51]=0;}
void ds_video_windows(DSVideo *v,unsigned engine,unsigned mask){assert(engine<2&&mask<8);ds_video_set_display(v,engine,(ds_video_display(v,engine)&~0xe000u)|(mask<<13));}
int ds_video_route(DSVideo *v,unsigned group,unsigned mask,unsigned *previous){
    static const unsigned allowed[]={0x0f,0x70,0x84,0x80,0x73,0x60,0x108,0x100,0x0f,0x70};
    if(group>=10||(mask&~allowed[group]))return 0;
    for(unsigned i=0;i<10;i++)if(i!=group&&(mask&v->banks[i]))return 0;
    if(previous)*previous=v->banks[group];v->banks[group]=mask;return 1;
}
int ds_video_transfer(DSVideo *v,unsigned group,size_t offset,const void *bytes,size_t size){
    static const size_t base[]={0,0x20000,0x40000,0x60000,0x80000,0x90000,0x94000,0x98000,0xa0000};
    static const size_t lengths[]={0x20000,0x20000,0x20000,0x20000,0x10000,0x4000,0x4000,0x8000,0x4000};
    if(!v||group>=10||(!bytes&&size))return 0;
    unsigned mask=v->banks[group];
    if(!((group==0&&mask==4)||(group==2&&mask==128)||(group==4&&mask==96)||(group==6&&mask==256)))return 0;
    size_t capacity=0;for(unsigned i=0;i<9;i++)if(mask&(1u<<i))capacity+=lengths[i];
    if(offset>capacity||size>capacity-offset)return 0;
    const uint8_t *source=bytes;
    for(unsigned i=0;i<9&&size;i++)if(mask&(1u<<i)){
        if(offset>=lengths[i]){offset-=lengths[i];continue;}
        size_t count=lengths[i]-offset;if(count>size)count=size;
        memcpy(v->vram+base[i]+offset,source,count);source+=count;size-=count;offset=0;
    }
    return 1;
}
