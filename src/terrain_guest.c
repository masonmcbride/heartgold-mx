#include "field_data.h"
#include "ds_thumb.h"
#include "terrain_thumb.inc"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define GUEST_BASE 0x02000000u
static void put(uint8_t *ram,unsigned offset,uint32_t value) {
    for(unsigned i=0;i<4;i++)ram[offset+i]=(uint8_t)(value>>(8*i));
}
static int math_boundary(DSThumb *cpu,void *user) {
    (void)user;
    if(cpu->r[15]==HG_THUMB_BOUNDARY__ll_mul) {
        uint64_t a=(uint64_t)cpu->r[0]|(uint64_t)cpu->r[1]<<32;
        uint64_t b=(uint64_t)cpu->r[2]|(uint64_t)cpu->r[3]<<32;
        uint64_t product=a*b;cpu->r[0]=(uint32_t)product;cpu->r[1]=(uint32_t)(product>>32);return 1;
    }
    if(cpu->r[15]==HG_THUMB_BOUNDARY_FX_Div) {
        /* Explicit synchronous Q32 quotient -> rounded Q12 arithmetic boundary.
         * SDK/hardware division implementation and timing are not executed. */
        int64_t numerator=(int64_t)(int32_t)cpu->r[0]*INT64_C(4294967296);
        int32_t denominator=(int32_t)cpu->r[1];
        if(!denominator || (numerator==INT64_MIN && denominator==-1))return -1;
        int64_t quotient=numerator/denominator;
        if(quotient>INT64_MAX-INT64_C(524288))return -1;
        quotient+=INT64_C(524288);
        /* Explicit arithmetic shift, including negative nonmultiples. */
        int64_t result=quotient/INT64_C(1048576);
        if(quotient<0 && quotient%INT64_C(1048576))result--;
        if(result<INT32_MIN || result>INT32_MAX)return -1;
        cpu->r[0]=(uint32_t)(int32_t)result;return 1;
    }
    return 0;
}
int hg_collision_height(const uint8_t *bytes,size_t size,unsigned mode,int32_t reference,
                        int32_t x,int32_t z,int32_t *height) {
    HGCollision collision;
    if(!height || mode>2 || !hg_collision_decode(bytes,size,&collision) || size>0xb000)return -1;
    uint8_t *ram=calloc(1,65536);if(!ram)return -1;
    memcpy(ram,hg_terrain_thumb,sizeof(hg_terrain_thumb));memcpy(ram+0x2000,bytes,size);
    const uint8_t *tables[]={collision.planes,collision.constants,collision.strips,
                            collision.indices,collision.points,collision.normals};
    for(unsigned i=0;i<6;i++)put(ram,0x1000+4*i,GUEST_BASE+0x2000+(uint32_t)(tables[i]-bytes));
    put(ram,0x1018,1);put(ram,0x101c,collision.strip_count);
    /* The original routine accepts the table descriptor and output pointer as
     * fifth/sixth AAPCS arguments, above its own stack frame. */
    put(ram,0xf000,GUEST_BASE+0x1000);put(ram,0xf004,GUEST_BASE+0x1800);
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,65536};cpu.code_start=GUEST_BASE;cpu.code_size=sizeof(hg_terrain_thumb);
    cpu.return_address=0x02010009;cpu.r[14]=cpu.return_address;cpu.r[13]=GUEST_BASE+0xf000;
    cpu.r[15]=GUEST_BASE+HG_THUMB_ov01_021FAE50;cpu.r[0]=mode;cpu.r[1]=(uint32_t)reference;
    cpu.r[2]=(uint32_t)x;cpu.r[3]=(uint32_t)z;cpu.boundary=math_boundary;
    DSThumbResult status=ds_thumb_run(&cpu,100000);
    int result=-1;
    if(status==DS_THUMB_RETURN && cpu.r[13]==GUEST_BASE+0xf000 && cpu.r[0]<=1) {
        result=(int)cpu.r[0];
        if(result) { uint32_t value=0;for(unsigned i=0;i<4;i++)value|=(uint32_t)ram[0x1800+i]<<(8*i);*height=(int32_t)value; }
    }
    free(ram);return result;
}
