#include "compat/prop_animation.h"
#include "compat/resources.h"
#include "field_data.h"
#include "map_header.h"
#include "ds_model.h"
#include "motion_guest.h"
#include "constants/maps.h"
#include <setjmp.h>
struct AnimationDecodeTrap {jmp_buf jump;const char *volatile error;};
static struct AnimationDecodeTrap *animation_decode_trap;
u16 FieldSystem_VarGet(FieldSystem *,u16);
/* Typed native arena and graphics objects: original playback uses native
 * pointers; these are never exposed to guest ARM or serialized as save data. */
typedef struct { enum HeapID heap;unsigned alignment; } NNSFndAllocator;
static void HeapExp_FndInitAllocator(NNSFndAllocator *a,enum HeapID heap,unsigned alignment) {
    GF_ASSERT(alignment==4);*a=(NNSFndAllocator){heap,alignment};
}
u16 NARC_GetFileCount(NARC *);
typedef struct HGNativeG3dAnimation { fx32 frame;DSJointAnimation joint;DSTextureAnimation texture;NNSG3dRenderObj *bound;const u8 *original_frame;DSAnimationResource resource;struct HGNativeG3dAnimation *next;unsigned slot; } NNSG3dAnmObj;
typedef struct NNSG3dResTex {DSTextureBank bank;} NNSG3dResTex;
#include "prop_animation_types.inc"
#define ANIM_ARCHIVE_ID_NONE -1
static void GF_AssertFail(void) { abort(); }
static fx32 NNS_G3dAnmObjGetNumFrame(const NNSG3dAnmObj *a) { return (fx32)(a->resource.frames?a->resource.frames:a->joint.frames?a->joint.frames:a->texture.frames)*FX32_ONE; }
static void NNS_G3dRenderObjAddAnmObj(NNSG3dRenderObj *r,NNSG3dAnmObj *a) { GF_ASSERT(r&&!r->anmJnt);r->anmJnt=a;a->bound=r; }
static void NNS_G3dRenderObjRemoveAnmObj(NNSG3dRenderObj *r,NNSG3dAnmObj *a) { GF_ASSERT(r&&r->anmJnt==a);r->anmJnt=NULL;a->bound=NULL; }
void NARC_ReadWholeMember(NARC *,u32,void *);
void *NARC_AllocAndReadWholeMember(NARC *,u32,enum HeapID);
static void ov01_02204500(FieldSystemUnkSubCC_Sub0 *pool,MapPropAnimation *a) { (void)pool;if(a->animObj&&a->animObj->bound)NNS_G3dRenderObjRemoveAnmObj(a->animObj->bound,a->animObj);Heap_Free(a->animObj); }
int hg_prop_model_uv_compatible(const NNSG3dResMdl *,const DSTextureAnimation *);
int hg_prop_model_animation_compatible(const NNSG3dResMdl *,const DSAnimationResource *);
static int decode_original_animation(const u8 *p,size_t n,NNSG3dResMdl *model,NNSG3dAnmObj *decoded){
    memset(decoded,0,sizeof(*decoded));
    return (ds_joint_animation_open(p,n,&decoded->joint)||ds_texture_animation_open(p,n,&decoded->texture))&&
        (!decoded->texture.kind||hg_prop_model_uv_compatible(model,&decoded->texture));
}
static void ov01_02204470(FieldSystemUnkSubCC_Sub0 *pool,MapPropAnimation *a,void *res,NNSG3dResMdl *model,NNSG3dResTex *texture) {
    (void)pool;(void)model;(void)texture;const u8 *p=res;size_t n=(u32)p[8]|(u32)p[9]<<8|(u32)p[10]<<16|(u32)p[11]<<24;
    a->res=NULL;a->animObj=NULL;
    NNSG3dAnmObj decoded={0};
    if(!decode_original_animation(p,n,model,&decoded)) {
        if(animation_decode_trap) {
            animation_decode_trap->error=!memcmp(p,"BTA0",4)?"NNS_G3dAnmObjInit: NSBTA/SRT0 UV tracks":"NNS_G3dAnmObjInit: unsupported animation resource";
            Heap_Free(res);longjmp(animation_decode_trap->jump,1);
        }
        GF_ASSERT(!"unsupported original animation resource");
    }
    a->animObj=Heap_Alloc(pool->unk0.heap,sizeof(*a->animObj));*a->animObj=decoded;a->res=res;
}
static void ov01_022046A4(NNSFndAllocator *a,NNSG3dAnmObj **out,void *res){(void)a;(void)out;(void)res;GF_ASSERT(!"custom land animation resource not implemented");}
#include "prop_animation_kernel.inc"
typedef struct { u8 tag;unsigned model,selected;double world[3];NNSG3dRenderObj render; } PropTrack;
struct HGPropAnimations {
    unsigned map,waiting,list_archive,model_archive;MapPropAnimationManager manager;MapPropOneShotAnimationManager shots;
    FieldSystemUnkSubC8 scheduler;FieldSystemUnkSubCC_Sub0 pool;
    MapPropAnimation storage[16];MapPropAnimation *free_slots[16];PropTrack tracks[16];
    PropTrack prepared;
};
HGPropAnimations *hg_prop_animations_new(unsigned map) {
    const u8 *p;size_t n;if(map>=MAP_ID_MAX||!native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(map),&p,&n)||n!=8||p[6]>1)return NULL;
    HGPropAnimations *s=calloc(1,sizeof(*s));if(!s)return NULL;s->map=map;
    s->manager.animNARC=NARC_New(NARC_a_1_0_6,HEAP_ID_FIELD1);
    s->model_archive=p[6]?NARC_a_0_4_0:NARC_a_1_4_8;
    s->list_archive=p[6]?NARC_a_1_0_7:NARC_a_1_0_8;s->manager.animListNARC=NARC_New((NarcId)s->list_archive,HEAP_ID_FIELD1);
    s->manager.unk138=&s->pool;s->pool.unk10=s->storage;s->pool.unk14=s->free_slots;s->pool.unk20=16;
    for(unsigned i=0;i<16;i++)s->free_slots[i]=s->storage+i;
    HeapExp_FndInitAllocator(&s->pool.unk0,HEAP_ID_FIELD1,4);s->scheduler.unk4=&s->pool;s->scheduler.unkC=1;return s;
}
static PropTrack *track(HGPropAnimations *s,unsigned tag) { for(unsigned i=0;i<16;i++)if(s->tracks[i].tag==tag)return s->tracks+i;return NULL; }
static void load_track(HGPropAnimations *s,PropTrack *t) {
    unsigned count=MapPropAnimationManager_GetPropAnimationCount(&s->manager,(int)t->model);
    MapPropOneShotAnimationManager_LoadPropAnimations(&s->manager,&s->shots,t->tag,(int)t->model,&t->render,NULL,NULL,(int)count,1,FALSE);
}
HGPropAnimations *hg_prop_animations_clone(const HGPropAnimations *source) {
    if(!source)return NULL;HGPropAnimations *s=hg_prop_animations_new(source->map);if(!s)return NULL;
    for(unsigned i=0;i<16;i++)if(source->tracks[i].tag) {
        s->tracks[i]=source->tracks[i];memset(&s->tracks[i].render,0,sizeof(s->tracks[i].render));load_track(s,s->tracks+i);
        const MapPropOneShotAnimation *old=MapPropOneShotAnimationManager_GetAnimation((MapPropOneShotAnimationManager *)&source->shots,source->tracks[i].tag);
        MapPropOneShotAnimation *copy=MapPropOneShotAnimationManager_GetAnimation(&s->shots,s->tracks[i].tag);
        if(old->currentAnimation)MapPropOneShotAnimation_SwitchAnimation(copy,s->tracks[i].selected);
        for(int j=0;j<old->animations.count;j++) {
            MapPropAnimation *to=copy->animations.list[j]->animation,*from=old->animations.list[j]->animation;
            to->animObj->frame=from->animObj->frame;to->paused=from->paused;to->looping=from->looping;to->unk18=from->unk18;
        }
    }
    s->waiting=source->waiting;s->prepared=source->prepared;return s;
}
void hg_prop_animations_free(HGPropAnimations *s) {
    if(!s)return;for(unsigned i=0;i<16;i++)if(s->tracks[i].tag)MapPropOneShotAnimationManager_UnloadAnimation(&s->manager,&s->shots,s->tracks[i].tag);
    NARC_Delete(s->manager.animNARC);NARC_Delete(s->manager.animListNARC);free(s);
}
int hg_prop_animations_busy(const HGPropAnimations *s) { return s&&s->waiting; }
int hg_prop_animations_tick(HGPropAnimations *s) {
    if(!s)return 1;ov01_02204350(&s->scheduler);
    if(s->waiting&&MapPropOneShotAnimationManager_IsAnimationLoopFinished(&s->shots,(int)s->waiting))s->waiting=0;
    return 1;
}
static unsigned half(const u8 *p) { return p[0]|(unsigned)p[1]<<8; }
int hg_prop_animations_validate(HGPropAnimations *s,HGFieldResources *resources,const ScriptContext *ctx,unsigned op) {
    if(!s||!ctx||op<307||op>311)return 0;const u8 *p=ctx->script_ptr;
    if(op==307) {
        unsigned tag=p[8];if(!tag||track(s,tag)||s->pool.unk22>12)return 0;
        unsigned x=half(p)*32+FieldSystem_VarGet(ctx->fieldSystem,(u16)half(p+4)),z=half(p+2)*32+FieldSystem_VarGet(ctx->fieldSystem,(u16)half(p+6));
        if(!x||x>32767||z>32767)return 0;HGFieldTile tile;if(!hg_field_resources_tile(resources,(int)x,(int)z,&tile))return 0;
        PropTrack chosen={0};unsigned found=0;
        for(size_t off=0;off+48<=tile.land.props_size;off+=48) {
            const u8 *b=tile.land.props+off;unsigned model=half(b)|(unsigned)half(b+2)<<16;const u8 *list;size_t n;
            if(!native_resource_member((int)s->list_archive,0,(int)model,&list,&n)||n!=24||list[0]!=1||(list[1]&1)!=1)continue;
            int32_t xyz[3];for(unsigned k=0;k<3;k++){uint32_t v=(uint32_t)half(b+4+4*k)|(uint32_t)half(b+6+4*k)<<16;memcpy(xyz+k,&v,4);}
            double world[]={tile.matrix_x*512.0+256+xyz[0]/4096.0,tile.altitude*8.0+xyz[1]/4096.0,tile.matrix_z*512.0+256+xyz[2]/4096.0};
            if(world[0]<(x-1)*16.0||world[0]>(x+2)*16.0||world[2]<z*16.0||world[2]>(z+1)*16.0)continue;
            const u8 *model_bytes;size_t model_size;DSModel mesh;
            if(!native_resource_member((int)s->model_archive,0,(int)model,&model_bytes,&model_size)||!ds_model_open(model_bytes,model_size,0,&mesh))return 0;
            unsigned count=0;
            for(unsigned j=0;j<4;j++) {
                uint32_t id=(uint32_t)half(list+8+4*j)|(uint32_t)half(list+10+4*j)<<16;
                if(id==UINT32_MAX)break;
                const u8 *anim;size_t size;DSJointAnimation joint;
                if(!native_resource_member(NARC_a_1_0_6,0,(int)id,&anim,&size)||!ds_joint_animation_open(anim,size,&joint))return 0;
                for(unsigned frame=0;frame<joint.frames;frame++) {
                    DSJointPose pose;DSModelStats stats;
                    if(!ds_joint_animation_pose(&joint,frame,&pose)||!ds_model_scene_pose(&mesh,-1,-1,&pose,NULL,NULL,&stats))return 0;
                }
                count++;
            }
            if(count<2||found++)return 0;
            chosen=(PropTrack){.tag=(u8)tag,.model=model};memcpy(chosen.world,world,sizeof(world));
        }
        if(!found)return 0;s->prepared=chosen;return 1;
    }
    PropTrack *t=track(s,p[0]);if(!t)return 0;
    const MapPropOneShotAnimation *shot=MapPropOneShotAnimationManager_GetAnimation(&s->shots,p[0]);
    if(op==308&&!shot->currentAnimation)return 0;
    if(op==309&&s->waiting)return 0;
    return !s->waiting;
}
static HGPropAnimations *session(FieldSystem *f) { GF_ASSERT(f->native_prop_animations);return f->native_prop_animations; }
void ov01_021E9AE8(FieldSystem *f,int x,int z,u8 tag) {
    (void)x;(void)z;HGPropAnimations *s=session(f);GF_ASSERT(s->prepared.tag==tag);
    for(unsigned i=0;i<16;i++)if(!s->tracks[i].tag){s->tracks[i]=s->prepared;load_track(s,s->tracks+i);return;}abort();
}
static void play(FieldSystem *f,u8 tag,unsigned index) { HGPropAnimations *s=session(f);PropTrack *t=track(s,tag);GF_ASSERT(t);t->selected=index;MapPropOneShotAnimationManager_PlayAnimation(&s->shots,tag,(int)index); }
void ov01_021E9BB8(FieldSystem *f,u8 tag) { play(f,tag,0); }
void ov01_021E9BDC(FieldSystem *f,u8 tag) { play(f,tag,1); }
void ov01_021E9C00(FieldSystem *f,u8 tag) { session(f)->waiting=tag; }
void ov01_021E9C20(FieldSystem *f,u8 tag) { HGPropAnimations *s=session(f);MapPropOneShotAnimationManager_UnloadAnimation(&s->manager,&s->shots,tag);memset(track(s,tag),0,sizeof(PropTrack)); }
unsigned hg_prop_animations_poses(const HGPropAnimations *s,HGPropPose out[16]) {
    unsigned count=0;if(!s)return 0;
    for(unsigned i=0;i<16;i++)if(s->tracks[i].tag) {
        const MapPropOneShotAnimation *shot=MapPropOneShotAnimationManager_GetAnimation((MapPropOneShotAnimationManager *)&s->shots,s->tracks[i].tag);
        if(!shot->currentAnimation)continue;NNSG3dAnmObj *obj=shot->currentAnimation->animation->animObj;
        memset(out+count,0,sizeof(*out));out[count].model=s->tracks[i].model;memcpy(out[count].world,s->tracks[i].world,sizeof(out[count].world));
        unsigned frame=(unsigned)obj->frame/FX32_ONE;
        if(obj->joint.frames)GF_ASSERT(ds_joint_animation_pose(&obj->joint,frame,&out[count].pose));
        else {out[count].texture=obj->texture;out[count].texture_frame=frame;}
        count++;
    }
    return count;
}

int hg_prop_animation_load(HGPropAnimations *s,unsigned tag,unsigned model,const double world[3]) {
    if(!s||!tag||tag>255||track(s,tag)||s->pool.unk22>12||!world)return 0;
    const u8 *list,*bytes;size_t n,size;DSModel mesh;
    if(!native_resource_member(s->list_archive,0,model,&list,&n)||n!=24||list[0]!=1||
       !native_resource_member(s->model_archive,0,model,&bytes,&size)||!ds_model_open(bytes,size,0,&mesh))return 0;
    unsigned count=0;
    for(unsigned i=0;i<4;i++) {
        uint32_t id=(uint32_t)half(list+8+4*i)|(uint32_t)half(list+10+4*i)<<16;
        if(id==UINT32_MAX)break;
        DSJointAnimation joint={0};DSTextureAnimation texture={0};if(!native_resource_member(NARC_a_1_0_6,0,id,&bytes,&size)||(!ds_joint_animation_open(bytes,size,&joint)&&!ds_texture_animation_open(bytes,size,&texture)))return 0;
        for(unsigned f=0;f<joint.frames;f++){DSJointPose pose;DSModelStats stats;if(!ds_joint_animation_pose(&joint,f,&pose)||!ds_model_scene_pose(&mesh,-1,-1,&pose,NULL,NULL,&stats))return 0;}
        if(texture.frames) {
            const u8 *model_bytes;size_t model_size;DSTextureBank bank;unsigned matched=0;
            if(!native_resource_member(s->model_archive,0,model,&model_bytes,&model_size)||!ds_texture_bank(model_bytes,model_size,&bank))return 0;
            for(unsigned m=0;m<mesh.materials;m++) {
                DSModelMaterial material;if(!ds_model_material(&mesh,m,&material))return 0;int animated=0;
                for(unsigned f=0;f<texture.frames;f++) {
                    char tex[17],pal[17];int result=ds_texture_animation_frame(&texture,f,material.name,tex,pal);DSTexture sample;
                    if(result<0||(result&&(!ds_texture_find(&bank,tex,pal,&sample)||sample.width!=material.width||sample.height!=material.height)))return 0;
                    animated|=result;
                }
                matched+=animated;
            }
            if(matched!=texture.materials)return 0;
        }
        count++;
    }
    if(!count||count>16-s->pool.unk22)return 0;
    for(unsigned i=0;i<16;i++)if(!s->tracks[i].tag){PropTrack *t=s->tracks+i;*t=(PropTrack){.tag=(u8)tag,.model=model};memcpy(t->world,world,sizeof(t->world));load_track(s,t);return 1;}
    return 0;
}
int hg_prop_animation_play(HGPropAnimations *s,unsigned tag,unsigned index) {
    PropTrack *t=s?track(s,tag):NULL;if(!t)return 0;
    MapPropOneShotAnimation *shot=MapPropOneShotAnimationManager_GetAnimation(&s->shots,tag);
    if(index>=(unsigned)shot->animations.count)return 0;t->selected=index;MapPropOneShotAnimationManager_PlayAnimation(&s->shots,tag,index);return 1;
}
int hg_prop_animation_finished(HGPropAnimations *s,unsigned tag) { return s&&track(s,tag)?MapPropOneShotAnimationManager_IsAnimationLoopFinished(&s->shots,tag):-1; }
int hg_prop_animation_unload(HGPropAnimations *s,unsigned tag) {
    PropTrack *t=s?track(s,tag):NULL;if(!t)return 0;MapPropOneShotAnimationManager_UnloadAnimation(&s->manager,&s->shots,tag);memset(t,0,sizeof(*t));return 1;
}

int hg_prop_animation_door_begin(HGPropAnimations *s,HGFieldResources *resources,int x,int z,unsigned tag,int exiting,unsigned *sound) {
    if(!s||!resources||!sound||x<1||z<1||x>32765||z>32766)return -1;
    PropTrack selected={0};unsigned found=0;unsigned cells[8],cell_count=0;const u8 *selected_list=NULL;
    /* ov01_021E90E4 calls sub_02054A60(x,z,-1,-1,3,1): inclusive
     * matrix-global rectangle [x-1,x+2] by [z-1,z]. Exit task
     * ov01_021E9374 uses [z,z+1]. Deduplicate chunks in either case. */
    for(int tz=z-1+!!exiting;tz<=z+!!exiting;tz++)for(int tx=x-1;tx<=x+2;tx++) {
        HGFieldTile tile;if(!hg_field_resources_tile(resources,tx,tz,&tile))continue;
        unsigned cell=tile.matrix_z*256+tile.matrix_x,i=0;for(;i<cell_count;i++)if(cells[i]==cell)break;
        if(i<cell_count)continue;cells[cell_count++]=cell;
        for(size_t off=0;off+48<=tile.land.props_size;off+=48) {
            const u8 *b=tile.land.props+off;unsigned model=half(b)|(unsigned)half(b+2)<<16;const u8 *list;size_t n;
            if(!native_resource_member(s->list_archive,0,model,&list,&n)||n!=24||!list[4])continue;
            int32_t xyz[3];for(unsigned k=0;k<3;k++){uint32_t v=(uint32_t)half(b+4+4*k)|(uint32_t)half(b+6+4*k)<<16;memcpy(xyz+k,&v,4);}
            double world[]={tile.matrix_x*512.0+256+xyz[0]/4096.0,tile.altitude*8.0+xyz[1]/4096.0,tile.matrix_z*512.0+256+xyz[2]/4096.0};
            if(world[0]<(x-1)*16.0||world[0]>(x+2)*16.0||world[2]<(z-1+!!exiting)*16.0||world[2]>(z+!!exiting)*16.0)continue;
            if(found++)return -1;selected=(PropTrack){.model=model};memcpy(selected.world,world,sizeof(world));selected_list=list;
        }
    }
    if(!found)return 0;
    if(!hg_motion_door_sound(selected_list,1,sound)||!hg_prop_animation_load(s,tag,selected.model,selected.world)){fprintf(stderr,"Unsupported door animation model %u\n",selected.model);return -1;}
    if(!hg_prop_animation_play(s,tag,0)){hg_prop_animation_unload(s,tag);return -1;}
    return 1;
}

int hg_prop_animation_open_door(HGPropAnimations *s,HGFieldResources *resources,int x,int z,unsigned tag,unsigned *sound) {
    return hg_prop_animation_door_begin(s,resources,x,z,tag,0,sound);
}
int hg_prop_animation_close_door(HGPropAnimations *s,unsigned tag,unsigned *sound) {
    PropTrack *t=s?track(s,tag):NULL;const u8 *list;size_t n;
    return t&&sound&&native_resource_member(s->list_archive,0,t->model,&list,&n)&&n==24&&
        hg_motion_door_sound(list,0,sound)&&hg_prop_animation_play(s,tag,1);
}

/* Original native owner graph, separate from the viewer's script session. */
#include "compat/original_prop_animation.h"
struct HGOriginalPropAnimations {FieldSystemUnkSubC8 *pools;MapPropAnimationManager *manager;FieldSystemUnkSubCC *land;const char *error;unsigned borrowed;};
HGOriginalPropAnimations *hg_original_prop_animations_new(NARC *list) {
    HGOriginalPropAnimations *o=Heap_Alloc(HEAP_ID_FIELD1,sizeof(*o));
    o->error=NULL;o->borrowed=0;o->pools=ov01_022041C4(HEAP_ID_FIELD1);
    o->manager=MapPropAnimationManager_Init(list,o->pools);o->land=ov01_0220460C(o->pools);return o;
}
unsigned hg_original_prop_animations_count(HGOriginalPropAnimations *o){return MapPropAnimationManager_GetAnimListNARCFileCount(o->manager);}
void hg_original_prop_animations_tick(HGOriginalPropAnimations *o){GF_ASSERT(!o->error);ov01_02204350(o->pools);}
void hg_original_prop_animations_free(HGOriginalPropAnimations *o) {
    if(!o)return;if(!o->borrowed){ov01_02204634(o->land);ov01_02204278(o->pools);MapPropAnimationManager_Free(o->manager);}Heap_Free(o);
}

int hg_original_prop_animations_preload(HGOriginalPropAnimations *o,unsigned id,NNSG3dRenderObj *render,const u8 *bytes,const DSTextureBank *bank) {
    MapPropAnimListFile list;memcpy(&list,bytes,sizeof(list));
    if(list.hasAnimations&&list.flags==8){o->error="original time-of-day visual owner";return 0;} /* Real time-visual owner still required. */
    NNSG3dResTex texture={*bank};struct AnimationDecodeTrap trap;struct AnimationDecodeTrap *previous=animation_decode_trap;
    animation_decode_trap=&trap;o->error=NULL;
    if(setjmp(trap.jump)) {animation_decode_trap=previous;o->error=trap.error;return 0;}
    ov01_021E8F3C(id,render->model,render,&texture,&list,o->manager,NULL);
    animation_decode_trap=previous;return 1;
}
unsigned hg_original_prop_animations_active(HGOriginalPropAnimations *o) {return o->manager->unk138->unk22;}

const char *hg_original_prop_animations_error(HGOriginalPropAnimations *o){return o->error;}

int hg_prop_render_animation_matrix(const NNSG3dRenderObj *render,const DSModelMaterial *material,double matrix[6]) {
    NNSG3dAnmObj *a=render->anmMat?render->anmMat:render->anmJnt;
    while(a&&a->resource.kind&&a->resource.kind!=DS_ANIMATION_SRT&&a->resource.kind!=DS_ANIMATION_PATTERN)a=a->next;
    if(!a)return 0;
    if(a->resource.kind&&!a->texture.bytes){
        NNSG3dAnmObj decoded;if(!decode_original_animation(a->resource.bytes,a->resource.size,render->model,&decoded))return -1;
        a->joint=decoded.joint;a->texture=decoded.texture;
    }fx32 frame=a->frame;
    if(a->original_frame){u32 bits=(u32)a->original_frame[0]|(u32)a->original_frame[1]<<8|(u32)a->original_frame[2]<<16|(u32)a->original_frame[3]<<24;memcpy(&frame,&bits,4);}
    return ds_texture_animation_matrix(&a->texture,frame/(double)FX32_ONE,material->name,material->width,material->height,matrix);
}

unsigned hg_original_prop_land_animation_count(HGOriginalPropAnimations *o){return ov01_02204698(o->land);}

HGOriginalPropAnimations *hg_original_prop_animations_attach(void *pools,void *manager,void *land){
    GF_ASSERT(pools&&manager&&land);HGOriginalPropAnimations *o=Heap_Alloc(HEAP_ID_FIELD1,sizeof(*o));
    *o=(HGOriginalPropAnimations){pools,manager,land,NULL,1};return o;
}
void hg_original_prop_animations_release(HGOriginalPropAnimations *o){
    GF_ASSERT(o&&o->borrowed&&o->pools&&o->manager&&o->land);
    ov01_02204634(o->land);ov01_02204278(o->pools);MapPropAnimationManager_Free(o->manager);
    o->pools=NULL;o->manager=NULL;o->land=NULL;
}

void *hg_native_field_animation_new(const u8 *bytes,size_t size,NNSG3dResMdl *model,unsigned heap){
    DSAnimationResource resource;if(!ds_animation_resource_open(bytes,size,0,&resource)||!hg_prop_model_animation_compatible(model,&resource))return NULL;
    NNSG3dAnmObj *a=Heap_Alloc((enum HeapID)heap,sizeof(*a));memset(a,0,sizeof(*a));a->resource=resource;return a;
}
int hg_native_field_animation_bind(NNSG3dRenderObj *object,void *animation){
    NNSG3dAnmObj *a=animation;if(!object||!object->model||!a||a->bound||a->next)return 0;
    a->slot=a->resource.kind==DS_ANIMATION_JOINT?1:2;
    NNSG3dAnmObj **head=(NNSG3dAnmObj **)(a->slot==1?&object->anmJnt:&object->anmMat);
    while(*head)head=&(*head)->next;*head=a;a->bound=object;return 1;
}
void hg_native_field_animation_detach(void *animation){
    NNSG3dAnmObj *a=animation;if(!a||!a->bound)return;
    if(!a->slot){NNS_G3dRenderObjRemoveAnmObj(a->bound,a);return;}
    NNSG3dAnmObj **head=(NNSG3dAnmObj **)(a->slot==1?&a->bound->anmJnt:&a->bound->anmMat);
    while(*head&&*head!=a)head=&(*head)->next;GF_ASSERT(*head==a);*head=a->next;a->next=NULL;a->bound=NULL;
}
int hg_native_field_animation_is_bound(void *animation,NNSG3dRenderObj *object){return animation&&((NNSG3dAnmObj *)animation)->bound==object;}
void hg_native_field_animation_detach_object(NNSG3dRenderObj *object){while(object->anmJnt)hg_native_field_animation_detach(object->anmJnt);while(object->anmMat)hg_native_field_animation_detach(object->anmMat);}
void hg_native_field_animation_free(void *animation){if(!animation)return;hg_native_field_animation_detach(animation);Heap_Free(animation);}
int hg_native_field_animation_frame(void *animation,s32 frame){NNSG3dAnmObj *a=animation;if(!a||frame<0||frame>=NNS_G3dAnmObjGetNumFrame(a))return 0;if(a->original_frame){u32 bits=(u32)a->original_frame[0]|(u32)a->original_frame[1]<<8|(u32)a->original_frame[2]<<16|(u32)a->original_frame[3]<<24;fx32 observed;memcpy(&observed,&bits,4);return observed==frame;}a->frame=frame;return 1;}

void hg_native_field_animation_observe_frame(void *animation,const u8 *word){NNSG3dAnmObj *a=animation;GF_ASSERT(a&&word&&!a->original_frame);a->original_frame=word;}
