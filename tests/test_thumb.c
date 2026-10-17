#include "ds_thumb.h"
#include "field_data.h"
#include "compat/resources.h"
#include "terrain_thumb.inc"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1); } } while(0)
#define BASE 0x02000000u
static uint8_t ram[65536];
static void put(unsigned off,uint32_t value) { for(unsigned i=0;i<4;i++)ram[off+i]=(uint8_t)(value>>(8*i)); }
static uint32_t get(unsigned off) { uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)ram[off+i]<<(8*i);return v; }
static int32_t signed_word(const uint8_t *p) { uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return (int32_t)v; }
static unsigned half(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static DSThumb cpu(unsigned entry) {
    DSThumb c={0};c.memory=(DSMainRAM){ram,sizeof(ram)};
    c.code_start=BASE;c.code_size=sizeof(hg_terrain_thumb);c.return_address=BASE+0x10001;
    c.r[13]=BASE+0xf000;c.r[14]=c.return_address;c.r[15]=BASE+entry;
    for(unsigned i=4;i<=11;i++)c.r[i]=0xa0000000+i;
    return c;
}
static void run(DSThumb *c) {
    DSThumbResult result=ds_thumb_run(c,10000);
    if(result!=DS_THUMB_RETURN)fprintf(stderr,"Thumb status=%d pc=%08x ins=%04x fault=%08x\n",result,c->r[15],c->fault_instruction,c->fault_address);
    CHECK(result==DS_THUMB_RETURN);CHECK(c->steps>0 && c->r[13]==BASE+0xf000);
    for(unsigned i=4;i<=11;i++)CHECK(c->r[i]==0xa0000000+i);
}
int main(void) {
    memcpy(ram,hg_terrain_thumb,sizeof(hg_terrain_thumb));
    /* Signed rectangle membership, inclusive edges, both endpoint orders. */
    for(unsigned reverse=0;reverse<2;reverse++)for(int x=-5;x<=5;x++)for(int z=-6;z<=6;z++) {
        put(0x1000,(uint32_t)(reverse?3:-3));put(0x1004,(uint32_t)(reverse?4:-4));
        put(0x1010,(uint32_t)(reverse?-3:3));put(0x1014,(uint32_t)(reverse?-4:4));
        put(0x1020,(uint32_t)x);put(0x1024,(uint32_t)z);
        DSThumb c=cpu(HG_THUMB_ov01_021FAD1C);c.r[0]=BASE+0x1000;c.r[1]=BASE+0x1010;c.r[2]=BASE+0x1020;run(&c);
        CHECK(c.r[0]==(unsigned)(x>=-3 && x<=3 && z>=-4 && z<=4));
    }
    const int32_t extremes[]={INT32_MIN,INT32_MIN+1,-1,0,1,INT32_MAX-1,INT32_MAX};
    for(unsigned i=0;i<7;i++)for(unsigned j=0;j<7;j++)for(unsigned k=0;k<7;k++) {
        int32_t lo=extremes[i]<extremes[j]?extremes[i]:extremes[j];
        int32_t hi=extremes[i]>extremes[j]?extremes[i]:extremes[j];
        put(0x1000,(uint32_t)extremes[i]);put(0x1004,0);
        put(0x1010,(uint32_t)extremes[j]);put(0x1014,0);
        put(0x1020,(uint32_t)extremes[k]);put(0x1024,0);
        DSThumb edge=cpu(HG_THUMB_ov01_021FAD1C);edge.r[0]=BASE+0x1000;edge.r[1]=BASE+0x1010;edge.r[2]=BASE+0x1020;run(&edge);
        CHECK(edge.r[0]==(unsigned)(extremes[k]>=lo && extremes[k]<=hi));
    }
    DSThumb c=cpu(HG_THUMB_ov01_021FADD4);memset(ram+0x1800,0x55,128);c.r[0]=BASE+0x1800;run(&c);
    for(unsigned i=0;i<10;i++) { CHECK(get(0x1800+12*i)==0);CHECK(get(0x1804+12*i)==UINT32_MAX);CHECK(get(0x1808+12*i)==UINT32_MAX); }
    CHECK(get(0x1878)==0x55555555);
    /* Original strip search: empty/singleton and sorted signed boundaries. */
    const int bounds[]={-4096,0,4096,8192};
    for(unsigned i=0;i<4;i++)put(0x2000+8*i,(uint32_t)bounds[i]);
    c=cpu(HG_THUMB_ov01_021FADEC);c.r[0]=BASE+0x2000;c.r[1]=0;c.r[3]=BASE+0x1800;put(0x1800,0x77777777);run(&c);CHECK(c.r[0]==0 && get(0x1800)==0x77777777);
    c=cpu(HG_THUMB_ov01_021FADEC);c.r[0]=BASE+0x2000;c.r[1]=1;c.r[2]=123;c.r[3]=BASE+0x1800;run(&c);CHECK(c.r[0]==1 && half(ram+0x1800)==0);
    for(int x=-8192;x<=12288;x+=1024) {
        c=cpu(HG_THUMB_ov01_021FADEC);c.r[0]=BASE+0x2000;c.r[1]=4;c.r[2]=(uint32_t)x;c.r[3]=BASE+0x1800;run(&c);
        unsigned expected=0;while(expected<3 && x>=bounds[expected])expected++;
        CHECK(c.r[0]==1 && half(ram+0x1800)==expected);
    }
    for(unsigned count=2;count<=16;count++) {
        for(unsigned i=0;i<count;i++)put(0x2000+8*i,(uint32_t)((int)i*1024-8192));
        for(int x=-10000;x<=10000;x+=127) {
            c=cpu(HG_THUMB_ov01_021FADEC);c.r[0]=BASE+0x2000;c.r[1]=count;c.r[2]=(uint32_t)x;c.r[3]=BASE+0x1800;run(&c);
            unsigned expected=0;while(expected<count-1 && x>=(int)expected*1024-8192)expected++;
            CHECK(c.r[0]==1 && half(ram+0x1800)==expected);
        }
    }
    unsigned planes=0;
    for(unsigned member=0;member<676;member++) {
        const uint8_t *bytes;size_t size;HGLandChunk land;HGCollision b;
        CHECK(native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&bytes,&size));
        CHECK(hg_land_decode(bytes,size,&land));CHECK(hg_collision_decode(land.collision,land.collision_size,&b));
        CHECK(land.collision_size<0xb000);memcpy(ram+0x2000,land.collision,land.collision_size);
        put(0x1000,BASE+0x2000+(uint32_t)(b.planes-land.collision));
        put(0x1004,BASE+0x2000+(uint32_t)(b.constants-land.collision));
        put(0x1010,BASE+0x2000+(uint32_t)(b.points-land.collision));
        put(0x1014,BASE+0x2000+(uint32_t)(b.normals-land.collision));
        for(unsigned i=0;i<b.plane_count;i++) {
            const uint8_t *plane=b.planes+8*i;
            c=cpu(HG_THUMB_ov01_021FAD6C);c.r[0]=BASE+0x1000;c.r[1]=i;c.r[2]=BASE+0x1800;run(&c);
            CHECK(!memcmp(ram+0x1800,b.points+8*half(plane),8));CHECK(!memcmp(ram+0x1808,b.points+8*half(plane+2),8));
            c=cpu(HG_THUMB_ov01_021FAD9C);c.r[0]=BASE+0x1000;c.r[1]=i;c.r[2]=BASE+0x1800;run(&c);
            CHECK(!memcmp(ram+0x1800,b.normals+12*half(plane+4),12));
            c=cpu(HG_THUMB_ov01_021FADBC);c.r[0]=BASE+0x1000;c.r[1]=i;c.r[2]=BASE+0x1800;run(&c);
            CHECK(!memcmp(ram+0x1800,b.constants+4*half(plane+6),4));planes++;
        }
    }
    /* Synthetic overlapping horizontal surfaces with analytical heights.
     * This exercises the complete original solver, BL/linking, all selection
     * modes, original min/max helpers and the native arithmetic boundaries. */
    uint8_t synthetic[104]={0};memcpy(synthetic,"BDHC",4);
    synthetic[4]=2;synthetic[6]=1;synthetic[8]=2;synthetic[10]=2;synthetic[12]=1;synthetic[14]=2;
    uint8_t saved[104];
    /* Use the guest write helper as a little-endian fixture encoder. */
    memset(ram+0x2000,0,104);memcpy(ram+0x2000,synthetic,104);
    put(0x2010,(uint32_t)-16384);put(0x2014,(uint32_t)-16384);
    put(0x2018,16384);put(0x201c,16384);
    put(0x2024,4096); /* normal=(0,1,0), Q12 */
    put(0x202c,(uint32_t)-4096);put(0x2030,(uint32_t)-12288);
    ram[0x2036]=1; /* first plane point index 1 at +2 */
    ram[0x203e]=1;ram[0x2042]=1; /* second plane point 1, constant 1 */
    ram[0x2048]=2; /* strip has 2 plane indices */
    ram[0x204e]=1; /* index table [0,1] */
    memcpy(saved,ram+0x2000,80);
    HGCollision synthetic_view;CHECK(hg_collision_decode(saved,80,&synthetic_view));
    int32_t height=777;
    CHECK(hg_collision_height(saved,80,1,0,0,0,&height)==1 && height==12288);
    CHECK(hg_collision_height(saved,80,2,0,0,0,&height)==1 && height==4096);
    CHECK(hg_collision_height(saved,80,0,5000,0,0,&height)==1 && height==4096);
    CHECK(hg_collision_height(saved,80,0,11000,0,0,&height)==1 && height==12288);
    height=777;CHECK(hg_collision_height(saved,80,0,0,20000,0,&height)==0 && height==777);
    CHECK(hg_collision_height(saved,80,3,0,0,0,&height)==-1 && height==777);
    CHECK(hg_collision_height(saved,79,0,0,0,0,&height)==-1 && height==777);
    const uint8_t *bark_bytes;size_t bark_size;HGLandChunk bark;HGCollision bark_collision;
    CHECK(native_resource_member(NARC_fielddata_landdata_land_data,0,0,&bark_bytes,&bark_size));
    CHECK(hg_land_decode(bark_bytes,bark_size,&bark));CHECK(hg_collision_decode(bark.collision,bark.collision_size,&bark_collision));
    unsigned hits=0,misses=0;
    for(int tz=0;tz<32;tz++)for(int tx=0;tx<32;tx++) {
        int32_t x=(tx*16-248)*4096,z=(tz*16-248)*4096;
        double lowest=1e100,highest=-1e100;unsigned candidates=0;
        /* Independent floating plane-equation oracle over all rectangles,
         * rather than a reimplementation of the original strip/selection code. */
        for(unsigned p=0;p<bark_collision.plane_count;p++) {
            const uint8_t *plane=bark_collision.planes+8*p;
            const uint8_t *a=bark_collision.points+8*half(plane),*b=bark_collision.points+8*half(plane+2);
            int32_t ax=signed_word(a),az=signed_word(a+4),bx=signed_word(b),bz=signed_word(b+4);
            if(x<(ax<bx?ax:bx) || x>(ax>bx?ax:bx) || z<(az<bz?az:bz) || z>(az>bz?az:bz))continue;
            const uint8_t *normal=bark_collision.normals+12*half(plane+4);
            double nx=signed_word(normal),ny=signed_word(normal+4),nz=signed_word(normal+8);
            CHECK(ny!=0);double constant=signed_word(bark_collision.constants+4*half(plane+6));
            double y=-(nx*x+nz*z+constant*4096)/ny;
            if(y<lowest)lowest=y;if(y>highest)highest=y;candidates++;
        }
        for(unsigned mode=1;mode<=2;mode++) {
            height=777;int result=hg_collision_height(bark.collision,bark.collision_size,mode,0,x,z,&height);
            CHECK(result==(candidates?1:0));
            if(candidates)CHECK(fabs(height-(mode==1?highest:lowest))<=2);else CHECK(height==777);
        }
        if(candidates)hits++;else misses++;
    }
    HGFieldResources *field=hg_field_resources_new(60);CHECK(field);
    for(int z=0;z<32;z++)for(int x=0;x<32;x++) {
        int32_t direct=0,world=0;
        CHECK(hg_collision_height(bark.collision,bark.collision_size,0,0,(x*16-248)*4096,(z*16-248)*4096,&direct)==1);
        CHECK(hg_field_resources_height(field,21*32+x,12*32+z,0,0,&world)==1 && world==direct);
        /* Intermediate walking positions, including exact tile boundaries and
         * fractional Q12 coordinates. Compare with direct chunk-local input. */
        const int offsets[][2]={{0,0},{8192,57344},{32768,32768},{65535,4095}};
        for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);i++) {
            int32_t lx=x*65536+offsets[i][0],lz=z*65536+offsets[i][1];
            direct=world=777;
            int result=hg_collision_height(bark.collision,bark.collision_size,0,0,lx-1048576,lz-1048576,&direct);
            CHECK(hg_field_resources_height_at(field,21*2097152+lx,12*2097152+lz,0,0,&world)==result);
            CHECK(world==direct);
        }
    }
    CHECK(hg_field_resources_loaded(field)==1);
    CHECK(hg_field_resources_height(field,-1,0,0,0,&height)==-1);
    hg_field_resources_free(field);
    /* Arithmetic and decoder failure leave output untouched. */
    saved[36]=saved[37]=saved[38]=saved[39]=0;height=777;
    CHECK(hg_collision_height(saved,80,0,0,0,0,&height)==-1 && height==777);
    CHECK(hits>0);printf("Original height solver: New Bark tile centers %u hits / %u misses match independent plane equations\n",hits,misses);
    /* Explicit traps: unsupported instruction, bad pointer, write to code,
     * misalignment and exhausted instruction budget. */
    c=cpu(HG_THUMB_ov01_021FAD1C);c.r[0]=0;CHECK(ds_thumb_run(&c,100)==DS_THUMB_BAD_MEMORY);
    c=cpu(HG_THUMB_ov01_021FAD1C);c.r[0]=BASE+0x1001;CHECK(ds_thumb_run(&c,100)==DS_THUMB_BAD_MEMORY);
    c=cpu(HG_THUMB_ov01_021FADD4);c.r[0]=BASE;CHECK(ds_thumb_run(&c,100)==DS_THUMB_BAD_MEMORY);
    c=cpu(HG_THUMB_ov01_021FADD4);c.r[0]=BASE+0x1800;CHECK(ds_thumb_run(&c,1)==DS_THUMB_BUDGET);
    c=cpu(0);c.code_start=BASE+0x1900;c.code_size=2;c.r[15]=BASE+0x1900;ram[0x1900]=0;ram[0x1901]=0xde;
    CHECK(ds_thumb_run(&c,100)==DS_THUMB_BAD_INSTRUCTION);
    printf("Original Thumb: solver and eight helpers executed; %u planes from all 676 land resources checked; guest traps passed\n",planes);
    return 0;
}
