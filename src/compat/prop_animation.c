#include "compat/prop_animation.h"
#include "compat/resources.h"
#include "field_data.h"
#include "map_header.h"
#include "ds_model.h"
#include "constants/maps.h"
u16 FieldSystem_VarGet(FieldSystem *,u16);
/* Typed native arena and graphics objects: original playback uses native
 * pointers; these are never exposed to guest ARM or serialized as save data. */
typedef struct { unsigned unused; } NNSFndAllocator;
typedef struct { fx32 frame;DSJointAnimation joint; } NNSG3dAnmObj;
typedef struct NNSG3dResTex NNSG3dResTex;
#include "prop_animation_types.inc"
#define ANIM_ARCHIVE_ID_NONE -1
static void GF_AssertFail(void) { abort(); }
static fx32 NNS_G3dAnmObjGetNumFrame(const NNSG3dAnmObj *a) { return (fx32)a->joint.frames*FX32_ONE; }
static void NNS_G3dRenderObjAddAnmObj(NNSG3dRenderObj *r,NNSG3dAnmObj *a) { GF_ASSERT(r&&!r->anmJnt);r->anmJnt=a; }
static void NNS_G3dRenderObjRemoveAnmObj(NNSG3dRenderObj *r,NNSG3dAnmObj *a) { GF_ASSERT(r&&r->anmJnt==a);r->anmJnt=NULL; }
void NARC_ReadWholeMember(NARC *,u32,void *);
void *NARC_AllocAndReadWholeMember(NARC *,u32,enum HeapID);
static MapPropAnimation *ov01_022042FC(FieldSystemUnkSubCC_Sub0 *pool) {
    if(pool->unk22>=pool->unk20)return NULL;
    return pool->unk14[pool->unk22++];
}
static void ov01_0220431C(FieldSystemUnkSubCC_Sub0 *pool,MapPropAnimation *a) {
    GF_ASSERT(pool->unk22);free(a->res);a->res=NULL;a->animObj=NULL;a->unk14=0;pool->unk14[--pool->unk22]=a;
}
static void ov01_02204500(FieldSystemUnkSubCC_Sub0 *pool,MapPropAnimation *a) { (void)pool;free(a->animObj); }
static void ov01_02204470(FieldSystemUnkSubCC_Sub0 *pool,MapPropAnimation *a,void *res,NNSG3dResMdl *model,NNSG3dResTex *texture) {
    (void)pool;(void)model;(void)texture;const u8 *p=res;size_t n=(u32)p[8]|(u32)p[9]<<8|(u32)p[10]<<16|(u32)p[11]<<24;
    a->animObj=calloc(1,sizeof(*a->animObj));GF_ASSERT(a->animObj&&ds_joint_animation_open(p,n,&a->animObj->joint));a->res=res;
}
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
    s->scheduler.unk4=&s->pool;s->scheduler.unkC=1;return s;
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
        out[count].model=s->tracks[i].model;memcpy(out[count].world,s->tracks[i].world,sizeof(out[count].world));
        GF_ASSERT(ds_joint_animation_pose(&obj->joint,(unsigned)obj->frame/FX32_ONE,&out[count].pose));count++;
    }
    return count;
}
