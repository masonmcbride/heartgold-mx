#include "field_data.h"
#include "ds_model.h"
#include "map_header.h"
#include "map_matrix.h"
#include "compat/resources.h"
#include "constants/maps.h"
#include <float.h>
#include <math.h>
#define SIDE 640
/* Supported land scene projection; no materials, textures or original camera. */
typedef struct { double minx,miny,maxx,maxy,scale,left,top;uint8_t *pixels;double *depth;int solid; } Preview;
static void project(DSSceneVertex v,double *x,double *y) { *x=(double)v.x-v.z;*y=((double)v.x+v.z)*0.5-v.y; }
static void line(Preview *p,int x,int y,int xx,int yy) {
    int dx=abs(xx-x),dy=-abs(yy-y),sx=x<xx?1:-1,sy=y<yy?1:-1,err=dx+dy;
    for(;;) {
        if(x>=0 && y>=0 && x<SIDE && y<SIDE) { size_t i=3*((size_t)y*SIDE+x);p->pixels[i]=72;p->pixels[i+1]=208;p->pixels[i+2]=160; }
        if(x==xx && y==yy)break;int e=2*err;if(e>=dy) { err+=dy;x+=sx; }if(e<=dx) { err+=dx;y+=sy; }
    }
}
static double edge(double ax,double ay,double bx,double by,double x,double y) { return (x-ax)*(by-ay)-(y-ay)*(bx-ax); }
static void fill(Preview *p,const DSSceneVertex tri[3],const double px[3],const double py[3]) {
    double x[3],y[3],z[3];
    for(unsigned i=0;i<3;i++) { x[i]=p->left+(px[i]-p->minx)*p->scale;y[i]=p->top+(py[i]-p->miny)*p->scale;z[i]=tri[i].x+tri[i].y+tri[i].z; }
    double area=edge(x[0],y[0],x[1],y[1],x[2],y[2]);if(fabs(area)<1e-9)return;
    double ux=tri[1].x-tri[0].x,uy=tri[1].y-tri[0].y,uz=tri[1].z-tri[0].z;
    double vx=tri[2].x-tri[0].x,vy=tri[2].y-tri[0].y,vz=tri[2].z-tri[0].z;
    double nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    double length=sqrt(nx*nx+ny*ny+nz*nz);double light=length>0?0.25+0.75*fabs((nx*0.3+ny+nz*0.2)/(length*sqrt(1.13))):0.25;
    if(!isfinite(light))return;
    int left=(int)floor(fmin(x[0],fmin(x[1],x[2]))),right=(int)ceil(fmax(x[0],fmax(x[1],x[2])));
    int top=(int)floor(fmin(y[0],fmin(y[1],y[2]))),bottom=(int)ceil(fmax(y[0],fmax(y[1],y[2])));
    if(left<0)left=0;if(right>=SIDE)right=SIDE-1;if(top<0)top=0;if(bottom>=SIDE)bottom=SIDE-1;
    for(int yy=top;yy<=bottom;yy++)for(int xx=left;xx<=right;xx++) {
        double a=edge(x[1],y[1],x[2],y[2],xx+0.5,yy+0.5)/area;
        double b=edge(x[2],y[2],x[0],y[0],xx+0.5,yy+0.5)/area,c=1-a-b;
        if(a<0 || b<0 || c<0)continue;double depth=a*z[0]+b*z[1]+c*z[2];size_t i=(size_t)yy*SIDE+xx;
        if(depth<=p->depth[i])continue;p->depth[i]=depth;
        p->pixels[i*3]=(uint8_t)(90*light);p->pixels[i*3+1]=(uint8_t)(200*light);p->pixels[i*3+2]=(uint8_t)(165*light);
    }
}
static void triangle(void *user,const DSSceneVertex tri[3]) {
    Preview *p=user;double x[3],y[3];
    for(unsigned i=0;i<3;i++) {
        project(tri[i],&x[i],&y[i]);
        if(!p->pixels) { if(x[i]<p->minx)p->minx=x[i];if(x[i]>p->maxx)p->maxx=x[i];if(y[i]<p->miny)p->miny=y[i];if(y[i]>p->maxy)p->maxy=y[i]; }
    }
    if(p->pixels && p->solid) { fill(p,tri,x,y);return; }
    if(p->pixels)for(unsigned i=0;i<3;i++) { unsigned j=(i+1)%3;line(p,(int)(p->left+(x[i]-p->minx)*p->scale),(int)(p->top+(y[i]-p->miny)*p->scale),(int)(p->left+(x[j]-p->minx)*p->scale),(int)(p->top+(y[j]-p->miny)*p->scale)); }
}
static int preview(unsigned map,FILE *out,int solid) {
    if(map>=MAP_ID_MAX || !out)return 0;
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);unsigned member=65535;
    for(unsigned z=0;z<MapMatrix_GetHeight(matrix) && member==65535;z++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,x,z)==map) { member=MapMatrix_GetMapModelNo((int)(z*matrix->width+x),matrix);break; }
    MapMatrix_Free(matrix);const uint8_t *bytes;size_t size;HGLandChunk land;DSModel model;
    if(!native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&bytes,&size) || !hg_land_decode(bytes,size,&land) || !ds_model_open(land.model,land.model_size,0,&model))return 0;
    Preview p={.minx=DBL_MAX,.miny=DBL_MAX,.maxx=-DBL_MAX,.maxy=-DBL_MAX,.solid=solid};size_t vertices=0,triangles=0;
    for(unsigned pass=0;pass<2;pass++) {
        DSModelStats stats;
        if(!ds_model_scene(&model,triangle,&p,&stats)) { free(p.depth);free(p.pixels);return 0; }
        if(!pass) { vertices=stats.vertices;triangles=stats.triangles; }
        if(!pass) {
            double width=p.maxx-p.minx,height=p.maxy-p.miny;
            if(!triangles || !isfinite(width) || !isfinite(height) || (width<=0 && height<=0))return 0;
            p.scale=(SIDE-40)/(width>height?width:height);p.left=(SIDE-width*p.scale)/2;p.top=(SIDE-height*p.scale)/2;p.pixels=malloc(SIDE*SIDE*3);if(!p.pixels)return 0;
            memset(p.pixels,18,SIDE*SIDE*3);
            if(solid) {
                p.depth=malloc(SIDE*SIDE*sizeof(*p.depth));if(!p.depth) { free(p.pixels);return 0; }
                for(size_t i=0;i<SIDE*SIDE;i++)p.depth[i]=-DBL_MAX;
            }
        }
    }
    int ok=fprintf(out,"P6\n%d %d\n255\n",SIDE,SIDE)>0 && fwrite(p.pixels,1,SIDE*SIDE*3,out)==SIDE*SIDE*3;
    free(p.depth);free(p.pixels);fprintf(stderr,"Land scene diagnostic: %s, %u shapes, %zu vertices, %zu triangles; supported root transform/SBC, no materials\n",model.name,model.shapes,vertices,triangles);return ok;
}

int hg_field_wireframe(unsigned map,FILE *out) { return preview(map,out,0); }
int hg_field_solid(unsigned map,FILE *out) { return preview(map,out,1); }
