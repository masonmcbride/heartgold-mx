#include "ds_resources.h"
#include <string.h>
static uint16_t u16le(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t u32le(const uint8_t *p) { return u16le(p) | (uint32_t)u16le(p+2)<<16; }
bool ds_narc_open(DSNarc *a, const uint8_t *p, size_t n) {
    *a=(DSNarc){0};
    if(n<16 || memcmp(p,"NARC",4) || u16le(p+4)!=0xfffe || u16le(p+6)!=0x100 || u32le(p+8)!=n || u16le(p+12)!=16 || u16le(p+14)!=3)return false;
    DSNarc v={.bytes=p,.size=n};size_t off=16;bool names=false;
    for(unsigned i=0;i<3;i++) {
        if(n-off<8)return false;const uint8_t *b=p+off;size_t len=u32le(b+4);
        if(len<8 || len>n-off)return false;
        if(!memcmp(b,"BTAF",4)) {
            if(v.fat || len<12)return false;v.count=u16le(b+8);
            if(v.count>(len-12)/8)return false;v.fat=b+12;
        } else if(!memcmp(b,"BTNF",4)) { if(names || len<16)return false;names=true; }
        else if(!memcmp(b,"GMIF",4)) { if(v.image)return false;v.image=b+8;v.image_size=len-8; }
        else return false;
        off+=len;
    }
    if(off!=n || !v.fat || !v.image || !names)return false;
    for(size_t i=0;i<v.count;i++) { size_t start=u32le(v.fat+i*8),end=u32le(v.fat+i*8+4);if(start>end || end>v.image_size)return false; }
    *a=v;return true;
}
bool ds_narc_member(const DSNarc *a,size_t index,const uint8_t **p,size_t *n) {
    if(!a->fat || index>=a->count)return false;
    size_t start=u32le(a->fat+index*8),end=u32le(a->fat+index*8+4);
    *p=a->image+start;*n=end-start;return true;
}
bool ds_lz10(const uint8_t *s,size_t n,uint8_t *d,size_t cap,size_t *written) {
    *written=0;if(n<4 || s[0]!=0x10)return false;
    size_t total=s[1]|(size_t)s[2]<<8|(size_t)s[3]<<16;
    if(!total || total>cap)return false;
    size_t in=4,out=0;
    while(out<total) {
        if(in>=n)return false;unsigned flags=s[in++];
        for(unsigned bit=0;bit<8 && out<total;bit++) {
            if(flags&(0x80>>bit)) {
                if(n-in<2)return false;unsigned pair=(unsigned)s[in]<<8|s[in+1];in+=2;
                size_t length=(pair>>12)+3,distance=(pair&4095)+1;
                if(distance>out || length>total-out)return false;
                for(size_t j=0;j<length;j++) { d[out]=d[out-distance];out++; }
            } else { if(in>=n)return false;d[out++]=s[in++]; }
        }
    }
    *written=out;return true;
}
