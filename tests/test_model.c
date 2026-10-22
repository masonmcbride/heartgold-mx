#include "ds_model.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static void word(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i)); }
typedef struct { DSModelVertex tri[8][3];unsigned count; } Capture;
static void capture(void *user,const DSModelVertex tri[3]) { Capture *c=user;if(c->count<8)memcpy(c->tri[c->count],tri,sizeof(c->tri[0]));c->count++; }
static size_t primitive(uint8_t *p,unsigned mode,unsigned count) {
    size_t n=0;word(p+n,0x40);n+=4;word(p+n,mode);n+=4;
    for(unsigned i=0;i<count;i++) { word(p+n,0x23);n+=4;word(p+n,i|((i+1)<<16));n+=4;word(p+n,i+2);n+=4; }
    word(p+n,0x41);return n+4;
}
typedef struct { DSSceneVertex tri[8][3];unsigned count; } SceneCapture;
static void capture_scene(void *user,const DSSceneVertex tri[3]) { SceneCapture *c=user;if(c->count<8)memcpy(c->tri[c->count],tri,sizeof(c->tri[0]));c->count++; }
static void dictionary(uint8_t *p,unsigned offset) {
    p[1]=1;p[2]=40;p[6]=16;p[16]=4;p[18]=8;word(p+20,offset);
}
static int test_scene(void) {
    uint8_t data[512]={0};
    word(data,512);word(data+4,128);word(data+8,160);word(data+12,192);word(data+16,512);
    word(data+28,8192);word(data+32,2048);
    dictionary(data+64,40);data[104]=6;word(data+108,4096);word(data+112,(uint32_t)-8192);word(data+116,12288);
    const uint8_t sbc[]={0x26,0,0,0,0,2,0,1,11,4,0,5,0,3,0,5,0,1};memcpy(data+128,sbc,sizeof(sbc));
    dictionary(data+192,40);data[234]=16;word(data+240,16);size_t n=primitive(data+248,0,3);word(data+244,(uint32_t)n);
    DSModel model={.bytes=data,.size=sizeof(data),.shapes_offset=192,.nodes=1,.materials=1,.shapes=1};
    DSModelStats stats;SceneCapture c={0};CHECK(ds_model_scene(&model,capture_scene,&c,&stats));
    CHECK(c.count==2 && stats.vertices==6 && stats.triangles==2);
    CHECK(c.tri[0][0].x==1 && c.tri[0][0].y==-2+2.0/4096 && c.tri[0][0].z==3+4.0/4096);
    CHECK(c.tri[1][0].y==-2+1.0/4096 && c.tri[1][0].z==3+2.0/4096);
    data[135]=0;c.count=0;CHECK(ds_model_scene(&model,capture_scene,&c,&stats));CHECK(c.count==0 && stats.vertices==0);data[135]=1;
    /* Scale-down restores the prior unit scale without scaling translation. */
    data[141]=43;data[142]=0;c.count=0;CHECK(ds_model_scene(&model,capture_scene,&c,&stats));
    CHECK(c.count==2 && c.tri[1][0].y==-2+1.0/4096 && c.tri[1][0].z==3+2.0/4096);
    memcpy(data+128,sbc,sizeof(sbc));
    data[132]=32;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[132]=0;
    data[142]=1;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[142]=0;
    data[140]=1;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[140]=0;
    data[138]=1;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[138]=0;
    data[104]=0;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[104]=6;
    data[128]=0xff;CHECK(!ds_model_scene(&model,NULL,NULL,&stats));data[128]=0x26;
    /* A bounded command stream must terminate; shortening material boundary rejects it. */
    word(data+8,145);CHECK(!ds_model_scene(&model,NULL,NULL,&stats));word(data+8,160);
    /* Pivot and full matrix encodings of the same +90 degree Y rotation. */
    data[104]=0x4d;data[105]=2;data[108]=data[109]=0;data[110]=0;data[111]=16;
    DSTransform pivot,full;CHECK(ds_model_node(&model,0,&pivot));
    c.count=0;CHECK(ds_model_scene(&model,capture_scene,&c,&stats));
    CHECK(c.tri[0][0].x==-4.0/4096 && c.tri[0][0].y==2.0/4096 && c.tri[0][0].z==0);
    for(unsigned form=0;form<9;form++) {
        unsigned flags=13|(form<<4)|512;data[104]=(uint8_t)flags;data[105]=(uint8_t)(flags>>8);
        CHECK(ds_model_node(&model,0,&full));CHECK(full.m[4*(form%3)+form/3]==1);
        for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++) {
            double dot=0;for(unsigned k=0;k<3;k++)dot+=full.m[4*k+a]*full.m[4*k+b];CHECK(dot==(a==b));
        }
    }
    data[104]=5;data[105]=0;data[106]=data[107]=0;
    const int16_t rotation_y[]={0,4096,0,4096,0,-4096,0,0};
    for(unsigned i=0;i<8;i++) { data[108+2*i]=(uint8_t)rotation_y[i];data[109+2*i]=(uint8_t)((uint16_t)rotation_y[i]>>8); }
    CHECK(ds_model_node(&model,0,&full));CHECK(!memcmp(&pivot,&full,sizeof(pivot)));
    DSTransform translation=ds_transform_identity();translation.m[3]=1;translation.m[7]=2;translation.m[11]=3;
    DSTransform composed=ds_transform_multiply(translation,pivot);double x,y,z;ds_transform_point(&composed,4,5,6,&x,&y,&z);CHECK(x==-5 && y==7 && z==7);
    composed=ds_transform_multiply(pivot,translation);ds_transform_point(&composed,4,5,6,&x,&y,&z);CHECK(x==-9 && y==7 && z==5);
    /* Explicit one-root scale and its reciprocal, as used by shop props. */
    word(data+4,144);data[104]=3;data[105]=0;
    const int scales[]={2048,8192,4096,8192,2048,4096};for(unsigned i=0;i<6;i++)word(data+108+4*i,(uint32_t)scales[i]);
    CHECK(ds_model_node(&model,0,&full));CHECK(full.m[0]==0.5&&full.m[5]==2&&full.m[10]==1);
    word(data+120,0);CHECK(!ds_model_node(&model,0,&full));word(data+120,8192);
    word(data+4,131);CHECK(!ds_model_node(&model,0,&full));word(data+4,144);
    CHECK(!ds_model_scene(NULL,NULL,NULL,&stats));CHECK(!ds_model_scene(&model,NULL,NULL,NULL));
    return 0;
}
int main(void) {
    CHECK(test_scene()==0);
    uint8_t bytes[256];DSModelStats stats;Capture c={0};
    size_t n=primitive(bytes,0,3);CHECK(ds_model_geometry(bytes,n,capture,&c,&stats));
    CHECK(stats.vertices==3 && stats.triangles==1 && c.count==1);
    CHECK(c.tri[0][0].x==0 && c.tri[0][1].y==2 && c.tri[0][2].z==4);
    n=primitive(bytes,1,4);c.count=0;CHECK(ds_model_geometry(bytes,n,capture,&c,&stats));
    CHECK(c.count==2 && c.tri[1][0].x==0 && c.tri[1][1].x==2 && c.tri[1][2].x==3);
    n=primitive(bytes,2,5);c.count=0;CHECK(ds_model_geometry(bytes,n,capture,&c,&stats));
    CHECK(c.count==3 && c.tri[1][0].x==2 && c.tri[1][1].x==1 && c.tri[1][2].x==3 && c.tri[2][2].x==4);
    n=primitive(bytes,3,6);c.count=0;CHECK(ds_model_geometry(bytes,n,capture,&c,&stats));
    CHECK(c.count==4 && c.tri[0][2].x==3 && c.tri[1][2].x==2 && c.tri[2][0].x==2 && c.tri[3][2].x==4);
    /* One packed command word with signed VTX10, partial axes, and delta. */
    word(bytes,0x28252440);word(bytes+4,0);word(bytes+8,0x3ff);word(bytes+12,0xfffe0001);word(bytes+16,0x3ff);word(bytes+20,0x41);
    c.count=0;CHECK(ds_model_geometry(bytes,24,capture,&c,&stats));
    CHECK(c.count==1 && c.tri[0][0].x==-64 && c.tri[0][1].x==1 && c.tri[0][1].y==-2 && c.tri[0][2].x==0);
    for(size_t i=1;i<24;i++)CHECK(!ds_model_geometry(bytes,i,NULL,NULL,&stats));
    /* Independent representation equivalence: identical triangles submitted as
     * absolute 16-bit coordinates and as signed 10-bit deltas (Q12 units). */
    uint8_t absolute[64]={0},delta[64]={0};
    const int points[3][3]={{4096,-2048,0},{4108,-2053,511},{3596,-2052,0}};
    size_t at=0;word(absolute+at,0x40);at+=4;word(absolute+at,0);at+=4;
    for(unsigned i=0;i<3;i++) { word(absolute+at,0x23);at+=4;word(absolute+at,(uint16_t)points[i][0]|((uint32_t)(uint16_t)points[i][1]<<16));at+=4;word(absolute+at,(uint16_t)points[i][2]);at+=4; }
    word(absolute+at,0x41);at+=4;
    word(delta,0x40);word(delta+4,0);word(delta+8,0x23);word(delta+12,0xf8001000);word(delta+16,0);
    word(delta+20,0x28);word(delta+24,12|(1019u<<10)|(511u<<20));
    word(delta+28,0x28);word(delta+32,512|(1u<<10)|(513u<<20));word(delta+36,0x41);
    Capture full={0},compact={0};CHECK(ds_model_geometry(absolute,at,capture,&full,&stats));CHECK(ds_model_geometry(delta,40,capture,&compact,&stats));
    CHECK(full.count==1 && compact.count==1 && !memcmp(full.tri,compact.tri,sizeof(full.tri)));
    word(bytes,0x99);CHECK(!ds_model_geometry(bytes,4,NULL,NULL,&stats));
    word(bytes,0x23);CHECK(!ds_model_geometry(bytes,4,NULL,NULL,&stats));
    word(bytes,0x41);CHECK(!ds_model_geometry(bytes,4,NULL,NULL,&stats));
    CHECK(!ds_model_geometry(NULL,0,NULL,NULL,&stats));CHECK(!ds_model_geometry(bytes,4,NULL,NULL,NULL));
    puts("Model geometry: signed coordinates, packed commands, four primitive modes, winding, land transforms, SBC and invalid streams passed");return 0;
}
