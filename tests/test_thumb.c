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
typedef struct {
    unsigned reads,writes,width;
    uint32_t address,value;
    int handled;
} Device;
static int device_read(void *user,uint32_t address,unsigned width,uint32_t *out) {
    Device *d=user;d->reads++;d->address=address;d->width=width;
    if(d->handled!=1)return d->handled;
    if(address!=0x04000008u)return 0;
    *out=d->value;return 1;
}
static int device_write(void *user,uint32_t address,unsigned width,uint32_t value) {
    Device *d=user;d->writes++;d->address=address;d->width=width;
    if(d->handled!=1)return d->handled;
    if(address!=0x04000008u)return 0;
    d->value=value;return 1;
}
static DSThumb device_cpu(unsigned instruction,Device *device) {
    ram[0]=(uint8_t)instruction;ram[1]=(uint8_t)(instruction>>8);ram[2]=0x70;ram[3]=0x47;
    DSThumb c=cpu(0);c.code_size=4;c.r[1]=0x04000008u;
    c.device_read=device_read;c.device_write=device_write;c.device_user=device;return c;
}
static void device_tests(void) {
    /* Synthetic Thumb instructions exercise the shared bus contract without a
     * renderer, loader import, or fabricated MMIO-backed pointer mapping. */
    const unsigned loads[]={0x7808,0x8808,0x6808},stores[]={0x7008,0x8008,0x6008};
    const uint32_t masks[]={0xff,0xffff,UINT32_MAX};
    for(unsigned i=0;i<3;i++) {
        Device d={.value=0xfedcba98u,.handled=1};DSThumb c=device_cpu(loads[i],&d);
        run(&c);CHECK(c.r[0]==(0xfedcba98u&masks[i]));
        CHECK(d.reads==1 && !d.writes && d.width==(1u<<i) && d.address==0x04000008u);
        c=device_cpu(stores[i],&d);c.r[0]=0x76543210u;run(&c);
        CHECK(d.reads==1 && d.writes==1 && d.width==(1u<<i) && d.value==(0x76543210u&masks[i]));
    }
    Device d={.value=0xeeee8080u,.handled=1};DSThumb c=device_cpu(0x5688,&d);c.r[2]=0;
    run(&c);CHECK(c.r[0]==0xffffff80u && d.width==1);
    c=device_cpu(0x5e88,&d);c.r[2]=0;run(&c);CHECK(c.r[0]==0xffff8080u && d.width==2);
    for(unsigned write=0;write<2;write++) {
        for(unsigned i=1;i<3;i++) {
            d=(Device){.value=0x12345678,.handled=1};c=device_cpu(write?stores[i]:loads[i],&d);c.r[1]++;
            CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && c.fault_address==0x04000009u);
            CHECK(!d.reads && !d.writes && d.value==0x12345678);
        }
        d=(Device){.value=0x12345678};c=device_cpu(write?stores[1]:loads[1],&d);c.r[0]=0x55555555;
        CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && c.fault_address==0x04000008u);
        CHECK(d.reads==!write && d.writes==write && d.value==0x12345678 && c.r[0]==0x55555555);
        c=device_cpu(write?stores[1]:loads[1],&d);c.device_read=NULL;c.device_write=NULL;
        CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY);
        CHECK(d.reads==!write && d.writes==write);
        for(int handled=-1;handled<=2;handled+=3) {
            d=(Device){.value=0x12345678,.handled=handled};c=device_cpu(write?stores[1]:loads[1],&d);
            CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && d.value==0x12345678);
        }
        /* Unmapped RAM, the far I/O bound, and address-space wrap reject before
         * callbacks. A partial ordinary-memory access cannot become a device. */
        const uint32_t bad[]={BASE+sizeof(ram)-2,0x05000000u,UINT32_MAX};
        for(unsigned i=0;i<3;i++) {
            d=(Device){.handled=1};c=device_cpu(write?stores[2]:loads[2],&d);c.r[1]=bad[i];
            CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && c.fault_address==bad[i]);
            CHECK(!d.reads && !d.writes);
        }
        d=(Device){.handled=1};c=device_cpu(write?stores[2]:loads[2],&d);c.r[1]=BASE+0x1000;
        put(0x1000,0x12345678);c.r[0]=0x87654321;run(&c);
        CHECK(!d.reads && !d.writes);
        CHECK(write?get(0x1000)==0x87654321:c.r[0]==0x12345678);
    }
    /* Code remains read-only with hooks, and neither instruction halfword of
     * a BL instruction may be fetched by a device callback. */
    d=(Device){.handled=1};c=device_cpu(0x6008,&d);c.r[1]=BASE;
    CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && c.fault_address==BASE && !d.writes);
    c=device_cpu(0x6008,&d);c.code_start=0x04000008u;c.code_size=4;c.r[15]=c.code_start;
    CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && !d.reads);
    ram[sizeof(ram)-2]=0;ram[sizeof(ram)-1]=0xf0;
    c=device_cpu(0x6008,&d);c.code_start=BASE+sizeof(ram)-2;c.code_size=4;c.r[15]=c.code_start;
    CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && !d.reads);
    /* Explicit RAM views keep their existing bounded semantics; adding device
     * dispatch does not redirect a compatible mapped view to a callback. */
    uint8_t bytes[]={0x34,0x12};DSNativeRegion view={.address=0x04000008u,.bytes=bytes,.size=sizeof(bytes)};
    c=device_cpu(0x8808,&d);c.memory.regions=&view;c.memory.region_count=1;run(&c);
    CHECK(c.r[0]==0x1234 && !d.reads);
    c=device_cpu(0x8008,&d);c.memory.regions=&view;c.memory.region_count=1;c.r[0]=0xaabbccdd;run(&c);
    CHECK(bytes[0]==0xdd && bytes[1]==0xcc && !d.writes);
    c=device_cpu(0x6808,&d);c.memory.regions=&view;c.memory.region_count=1;
    CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && !d.reads);
    c=device_cpu(0x6008,&d);c.memory.regions=&view;c.memory.region_count=1;
    CHECK(ds_thumb_run(&c,10)==DS_THUMB_BAD_MEMORY && !d.writes && bytes[0]==0xdd && bytes[1]==0xcc);
}
static void compiler_multiply_tests(void) {
    const uint64_t vectors[][3]={
        {0,UINT64_MAX,0},{1,UINT64_MAX,UINT64_MAX},
        {UINT64_MAX,2,UINT64_MAX-1},
        {0xffffffffULL,0xffffffffULL,0xfffffffe00000001ULL},
        {0x100000000ULL,0x100000000ULL,0},
        {0x8000000000000000ULL,3,0x8000000000000000ULL},
        {0x100000001ULL,0x100000001ULL,0x200000001ULL}
    };
    for(unsigned i=0;i<sizeof(vectors)/sizeof(vectors[0]);i++) {
        DSThumb c=cpu(0);
        c.r[0]=vectors[i][0];c.r[1]=vectors[i][0]>>32;
        c.r[2]=vectors[i][1];c.r[3]=vectors[i][1]>>32;
        CHECK(ds_thumb_math_boundary(&c,"__aeabi_lmul")==1);
        CHECK((c.r[0]|(uint64_t)c.r[1]<<32)==vectors[i][2]);
    }
}
static void compiler_shift_tests(void) {
    const uint64_t results[]={0x8000000100000001ULL,0x4000000080000000ULL,
        0x100000002ULL,0x80000001ULL,1};
    const unsigned counts[]={0,1,31,32,63};
    for(unsigned i=0;i<5;i++) {
        DSThumb c=cpu(0);c.r[0]=1;c.r[1]=0x80000001;c.r[2]=counts[i];
        CHECK(ds_thumb_math_boundary(&c,"__aeabi_llsr")==1);
        CHECK((c.r[0]|(uint64_t)c.r[1]<<32)==results[i]);
    }
    DSThumb c=cpu(0);c.r[0]=1;c.r[1]=2;c.r[2]=64;
    CHECK(ds_thumb_math_boundary(&c,"__aeabi_llsr")==-1&&c.r[0]==1&&c.r[1]==2);
}
static void high_register_pc_tests(void) {
    /* ARM7 Thumb MOV to PC stays in Thumb state and ignores target bit zero;
     * it neither performs BX interworking nor updates the condition flags. */
    for(unsigned odd=0;odd<2;odd++) {
        memset(ram,0xff,16);
        ram[0]=0x87;ram[1]=0x46; /* MOV pc,r0 */
        ram[8]=0x70;ram[9]=0x47; /* BX lr */
        DSThumb c=cpu(0);c.code_size=16;c.r[0]=BASE+8+odd;
        c.n=1;c.z=0;c.c=1;c.v=1;
        run(&c);
        CHECK(c.steps==2&&c.n==1&&!c.z&&c.c==1&&c.v==1);
        CHECK(c.r[0]==BASE+8+odd);
    }
    ram[0]=0xff;ram[1]=0x46; /* MOV pc,pc uses the pipelined PC + 4 */
    ram[4]=0x70;ram[5]=0x47;
    DSThumb c=cpu(0);c.code_size=16;run(&c);CHECK(c.steps==2);
    ram[0]=0xf7;ram[1]=0x46; /* MOV pc,lr can return without BX */
    c=cpu(0);c.code_size=16;run(&c);CHECK(c.steps==1);
}
int main(void) {
    device_tests();
    high_register_pc_tests();
    compiler_multiply_tests();
    compiler_shift_tests();
    const uint32_t arithmetic[]={0,1,0x7fffffff,0x80000000u,0xfffffffeu,0xffffffffu};
    for(unsigned i=0;i<6;i++)for(unsigned j=0;j<6;j++)for(unsigned carry=0;carry<2;carry++){
        /* SBC r0,r1; bx lr. Independent signed/unsigned mathematical results. */
        ram[0]=0x88;ram[1]=0x41;ram[2]=0x70;ram[3]=0x47;
        DSThumb subc=cpu(0);subc.code_size=4;subc.r[0]=arithmetic[i];subc.r[1]=arithmetic[j];subc.c=carry;
        int64_t signed_result=(int64_t)(int32_t)arithmetic[i]-(int32_t)arithmetic[j]-(1-carry);
        int64_t unsigned_result=(int64_t)arithmetic[i]-arithmetic[j]-(1-carry);
        run(&subc);CHECK(subc.r[0]==(uint32_t)unsigned_result);
        CHECK(subc.c==(unsigned_result>=0));CHECK(subc.v==(signed_result<INT32_MIN||signed_result>INT32_MAX));
        CHECK(subc.n==(subc.r[0]>>31)&&subc.z==(subc.r[0]==0));
    }
    for(unsigned i=0;i<6;i++){
        DSThumb arithmetic_cpu={0};arithmetic_cpu.r[0]=arithmetic[i];
        int status=ds_thumb_math_boundary(&arithmetic_cpu,"abs");
        if(arithmetic[i]==0x80000000u){CHECK(status==-1&&arithmetic_cpu.r[0]==arithmetic[i]);}
        else {int64_t value=(int32_t)arithmetic[i];CHECK(status==1&&arithmetic_cpu.r[0]==(uint32_t)(value<0?-value:value));}
    }
    /* Register shifts use the low eight count bits and preserve V; edge
     * counts differ from immediate shifts and from native C shift rules. */
    const unsigned ops[]={2,3,4},counts[]={0,1,31,32,33,255,256};
    const uint32_t values[]={0,1,0x80000000u,0x80000001u,0xffffffffu};
    for(unsigned op=0;op<3;op++)for(unsigned k=0;k<7;k++)for(unsigned v=0;v<5;v++)for(unsigned carry=0;carry<2;carry++) {
        unsigned instruction=0x4000|(ops[op]<<6)|(1<<3);
        ram[0]=(uint8_t)instruction;ram[1]=(uint8_t)(instruction>>8);ram[2]=0x70;ram[3]=0x47;
        DSThumb shift=cpu(0);shift.code_size=4;shift.r[0]=values[v];shift.r[1]=counts[k];shift.c=carry;shift.v=1;
        uint32_t expected=values[v];unsigned expected_carry=carry;
        for(unsigned bit=0;bit<(counts[k]&255);bit++) {
            if(ops[op]==2){expected_carry=expected>>31;expected<<=1;}
            else {expected_carry=expected&1;expected=(expected>>1)|((ops[op]==4)?expected&0x80000000u:0);}
        }
        run(&shift);CHECK(shift.r[0]==expected&&shift.c==expected_carry&&shift.v==1);
        CHECK(shift.z==(expected==0)&&shift.n==(expected>>31));
    }
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
