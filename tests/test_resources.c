#include "ds_resources.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
int main(void) {
    uint8_t file[64]={0};memcpy(file,"NARC\xfe\xff\x00\x01",8);put32(file+8,64);file[12]=16;file[14]=3;
    memcpy(file+16,"BTAF",4);put32(file+20,20);file[24]=1;put32(file+28,0);put32(file+32,4);
    memcpy(file+36,"BTNF",4);put32(file+40,16);put32(file+44,4);file[50]=1;
    memcpy(file+52,"GMIF",4);put32(file+56,12);memcpy(file+60,"test",4);
    DSNarc a;const uint8_t *p;size_t n;
    CHECK(ds_narc_open(&a,file,sizeof(file)));CHECK(ds_narc_member(&a,0,&p,&n));CHECK(n==4&&!memcmp(p,"test",4));CHECK(!ds_narc_member(&a,1,&p,&n));
    for(size_t i=0;i<sizeof(file);i++)CHECK(!ds_narc_open(&a,file,i));
    put32(file+32,5);CHECK(!ds_narc_open(&a,file,64));put32(file+32,4);
    put32(file+28,5);CHECK(!ds_narc_open(&a,file,64));put32(file+28,0);
    file[24]=2;CHECK(!ds_narc_open(&a,file,64));file[24]=1;
    memcpy(file+36,"BTAF",4);CHECK(!ds_narc_open(&a,file,64));memcpy(file+36,"BTNF",4);
    put32(file+20,0xffffffff);CHECK(!ds_narc_open(&a,file,64));put32(file+20,20);
    /* Mutate headers/offsets with the declared file size kept valid. */
    uint32_t random=0x845271;
    for(unsigned trial=0;trial<4096;trial++) {
        uint8_t mutated[64];memcpy(mutated,file,64);
        random=random*1664525u+1013904223u;unsigned offset=16+(random%44);
        random=random*1664525u+1013904223u;mutated[offset]=(uint8_t)random;
        if(ds_narc_open(&a,mutated,sizeof(mutated))) {
            for(size_t i=0;i<a.count;i++) { CHECK(ds_narc_member(&a,i,&p,&n));CHECK(p>=mutated && p<=mutated+64 && n<=(size_t)(mutated+64-p)); }
        }
    }
    uint8_t encoded[]={0x10,9,0,0,0x10,'a','b','c',0x30,2},out[16];size_t written;
    CHECK(ds_lz10(encoded,sizeof(encoded),out,sizeof(out),&written));CHECK(written==9&&!memcmp(out,"abcabcabc",9));
    CHECK(!ds_lz10(encoded,sizeof(encoded),out,8,&written));
    for(size_t i=0;i<sizeof(encoded);i++)CHECK(!ds_lz10(encoded,i,out,sizeof(out),&written));
    encoded[4]=0x80;CHECK(!ds_lz10(encoded,sizeof(encoded),out,sizeof(out),&written));
    puts("Bounded NARC and LZ10 tests passed");return 0;
}
