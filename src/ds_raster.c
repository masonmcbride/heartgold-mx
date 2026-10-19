#include "ds_raster.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
typedef struct { double x,y,z,u,v,r,g,b; } V;
typedef struct { V v[3];unsigned material;double depth; } Triangle;
static double edge(const V *a,const V *b,double x,double y) { return (x-a->x)*(b->y-a->y)-(y-a->y)*(b->x-a->x); }
static int inclusive(const V *a,const V *b,double area) {
    if(area<0) { const V *t=a;a=b;b=t; }
    double dy=b->y-a->y,dx=b->x-a->x;return dy>0 || (dy==0 && dx<0);
}
static int order(const void *a,const void *b) { double x=((const Triangle *)a)->depth,y=((const Triangle *)b)->depth;return (x>y)-(x<y); }
static int needs_alpha(const DSRasterMaterial *m) {
    return ((m->polygon>>16)&31)<31 || (m->textured && (m->texture.format==1 || m->texture.format==6));
}
static int paint(const Triangle *t,const DSRasterMaterial *m,uint8_t *rgba,double *depth,unsigned w,unsigned h,int alpha_pass) {
    if(alpha_pass && !needs_alpha(m))return 1;
    if(!alpha_pass && ((m->polygon>>16)&31)<31)return 1;
    const V *v=t->v;double area=edge(v,v+1,v[2].x,v[2].y);if(fabs(area)<1e-10)return 1;
    double lo_x=fmax(0,fmin(v[0].x,fmin(v[1].x,v[2].x))),hi_x=fmin(w-1,fmax(v[0].x,fmax(v[1].x,v[2].x)));
    double lo_y=fmax(0,fmin(v[0].y,fmin(v[1].y,v[2].y))),hi_y=fmin(h-1,fmax(v[0].y,fmax(v[1].y,v[2].y)));
    if(lo_x>hi_x || lo_y>hi_y)return 1;
    for(int y=(int)floor(lo_y);y<=(int)ceil(hi_y);y++)for(int x=(int)floor(lo_x);x<=(int)ceil(hi_x);x++) {
        if((unsigned)x>=w || (unsigned)y>=h)continue;
        double a=edge(v+1,v+2,x+0.5,y+0.5)/area,b=edge(v+2,v,x+0.5,y+0.5)/area,c=1-a-b;
        if(a < -1e-10 || b < -1e-10 || c < -1e-10)continue;
        if((fabs(a)<1e-10 && !inclusive(v+1,v+2,area)) || (fabs(b)<1e-10 && !inclusive(v+2,v,area)) || (fabs(c)<1e-10 && !inclusive(v,v+1,area)))continue;size_t i=(size_t)y*w+x;double z=a*v[0].z+b*v[1].z+c*v[2].z;
        if(z<depth[i]-1e-8)continue;
        uint32_t tex=0xffffffff;
        if(m->textured && !ds_texture_sample(&m->texture,a*v[0].u+b*v[1].u+c*v[2].u+m->uv_u,a*v[0].v+b*v[1].v+c*v[2].v+m->uv_v,m->params,&tex))return 0;
        unsigned alpha=(tex>>24)*((m->polygon>>16)&31)/31;
        if(!alpha || ((alpha<255)!=alpha_pass))continue;
        double color[3]={a*v[0].r+b*v[1].r+c*v[2].r,a*v[0].g+b*v[1].g+c*v[2].g,a*v[0].b+b*v[1].b+c*v[2].b};
        for(unsigned k=0;k<3;k++) {
            double source=((tex>>(8*k))&255)*color[k]/31.0;
            rgba[4*i+k]=(uint8_t)fmin(255,fmax(0,(source*alpha+rgba[4*i+k]*(255-alpha))/255));
        }
        if(alpha==255)depth[i]=z;
    }
    return 1;
}
int ds_raster_project(const DSCamera *camera,const double world[3],unsigned w,unsigned h,double screen[3]) {
    if(!camera||!world||!screen||!w||!h)return 0;
    const double fields[]={camera->yaw,camera->pitch,camera->scale,camera->x,camera->y,camera->z,camera->pan_x,camera->pan_y,world[0],world[1],world[2]};
    for(unsigned i=0;i<11;i++)if(!isfinite(fields[i]))return 0;
    if(camera->scale<=0||camera->scale>1000)return 0;
    double x=world[0]-camera->x,y=world[1]-camera->y,z=world[2]-camera->z;
    double xr=cos(camera->yaw)*x-sin(camera->yaw)*z,zr=sin(camera->yaw)*x+cos(camera->yaw)*z;
    screen[0]=w/2.0+camera->scale*xr+camera->pan_x;
    screen[1]=h/2.0+camera->scale*(sin(camera->pitch)*zr-cos(camera->pitch)*y)+camera->pan_y;
    screen[2]=cos(camera->pitch)*zr+sin(camera->pitch)*y;
    return isfinite(screen[0])&&isfinite(screen[1])&&isfinite(screen[2]);
}
int ds_raster_draw(const DSRasterTriangle *src,size_t count,const DSRasterMaterial *materials,size_t material_count,const DSCamera *camera,uint8_t *rgba,unsigned w,unsigned h) {
    if(!src || !count || count>1000000 || !materials || !material_count || !camera || !rgba || !w || !h || w>2048 || h>2048)return 0;
    const double fields[]={camera->yaw,camera->pitch,camera->scale,camera->x,camera->y,camera->z,camera->pan_x,camera->pan_y};for(unsigned i=0;i<8;i++)if(!isfinite(fields[i]))return 0;
    if(camera->scale<=0 || camera->scale>1000 || fabs(camera->pan_x)>100000 || fabs(camera->pan_y)>100000)return 0;
    Triangle *tri=malloc(count*sizeof(*tri));double *depth=malloc((size_t)w*h*sizeof(*depth));if(!tri || !depth) { free(tri);free(depth);return 0; }
    int alpha_needed=0;
    double cy=cos(camera->yaw),sy=sin(camera->yaw),cp=cos(camera->pitch),sp=sin(camera->pitch);
    for(size_t i=0;i<count;i++) {
        tri[i].material=src[i].v[0].material;if(tri[i].material>=material_count)goto fail;tri[i].depth=0;alpha_needed|=needs_alpha(materials+tri[i].material);
        for(unsigned j=0;j<3;j++) {
            const DSSceneVertex *s=&src[i].v[j];if(s->material!=tri[i].material)goto fail;
            double x=s->x-camera->x,y=s->y-camera->y,z=s->z-camera->z,xr=cy*x-sy*z,zr=sy*x+cy*z;
            V v={w/2.0+camera->scale*xr+camera->pan_x,h/2.0+camera->scale*(sp*zr-cp*y)+camera->pan_y,cp*zr+sp*y,s->u,s->v,s->color&31,(s->color>>5)&31,(s->color>>10)&31};
            if(!isfinite(v.x) || !isfinite(v.y) || !isfinite(v.z) || !isfinite(v.u) || !isfinite(v.v))goto fail;
            tri[i].v[j]=v;tri[i].depth+=v.z/3;
        }
    }
    for(size_t i=0;i<(size_t)w*h;i++) { depth[i]=-DBL_MAX;rgba[i*4]=24;rgba[i*4+1]=31;rgba[i*4+2]=38;rgba[i*4+3]=255; }
    for(size_t i=0;i<count;i++)if(!paint(tri+i,materials+tri[i].material,rgba,depth,w,h,0))goto fail;
    if(alpha_needed) {
        qsort(tri,count,sizeof(*tri),order);
        for(size_t i=0;i<count;i++)if(!paint(tri+i,materials+tri[i].material,rgba,depth,w,h,1))goto fail;
    }
    free(tri);free(depth);return 1;
fail:free(tri);free(depth);return 0;
}
