#include "ds_texture_animation.h"
#include <string.h>
static unsigned r16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t r32(const uint8_t *p){return r16(p)|(uint32_t)r16(p+2)<<16;}
static int span(size_t n,size_t at,size_t len){return at<=n&&len<=n-at;}
static void name(const uint8_t *p,char out[17]){memcpy(out,p,16);out[16]=0;}
int ds_texture_animation_open(const uint8_t *p,size_t n,DSTextureAnimation *out){
    if(!p||!out||n<52||memcmp(p,"BTP0",4)||r16(p+4)!=0xfeff||r16(p+6)!=1||r32(p+8)!=n||r16(p+12)!=16||r16(p+14)!=1)return 0;
    size_t block=r32(p+16);if(block!=20||!span(n,block,16)||memcmp(p+block,"PAT0",4)||r32(p+block+4)!=n-block)return 0;
    size_t dict=block+8,len=r16(p+dict+2),table=r16(p+dict+6);
    if(p[dict]||p[dict+1]!=1||!span(n,dict,len)||!span(len,table,8)||r16(p+dict+table)!=4)return 0;
    size_t start=r32(p+dict+table+4);if(start<8+len||!span(n-block,start,20))return 0;p+=block+start;n-=block+start;
    if(memcmp(p,"M\0PT",4))return 0;
    DSTextureAnimation a={.bytes=p,.size=n,.frames=r16(p+4),.textures=p[6],.palettes=p[7],.texture_names=r16(p+8),.palette_names=r16(p+10)};
    if(!a.frames||a.frames>4096||!a.textures||!a.palettes||!span(n,a.texture_names,16*a.textures)||!span(n,a.palette_names,16*a.palettes))return 0;
    dict=12;len=r16(p+dict+2);table=r16(p+dict+6);a.materials=p[dict+1];
    if(p[dict]||!a.materials||a.materials>16||!span(n,dict,len)||!span(len,table,4+8*a.materials)||r16(p+dict+table)!=8)return 0;
    a.table=(unsigned)(dict+table+4);a.names=(unsigned)(dict+table+r16(p+dict+table+2));
    if(a.names<a.table+8*a.materials||!span(dict+len,a.names,16*a.materials))return 0;
    for(unsigned i=0;i<a.materials;i++){
        const uint8_t *entry=p+a.table+8*i;unsigned count=r16(entry),keys=r16(entry+6);
        if(!count||r16(entry+2)||!span(n,keys,4*count)||keys<dict+len)return 0;
        unsigned last=0;
        for(unsigned j=0;j<count;j++){const uint8_t *key=p+keys+4*j;unsigned frame=r16(key);if(frame>=a.frames||(j&&frame<=last)||(!j&&frame)||key[2]>=a.textures||key[3]>=a.palettes)return 0;last=frame;}
        for(unsigned j=0;j<i;j++)if(!memcmp(p+a.names+16*i,p+a.names+16*j,16))return 0;
    }
    *out=a;return 1;
}
int ds_texture_animation_frame(const DSTextureAnimation *a,unsigned frame,const char *material,char texture[17],char palette[17]){
    if(!a||!material||!texture||!palette||frame>=a->frames)return -1;
    for(unsigned i=0;i<a->materials;i++){
        char mat[17];name(a->bytes+a->names+16*i,mat);if(strcmp(mat,material))continue;
        const uint8_t *entry=a->bytes+a->table+8*i;unsigned count=r16(entry),keys=r16(entry+6),selected=0;
        for(unsigned j=1;j<count&&r16(a->bytes+keys+4*j)<=frame;j++)selected=j;
        const uint8_t *key=a->bytes+keys+4*selected;name(a->bytes+a->texture_names+16*key[2],texture);name(a->bytes+a->palette_names+16*key[3],palette);return 1;
    }return 0;
}
