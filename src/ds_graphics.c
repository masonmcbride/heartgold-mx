#include "ds_graphics.h"
#include <string.h>
static uint16_t le16(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t le32(const uint8_t *p) { return le16(p) | (uint32_t)le16(p+2)<<16; }
static const uint8_t *block(const uint8_t *p, size_t n, const char *magic, const char *tag, size_t *size) {
    if (n<16 || memcmp(p,magic,4) || le16(p+4)!=0xfeff || le32(p+8)!=n) return NULL;
    size_t off=le16(p+12);
    if (off<16 || off>n) return NULL;
    for (unsigned i=0; i<le16(p+14); ++i) {
        if (n-off<8) return NULL;
        size_t len=le32(p+off+4);
        if (len<8 || len>n-off) return NULL;
        if (!memcmp(p+off,tag,4)) { *size=len; return p+off; }
        off+=len;
    }
    return NULL;
}
bool ds_palette_open(const uint8_t *data,size_t n,const uint8_t **pixels,size_t *length,unsigned *format,unsigned *extended){
    if(!data||!pixels||!length||!format||!extended)return false;
    size_t size;const uint8_t *p=block(data,n,"RLCN","TTLP",&size);if(!p||size<24)return false;
    unsigned fmt=le32(p+8),ext=le32(p+12);size_t count=le32(p+16),offset=8+(size_t)le32(p+20);
    if((fmt!=3&&fmt!=4)||ext>1||!count||(count&1)||offset<24||offset>size||count>size-offset)return false;
    *pixels=p+offset;*length=count;*format=fmt;*extended=ext;return true;
}
bool ds_decode_icon(const uint8_t *g,size_t gn,const uint8_t *p,size_t pn,uint8_t out[32*32*4]) {
    size_t gs,ps;
    const uint8_t *gb=block(g,gn,"RGCN","RAHC",&gs), *pb=block(p,pn,"RLCN","TTLP",&ps);
    if (!gb || !pb || gs<32 || ps<24 || le32(gb+12)!=3 || le32(pb+8)!=3) return false;
    /* NCGR offsets are relative to CHAR data (block+8); likewise PLTT. */
    size_t go=8+(size_t)le32(gb+28), po=8+(size_t)le32(pb+20);
    if (go>gs || gs-go<512 || le32(gb+24)!=512 || po>ps || ps-po<32 || le32(pb+16)<32) return false;
    for (unsigned y=0;y<32;y++) for (unsigned x=0;x<32;x++) {
        unsigned pixel=((y/8)*4+x/8)*64+(y%8)*8+x%8;
        unsigned index=(gb[go+pixel/2]>>((pixel%2)*4))&15;
        uint16_t c=le16(pb+po+index*2);
        uint8_t *dst=out+(y*32+x)*4;
        for (unsigned channel=0;channel<3;channel++) { unsigned v=(c>>(channel*5))&31; dst[channel]=(v<<3)|(v>>2); }
        dst[3]=index?255:0;
    }
    return true;
}
