#include "ds_texture_animation.h"
#include <string.h>
static unsigned r16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t r32(const uint8_t *p){return r16(p)|(uint32_t)r16(p+2)<<16;}
static int span(size_t n,size_t at,size_t len){return at<=n&&len<=n-at;}
static void name(const uint8_t *p,char out[17]){memcpy(out,p,16);out[16]=0;}
static int pattern_open(const uint8_t *p,size_t n,DSTextureAnimation *out){
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
    if(a->kind)return 0;
    for(unsigned i=0;i<a->materials;i++){
        char mat[17];name(a->bytes+a->names+16*i,mat);if(strcmp(mat,material))continue;
        const uint8_t *entry=a->bytes+a->table+8*i;unsigned count=r16(entry),keys=r16(entry+6),selected=0;
        for(unsigned j=1;j<count&&r16(a->bytes+keys+4*j)<=frame;j++)selected=j;
        const uint8_t *key=a->bytes+keys+4*selected;name(a->bytes+a->texture_names+16*key[2],texture);name(a->bytes+a->palette_names+16*key[3],palette);return 1;
    }return 0;
}

/* SRT components: bit29 constant, bit28 fx16, otherwise fx32. Rotation
 * always packs signed Q12 sine/cosine halves. Reduced-rate records reject. */
static int srt_open(const uint8_t *p,size_t n,DSTextureAnimation *out) {
    if(!p||!out||n<52||memcmp(p,"BTA0",4)||r16(p+4)!=0xfeff||r16(p+6)!=1||r32(p+8)!=n||r16(p+12)!=16||r16(p+14)!=1)return 0;
    size_t block=r32(p+16);if(block!=20||!span(n,block,16)||memcmp(p+block,"SRT0",4)||r32(p+block+4)!=n-block)return 0;
    size_t dict=block+8,len=r16(p+dict+2),table=r16(p+dict+6);
    if(p[dict]||p[dict+1]!=1||!span(n,dict,len)||!span(len,table,8)||r16(p+dict+table)!=4)return 0;
    size_t start=r32(p+dict+table+4);if(start<8+len||!span(n-block,start,16))return 0;p+=block+start;n-=block+start;
    if(memcmp(p,"M\0AT",4)||p[6]>1||p[7])return 0;
    DSTextureAnimation a={.bytes=p,.size=n,.frames=r16(p+4),.kind=1};
    dict=8;len=r16(p+dict+2);table=r16(p+dict+6);a.materials=p[dict+1];
    if(!a.frames||a.frames>4096||p[dict]||!a.materials||!span(n,dict,len)||!span(len,table,4+40*a.materials)||r16(p+dict+table)!=40)return 0;
    a.table=(unsigned)(dict+table+4);a.names=(unsigned)(dict+table+r16(p+dict+table+2));
    if(a.names<a.table+40*a.materials||!span(dict+len,a.names,16*a.materials))return 0;
    for(unsigned i=0;i<a.materials;i++) {
        for(unsigned j=0;j<i;j++)if(!memcmp(p+a.names+16*i,p+a.names+16*j,16))return 0;
        for(unsigned k=0;k<5;k++) {
            const uint8_t *e=p+a.table+40*i+8*k;uint32_t f=r32(e),v=r32(e+4);
            if((f&0xcfff0000u)||(f&0xffff)!=a.frames||(k==2&&(f&0x10000000)))return 0;
            if(!(f&0x20000000)) {unsigned size=(f&0x10000000)?2:4;if(v<dict+len||!span(n,v,size*a.frames))return 0;}
        }
    }
    *out=a;return 1;
}
int ds_texture_animation_open(const uint8_t *p,size_t n,DSTextureAnimation *out) {
    return pattern_open(p,n,out)||srt_open(p,n,out);
}
static double srt_value(const uint8_t *base,const uint8_t *entry,unsigned frame,unsigned half) {
    uint32_t f=r32(entry),v=r32(entry+4);
    if(!(f&0x20000000))v=(f&0x10000000)?r16(base+v+2*frame):r32(base+v+4*frame);
    if(half)return (int16_t)(v>>(half==2?16:0))/4096.0;
    return (f&0x10000000)&&!(f&0x20000000)?(int16_t)v/4096.0:(int32_t)v/4096.0;
}
int ds_texture_animation_matrix(const DSTextureAnimation *a,double frame,const char *material,unsigned w,unsigned h,double m[6]) {
    if(!a||!material||!m||!w||!h||!(frame>=0&&frame<a->frames))return -1;
    if(!a->kind)return 0;
    for(unsigned i=0;i<a->materials;i++) {
        char mat[17];name(a->bytes+a->names+16*i,mat);if(strcmp(mat,material))continue;
        unsigned f=(unsigned)frame,next=f+1<a->frames?f+1:f;double t=frame-f,x[6];
        for(unsigned k=0;k<6;k++) {
            unsigned channel=k<2?k:k<4?2:k-1,half=k==2?1:k==3?2:0;
            const uint8_t *e=a->bytes+a->table+40*i+8*channel;
            double v=srt_value(a->bytes,e,f,half);x[k]=v+t*(srt_value(a->bytes,e,next,half)-v);
        }
        double sx=x[0],sy=x[1],sn=x[2],cs=x[3],u=x[4],v=x[5];
        /* Maya texture convention, in the renderer's top-down pixel UV space. */
        m[0]=sx*cs;m[1]=-sy*sn*h/w;m[2]=sx*sn*w/h;m[3]=sy*cs;
        m[4]=w*sx*(-0.5*cs-0.5*sn+0.5-u);
        m[5]=h*(sy*(-0.5*cs+0.5*sn-0.5+v)+1);
        return 1;
    }
    return 0;
}

int ds_animation_resource_open(const uint8_t *p,size_t n,unsigned index,DSAnimationResource *out){
    if(!p||!out||n<52||r16(p+4)!=0xfeff||r16(p+6)!=1||r32(p+8)!=n||r16(p+12)!=16||r16(p+14)!=1)return 0;
    unsigned kind;const char *tag,*magic;
    if(!memcmp(p,"BCA0",4)){kind=DS_ANIMATION_JOINT;tag="JNT0";magic="J\0AC";}
    else if(!memcmp(p,"BTP0",4)){kind=DS_ANIMATION_PATTERN;tag="PAT0";magic="M\0PT";}
    else if(!memcmp(p,"BTA0",4)){kind=DS_ANIMATION_SRT;tag="SRT0";magic="M\0AT";}
    else if(!memcmp(p,"BMA0",4)){kind=DS_ANIMATION_COLOR;tag="MAT0";magic="M\0AM";}
    else return 0;
    size_t block=r32(p+16);if(block!=20||!span(n,block,16)||memcmp(p+block,tag,4)||r32(p+block+4)!=n-block)return 0;
    size_t dict=block+8,len=r16(p+dict+2),table=r16(p+dict+6);unsigned count=p[dict+1];
    if(p[dict]||!count||index>=count||!span(n,dict,len)||!span(len,table,4+4*count)||r16(p+dict+table)!=4)return 0;
    size_t start=r32(p+dict+table+4+4*index),end=n-block;
    if(start<8+len||!span(n-block,start,8))return 0;
    for(unsigned i=0;i<count;i++){size_t other=r32(p+dict+table+4+4*i);if(other>start&&other<end)end=other;}
    const uint8_t *record=p+block+start;size_t size=end-start;
    if(memcmp(record,magic,4))return 0;
    DSAnimationResource a={.bytes=p,.size=n,.record=record,.record_size=size,.kind=kind,.frames=r16(record+4)};
    if(!a.frames||a.frames>4096)return 0;
    if(kind==DS_ANIMATION_JOINT){
        a.nodes=r16(record+6);if(!a.nodes||a.nodes>32||!span(size,20,2*a.nodes))return 0;
        unsigned used=0;for(unsigned i=0;i<a.nodes;i++){unsigned at=r16(record+20+2*i);if(!span(size,at,4)||record[at+2]||record[at+3]>=32||(used&(1u<<record[at+3])))return 0;if(!(r16(record+at)&1))used|=1u<<record[at+3];}
    }else{
        dict=kind==DS_ANIMATION_PATTERN?12:8;if(!span(size,dict,8))return 0;
        len=r16(record+dict+2);table=r16(record+dict+6);a.materials=record[dict+1];a.stride=kind==DS_ANIMATION_PATTERN?8:kind==DS_ANIMATION_SRT?40:20;
        if(record[dict]||!a.materials||!span(size,dict,len)||!span(len,table,4+a.stride*a.materials)||r16(record+dict+table)!=a.stride)return 0;
        a.table=(unsigned)(dict+table+4);a.names=(unsigned)(dict+table+r16(record+dict+table+2));
        if(a.names<a.table+a.stride*a.materials||!span(dict+len,a.names,16*a.materials))return 0;
        for(unsigned i=0;i<a.materials;i++)for(unsigned j=0;j<i;j++)if(!memcmp(record+a.names+16*i,record+a.names+16*j,16))return 0;
    }
    *out=a;return 1;
}
