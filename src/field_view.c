#include "field_view.h"
#include "prop_runtime.h"
#include "field_data.h"
#include "map_header.h"
#include "map_matrix.h"
#include "constants/maps.h"
#include "compat/resources.h"
#include <math.h>
struct HGFieldView {
    DSRasterTriangle *triangles;size_t count,capacity;
    DSRasterMaterial materials[256];unsigned material_count;int failed;
    double min[3],max[3];
    unsigned material_base;int prop_materials[550];DSTransform transform;
};
static void collect(void *user,const DSSceneVertex tri[3]) {
    HGFieldView *v=user;if(v->failed)return;
    if(v->count==v->capacity) {
        if(v->capacity>=100000) { v->failed=1;return; }
        size_t cap=v->capacity?v->capacity*2:1024;void *p=realloc(v->triangles,cap*sizeof(*v->triangles));
        if(!p) { v->failed=1;return; }v->triangles=p;v->capacity=cap;
    }
    for(unsigned i=0;i<3;i++) {
        unsigned material=v->material_base+tri[i].material;
        if(material>=v->material_count || !v->materials[material].textured) { v->failed=1;return; }
        DSSceneVertex vertex=tri[i];vertex.material=material;ds_transform_point(&v->transform,tri[i].x,tri[i].y,tri[i].z,&vertex.x,&vertex.y,&vertex.z);
        v->triangles[v->count].v[i]=vertex;double xyz[]={vertex.x,vertex.y,vertex.z};
        for(unsigned k=0;k<3;k++) { if(!v->count && !i)v->min[k]=v->max[k]=xyz[k];if(xyz[k]<v->min[k])v->min[k]=xyz[k];if(xyz[k]>v->max[k])v->max[k]=xyz[k]; }
    }
    v->count++;
}
static int bind_materials(HGFieldView *view,const DSModel *model,const DSTextureBank *bank,unsigned base) {
    if(base+model->materials>256)return 0;
    for(unsigned i=0;i<model->materials;i++) {
        DSModelMaterial mat;if(!ds_model_material(model,i,&mat))return 0;if(!mat.texture[0])continue;
        if((!mat.identity_uv && !mat.translation_uv) || !ds_texture_find(bank,mat.texture,mat.palette,&view->materials[base+i].texture)) { fprintf(stderr,"binding %u %s/%s identity=%d\n",i,mat.texture,mat.palette,mat.identity_uv);return 0; }
        view->materials[base+i].uv_u=mat.uv_u;view->materials[base+i].uv_v=mat.uv_v;view->materials[base+i].params=mat.texture_params;view->materials[base+i].polygon=mat.polygon_attrs;view->materials[base+i].textured=1;
    }return 1;
}
static int prop_draw(void *user,unsigned id,const uint8_t *bytes,size_t size,const DSTransform *transform,int material,int shape) {
    HGFieldView *view=user;DSModel model;DSTextureBank bank;DSModelStats stats;
    if(id>=550 || !ds_model_open(bytes,size,0,&model))return 0;
    if(view->prop_materials[id]<0) {
        if(!ds_texture_bank(bytes,size,&bank) || !bind_materials(view,&model,&bank,view->material_count)) { fprintf(stderr,"Unsupported prop material: %u\n",id);return 0; }
        view->prop_materials[id]=(int)view->material_count;view->material_count+=model.materials;
    }
    view->material_base=(unsigned)view->prop_materials[id];view->transform=*transform;
    int ok=shape<0?ds_model_scene(&model,collect,view,&stats):ds_model_scene_shape(&model,(unsigned)shape,(unsigned)material,collect,view,&stats);
    return ok && !view->failed;
}
static int append_props(HGFieldView *view,const HGLandChunk *land,unsigned list_id) {
    const uint8_t *bytes;size_t size;Field3dRenderObjManager *resources=hg_prop_resources_new();MapPropManager *props=NULL;AreaDataManager *area=NULL;int ok=0;
    if(!resources || !hg_props_validate(land->props,land->props_size))goto cleanup;
    if(!native_resource_member(NARC_a_0_4_3,0,(int)list_id,&bytes,&size) || size<2)goto cleanup;
    unsigned count=bytes[0]|(unsigned)bytes[1]<<8;if(size!=2+2*count || count>=550)goto cleanup;
    unsigned ids[550];for(unsigned i=0;i<count;i++)ids[i]=bytes[2+2*i]|(unsigned)bytes[3+2*i]<<8;
    for(unsigned i=0;i<=count;i++) { unsigned id=i<count?ids[i]:0;if(!native_resource_member(NARC_a_0_4_0,0,(int)id,&bytes,&size) || !hg_prop_resources_load(resources,id,bytes,size))goto cleanup; }
    props=MapPropManager_New(HEAP_ID_FIELD1,resources);if(!props)goto cleanup;
    NARC *stream=native_resource_stream(land->props,land->props_size);if(!stream)goto cleanup;
    MapPropManager_LoadFromNARC(stream,(u32)land->props_size,props,0);NARC_Delete(stream);
    if(!native_resource_file("bm_field_matshp.dat",&bytes,&size) || !(area=hg_prop_area_new(bytes,size)))goto cleanup;
    hg_prop_draw_begin(prop_draw,view);ov01_021F3C9C(props,area);ok=hg_prop_draw_end(NULL) && !view->failed;
cleanup:if(props) { MapPropManager_Reset(props);MapPropManager_Free(props); }hg_prop_area_free(area);hg_prop_resources_free(resources);return ok;
}
HGFieldView *hg_field_view_new(unsigned map) {
    if(map>=MAP_ID_MAX)return NULL;
    const uint8_t *p;size_t n;
    if(!native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(map),&p,&n) || n!=8)return NULL;
    if(p[6]!=1)return NULL;unsigned list_id=p[0]|(unsigned)p[1]<<8;
    unsigned tex_id=p[2]|(unsigned)p[3]<<8;DSTextureBank bank;
    if(!native_resource_member(NARC_a_0_4_4,0,(int)tex_id,&p,&n) || !ds_texture_bank(p,n,&bank))return NULL;
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);unsigned member=65535,matches=0;
    for(unsigned z=0;z<MapMatrix_GetHeight(matrix);z++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,x,z)==map) { member=MapMatrix_GetMapModelNo((int)(z*matrix->width+x),matrix);matches++; }
    MapMatrix_Free(matrix);
    /* One complete chunk is the current scene contract; never silently crop a route. */
    HGLandChunk land;DSModel model;
    if(matches!=1 || !native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&p,&n) || !hg_land_decode(p,n,&land) || !ds_model_open(land.model,land.model_size,0,&model))return NULL;
    HGFieldView *view=calloc(1,sizeof(*view));if(!view)return NULL;view->material_count=model.materials;view->transform=ds_transform_identity();for(unsigned i=0;i<550;i++)view->prop_materials[i]=-1;
    if(!bind_materials(view,&model,&bank,0))goto fail;
    DSModelStats stats;if(!ds_model_scene(&model,collect,view,&stats) || view->failed || !view->count)goto fail;
    if(!append_props(view,&land,list_id))goto fail;return view;
fail:hg_field_view_free(view);return NULL;
}
void hg_field_view_free(HGFieldView *v) { if(v) { free(v->triangles);free(v); } }
void hg_field_view_camera(const HGFieldView *v,DSCamera *c,unsigned w,unsigned h) {
    if(!v || !c)return;double radius=0;
    for(unsigned i=0;i<3;i++)radius+=(v->max[i]-v->min[i])*(v->max[i]-v->min[i]);radius=sqrt(radius);
    *c=(DSCamera){.yaw=0.7853981634,.pitch=0.85,.scale=(w<h?w:h)*0.85/(radius>0?radius:1),.x=(v->min[0]+v->max[0])/2,.y=(v->min[1]+v->max[1])/2,.z=(v->min[2]+v->max[2])/2};
}
int hg_field_view_render(const HGFieldView *v,const DSCamera *c,uint8_t *out,unsigned w,unsigned h) { return v && ds_raster_draw(v->triangles,v->count,v->materials,v->material_count,c,out,w,h); }
size_t hg_field_view_triangles(const HGFieldView *v) { return v?v->count:0; }
