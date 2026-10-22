#include "ds_joint_animation.h"
#include <string.h>
static unsigned r16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t r32(const uint8_t *p) { return r16(p)|(uint32_t)r16(p+2)<<16; }
static int span(size_t n,size_t at,size_t len) { return at<=n&&len<=n-at; }
static double q12(unsigned v) { return ((int)(v&32767)-(int)(v&32768))/4096.0; }
static int pivot(const DSJointAnimation *a,unsigned ref,DSTransform *out) {
    if(!(ref&0x8000))return 0; /* Packed basis rotations remain unsupported. */
    size_t at=r32(a->bytes+12)+(size_t)(ref&32767)*6;
    if(!span(a->size,at,6))return 0;
    const uint8_t *p=a->bytes+at;unsigned flags=r16(p),form=flags&15;
    if(form>8||(flags&~127u))return 0;
    double x=q12(r16(p+2)),y=q12(r16(p+4));DSTransform t=ds_transform_identity();
    unsigned row=form%3,col=form/3,rows[2],cols[2],ri=0,ci=0;
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)t.m[4*r+c]=0;
    for(unsigned k=0;k<3;k++){if(k!=row)rows[ri++]=k;if(k!=col)cols[ci++]=k;}
    t.m[4*row+col]=(flags&16)?-1:1;
    t.m[4*rows[0]+cols[0]]=x;t.m[4*rows[1]+cols[0]]=y;
    t.m[4*rows[0]+cols[1]]=(flags&32)?-y:y;
    t.m[4*rows[1]+cols[1]]=(flags&64)?-x:x;*out=t;return 1;
}
/* Constant or every-frame signed Q12 scalar curves. Scale stores a paired
 * reciprocal for DS hierarchy compensation; the host admits scaled poses only
 * on single-node models, where that reciprocal has no child to affect. */
static int scalar(const DSJointAnimation *a,size_t *at,unsigned frame,int constant,int scale,double *out) {
    unsigned width=4;size_t sample=*at;
    if(constant){if(!span(a->size,*at,scale?8:4))return 0;*at+=scale?8:4;}
    else {
        if(!span(a->size,*at,8))return 0;uint32_t info=r32(a->bytes+*at);unsigned begin=info&65535,end=(info>>16)&4095,format=(info>>28)&3;
        if((info>>30)||format>1||begin||end!=a->frames)return 0;
        width=format?2:4;sample=r32(a->bytes+*at+4);*at+=8;
        unsigned stride=width*(scale?2:1);if(!span(a->size,sample,(size_t)stride*a->frames))return 0;sample+=(size_t)frame*stride;
    }
    if(width==4){uint32_t bits=r32(a->bytes+sample);int32_t value;memcpy(&value,&bits,4);*out=value/4096.0;}
    else *out=q12(r16(a->bytes+sample));
    return 1;
}
int ds_joint_animation_pose(const DSJointAnimation *a,unsigned frame,DSJointPose *out) {
    if(!a||!a->bytes||!out||a->size<20||!a->frames||a->frames>4096||frame>=a->frames||!a->nodes||a->nodes>32||!span(a->size,20,2*a->nodes))return 0;
    DSJointPose pose={0};
    for(unsigned i=0;i<a->nodes;i++) {
        size_t at=r16(a->bytes+20+2*i);if(!span(a->size,at,4))return 0;
        const uint8_t *p=a->bytes+at;unsigned flags=r16(p),node=p[3];at+=4;
        if(node>=32||p[2]||(pose.mask&(1u<<node)))return 0;
        if(flags&1)continue;
        if((flags&6)==4||(flags&0xc0)==0x80||(flags&0x600)==0x400||flags&0xc000)return 0;
        if((flags&6)!=0&&(flags&6)!=2)return 0;
        if((flags&0xc0)!=0&&(flags&0xc0)!=0x40)return 0;
        if((flags&0x600)!=0&&(flags&0x600)!=0x200)return 0;
        double translation[3]={0},scale[3]={1,1,1};DSTransform transform=ds_transform_identity();
        if(!(flags&6))for(unsigned k=0;k<3;k++)if(!scalar(a,&at,frame,flags&(8u<<k),0,translation+k))return 0;
        if(!(flags&0xc0)) {
            unsigned ref;
            if(flags&0x100){if(!span(a->size,at,4)||r16(a->bytes+at+2))return 0;ref=r16(a->bytes+at);at+=4;}
            else {
                if(!span(a->size,at,8))return 0;uint32_t info=r32(a->bytes+at);unsigned begin=info&65535,end=(info>>16)&4095;
                if((info>>28)||begin||end!=a->frames)return 0;
                size_t samples=r32(a->bytes+at+4);at+=8;if(!span(a->size,samples,2*a->frames))return 0;
                ref=r16(a->bytes+samples+2*frame);
            }
            if(!pivot(a,ref,&transform))return 0;
        }
        if(!(flags&0x600)) {
            for(unsigned k=0;k<3;k++)if(!scalar(a,&at,frame,flags&(0x800u<<k),1,scale+k))return 0;
            if(scale[0]!=1||scale[1]!=1||scale[2]!=1)pose.scale_mask|=1u<<node;
        }
        for(unsigned k=0;k<3;k++){transform.m[4*k+3]=translation[k];for(unsigned row=0;row<3;row++)transform.m[4*row+k]*=scale[k];}
        pose.nodes[node]=transform;pose.mask|=1u<<node;
    }
    *out=pose;return 1;
}
int ds_joint_animation_open(const uint8_t *p,size_t n,DSJointAnimation *out) {
    if(!p||!out||n<52||memcmp(p,"BCA0",4)||r16(p+4)!=0xfeff||r16(p+6)!=1||r32(p+8)!=n||r16(p+12)!=16||r16(p+14)!=1)return 0;
    size_t block=r32(p+16);if(block!=20||!span(n,block,16)||memcmp(p+block,"JNT0",4)||r32(p+block+4)!=n-block)return 0;
    size_t dict=block+8,len=r16(p+dict+2),table=r16(p+dict+6);
    if(p[dict]!=0||p[dict+1]!=1||!span(n,dict,len)||!span(len,table,8)||r16(p+dict+table)!=4)return 0;
    size_t names=table+r16(p+dict+table+2);if(names<table+8||!span(len,names,16))return 0;
    size_t start=r32(p+dict+table+4);if(start<8+len||!span(n-block,start,22))return 0;
    p+=block+start;n-=block+start;
    if(memcmp(p,"J\0AC",4))return 0;DSJointAnimation a={p,n,r16(p+4),r16(p+6)};
    if(!a.frames||a.frames>4096||!a.nodes||a.nodes>32)return 0;
    for(unsigned f=0;f<a.frames;f++){DSJointPose pose;if(!ds_joint_animation_pose(&a,f,&pose))return 0;}
    *out=a;return 1;
}
