#include "prop_runtime.h"
#include <mach-o/ldsyms.h>
#include "ds_model.h"
#include <math.h>
struct HGPropModel { const uint8_t *bytes;size_t size;DSModel model;unsigned id; };
struct Field3dRenderObjManager { struct HGPropModel models[550];Field3dRenderObj *handles[128];unsigned live; };
static unsigned r16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t r32(const uint8_t *p) { return r16(p)|(uint32_t)r16(p+2)<<16; }
Field3dRenderObjManager *hg_prop_resources_new(void) { return calloc(1,sizeof(Field3dRenderObjManager)); }
int hg_prop_resources_load(Field3dRenderObjManager *r,unsigned id,const uint8_t *p,size_t n) {
    if(!r || id>=550 || !p || n<20 || memcmp(p,"BMD0",4) || r16(p+4)!=0xfeff || r32(p+8)!=n || r16(p+12)!=16)return 0;
    unsigned blocks=r16(p+14);if(!blocks || blocks>2 || n<16+4*blocks)return 0;
    size_t offset=16+4*blocks;
    for(unsigned i=0;i<blocks;i++) {
        size_t start=r32(p+16+4*i);if(start<offset || start>n-8)return 0;
        size_t size=r32(p+start+4);if(size<8 || size>n-start)return 0;
        if(memcmp(p+start,i?"TEX0":"MDL0",4))return 0;offset=start+size;
    }
    if(offset!=n)return 0;DSModel model;if(!ds_model_open(p,n,0,&model))return 0;r->models[id]=(struct HGPropModel){p,n,model,id};return 1;
}
void hg_prop_resources_free(Field3dRenderObjManager *r) { if(!r)return;if(r->live)abort();free(r); }
unsigned hg_prop_resources_live(const Field3dRenderObjManager *r) { return r->live; }
BOOL Field3dRenderObjManager_IsResFileHeaderLoadedByIndex(Field3dRenderObjManager *r,int id) { return id>=0 && id<550 && r->models[id].bytes!=NULL; }
Field3dRenderObj *Field3dRenderObjManager_GetOrAllocRenderObjByID(Field3dRenderObjManager *r,int id) {
    if(!Field3dRenderObjManager_IsResFileHeaderLoadedByIndex(r,id))abort();
    for(unsigned i=0;i<128;i++)if(!r->handles[i]) {
        Field3dRenderObj *obj=calloc(1,sizeof(*obj));if(!obj)abort();obj->index=id;obj->model=&r->models[id];obj->renderObj.model=obj->model;r->handles[i]=obj;r->live++;return obj;
    }
    abort();
}
BOOL Field3dRenderObjManager_FreeRenderObj(Field3dRenderObjManager *r,Field3dRenderObj *obj) {
    if(!obj)return FALSE;for(unsigned i=0;i<128;i++)if(r->handles[i]==obj) { free(obj);r->handles[i]=NULL;r->live--;return TRUE; }abort();
}
int hg_props_validate(const uint8_t *p,size_t n) {
    if((!p && n) || n%48 || n/48>MAP_PROP_MAX)return 0;
    for(size_t i=0;i<n;i+=48)if(r32(p+i)>=550)return 0;return 1;
}
void MI_CpuClearFast(void *p,u32 n) { memset(p,0,n); }
static void unsupported(const char *name) { fprintf(stderr,"Unsupported prop boundary: %s\n",name);abort(); }
void GetSafariObjectConfig(SafariObjectConfig *p,int i,int gender) { (void)p;(void)i;(void)gender;unsupported("Safari placement"); }
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
void hg_prop_area_free(AreaDataManager *a) { if(a) { free(a->attributes.models);free(a->attributes.shapes);free(a); } }
AreaDataManager_Sub8AC *ov01_021FB9F4(AreaDataManager *a) { if(!a)abort();return &a->attributes; }
void ov01_021EA9B0(ModelAttributes *a,NNSG3dResMdl *b,u32 c) { (void)a;(void)b;(void)c;unsupported("model lighting attributes"); }
void ov01_021EA7F8(int id,AreaDataManager_Sub8AC *a,u16 *count) { if(!a || id<0 || (unsigned)id>=a->model_count || !count)abort();*count=a->models[id][0]; }
void ov01_021EA804(int id,AreaDataManager_Sub8AC *a,u16 *count,u16 *start) { ov01_021EA7F8(id,a,count);if(!start)abort();*start=a->models[id][1]; }
u16 (*ov01_021EA81C(int start,AreaDataManager_Sub8AC *a))[2] { if(!a || start<0 || (unsigned)start>=a->shape_count)abort();return a->shapes+start; }
static struct { HGPropDraw callback;void *user;VecFx32 translation,scale;MtxFx33 rotation;DSTransform transform;unsigned count;const NNSG3dResMdl *material_model;int material_id,material_valid;int failed,active,ready; } draw;
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
    DSTransform node,identity=ds_transform_identity();
    if(model->model.nodes!=1 || !ds_model_node(&model->model,0,&node)) { draw.failed=1;return; }
    for(unsigned i=0;i<12;i++)if(node.m[i]!=identity.m[i]) { draw.failed=1;return; }
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
