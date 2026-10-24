#include "ds_memory.h"
#include "ds_graphics.h"
#include "ds_irq.h"
#include "ds_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void put32(uint8_t *p,uint32_t v) { for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static void header(uint8_t *p,size_t n,const char *magic,const char *tag) { memcpy(p,magic,4);p[4]=255;p[5]=254;put32(p+8,(uint32_t)n);p[12]=16;p[14]=1;memcpy(p+16,tag,4);put32(p+20,(uint32_t)n-16); }
static unsigned irq_count;
static void irq_callback(void){irq_count++;}
int main(void) {
    irq_count=0;
    DSVideo video={0};ds_video_set_display(&video,0,0xffffffff);ds_video_set_display(&video,1,0x12345678);
    video.engine[0][0x50]=255;video.engine[0][0x51]=255;video.engine[0][0x52]=123;
    ds_video_blend_none(&video,0);CHECK(!video.engine[0][0x50]&&!video.engine[0][0x51]&&video.engine[0][0x52]==123);
    ds_video_windows(&video,0,0);CHECK(ds_video_display(&video,0)==0xffff1fff&&ds_video_display(&video,1)==0x12345678);
    unsigned previous;CHECK(ds_video_route(&video,0,4,&previous)&&!previous);
    CHECK(!ds_video_route(&video,8,4,&previous)&&!video.banks[8]);
    CHECK(!ds_video_route(&video,0,0x100,&previous)&&video.banks[0]==4);
    CHECK(ds_video_route(&video,0,0,&previous)&&previous==4);
    CHECK(ds_video_route(&video,8,4,&previous));
    uint8_t transfer[]={1,2,3,4};
    CHECK(ds_video_route(&video,4,96,&previous));
    CHECK(ds_video_transfer(&video,4,0x3ffe,transfer,4));
    CHECK(video.vram[0x93ffe]==1&&video.vram[0x93fff]==2&&video.vram[0x94000]==3&&video.vram[0x94001]==4);
    CHECK(!ds_video_transfer(&video,4,0x7fff,transfer,4)&&!video.vram[0x97fff]);
    CHECK(ds_video_transfer(&video,4,0x8000,NULL,0));
    CHECK(!ds_video_transfer(&video,4,SIZE_MAX,transfer,4));
    CHECK(ds_video_route(&video,4,0,&previous)&&!ds_video_transfer(&video,4,0,transfer,4));
    CHECK(ds_video_route(&video,4,96,&previous)&&video.vram[0x94000]==3);
    DSIrq irq;ds_irq_init(&irq);
    ds_irq_set_handler(&irq,DS_IRQ_HBLANK,irq_callback);
    ds_irq_hblank_edge(&irq);CHECK(!irq_count&&!irq.pending);
    CHECK(!ds_irq_hblank_request(&irq,1));ds_irq_hblank_edge(&irq);
    CHECK(!irq_count&&irq.pending==DS_IRQ_HBLANK);
    CHECK(ds_irq_enable_mask(&irq,DS_IRQ_HBLANK)==0&&irq_count==1&&!irq.pending);
    CHECK(ds_irq_cpu_enable(&irq,0));ds_irq_hblank_edge(&irq);ds_irq_hblank_edge(&irq);
    CHECK(irq_count==1&&irq.pending==DS_IRQ_HBLANK);
    CHECK(!ds_irq_cpu_enable(&irq,1)&&irq_count==2&&!irq.pending);
    CHECK(ds_irq_disable_mask(&irq,DS_IRQ_HBLANK)==DS_IRQ_HBLANK);
    CHECK(ds_irq_hblank_request(&irq,0));ds_irq_hblank_edge(&irq);CHECK(irq_count==2&&!irq.pending);
    DSMainRAM m={calloc(1,4*1024*1024),4*1024*1024}; CHECK(m.ram);
    CHECK(ds_translate(&m,0x02000000,4)==m.ram);
    CHECK(ds_translate(&m,0x023fffff,1)==m.ram+m.size-1);
    CHECK(!ds_translate(&m,0x023fffff,2)); CHECK(!ds_translate(&m,0xffffffff,4));
    CHECK(!ds_translate(&m,0x01ffffff,1)); CHECK(!ds_translate(&m,0x02000000,SIZE_MAX));
    uint8_t native[16]={0};DSNativeRegion region={0x02800000,native,sizeof(native)};
    m.regions=&region;m.region_count=1;
    CHECK(ds_translate(&m,region.address,sizeof(native))==native);
    *(uint8_t *)ds_translate(&m,region.address+3,1)=42;CHECK(native[3]==42);
    CHECK(!ds_translate(&m,region.address-1,1));
    CHECK(!ds_translate(&m,region.address+15,2));
    CHECK(!ds_translate(&m,region.address,SIZE_MAX));
    region.bytes=NULL;region.size=0;CHECK(!ds_translate(&m,region.address,1));
    free(m.ram);
    uint8_t g[560]={0},p[72]={0},out[4096];
    header(g,sizeof(g),"RGCN","RAHC");header(p,sizeof(p),"RLCN","TTLP");
    put32(g+28,3);put32(g+40,512);put32(g+44,24);
    put32(p+24,3);put32(p+32,32);put32(p+36,16);
    g[48]=0x21; p[42]=31; p[44]=0xe0;p[45]=3;
    CHECK(ds_decode_icon(g,sizeof(g),p,sizeof(p),out));
    CHECK(out[0]==255 && out[1]==0 && out[3]==255);
    CHECK(out[4]==0 && out[5]==255 && out[7]==255);CHECK(out[11]==0);
    CHECK(!ds_decode_icon(g,40,p,sizeof(p),out));
    put32(g+44,0xffffffff);CHECK(!ds_decode_icon(g,sizeof(g),p,sizeof(p),out));
    puts("DS address translation and tiled graphics tests passed");return 0;
}
