#include "compat/native_video.h"
#include "prop_runtime.h"
#include <mach-o/ldsyms.h>
#include "ds_model.h"
#include "ds_texture_animation.h"
#include <math.h>
struct HGPropModel { const uint8_t *bytes;size_t size;DSModel model;unsigned id;DSTextureBank textures; };
/* Only decoded resource ownership is native. Original C owns object allocation,
 * lookup and pool lifetime through the unmodified manager declaration. */
typedef struct PropResources {
    Field3dRenderObjManager *manager;
    struct HGPropModel models[550];
    NNSG3dResFileHeader *headers[550];
    struct PropResources *next;
} PropResources;
static PropResources *resources;
static NNSG3dResMdlSet *NNS_G3dGetMdlSet(NNSG3dResFileHeader *file) {
    GF_ASSERT(file&&file->bytes);return file;
}
static NNSG3dResMdl *NNS_G3dGetMdlByIdx(NNSG3dResMdlSet *set,unsigned index) {
    GF_ASSERT(set&&set->bytes&&index==0);return set;
}
static void NNS_G3dRenderObjInit(NNSG3dRenderObj *object,NNSG3dResMdl *model) {
    GF_ASSERT(object&&model&&model->bytes);*object=(NNSG3dRenderObj){.model=model};
}
#include "render_object_manager.inc"
static PropResources *resource_owner(Field3dRenderObjManager *manager) {
    for(PropResources *p=resources;p;p=p->next)if(p->manager==manager||(!p->manager&&p->headers==manager->resFileHeaders)){p->manager=manager;return p;}
    GF_ASSERT(!"unknown native model resource owner");abort();
}
static unsigned r16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t r32(const uint8_t *p) { return r16(p)|(uint32_t)r16(p+2)<<16; }
NNSG3dResFileHeader **hg_prop_resources_headers_new(void){
    PropResources *p=calloc(1,sizeof(*p));if(!p)return NULL;p->next=resources;resources=p;return p->headers;
}
Field3dRenderObjManager *hg_prop_resources_new(void) {
    NNSG3dResFileHeader **headers=hg_prop_resources_headers_new();if(!headers)return NULL;
    Field3dRenderObjManager *manager=Field3dRenderObjManager_New(HEAP_ID_FIELD1,550,128,headers);
    (void)resource_owner(manager);return manager;
}
int hg_prop_resources_load(Field3dRenderObjManager *r,unsigned id,const uint8_t *p,size_t n) {
    if(!r || id>=550 || !p || n<20 || memcmp(p,"BMD0",4) || r16(p+4)!=0xfeff || r32(p+8)!=n || r16(p+12)!=16)return 0;
    unsigned blocks=r16(p+14);if(!blocks || blocks>2 || n<16+4*blocks)return 0;
    size_t offset=16+4*blocks;
    for(unsigned i=0;i<blocks;i++) {
        size_t start=r32(p+16+4*i);if(start<offset || start>n-8)return 0;
        size_t size=r32(p+start+4);if(size<8 || size>n-start)return 0;
        if(memcmp(p+start,i?"TEX0":"MDL0",4))return 0;offset=start+size;
    }
    if(offset!=n)return 0;DSModel model;if(!ds_model_open(p,n,0,&model))return 0;
    PropResources *owner=resource_owner(r);
    /* Existing resource bytes remain borrowed from the scene's archive cache. */
    owner->models[id]=(struct HGPropModel){p,n,model,id};owner->headers[id]=&owner->models[id];return 1;
}
static void free_resources(Field3dRenderObjManager *r) {
    PropResources *owner=resource_owner(r);PropResources **at=&resources;
    while(*at!=owner)at=&(*at)->next;
    *at=owner->next;Field3dRenderObjManager_Delete(r);free(owner);
}
void hg_prop_resources_free(Field3dRenderObjManager *r) {
    if(!r)return;GF_ASSERT(!r->numObjects);free_resources(r);
}
void hg_prop_resources_free_preloaded(Field3dRenderObjManager *r) {
    if(!r)return;
    for(unsigned i=0;i<r->numObjects;i++)GF_ASSERT(r->modelNumToLoadedSlot[r->objects[i]->index]!=0xff);
    free_resources(r);
}
int hg_prop_resources_bind(Field3dRenderObjManager *r,unsigned id,const DSTextureBank *bank) {
    if(id>=550||!bank||!bank->bytes)return 0;
    struct HGPropModel *m=&resource_owner(r)->models[id];if(!m->bytes)return 0;
    for(unsigned i=0;i<m->model.materials;i++) {
        DSModelMaterial mat;DSTexture texture;
        if(!ds_model_material(&m->model,i,&mat))return 0;
        if(mat.texture[0]&&!ds_texture_find(bank,mat.texture,mat.palette,&texture))return 0;
    }
    m->textures=*bank;return 1;
}
int hg_prop_resources_material(Field3dRenderObjManager *r,unsigned id,unsigned index,DSModelMaterial *mat,DSTexture *tex) {
    if(id>=550)return 0;struct HGPropModel *m=&resource_owner(r)->models[id];
    return m->bytes&&m->textures.bytes&&ds_model_material(&m->model,index,mat)&&
        mat->texture[0]&&ds_texture_find(&m->textures,mat->texture,mat->palette,tex);
}
unsigned hg_prop_resources_live(const Field3dRenderObjManager *r) { return r->numObjects; }
int hg_props_validate(const uint8_t *p,size_t n) {
    if((!p && n) || n%48 || n/48>MAP_PROP_MAX)return 0;
    for(size_t i=0;i<n;i+=48)if(r32(p+i)>=550)return 0;return 1;
}
void MI_CpuClearFast(void *p,u32 n) { memset(p,0,n); }
static void unsupported(const char *name) { fprintf(stderr,"Unsupported prop boundary: %s\n",name);abort(); }
__attribute__((weak)) void GetSafariObjectConfig(SafariObjectConfig *p,int i,int gender) { (void)p;(void)i;(void)gender;unsupported("Safari placement"); }
BOOL sub_0201F990(NNSG3dResMdl *a,const VecFx32 *b,const MtxFx33 *c,VecFx32 *d) { (void)a;(void)b;(void)c;(void)d;unsupported("original view culling");return FALSE; }
struct AreaDataManager_Sub8AC { u16 (*models)[2],(*shapes)[2];unsigned model_count,shape_count; };
struct AreaDataManager { AreaDataManager_Sub8AC attributes; };
AreaDataManager *hg_prop_area_new(const uint8_t *p,size_t n) {
    if(!p || n<4)return NULL;unsigned models=r16(p),shapes=r16(p+2);
    if(!models || n!=4+4*(size_t)(models+shapes))return NULL;
    AreaDataManager *area=calloc(1,sizeof(*area));if(!area)return NULL;
    area->attributes.model_count=models;area->attributes.shape_count=shapes;
    area->attributes.models=calloc(models,sizeof(*area->attributes.models));area->attributes.shapes=calloc(shapes?shapes:1,sizeof(*area->attributes.shapes));
    if(!area->attributes.models || !area->attributes.shapes) { hg_prop_area_free(area);return NULL; }
    for(unsigned i=0;i<models;i++) {
        unsigned count=r16(p+4+4*i),start=r16(p+6+4*i);
        if(count && (start>shapes || count>shapes-start)) { hg_prop_area_free(area);return NULL; }
        area->attributes.models[i][0]=count;area->attributes.models[i][1]=start;
    }
    for(unsigned i=0;i<shapes;i++) { area->attributes.shapes[i][0]=r16(p+4+4*models+4*i);area->attributes.shapes[i][1]=r16(p+6+4*models+4*i); }
    return area;
}
int hg_prop_area_has_model(const AreaDataManager *area,unsigned model) { return area&&model<area->attributes.model_count; }
void hg_prop_area_free(AreaDataManager *a) { if(a) { free(a->attributes.models);free(a->attributes.shapes);free(a); } }
AreaDataManager_Sub8AC *ov01_021FB9F4(AreaDataManager *a) { if(!a)abort();return &a->attributes; }
void ov01_021EA9B0(ModelAttributes *a,NNSG3dResMdl *b,u32 c) { (void)a;(void)b;(void)c;unsupported("model lighting attributes"); }
void ov01_021EA7F8(int id,AreaDataManager_Sub8AC *a,u16 *count) { if(!a || id<0 || (unsigned)id>=a->model_count || !count)abort();*count=a->models[id][0]; }
void ov01_021EA804(int id,AreaDataManager_Sub8AC *a,u16 *count,u16 *start) { ov01_021EA7F8(id,a,count);if(!start)abort();*start=a->models[id][1]; }
u16 (*ov01_021EA81C(int start,AreaDataManager_Sub8AC *a))[2] { if(!a || start<0 || (unsigned)start>=a->shape_count)abort();return a->shapes+start; }
static struct { HGPropDraw callback;void *user;VecFx32 translation,scale;MtxFx33 rotation;DSTransform transform;unsigned count;const NNSG3dResMdl *material_model;int material_id,material_valid;int failed,active,ready; } draw;
void hg_prop_renderer_reset_matrices(void){
    GF_ASSERT(!draw.active);draw.translation=(VecFx32){0};memset(&draw.rotation,0,sizeof(draw.rotation));
    draw.rotation.m[0]=draw.rotation.m[4]=draw.rotation.m[8]=4096;draw.scale=(VecFx32){4096,4096,4096};
    draw.transform=ds_transform_identity();hg_native_matrix_init();
}
void hg_prop_renderer_initialize(void){GF_ASSERT(!draw.active);memset(&draw,0,sizeof(draw));hg_native_geometry_init();hg_prop_renderer_reset_matrices();}
void hg_prop_draw_begin(HGPropDraw callback,void *user) { if(draw.active || !callback)abort();memset(&draw,0,sizeof(draw));draw.callback=callback;draw.user=user;draw.active=1; }
int hg_prop_draw_end(unsigned *submissions) { if(!draw.active)return 0;if(submissions)*submissions=draw.count;int ok=!draw.failed;memset(&draw,0,sizeof(draw));return ok; }
void NNS_G3dGlbSetBaseTrans(const VecFx32 *p) { if(!draw.active || !p)abort();draw.translation=*p;draw.ready=0; }
void NNS_G3dGlbSetBaseRot(const MtxFx33 *p) { if(!draw.active || !p)abort();draw.rotation=*p;draw.ready=0; }
void NNS_G3dGlbSetBaseScale(const VecFx32 *p) { if(!draw.active || !p)abort();draw.scale=*p;draw.ready=0; }
void NNS_G3dGlbFlush(void) {
    if(!draw.active)abort();draw.transform=ds_transform_identity();
    for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++)draw.transform.m[4*row+col]=draw.rotation.m[col*3+row]/4096.0;
    draw.transform.m[3]=draw.translation.x/4096.0;draw.transform.m[7]=draw.translation.y/4096.0;draw.transform.m[11]=draw.translation.z/4096.0;
    ds_transform_scale(&draw.transform,draw.scale.x/4096.0,draw.scale.y/4096.0,draw.scale.z/4096.0);draw.ready=1;draw.material_valid=0;
}
static void submit(const NNSG3dResMdl *model,int material,int shape) {
    if(!draw.active || !draw.ready || !model)abort();draw.count++;
    if(!draw.callback(draw.user,model->id,model->bytes,model->size,&draw.transform,material,shape))draw.failed=1;
}
void GF3dRender_DrawModel(NNSG3dRenderObj *obj,const VecFx32 *t,const MtxFx33 *r,const VecFx32 *s) {
    NNS_G3dGlbSetBaseTrans(t);NNS_G3dGlbSetBaseRot(r);NNS_G3dGlbSetBaseScale(s);NNS_G3dGlbFlush();submit(obj->model,-1,-1);
}
void NNS_G3dDraw1Mat1Shp(const NNSG3dResMdl *model,int material,int shape,BOOL send) {
    if(!model || material<0 || (unsigned)material>=model->model.materials || shape<0 || (unsigned)shape>=model->model.shapes) { draw.failed=1;return; }
    DSTransform node;
    if(model->model.nodes!=1 || !ds_model_node(&model->model,0,&node)) { draw.failed=1;return; }
    /* The native shape renderer evaluates the single root's SBC transform.
     * Parent scale compensation and transformed multi-node draws stay refused. */
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)if(c!=r&&node.m[4*r+c]){draw.failed=1;return;}
    if(send) { draw.material_valid=1;draw.material_id=material;draw.material_model=model; }
    else if(!draw.material_valid || draw.material_id!=material || draw.material_model!=model) { draw.failed=1;return; }
    submit(model,material,shape);
}
void sub_02020D2C(MtxFx33 *p,VecFx32 *v) {
    if(!p || !v)abort();const s32 angles[]={v->x,v->y,v->z};DSTransform result=ds_transform_identity();
    /* Native graphics Euler conversion: indexed turns, quantized to 4096 steps.
     * Host trigonometry/Q12 rounding is not a bit-exact SDK sine-table port. */
    for(unsigned axis=0;axis<3;axis++) {
        unsigned index=((uint32_t)angles[axis]&65535u)>>4;double radians=index*(6.2831853071795864769/4096.0);
        double s=lround(sin(radians)*4096)/4096.0,c=lround(cos(radians)*4096)/4096.0;DSTransform r=ds_transform_identity();
        unsigned a=(axis+1)%3,b=(axis+2)%3;r.m[4*a+a]=c;r.m[4*b+b]=c;r.m[4*a+b]=-s;r.m[4*b+a]=s;
        result=ds_transform_multiply(r,result);
    }
    for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++)p->m[col*3+row]=(fx32)lround(result.m[row*4+col]*4096);
}
/* Compiler-generated hooks instrument only the unchanged upstream prop unit.
 * Offsets are relative to the Mach-O image, so ASLR does not affect evidence. */
typedef struct { uintptr_t offset;int enter;const char *phase; } Event;
static Event events[8192];static size_t event_count;static int tracing,overflow;
void hg_prop_trace_begin(void) { event_count=0;overflow=0;tracing=1; }
void hg_prop_trace_mark(const char *phase,unsigned live) { if(event_count==8192) { overflow=1;return; }events[event_count++]=(Event){live,2,phase}; }
void __cyg_profile_func_enter(void *function,void *caller) { (void)caller;if(!tracing)return;if(event_count==8192) { overflow=1;return; }events[event_count++]=(Event){(uintptr_t)function-(uintptr_t)&_mh_execute_header,1}; }
void __cyg_profile_func_exit(void *function,void *caller) { (void)caller;if(!tracing)return;if(event_count==8192) { overflow=1;return; }events[event_count++]=(Event){(uintptr_t)function-(uintptr_t)&_mh_execute_header,0}; }
int hg_prop_trace_write(FILE *out) {
    tracing=0;if(overflow)return 0;for(size_t i=0;i<event_count;i++) {
        if(events[i].enter==2) { fprintf(out,"{\"kind\":\"phase\",\"name\":\"%s\",\"live\":%llu}\n",events[i].phase,(unsigned long long)events[i].offset);continue; }
        fprintf(out,"{\"kind\":\"%s\",\"offset\":%llu}\n",events[i].enter?"enter":"exit",(unsigned long long)events[i].offset); }return !ferror(out);
}

int hg_prop_model_uv_compatible(const NNSG3dResMdl *m,const DSTextureAnimation *a) {
    if(!m||!m->bytes||m->model.bytes[22])return 0;
    for(unsigned i=0;i<a->materials;i++) {
        char name[17];memcpy(name,a->bytes+a->names+16*i,16);name[16]=0;unsigned matched=0;
        for(unsigned j=0;j<m->model.materials;j++){DSModelMaterial mat;if(!ds_model_material(&m->model,j,&mat))return 0;if(!strcmp(mat.name,name))matched++;}
        if(matched!=1)return 0;
    }
    return 1;
}

/* Borrowed buffer resources use the same native model/material representation
 * as archive props. The caller retires render references before deleting a view. */
NNSG3dResMdl *hg_prop_model_view_new(const uint8_t *bytes,size_t size,const DSTextureBank *bank) {
    DSModel model;if(!bank||!ds_model_open(bytes,size,0,&model))return NULL;
    for(unsigned i=0;i<model.materials;i++) {
        DSModelMaterial material;DSTexture texture;
        if(!ds_model_material(&model,i,&material)||(material.texture[0]&&!ds_texture_find(bank,material.texture,material.palette,&texture)))return NULL;
    }
    struct HGPropModel *m=Heap_Alloc(HEAP_ID_FIELD1,sizeof(*m));
    *m=(struct HGPropModel){bytes,size,model,0,*bank};return m;
}
void hg_prop_model_view_delete(NNSG3dResMdl *m){Heap_Free(m);}
int hg_prop_model_view_scene(const NNSG3dResMdl *m,DSSceneTriangle callback,void *user,DSModelStats *stats) {
    return m&&ds_model_scene(&m->model,callback,user,stats);
}

int hg_prop_model_animation_compatible(const NNSG3dResMdl *model,const DSAnimationResource *a){
    if(!model||!a)return 0;
    if(a->kind==DS_ANIMATION_JOINT){for(unsigned i=0;i<a->nodes;i++){unsigned at=a->record[20+2*i]|(unsigned)a->record[21+2*i]<<8;if(a->record[at+3]>=model->model.nodes)return 0;}return 1;}
    DSTextureAnimation names={.bytes=a->record,.size=a->record_size,.materials=a->materials,.names=a->names};
    for(unsigned i=0;i<names.materials;i++){
        char name[17];memcpy(name,names.bytes+names.names+16*i,16);name[16]=0;unsigned matched=0;
        for(unsigned j=0;j<model->model.materials;j++){DSModelMaterial mat;if(!ds_model_material(&model->model,j,&mat))return 0;matched+=!strcmp(mat.name,name);}
        if(matched!=1)return 0;
    }return 1;
}
