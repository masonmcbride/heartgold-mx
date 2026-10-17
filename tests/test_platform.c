#include "ds_memory.h"
#include "ds_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void put32(uint8_t *p,uint32_t v) { for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static void header(uint8_t *p,size_t n,const char *magic,const char *tag) { memcpy(p,magic,4);p[4]=255;p[5]=254;put32(p+8,(uint32_t)n);p[12]=16;p[14]=1;memcpy(p+16,tag,4);put32(p+20,(uint32_t)n-16); }
int main(void) {
    DSMainRAM m={calloc(1,4*1024*1024),4*1024*1024}; CHECK(m.ram);
    CHECK(ds_translate(&m,0x02000000,4)==m.ram);
    CHECK(ds_translate(&m,0x023fffff,1)==m.ram+m.size-1);
    CHECK(!ds_translate(&m,0x023fffff,2)); CHECK(!ds_translate(&m,0xffffffff,4));
    CHECK(!ds_translate(&m,0x01ffffff,1)); CHECK(!ds_translate(&m,0x02000000,SIZE_MAX));free(m.ram);
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
