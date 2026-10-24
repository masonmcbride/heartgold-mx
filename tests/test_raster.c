#include "ds_raster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static DSRasterTriangle triangle(double z,unsigned material) {
    DSRasterTriangle t={0};double x[]={-6,6,0},y[]={-6,-6,6};
    for(unsigned i=0;i<3;i++)t.v[i]=(DSSceneVertex){.x=x[i],.y=y[i],.z=z,.color=32767,.material=material};return t;
}
int main(void) {
    const uint8_t palette[]={31,0,224,3,0,124,255,127};uint8_t texels[16];memset(texels,0xe4,sizeof(texels));
    DSTexture tex={texels,palette,16,8,8,8,2,0};uint32_t pixel;
    CHECK(ds_texture_sample(&tex,0,0,0,&pixel) && pixel==0xff0000ff);
    CHECK(ds_texture_sample(&tex,1,0,0,&pixel) && pixel==0xff00ff00);
    CHECK(ds_texture_sample(&tex,2,0,0,&pixel) && pixel==0xffff0000);
    CHECK(ds_texture_sample(&tex,-1,0,0,&pixel) && pixel==0xff0000ff);
    CHECK(ds_texture_sample(&tex,-1,0,1<<16,&pixel) && pixel==0xffffffff);
    CHECK(ds_texture_sample(&tex,8,0,1<<16,&pixel) && pixel==0xff0000ff);
    CHECK(ds_texture_sample(&tex,8,0,(1<<16)|(1<<18),&pixel) && pixel==0xffffffff);
    CHECK(ds_texture_sample(&tex,15,0,(1<<16)|(1<<18),&pixel) && pixel==0xff0000ff);
    tex.transparent_zero=1;CHECK(ds_texture_sample(&tex,0,0,0,&pixel) && (pixel>>24)==0);tex.transparent_zero=0;
    CHECK(!ds_texture_sample(&tex,NAN,0,0,&pixel));
    const uint8_t a3[]={0xa1},a5[]={0x81};
    DSTexture alpha={a3,palette,1,8,1,1,1,0};CHECK(ds_texture_sample(&alpha,0,0,0,&pixel) && pixel>>24==182);
    alpha.texels=a5;alpha.format=6;CHECK(ds_texture_sample(&alpha,0,0,0,&pixel) && pixel>>24==131);
    const uint8_t red[]={31,128},blue[]={0,252},clear[]={31,0};
    DSRasterMaterial mats[2]={{{red,NULL,2,0,1,1,7,0},0,31u<<16,1},{{blue,NULL,2,0,1,1,7,0},0,31u<<16,1}};
    DSRasterTriangle tris[]={triangle(2,0),triangle(0,1)},reverse[]={tris[1],tris[0]};
    DSCamera camera={.scale=1};uint8_t front[16*16*4],back[sizeof(front)];size_t center=4*(8*16+8);
    /* Applying a material UV matrix must equal transforming the mesh UVs. */
    DSRasterMaterial uvmat={.texture=tex,.polygon=31u<<16,.textured=1,.uv_transform=1,.uv_matrix={0,2,-.5,0,3,1}};
    DSRasterTriangle uvtri=triangle(0,0),mapped=uvtri;
    for(unsigned i=0;i<3;i++){uvtri.v[i].u=i;uvtri.v[i].v=2*i;mapped.v[i].u=3-i;mapped.v[i].v=1+2*i;}
    CHECK(ds_raster_draw(&uvtri,1,&uvmat,1,&camera,front,16,16));uvmat.uv_transform=0;
    CHECK(ds_raster_draw(&mapped,1,&uvmat,1,&camera,back,16,16));CHECK(!memcmp(front,back,sizeof(front)));
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));CHECK(front[center]==255 && front[center+2]==0);
    CHECK(ds_raster_draw(reverse,2,mats,2,&camera,back,16,16));CHECK(!memcmp(front,back,sizeof(front)));
    mats[0].texture.texels=clear;CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));CHECK(front[center]==0 && front[center+2]==255);
    mats[0].texture.texels=red;mats[0].polygon=16u<<16;
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));CHECK(front[center]==131 && front[center+2]==124);
    CHECK(ds_raster_draw(reverse,2,mats,2,&camera,back,16,16));CHECK(!memcmp(front,back,sizeof(front)));
    /* Texture alpha still needs blending even with fully opaque polygons. */
    mats[0].polygon=31u<<16;mats[0].texture=(DSTexture){a3,palette,1,8,1,1,1,0};
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    CHECK(front[center+1]==182 && front[center+2]==73);
    mats[0].texture=(DSTexture){a5,palette,1,8,1,1,6,0};
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    CHECK(front[center+1]==131 && front[center+2]==124);
    mats[0].texture=(DSTexture){red,NULL,2,0,1,1,7,0};mats[0].polygon=16u<<16;
    /* A shared diagonal must not blend twice. One large square split in two. */
    double xs[]={-6,6,6,-6},ys[]={6,6,-6,-6};unsigned ids[2][3]={{0,1,2},{0,2,3}};
    for(unsigned t=0;t<2;t++)for(unsigned i=0;i<3;i++)tris[t].v[i]=(DSSceneVertex){.x=xs[ids[t][i]],.y=ys[ids[t][i]],.color=32767,.material=0};
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    for(unsigned y=3;y<13;y++)for(unsigned x=3;x<13;x++) { size_t at=4*(y*16+x);CHECK(front[at]==142 && front[at+2]==18); }
    /* Textureless surfaces use vertex color and retain polygon opacity. */
    mats[0].textured=0;mats[0].polygon=31u<<16;
    for(unsigned t=0;t<2;t++)for(unsigned i=0;i<3;i++)tris[t].v[i].color=31u<<5;
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    CHECK(front[center]==0 && front[center+1]==255 && front[center+2]==0);
    unsigned alternative[2][3]={{0,1,3},{1,2,3}};
    for(unsigned t=0;t<2;t++)for(unsigned i=0;i<3;i++) {
        unsigned id=ids[t][i];tris[t].v[i].x=xs[id]*0.93+ys[id]*0.17+0.13;tris[t].v[i].y=ys[id]*0.93-xs[id]*0.17+0.21;
    }
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    for(unsigned t=0;t<2;t++)for(unsigned i=0;i<3;i++) {
        unsigned id=alternative[t][i];tris[t].v[i].x=xs[id]*0.93+ys[id]*0.17+0.13;tris[t].v[i].y=ys[id]*0.93-xs[id]*0.17+0.21;
    }
    CHECK(ds_raster_draw(tris,2,mats,2,&camera,back,16,16));
    for(unsigned i=0;i<16*16;i++) {
        CHECK((front[4*i+1]>200)==(back[4*i+1]>200));
        for(unsigned k=0;k<4;k++)CHECK(abs((int)front[4*i+k]-(int)back[4*i+k])<=1);
    }
    mats[0].polygon=0;CHECK(ds_raster_draw(tris,2,mats,2,&camera,back,16,16));
    CHECK(memcmp(front,back,sizeof(front)));
    camera.yaw=NAN;CHECK(!ds_raster_draw(tris,2,mats,2,&camera,front,16,16));
    puts("Texture addressing/alpha, depth order independence, cutout, blending and shared-edge coverage passed");return 0;
}
