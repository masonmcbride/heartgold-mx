#include "ds_thumb.h"
#include <string.h>
static int read_mem(DSThumb *c,uint32_t a,unsigned width,uint32_t *out) {
    const uint8_t *p=ds_translate(&c->memory,a,width);
    if(!p || a%width) { c->fault_address=a;return 0; }
    *out=0;for(unsigned i=0;i<width;i++)*out|=(uint32_t)p[i]<<(8*i);return 1;
}
static int write_mem(DSThumb *c,uint32_t a,unsigned width,uint32_t value) {
    uint8_t *p=ds_translate(&c->memory,a,width);
    if(!p || a%width || ((uint64_t)a<c->code_start+(uint64_t)c->code_size &&
       (uint64_t)a+width>c->code_start)) { c->fault_address=a;return 0; }
    for(unsigned i=0;i<width;i++)p[i]=(uint8_t)(value>>(8*i));return 1;
}
static void nz(DSThumb *c,uint32_t r) { c->n=r>>31;c->z=r==0; }
static uint32_t add(DSThumb *c,uint32_t a,uint32_t b) {
    uint32_t r=a+b;nz(c,r);c->c=((uint64_t)a+b)>>32;c->v=((~(a^b)&(a^r))>>31);return r;
}
static uint32_t sub(DSThumb *c,uint32_t a,uint32_t b) {
    uint32_t r=a-b;nz(c,r);c->c=a>=b;c->v=(((a^b)&(a^r))>>31);return r;
}
static int condition(const DSThumb *c,unsigned cond) {
    switch(cond) {
    case 0:return c->z;case 1:return !c->z;case 2:return c->c;case 3:return !c->c;
    case 4:return c->n;case 5:return !c->n;case 6:return c->v;case 7:return !c->v;
    case 8:return c->c&&!c->z;case 9:return !c->c||c->z;
    case 10:return c->n==c->v;case 11:return c->n!=c->v;
    case 12:return !c->z&&c->n==c->v;case 13:return c->z||c->n!=c->v;
    }return 0;
}
DSThumbResult ds_thumb_run(DSThumb *c,size_t budget) {
    if(!c || !c->memory.ram)return DS_THUMB_BAD_MEMORY;
    c->steps=0;c->fault_address=0;c->fault_instruction=0;
    while(c->steps<budget) {
        uint32_t pc=c->r[15],ins;
        if((pc|1u)==c->return_address)return DS_THUMB_RETURN;
        if((pc<c->code_start || (uint64_t)pc+2>c->code_start+(uint64_t)c->code_size) && c->boundary) {
            c->steps++;int handled=c->boundary(c,c->boundary_user);
            if(handled<0)return DS_THUMB_BOUNDARY;
            if(handled>0) { if(!(c->r[14]&1))return DS_THUMB_BAD_INSTRUCTION;c->r[15]=c->r[14]&~1u;continue; }
        }
        if(pc<c->code_start || (uint64_t)pc+2>c->code_start+(uint64_t)c->code_size ||
           !read_mem(c,pc,2,&ins)) { c->fault_address=pc;return DS_THUMB_BAD_MEMORY; }
        c->steps++;c->fault_instruction=(uint16_t)ins;c->r[15]=pc+2;
        unsigned d=ins&7,s=(ins>>3)&7,t=(ins>>6)&7;uint32_t a,b,r,address;
        if((ins&0xf800)==0x1800) {
            a=c->r[s];b=(ins&0x400)?t:c->r[t];
            c->r[d]=(ins&0x200)?sub(c,a,b):add(c,a,b);
        } else if((ins&0xe000)==0) {
            unsigned shift=(ins>>6)&31,op=(ins>>11)&3;a=c->r[s];r=a;
            if(op==0) { if(shift) { c->c=(a>>(32-shift))&1;r=a<<shift; } }
            else if(op==1) { if(!shift)shift=32;c->c=(a>>(shift-1))&1;r=shift==32?0:a>>shift; }
            else if(op==2) { if(!shift)shift=32;c->c=(a>>(shift-1))&1;
                r=shift==32?(a>>31?UINT32_MAX:0):((a>>shift)|((a>>31)?(UINT32_MAX<<(32-shift)):0)); }
            else return DS_THUMB_BAD_INSTRUCTION;
            c->r[d]=r;nz(c,r);
        } else if((ins&0xe000)==0x2000) {
            d=(ins>>8)&7;b=ins&255;a=c->r[d];
            switch((ins>>11)&3) {
            case 0:c->r[d]=b;nz(c,b);break;case 1:(void)sub(c,a,b);break;
            case 2:c->r[d]=add(c,a,b);break;case 3:c->r[d]=sub(c,a,b);break;
            }
        } else if((ins&0xfc00)==0x4000) {
            a=c->r[d];b=c->r[s];
            switch((ins>>6)&15) {
            case 5:{ unsigned carry=c->c;uint64_t sum=(uint64_t)a+b+carry;
                r=(uint32_t)sum;nz(c,r);c->c=(unsigned)(sum>>32);c->v=(~(a^b)&(a^r))>>31;c->r[d]=r;break; }
            case 9:c->r[d]=sub(c,0,b);break;
            case 10:(void)sub(c,a,b);break;
            case 12:c->r[d]=a|b;nz(c,c->r[d]);break;
            case 13:c->r[d]=a*b;nz(c,c->r[d]);break;
            default:return DS_THUMB_BAD_INSTRUCTION;
            }
        } else if((ins&0xfc00)==0x4400) {
            d=(ins&7)|((ins>>4)&8);s=(ins>>3)&15;
            a=d==15?pc+4:c->r[d];b=s==15?pc+4:c->r[s];
            switch((ins>>8)&3) {
            case 0:if(d==15)return DS_THUMB_BAD_INSTRUCTION;c->r[d]=a+b;break;
            case 1:(void)sub(c,a,b);break;
            case 2:if(d==15)return DS_THUMB_BAD_INSTRUCTION;c->r[d]=b;break;
            case 3:if(!(b&1) || (ins&0x0087))return DS_THUMB_BAD_INSTRUCTION;c->r[15]=b&~1u;break;
            }
        } else if((ins&0xf800)==0x4800) {
            address=((pc+4)&~3u)+4*(ins&255);
            if(!read_mem(c,address,4,&r))return DS_THUMB_BAD_MEMORY;c->r[(ins>>8)&7]=r;
        } else if((ins&0xf000)==0x9000) {
            address=c->r[13]+4*(ins&255);d=(ins>>8)&7;
            if(ins&0x800) { if(!read_mem(c,address,4,&r))return DS_THUMB_BAD_MEMORY;c->r[d]=r; }
            else if(!write_mem(c,address,4,c->r[d]))return DS_THUMB_BAD_MEMORY;
        } else if((ins&0xf000)==0xa000) {
            c->r[(ins>>8)&7]=((ins&0x800)?c->r[13]:(pc+4)&~3u)+4*(ins&255);
        } else if((ins&0xff00)==0xb000) {
            unsigned offset=4*(ins&127);c->r[13]+=(ins&128)?0u-offset:offset;
        } else if((ins&0xf800)==0xf000) {
            if((uint64_t)pc+4>c->code_start+(uint64_t)c->code_size || !read_mem(c,pc+2,2,&r))return DS_THUMB_BAD_MEMORY;
            if((r&0xf800)!=0xf800)return DS_THUMB_BAD_INSTRUCTION;
            int32_t offset=(int32_t)(((ins&2047)<<12)|((r&2047)<<1));
            if(offset&(1<<22))offset-=1<<23;
            c->r[14]=pc+5;c->r[15]=pc+4+(uint32_t)offset;
        } else if((ins&0xf000)==0x5000) {
            unsigned op=(ins>>9)&7;address=c->r[s]+c->r[t];
            unsigned width=op==1||op==5?2:op==2||op==6?1:4;
            if(op==3||op==7)return DS_THUMB_BAD_INSTRUCTION;
            if(op<3) { if(!write_mem(c,address,width,c->r[d]))return DS_THUMB_BAD_MEMORY; }
            else { if(!read_mem(c,address,width,&r))return DS_THUMB_BAD_MEMORY;c->r[d]=r; }
        } else if((ins&0xe000)==0x6000 || (ins&0xf000)==0x8000) {
            unsigned width=(ins&0x8000)?2:(ins&0x1000)?1:4;
            address=c->r[s]+((ins>>6)&31)*width;
            if(ins&0x800) { if(!read_mem(c,address,width,&r))return DS_THUMB_BAD_MEMORY;c->r[d]=r; }
            else if(!write_mem(c,address,width,c->r[d]))return DS_THUMB_BAD_MEMORY;
        } else if((ins&0xf600)==0xb400) {
            unsigned mask=ins&255,load=ins&0x800,count=0;for(unsigned i=0;i<8;i++)count+=(mask>>i)&1;count+=(ins>>8)&1;
            address=load?c->r[13]:c->r[13]-4*count;
            for(unsigned i=0;i<8;i++)if(mask&(1u<<i)) {
                if(load) { if(!read_mem(c,address,4,&r))return DS_THUMB_BAD_MEMORY;c->r[i]=r; }
                else if(!write_mem(c,address,4,c->r[i]))return DS_THUMB_BAD_MEMORY;
                address+=4;
            }
            if(ins&0x100) {
                if(load) { if(!read_mem(c,address,4,&r))return DS_THUMB_BAD_MEMORY;if(!(r&1))return DS_THUMB_BAD_INSTRUCTION;c->r[15]=r&~1u; }
                else if(!write_mem(c,address,4,c->r[14]))return DS_THUMB_BAD_MEMORY;
            }
            c->r[13]+=load?4*count:0u-4*count;
        } else if((ins&0xf000)==0xc000) {
            unsigned base=(ins>>8)&7,mask=ins&255;address=c->r[base];
            if(!mask || (mask&(1u<<base)))return DS_THUMB_BAD_INSTRUCTION;
            for(unsigned i=0;i<8;i++)if(mask&(1u<<i)) {
                if(ins&0x800) { if(!read_mem(c,address,4,&r))return DS_THUMB_BAD_MEMORY;c->r[i]=r; }
                else if(!write_mem(c,address,4,c->r[i]))return DS_THUMB_BAD_MEMORY;
                address+=4;
            }c->r[base]=address;
        } else if((ins&0xf000)==0xd000 && ((ins>>8)&15)<14) {
            if(condition(c,(ins>>8)&15))c->r[15]=pc+4+(uint32_t)((int32_t)(int8_t)ins*2);
        } else if((ins&0xf800)==0xe000) {
            int32_t offset=(int32_t)(ins&2047);if(offset&1024)offset-=2048;c->r[15]=pc+4+(uint32_t)(offset*2);
        } else return DS_THUMB_BAD_INSTRUCTION;
    }
    return DS_THUMB_BUDGET;
}
