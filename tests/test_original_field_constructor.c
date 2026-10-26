/* Original native field construction on the existing complete save owner.
 * Field-map lifecycle and production startup remain separate acceptance gates. */
#include "original_field_constructor_names.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
static double profile_previous;
static void profile_phase(const char *name){
    if(!getenv("HG_PROFILE_RESOURCES"))return;
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);double now=t.tv_sec+t.tv_nsec*1e-9;
    if(profile_previous)fprintf(stderr,"HG_TIMING {\"phase\":\"%s\",\"seconds\":%.9f}\n",name,now-profile_previous);
    profile_previous=now;
}

#include "original_field_scripts_names.h"
#include "original_field_entry_names.h"
#include "original_field_terrain_names.h"
#include "original_field_transition_names.h"
#include "original_field_objects_names.h"
#include "original_field_follow_names.h"
#include "save_follow_mon.h"
#include "gymmick.h"
#include "constants/init_script_types.h"
#define POKEHEARTGOLD_SAV_SYSTEM_INFO_H
static u32 transition_spawn(u32);
#define MapHeader_GetSpawnIdForDeathWarp transition_spawn
#include "sys_flags.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "original_field_owner.h"
#include "compat/original_loader.h"
#include "ds_audio.h"
#include "compat/resources.h"
#include "field_data.h"
#include "task.h"
#include "original_field_modules.h"
#include "compat/native_heap.h"
#include "compat/scheduler.h"
#include "compat/field.h"
#undef POKEHEARTGOLD_FIELD_TYPES_DEF_H
#include "field_types_def.h"
#include "pokemon_types_def.h"
#include "map_events_internal.h"
#include "map_matrix.h"
#include "bag_cursor.h"
#include "gear_phone.h"
#include "save_pokegear.h"
#include "player_data.h"
#include "main.h"
#include "unk_02092BB8.h"
#include "constants/sndseq.h"
#include "sys_task_api.h"
#include "compat/save_manager.h"
#include "save_vars_flags.h"
#include "sys_vars.h"
#include "map_header.h"
#include "constants/std_script.h"
#include "constants/maps.h"
#include "files/fielddata/script/scr_seq.naix"
#include "files/msgdata/msg.naix"
typedef struct LocalFieldData LocalFieldData;
#include "constants/scrcmd.h"
#include "constants/global_fieldmap.h"
#include "constants/player_avatar.h"
#include "constants/sprites.h"
#include "player_avatar.h"
#include "map_object.h"
#include "follow_mon.h"
#include "party.h"

#include "constants/follow_mon_idx.h"
#include "constants/species.h"
#include "constants/pokemon.h"
#include "metatile_behavior.h"
#include "original_field_constructor_types.inc"
#include <setjmp.h>
#include <math.h>

#define MI_PROCESSOR_ARM9 HG_PROCESSOR_ARM9
static jmp_buf rejected,parent_rejected;
static int parent_probe;
static const char *open_contract;
static _Noreturn void unsupported(const char *name) {
    open_contract=name;if(parent_probe)longjmp(parent_rejected,1);longjmp(rejected,1);
}
static BOOL IsSEPlaying(unsigned sequence) { (void)sequence;unsupported("original phone task: sound-handle provider unavailable"); }
static void PlaySE_SetPitch(unsigned sequence,int pitch) { (void)sequence;(void)pitch;unsupported("original phone task: sound-handle provider unavailable"); }
#include "original_field_constructor.inc"
#define POKEHEARTGOLD_SCRIPT_H
#define SCRIPT_MODE_STOPPED 0
#define SCRIPT_MODE_BYTECODE 1
#define SCRIPT_MODE_NATIVE 2
#define ScriptReadByte(ctx) *(ctx->script_ptr++)
#include "script_types.inc"
static BOOL unsupported_original_command(ScriptContext *ctx) {
    (void)ctx;unsupported("original map-load script: command not implemented on the canonical field owner");
}
#include "original_field_scripts.inc"
#include HG_ORIGINAL_SCRIPT_VM
#define ENCDATA_NARC NARC_fielddata_encountdata_g_enc_data
#define MI_CpuClearFast MI_CpuClear8
_Static_assert(sizeof(BgEvent)==20&&sizeof(ObjectEvent)==32&&sizeof(WarpEvent)==12&&sizeof(CoordEvent)==16,
    "event archive records must preserve their original pointer-free ABI");
#include "original_field_entry.inc"
#define sub_0205489C hg_original_terrain_install
#include "original_field_terrain.inc"
#undef sub_0205489C
/* Call original transition code before rejecting an unprovided machine ABI. */
static void sub_02067AE4(FieldSystem *);
static void sub_02067A88(FieldSystem *);
#include "compat/field_time_owner.h"
static void FieldSystem_StartBugContestTimer(FieldSystem *field){
    static char error[256];
    if(!hg_field_time_update_live(field->saveData,field->phoneRingManager,field->bugContest,error,sizeof(error)))unsupported(error);
}
static void StartMapSceneScript(FieldSystem *field,u16 script,void *object){(void)field;(void)script;(void)object;unsupported("canonical asynchronous map-scene task");}
static u16 GetMapSceneScriptId(FieldSystem *field,u8 *header,u8 type){(void)field;(void)header;(void)type;unsupported("canonical map-scene variable owner");}
typedef struct { u32 hour,minute,second; } RTCTime;
void GF_RTC_CopyDateTime(RTCDate *,RTCTime *);
static void transition_copy_date(RTCDate *date){RTCTime time;GF_RTC_CopyDateTime(date,&time);}
#define GF_RTC_CopyDate transition_copy_date
void GF_RTC_CopyDateTime(RTCDate *,RTCTime *);
Gymmick *Save_GetGymmickPtr(SaveData *);
void Save_Gymmick_Clear(Gymmick *);
GymmickType Save_Gymmick_GetType(Gymmick *);
#include "field_system_rtc_weather.h"
#include "roamer.h"
RoamerSaveData *Save_Roamers_Get(SaveData *);
u16 LCRandom(void);
#include "original_field_transition.inc"
#include "compat/original_transition_bridge.inc"
#undef GF_RTC_CopyDate
/* Explicit unimplemented owner boundaries; never successful substitutes. */
static void MapObject_ClearHeldMovement(LocalMapObject *);
static UnkLMOCallbackStruct *canonical_movement_callbacks(u32 movement);
static void canonical_graphics_mode(LocalMapObject *,BOOL);
static void sub_0205FD30(LocalMapObject *object);
static HGOriginalLoader *callback_loader;
static unsigned autonomous_calls,movement_calls,follower_calls,follower_teardowns;
_Static_assert(sizeof(((LocalMapObject *)0)->unkD8)==16,"original movement work extent");
_Static_assert(offsetof(FollowMon,unk1C)-offsetof(FollowMon,unk4)==24&&
    offsetof(FollowMon,form)-offsetof(FollowMon,unk4)==20&&
    offsetof(FollowMon,gender)-offsetof(FollowMon,unk4)==16,
    "DS follower scalar tail must stay contiguous without native pointers");

static void ov01_021F92A0(LocalMapObject *object){(void)object;unsupported("ov01_021F92A0: canonical object render update");}
static void canonical_object_reset(LocalMapObject *object);
static void canonical_object_arm(LocalMapObject *,u32);
static u16 canonical_graphics_var(FieldSystem *field,u16 index){(void)field;(void)index;unsupported("canonical variable sprite lookup");}
static void canonical_movement_change(LocalMapObject *object,u32 movement){(void)object;(void)movement;unsupported("sub_0205FC94: canonical movement replacement");}

#define sub_0205FB00 canonical_movement_callbacks
#define ov01_021FA2B8 canonical_graphics_mode

#define sub_0205FD20 canonical_object_reset
#define FieldSystem_VarGetObjectEventGraphicsId canonical_graphics_var
#define sub_0205FC94 canonical_movement_change

static void ov01_022008B4(PlayerAvatar *avatar);
static u32 ov01_021FE7DC(LocalMapObject *object,u32 x,u32 z,u32 direction,u32 kind){(void)object;(void)x;(void)z;(void)direction;(void)kind;unsupported("canonical surfing effect ABI");}
static ObjectEventGraphicsInfo *ObjectEvent_GetGraphicsInfo(u32 sprite);
static void sub_02061070(LocalMapObject *object){canonical_object_arm(object,HG_TRANSITION_sub_02061070);}
static void sub_020611DC(LocalMapObject *object){canonical_object_arm(object,HG_TRANSITION_sub_020611DC);}
#include "original_loader_callbacks.inc"
#include "original_field_objects.inc"
#undef sub_0205FB00
#undef sub_0205EF8C
#undef sub_0205ED18
#undef ov01_021FA2B8

#undef sub_0205FD20
#undef FieldSystem_VarGetObjectEventGraphicsId
#include "original_field_follow.inc"
#undef sub_0205FC94

static UnkLMOCallbackStruct *canonical_movement_callbacks(u32 movement){
    if(movement==1)return &canonical_player_movement;
    if(movement==48)return &canonical_follower_movement;
    unsupported("sub_0205FB00: movement descriptor not admitted on canonical objects");
}

static int held_import(DSThumb *cpu,void *opaque){
    LocalMapObject *object=opaque;
    const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_transition_imports);i++)
        if(cpu->r[15]==hg_transition_imports[i].address)name=hg_transition_imports[i].name;
    if(!name)return -1;
    FieldSystem *field=MapObject_GetFieldSystem(object);
    if(!strcmp(name,"sub_02054940")){
        if(cpu->r[0]!=0x03004000u||!callback_loader)return -1;
        const u8 *stack=ds_translate(&cpu->memory,cpu->r[13],4);if(!stack)return -1;
        u32 out=stack[0]|(u32)stack[1]<<8|(u32)stack[2]<<16|(u32)stack[3]<<24;
        u8 *kind_out=ds_translate(&cpu->memory,out,1);if(!kind_out)return -1;
        int32_t height;unsigned kind;
        if(!hg_original_loader_height(callback_loader,field->unk60,(s32)cpu->r[2],(s32)cpu->r[3],(s32)cpu->r[1],&height,&kind)){open_contract=hg_original_loader_error(callback_loader);return -1;}
        *kind_out=kind;cpu->r[0]=(u32)height;return 1;
    }
    if(!strcmp(name,"GetMetatileBehavior")){
        if(cpu->r[0]!=0x03004000u||!callback_loader)return -1;
        unsigned behavior;
        if(!hg_original_loader_behavior(callback_loader,field->unk60,(s32)cpu->r[1],(s32)cpu->r[2],&behavior)){open_contract=hg_original_loader_error(callback_loader);return -1;}
        cpu->r[0]=behavior;return 1;
    }
    if(!strcmp(name,"GetMetatileBehavior_None")){cpu->r[0]=GetMetatileBehavior_None();return 1;}
    if(!strcmp(name,"MetatileBehavior_IsNone")){cpu->r[0]=MetatileBehavior_IsNone(cpu->r[0]);return 1;}
    if(!strcmp(name,"FieldSystem_GetPlayerAvatar")&&cpu->r[0]==0x03004000u){cpu->r[0]=0x03008000u;return 1;}
    if(cpu->r[0]==0x03008000u&&field->playerAvatar){
        if(!strcmp(name,"PlayerAvatar_GetXCoord")){cpu->r[0]=PlayerAvatar_GetXCoord(field->playerAvatar);return 1;}
        if(!strcmp(name,"PlayerAvatar_GetZCoord")){cpu->r[0]=PlayerAvatar_GetZCoord(field->playerAvatar);return 1;}
    }
    if(!strcmp(name,"MapObjectManager_GetFirstActiveObjectWithMovement1")&&cpu->r[0]==0x03007000u){
        LocalMapObject *player=MapObjectManager_GetFirstActiveObjectWithMovement1(MapObject_GetManager(object));
        cpu->r[0]=player?0x03009000u:0;return 1;
    }
    if(cpu->r[0]!=0x03003000u){open_contract=name;return -1;}
    if(!strcmp(name,"MapObject_GetManager")){cpu->r[0]=0x03007000u;return 1;}
    if(!strcmp(name,"sub_0205F370")){
        if((s32)cpu->r[1]<0||cpu->r[1]>sizeof(object->unkD8)){open_contract=name;return -1;}
        (void)sub_0205F370(object,cpu->r[1]);cpu->r[0]=0x0300a000u;return 1;
    }
    if(!strcmp(name,"sub_0205F394")){cpu->r[0]=0x0300a000u;return 1;}
    if(!strcmp(name,"MapObject_GetFieldSystem")){cpu->r[0]=0x03004000u;return 1;}
    if(!strcmp(name,"MapObject_GetType")){cpu->r[0]=MapObject_GetType(object);return 1;}
    if(!strcmp(name,"MapObject_CopyPositionVector")||!strcmp(name,"MapObject_SetPositionVector")){
        VecFx32 *v=ds_translate(&cpu->memory,cpu->r[1],sizeof(*v));if(!v)return -1;
        VecFx32 value;
        if(!strcmp(name,"MapObject_CopyPositionVector")){MapObject_CopyPositionVector(object,&value);memcpy(v,&value,sizeof(value));}
        else {memcpy(&value,v,sizeof(value));MapObject_SetPositionVector(object,&value);}
        return 1;
    }
    if(!strcmp(name,"sub_0205F430"))movement_calls++;
#include "original_field_object_imports.inc"
#include "original_field_object_vectors.inc"
    open_contract=name;return -1;
    return 1;
}
static void canonical_object_arm(LocalMapObject *object,u32 function){
    u8 ram[65536]={0};GF_ASSERT(sizeof(hg_transition_image)<0x8000);
    memcpy(ram,hg_transition_image,sizeof(hg_transition_image));
    FieldSystem *field=MapObject_GetFieldSystem(object);
    DSNativeRegion regions[]={
        {0x0300a000u,object->unkD8,sizeof(object->unkD8)},
        /* Original pointer-free follower tail, excluding its native object pointer. */
        {0x030040e8u,(u8 *)&field->followMon.unk4,
            offsetof(FollowMon,unk1C)+sizeof(field->followMon.unk1C)-offsetof(FollowMon,unk4)}
    };
    if(function==HG_TRANSITION_sub_02065900)follower_calls++;
    if(function==HG_TRANSITION_sub_02065998)follower_teardowns++;
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,sizeof(ram),regions,NELEMS(regions)};
    cpu.code_start=TRANSITION_BASE;cpu.code_size=HG_TRANSITION_CODE_SIZE;
    cpu.r[0]=0x03003000u;cpu.r[13]=TRANSITION_BASE+0xf000;
    cpu.return_address=0x02010009;cpu.r[14]=cpu.return_address;
    cpu.r[15]=function;
    cpu.boundary=held_import;cpu.boundary_user=object;
    if(ds_thumb_run(&cpu,1000)!=DS_THUMB_RETURN)unsupported(open_contract?open_contract:"canonical object ARM boundary");
    GF_ASSERT(cpu.r[13]==TRANSITION_BASE+0xf000);
}

static void sub_0205FD30(LocalMapObject *object){autonomous_calls++;canonical_object_arm(object,HG_TRANSITION_sub_0205FD30);}
static void MapObject_ClearHeldMovement(LocalMapObject *object){canonical_object_arm(object,HG_TRANSITION_MapObject_ClearHeldMovement);}
static void canonical_object_reset(LocalMapObject *object){canonical_object_arm(object,HG_TRANSITION_sub_0205FD20);}
/* The original process status/application owners use the canonical declaration.
 * Unentered field control/presentation and parent initialization remain explicit. */
void ov01_021F6830(FieldSystem *field,int a,int b){(void)field;(void)a;(void)b;unsupported("ov01_021F6830: canonical field presentation");}
typedef struct FieldSystemUnkSubCC FieldSystemUnkSubCC;
static DSAudio *parent_audio;
static HGOriginalLoader *parent_resources;static unsigned weather_construction_imports;
static void BillboardLists_Create(int count,enum HeapID heap){
    GF_ASSERT(parent_resources);if(!hg_original_billboards_create(parent_resources,count,heap))unsupported(hg_original_loader_error(parent_resources));
}
static void BillboardLists_Delete(void){GF_ASSERT(hg_original_billboards_delete(parent_resources));}
static void NNS_G2dInitOamManagerModule(void){hg_original_oam_module_init(parent_resources);}
static void OamManager_Create(int a,int b,int c,int d,int e,int f,int g,int h,enum HeapID heap){
    uint32_t args[]={a,b,c,d,e,f,g,h,heap};if(!hg_original_oam_create(parent_resources,args))unsupported(hg_original_loader_error(parent_resources));
}
static void OamManager_Free(void){GF_ASSERT(hg_original_oam_free(parent_resources));}
static void OamManager_ApplyAndResetBuffers(void){if(!hg_original_oam_apply(parent_resources))unsupported(hg_original_loader_error(parent_resources));}
static void *ov02_0224F864(enum HeapID heap){void *p=hg_original_aux_new(parent_resources,heap);if(!p)unsupported(hg_original_loader_error(parent_resources));return p;}
static void ov02_0224F8F4(void *owner){GF_ASSERT(hg_original_aux_free(parent_resources,owner));}

/* Canonical C owners retain native pointers; original ARM resource records are
 * opaque mapped allocations consumed only by the checked resource context. */
FieldSystemUnkSubC8 *ov01_022041C4(enum HeapID);
FieldSystemUnkSubCC *ov01_0220460C(FieldSystemUnkSubC8 *);
MapPropAnimationManager *MapPropAnimationManager_Init(NARC *,FieldSystemUnkSubC8 *);
MapPropOneShotAnimationManager *MapPropOneShotAnimationManager_New(void);
void MapPropOneShotAnimationManager_Free(MapPropOneShotAnimationManager **);
Field3dRenderObjManager *Field3dRenderObjManager_New(enum HeapID,int,int,NNSG3dResFileHeader **);
static void GF_RTC_CopyTime(RTCTime *time){RTCDate date;GF_RTC_CopyDateTime(&date,time);}
static FieldSystemUnkSubC8 *canonical_pool_new(enum HeapID heap){FieldSystemUnkSubC8 *p=ov01_022041C4(heap);hg_original_bind_pools(parent_resources,p);return p;}
static AreaDataManager *canonical_area_new(int bank){void *p=hg_original_area_new(parent_resources,bank);if(!p)unsupported(hg_original_loader_error(parent_resources));return p;}
static NNSG3dResFileHeader **canonical_area_headers(AreaDataManager *area){void *p=hg_original_area_headers(parent_resources,area);GF_ASSERT(p);return p;}
static NARC *canonical_area_list(AreaDataManager *area){NARC *p=hg_original_area_animation_list(parent_resources,area);GF_ASSERT(p);return p;}
static void canonical_area_load(AreaDataManager *area,Field3dRenderObjManager *render,MapPropAnimationManager *manager,FieldSystemUnkSubCC *land,FieldSystemUnkSub104 *time){if(!hg_original_area_load(parent_resources,area,render,manager,land,time))unsupported(hg_original_loader_error(parent_resources));}
static MapLoadManager *canonical_map_new(MapMatrix *matrix,AreaDataManager *area,Field3dRenderObjManager *render,MapPropAnimationManager *animations,FieldSystemUnkSubCC *land,int mode,SaveData *save){
    GF_ASSERT(area&&render&&animations&&land&&save);hg_original_loader_bind_save(parent_resources,save);GF_ASSERT(hg_original_loader_attach_map(parent_resources,matrix,mode));return hg_original_map_owner(parent_resources);
}
typedef struct SoundplateStruct SoundplateStruct;
typedef struct HBlankSystem HBlankSystem;
static HBlankSystem *canonical_hblank_new(enum HeapID heap){
    void *p=hg_original_hblank_new(parent_resources,heap);
    if(!p)unsupported(hg_original_loader_error(parent_resources));
    return p;
}
static void canonical_hblank_start(HBlankSystem *p){
    if(!hg_original_hblank_start(parent_resources,p))unsupported(hg_original_loader_error(parent_resources));
}
static void canonical_hblank_free(HBlankSystem *p){
    if(!hg_original_hblank_free(parent_resources,p))unsupported(hg_original_loader_error(parent_resources));
}
static void *canonical_poison_new(enum HeapID heap,HBlankSystem *hblank){
    void *p=hg_original_poison_new(parent_resources,heap,hblank);
    if(!p)unsupported(hg_original_loader_error(parent_resources));
    return p;
}
static void canonical_poison_free(void *p){
    if(!hg_original_poison_free(parent_resources,p))unsupported(hg_original_loader_error(parent_resources));
}
static SoundplateStruct *canonical_soundplate_at(FieldSystem *field,int x,int z){
    GF_ASSERT(field&&field->mapLoadManager==hg_original_map_owner(parent_resources));
    void *view=hg_original_loader_soundplate(parent_resources,x,z);
    if(!view)unsupported(hg_original_loader_error(parent_resources));
    return view;
}
static DynamicTerrainHeightManager *canonical_height_new(unsigned count,enum HeapID heap){void *p=hg_original_height_new(parent_resources,count,heap);GF_ASSERT(p);return p;}
static void canonical_initial_load(MapLoadManager *owner,int x,int z){GF_ASSERT(owner==hg_original_map_owner(parent_resources));if(!hg_original_loader_initial_load(parent_resources,x,z))unsupported(hg_original_loader_error(parent_resources));}

u32 canonical_effect_manager_priority(void *owner){FieldSystem *field=owner;GF_ASSERT(field&&field->mapObjectManager);return MapObjectManager_GetPriority(field->mapObjectManager);}
extern int defaultFieldEffectRenderers[];
static FieldEffectManager *canonical_effects_new(FieldSystem *field,u32 count,enum HeapID heap){void *p=hg_original_effects_new(parent_resources,field,count,heap);if(!p)unsupported(hg_original_loader_error(parent_resources));return p;}
static void canonical_effects_anim(FieldEffectManager *effects,u32 count){GF_ASSERT(effects==hg_original_effects_owner(parent_resources));if(!hg_original_effects_anim_init(parent_resources,count))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_effects_resources(FieldEffectManager *effects,enum HeapID heap,int a,int b,int c,int d,int e,int f,int g){GF_ASSERT(effects==hg_original_effects_owner(parent_resources));uint32_t values[]={heap,a,b,c,d,e,f,g};if(!hg_original_effects_resources_init(parent_resources,values))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_effects_register(FieldEffectManager *effects,const u32 *ids){GF_ASSERT(effects==hg_original_effects_owner(parent_resources)&&ids==(const u32 *)defaultFieldEffectRenderers);if(!hg_original_effects_register(parent_resources))unsupported(hg_original_loader_error(parent_resources));}
static u32 canonical_effects_size(FieldEffectManager *effects,u32 member){GF_ASSERT(effects==hg_original_effects_owner(parent_resources));u32 size=hg_original_effect_member_size(parent_resources,member);if(!size)unsupported(hg_original_loader_error(parent_resources));return size;}
static void canonical_effects_read(FieldEffectManager *effects,u32 member,void *bytes){GF_ASSERT(effects==hg_original_effects_owner(parent_resources));if(!hg_original_effect_member_read(parent_resources,member,bytes))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_effects_heap_free(void *bytes){GF_ASSERT(hg_original_effect_native_free(parent_resources,bytes));}
static void canonical_effects_model(NNSG3dRenderObj *object,NNSG3dResMdl **model,NNSG3dResFileHeader **header){if(!hg_original_effect_native_model(parent_resources,*header,object,(void **)model))unsupported("GF3dRender_InitObjFromHeader: effect model/resource ownership");}
u32 canonical_manager_operation(void *owner,unsigned op,u32 value){
    FieldSystem *field=owner;MapObjectManager *m=field->mapObjectManager;GF_ASSERT(m);
    switch(op){case 0:return MapObjectManager_GetFlagsBitsMask(m,value);case 1:return MapObjectManager_GetObjectCount(m);
    case 2:MapObjectManager_SetFlagsBits(m,value);return 0;case 3:MapObjectManager_ClearFlagsBits(m,value);return 0;case 4:return sub_0205F5D4(m);case 5:return m->mapModelNarc!=NULL;}
    GF_ASSERT(FALSE);return 0;
}
void *canonical_manager_storage(void *owner,size_t *size){FieldSystem *f=owner;*size=sizeof(f->mapObjectManager->unk18)+sizeof(f->mapObjectManager->unk1C);return sub_0205F1A0(f->mapObjectManager);}
void canonical_manager_narc(void *owner,NARC *narc){FieldSystem *f=owner;MapObjectManager_SetMapModelNarc(f->mapObjectManager,narc);}
static void canonical_objects_graphics(MapObjectManager *manager,int slots,int count,const int *ids,int mode){
    GF_ASSERT(manager&&manager->fieldSystem&&manager->fieldSystem->mapObjectManager==manager);
    if(!hg_original_objects_graphics_new(parent_resources,slots,count,ids,mode))unsupported(hg_original_loader_error(parent_resources));
}
/* A canonical object identity denotes an original array slot, just as the
 * original pointer does. No native object bytes/pointers are visible in ARM. */
static unsigned canonical_object_index(LocalMapObject *object){
    MapObjectManager *manager=MapObject_GetManager(object);FieldSystem *field=MapObject_GetFieldSystem(object);
    GF_ASSERT(manager&&field&&field->mapObjectManager==manager);
    unsigned index=0;while(index<manager->objectCount&&manager->objects+index!=object)index++;
    GF_ASSERT(index<manager->objectCount);return index;
}
static void canonical_graphics_callback(LocalMapObject *object,u32 callback){if(!hg_original_object_callback(parent_resources,canonical_object_index(object),callback))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_graphics_mode(LocalMapObject *object,BOOL enabled){if(!hg_original_object_graphics_mode(parent_resources,canonical_object_index(object),enabled))unsupported(hg_original_loader_error(parent_resources));}
static ObjectEventGraphicsInfo *ObjectEvent_GetGraphicsInfo(u32 sprite){_Static_assert(sizeof(ObjectEventGraphicsInfo)==6,"pointer-free ARM graphics info");ObjectEventGraphicsInfo *info=hg_original_object_graphics_info(parent_resources,sprite);if(!info)unsupported(hg_original_loader_error(parent_resources));return info;}
static unsigned canonical_avatar_reads,canonical_id_reads;
uint32_t hg_original_object_facing_vector(
    HGOriginalLoader *, unsigned, void *, size_t
);

int canonical_loader_object_import(void *owner,const char *name,DSThumb *cpu){
    FieldSystem *field=owner;MapObjectManager *manager=field->mapObjectManager;
    if(!manager)return 0;
    if(!strcmp(name,"PlayerAvatar_GetMapObject")){
        if(cpu->r[0]!=0x03104000||!field->playerAvatar)return -1;
        canonical_avatar_reads++;LocalMapObject *object=PlayerAvatar_GetMapObject(field->playerAvatar);
        unsigned index=0;while(index<manager->objectCount&&manager->objects+index!=object)index++;
        if(index==manager->objectCount)return -1;cpu->r[0]=0x03110000+4*index;return 1;
    }
    if(cpu->r[0]<0x03110000||(cpu->r[0]-0x03110000)%4)return 0;
    unsigned index=(cpu->r[0]-0x03110000)/4;if(index>=manager->objectCount)return 0;
    LocalMapObject *object=manager->objects+index;
    _Static_assert(offsetof(LocalMapObject,unk108)+32-offsetof(LocalMapObject,unkD8)==80,"canonical opaque work extent");
    const char *work_names[]={"sub_0205F370","sub_0205F394","sub_0205F398","sub_0205F3BC","sub_0205F3C0","sub_0205F3E4","sub_0205F3E8","sub_0205F40C"};
    for(unsigned work=0;work<8;work++)if(!strcmp(name,work_names[work])){
        unsigned capacity=work>=6?32:16;if(!(work&1)&&(s32)cpu->r[1]<0)return -1;
        if(!(work&1)&&cpu->r[1]>capacity)return -1;
        u8 *bytes=NULL;
        switch(work){case 0:bytes=sub_0205F370(object,cpu->r[1]);break;case 1:bytes=sub_0205F394(object);break;
        case 2:bytes=sub_0205F398(object,cpu->r[1]);break;case 3:bytes=sub_0205F3BC(object);break;
        case 4:bytes=sub_0205F3C0(object,cpu->r[1]);break;case 5:bytes=sub_0205F3E4(object);break;
        case 6:bytes=sub_0205F3E8(object,cpu->r[1]);break;case 7:bytes=sub_0205F40C(object);break;}
        uint32_t base=hg_original_object_work(parent_resources,index,object->unkD8,80);if(!base)return -1;
        GF_ASSERT((uintptr_t)bytes>=(uintptr_t)object->unkD8&&(uintptr_t)bytes-(uintptr_t)object->unkD8<80);
        cpu->r[0]=base+(unsigned)((uintptr_t)bytes-(uintptr_t)object->unkD8);return 1;
    }
    if (!strcmp(name, "MapObject_GetFacingVector")) {
        VecFx32 *vector = MapObject_GetFacingVector(object);
        GF_ASSERT(sizeof(*vector) == 12);

        uint32_t address = hg_original_object_facing_vector(
            parent_resources, index, vector, sizeof(*vector)
        );

        if (!address) return -1;
        cpu->r[0] = address;
        return 1;
    }
    if(!strcmp(name,"MapObject_GetID")){canonical_id_reads++;cpu->r[0]=MapObject_GetID(object);return 1;}
    if(!strcmp(name,"MapObject_GetFieldSystem")){GF_ASSERT(MapObject_GetFieldSystem(object)==field);cpu->r[0]=0x03100000;return 1;}
    if(!strcmp(name,"MapObject_GetManager")){GF_ASSERT(MapObject_GetManager(object)==manager);cpu->r[0]=0x03101000;return 1;}
#include "original_field_object_imports.inc"
#include "original_field_object_vectors.inc"
    return 0;
}
static void ov01_022008B4(PlayerAvatar *avatar){GF_ASSERT(avatar&&avatar==MapObject_GetFieldSystem(PlayerAvatar_GetMapObject(avatar))->playerAvatar);if(!hg_original_avatar_features_start(parent_resources))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_objects_prepare(MapObjectManager *manager){GF_ASSERT(manager&&manager->fieldSystem->mapObjectManager==manager);if(!hg_original_objects_graphics_prepare(parent_resources,0))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_effects_render_init(FieldEffectManager *manager){GF_ASSERT(manager==hg_original_effects_owner(parent_resources));if(!hg_original_objects_graphics_prepare(parent_resources,1))unsupported(hg_original_loader_error(parent_resources));}

static void canonical_followmon_pending(FieldSystem *field) {
    GF_ASSERT(field && parent_resources);

    if (!hg_original_followmon_pending(
            parent_resources, field, &field->followMon.unk15))
        unsupported(hg_original_loader_error(parent_resources));

    GF_ASSERT(field->followMon.unk15 == 0);
}

static void canonical_comm_force_position(void) {
    if (!hg_original_comm_force_position(parent_resources))
        unsupported(hg_original_loader_error(parent_resources));
}

static void canonical_map_track_target(VecFx32 *vec, MapLoadManager *manager) {
    GF_ASSERT(manager == hg_original_map_owner(parent_resources));
    if (!hg_original_map_track_target(parent_resources, vec, sizeof(*vec)))
        unsupported(hg_original_loader_error(parent_resources));
}
static ModelAttributes *canonical_model_attributes_new(void){
    void *owner=hg_original_model_attributes_new(parent_resources);
    if(!owner)unsupported(hg_original_loader_error(parent_resources));return owner;
}
static void canonical_model_attributes_configure(ModelAttributes *owner){
    if(!hg_original_model_attributes_configure(parent_resources,owner))unsupported("model attribute ownership/configuration");
}
static void canonical_model_attributes_free(ModelAttributes **owner){
    void *bytes=*owner;if(!hg_original_model_attributes_free(parent_resources,&bytes))unsupported("model attribute destruction ownership");*owner=bytes;
}
static FogData *canonical_fog_new(void){
    void *owner=hg_original_fog_new(parent_resources);
    if(!owner)unsupported(hg_original_loader_error(parent_resources));return owner;
}
static void canonical_fog_free(FogData **owner){
    void *bytes=*owner;if(!hg_original_fog_free(parent_resources,&bytes))unsupported("fog destruction ownership");*owner=bytes;
}
static struct WeatherManager *canonical_weather_new(FieldSystem *field){
    GF_ASSERT(field&&field->camera&&field->areaLightManager);
    void *owner=hg_original_weather_new(parent_resources);
    if(!owner)unsupported(hg_original_loader_error(parent_resources));weather_construction_imports=hg_original_owner_imports(parent_resources);return owner;
}
void canonical_field_rtc_time(void *owner,u32 out[3]){GF_ASSERT(owner);RTCTime time;GF_RTC_CopyTime(&time);memcpy(out,&time,12);}
static u32 canonical_area_light_archive(AreaDataManager *area){u32 id=hg_original_area_light_archive(parent_resources,area);if(id==UINT32_MAX)unsupported("area light archive ownership");return id;}
static AreaLightManager *canonical_light_new(ModelAttributes *attributes,u32 archive){void *p=hg_original_light_new(parent_resources,attributes,archive);if(!p)unsupported(hg_original_loader_error(parent_resources));return p;}
static void canonical_light_free(AreaLightManager **owner){void *p=*owner;if(!hg_original_light_free(parent_resources,&p))unsupported("light owner destruction");*owner=p;}
static void canonical_light_update(AreaLightManager *owner){if(!hg_original_light_update(parent_resources,owner))unsupported(hg_original_loader_error(parent_resources));}
static int gG3dDepthBufferingMode;
static void canonical_camera_create(const VecFx32 *target,FieldSystem *field,u32 type,BOOL history){
    GF_ASSERT(field&&field->playerAvatar&&target==PlayerAvatar_GetPositionVector(field->playerAvatar));
    if(!hg_original_camera_new(parent_resources,(void *)target,type,history))unsupported(hg_original_loader_error(parent_resources));
    field->camera=hg_original_camera_owner(parent_resources);field->unk28=hg_original_camera_transition(parent_resources);
    gG3dDepthBufferingMode=hg_original_camera_depth(parent_resources);
}
static void canonical_camera_delete(FieldSystem *field){
    GF_ASSERT(field->camera==hg_original_camera_owner(parent_resources));
    if(!hg_original_camera_delete(parent_resources))unsupported(hg_original_loader_error(parent_resources));
}
static void canonical_camera_push(void){if(!hg_original_camera_push(parent_resources))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_camera_update(void *transition){GF_ASSERT(transition==hg_original_camera_transition(parent_resources));if(!hg_original_camera_update(parent_resources))unsupported(hg_original_loader_error(parent_resources));}
#include "original_field_windows_types.inc"
static FieldDrawMapNameInfo *canonical_map_name_new(BgConfig *);
static void canonical_map_name_free(FieldDrawMapNameInfo *);
typedef struct NNSG3dResTex NNSG3dResTex;
static FieldTextureManager *canonical_field_texture_new(void);
static int canonical_field_texture_load(FieldTextureManager *,NNSG3dResTex *);
static void canonical_field_texture_clear(FieldTextureManager *);
static void canonical_field_texture_destroy(FieldTextureManager *);
static NNSG3dResTex *canonical_area_texture(AreaDataManager *);
static void canonical_weather_set(struct WeatherManager *,u32);
static void canonical_weather_delete(struct WeatherManager *);
static void canonical_sound_field_bgm(u16 sequence){
    if(!hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_BGM,sequence,NULL))unsupported(hg_original_loader_error(parent_resources));
}
static void canonical_sound_field_guard(BOOL busy){
    GF_ASSERT(busy==FALSE||busy==TRUE);
    if(!hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_GUARD,busy,NULL))unsupported(hg_original_loader_error(parent_resources));
}
static BOOL canonical_sound_field_scene(u16 sequence){
    uint32_t result;
    if(!hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_SCENE,sequence,&result)){
        const char *error=hg_original_loader_error(parent_resources);
        canonical_sound_field_guard(FALSE);unsupported(error);
    }
    return result;
}
static void *canonical_graphics_pool(void *storage){
    void *pool=hg_original_objects_pool(parent_resources,storage);if(!pool)unsupported("retired original object-graphics pool");return pool;
}
static void canonical_graphics_pool_publish(void *pool){
    if(!hg_original_objects_pool_publish(parent_resources,pool))unsupported("retired original object-graphics publication");
}
static void canonical_sound_request(enum HGOriginalSoundPolicy op,const u32 *args,unsigned count){
    if(!hg_original_sound_request(parent_resources,op,args,count,NULL))
        unsupported(hg_original_loader_error(parent_resources));
}
static void canonical_stop_se(int sequence,int frames){
    u32 args[]={sequence,frames};canonical_sound_request(HG_SOUND_STOP_SE,args,2);
}
static void canonical_sound_volume(int handle,int volume,int frames){
    u32 args[]={handle,volume,frames};canonical_sound_request(HG_SOUND_MOVE_VOLUME,args,3);
}
static void canonical_play_se(int sequence){
    u32 args[]={sequence};canonical_sound_request(HG_SOUND_PLAY_SE,args,1);
}
static void canonical_environment_sound(int sequence){
    u32 args[]={sequence};canonical_sound_request(HG_SOUND_ENVIRONMENT,args,1);
}
#include "original_field_parent.inc"
#include "original_field_windows.inc"
static jmp_buf window_constructor_rejected;static unsigned window_capture_active,window_fail_lease;
static NARC *canonical_original_message_archive(NarcId id,enum HeapID heap){
    GF_ASSERT(id==NARC_msgdata_msg);if(window_fail_lease){GF_ASSERT(window_capture_active);longjmp(window_constructor_rejected,1);}
    return NARC_New(HG_RESOURCE_ORIGINAL_MESSAGES,heap);
}
static FieldDrawMapNameInfo *canonical_map_name_new(BgConfig *bg){
    unsigned scope=0;const void *previous=hg_heap_capture(&scope);GF_ASSERT(!window_capture_active);window_capture_active=1;
    if(setjmp(window_constructor_rejected)){
        hg_heap_capture(previous);hg_heap_finish_capture(&scope,FALSE);window_capture_active=0;return NULL;
    }
    FieldDrawMapNameInfo *owner=hg_field_map_name_new(bg);
    hg_heap_capture(previous);hg_heap_finish_capture(&scope,TRUE);window_capture_active=0;return owner;
}
static FieldTextureManager *canonical_field_texture_new(void){void *p=hg_original_field_texture_new(parent_resources);if(!p)unsupported(hg_original_loader_error(parent_resources));return p;}
static int canonical_field_texture_load(FieldTextureManager *owner,NNSG3dResTex *texture){unsigned count;if(!hg_original_field_texture_load(parent_resources,owner,texture,&count))unsupported(hg_original_loader_error(parent_resources));return count;}
static void canonical_field_texture_clear(FieldTextureManager *owner){if(!hg_original_field_texture_clear(parent_resources,owner))unsupported(hg_original_loader_error(parent_resources));}
static void canonical_field_texture_destroy(FieldTextureManager *owner){if(!hg_original_field_texture_destroy(parent_resources,owner))unsupported(hg_original_loader_error(parent_resources));}
static NNSG3dResTex *canonical_area_texture(AreaDataManager *area){void *p=hg_original_area_texture(parent_resources,area);if(!p)unsupported("area texture ownership");return p;}

static void canonical_map_name_free(FieldDrawMapNameInfo *info){GF_ASSERT(info&&!info->task);hg_field_map_name_delete(info);}
static void canonical_weather_set(struct WeatherManager *owner,u32 type){
    GF_ASSERT(owner==hg_original_weather_owner(parent_resources));
    if(!hg_original_weather_set(parent_resources,type))unsupported(hg_original_loader_error(parent_resources));
}
static void canonical_weather_delete(struct WeatherManager *owner){
    void *p=owner;if(!hg_original_weather_free(parent_resources,&p))unsupported(hg_original_loader_error(parent_resources));
}
static unsigned paused_controller_updates;
static void trace_original_control(FieldSystem *field){
    GF_ASSERT(!FieldSystem_IsPlayerMovementAllowed(field));
    FieldSystem_Control(field);paused_controller_updates++;
}
#pragma push_macro("FieldSystem_Control")
#undef FieldSystem_Control
#define FieldSystem_Control trace_original_control
#include "original_field_process.inc"
#pragma pop_macro("FieldSystem_Control")
static BOOL original_restore_task(TaskManager *);
static BOOL original_restore_wait(TaskManager *);
static int restore_import(DSThumb *cpu,void *opaque){
    TaskManager *task=opaque;const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_transition_imports);i++)
        if(cpu->r[15]==hg_transition_imports[i].address)name=hg_transition_imports[i].name;
    if(!name)return -1;
    if(!strcmp(name,"TaskManager_GetFieldSystem")&&cpu->r[0]==0x03005000u){cpu->r[0]=0x03004000u;return 1;}
    if(!strcmp(name,"TaskManager_GetStatePtr")&&cpu->r[0]==0x03005000u){cpu->r[0]=0x03006000u;return 1;}
    if(!strcmp(name,"TaskManager_Call")&&cpu->r[0]==0x03005000u&&!cpu->r[2]){
        TaskFunc callback=NULL;u32 target=cpu->r[1]&~1u;
        if(target==HG_TRANSITION_sub_020553C0)callback=original_restore_task;
        else if(target==HG_TRANSITION_sub_0205528C)callback=original_restore_wait;
        if(!callback){open_contract="restore task callback not admitted";return -1;}
        TaskManager_Call(task,callback,NULL);return 1;
    }
    FieldSystem *field=TaskManager_GetFieldSystem(task);
    if(cpu->r[0]==0x03004000u){
        if(!strcmp(name,"sub_0203DF7C")){cpu->r[0]=sub_0203DF7C(field);return 1;}
        if(!strcmp(name,"sub_020505C8")){cpu->r[0]=sub_020505C8(field);return 1;}
        if(!strcmp(name,"FieldSystem_LoadFieldOverlay")){FieldSystem_LoadFieldOverlay(field);return 1;}
    }
    open_contract=name;return -1;
}
static u32 original_restore_run(TaskManager *task,u32 entry){
    u8 ram[65536]={0};GF_ASSERT(sizeof(hg_transition_image)<0x8000);
    memcpy(ram,hg_transition_image,sizeof(hg_transition_image));
    DSNativeRegion state={0x03006000u,(u8 *)TaskManager_GetStatePtr(task),sizeof(u32)};
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,sizeof(ram),&state,1};
    cpu.code_start=TRANSITION_BASE;cpu.code_size=HG_TRANSITION_CODE_SIZE;
    cpu.r[0]=0x03005000u;cpu.r[13]=TRANSITION_BASE+0xf000;
    cpu.return_address=0x02010009;cpu.r[14]=cpu.return_address;cpu.r[15]=entry;
    cpu.boundary=restore_import;cpu.boundary_user=task;
    if(ds_thumb_run(&cpu,10000)!=DS_THUMB_RETURN)unsupported(open_contract?open_contract:"original restore ARM execution");
    GF_ASSERT(cpu.r[13]==TRANSITION_BASE+0xf000);return cpu.r[0];
}
static BOOL original_restore_task(TaskManager *task){return original_restore_run(task,HG_TRANSITION_sub_020553C0);}
static BOOL original_restore_wait(TaskManager *task){return original_restore_run(task,HG_TRANSITION_sub_0205528C);}
static void sub_02055408(TaskManager *task){(void)original_restore_run(task,HG_TRANSITION_sub_02055408);}
#include "original_field_newgame.inc"
static BOOL probe_done(TaskManager *task){(void)task;return TRUE;}
static void probe_object_entry(FieldSystem *field,HGOriginalLoader *loader) {
    GF_ASSERT(!field->taskman&&!field->unk60);GF_ASSERT(hg_scheduler_init());CallFieldTask_NewGame(field);
    GF_ASSERT(!FieldSystem_RunTaskFrame(field));
    GF_ASSERT(*TaskManager_GetStatePtr(field->taskman)==1);
    TaskManager_Jump(field->taskman,probe_done,NULL);GF_ASSERT(FieldSystem_RunTaskFrame(field)&&!field->taskman);
    GF_ASSERT(field->mapObjectManager&&field->playerAvatar);
    GF_ASSERT(MapObjectManager_GetFieldSystem(field->mapObjectManager)==field);
    GF_ASSERT(MapObjectManager_GetObjectCount(field->mapObjectManager)==64);
    LocalMapObject *player=MapObjectManager_GetObjects(field->mapObjectManager);
    GF_ASSERT(MapObject_CheckActive(player)&&MapObject_GetMovement(player)==1);
    GF_ASSERT(MapObject_GetXCoord(player)==6&&MapObject_GetZCoord(player)==6);
    GF_ASSERT(MapObject_GetManager(player)==field->mapObjectManager);
    GF_ASSERT(player->unkB8==sub_0205FCB4&&player->unkBC==sub_0205FCB8&&player->unkC0==sub_0205FCBC);
    GF_ASSERT(MapObject_GetMovementCommand(player)==0xff&&MapObject_GetMovementStep(player)==0);
    GF_ASSERT(PlayerAvatar_GetMapObject(field->playerAvatar)==player);
    GF_ASSERT(!field->followMon.active&&!field->followMon.mapObject);
    callback_loader=loader;autonomous_calls=movement_calls=0;
    for(unsigned frame=0;frame<2;frame++)GF_ASSERT(hg_scheduler_step());
    GF_ASSERT(autonomous_calls==2&&!movement_calls);
    sub_0205F568(field->mapObjectManager);
    autonomous_calls=0;
    for(unsigned frame=0;frame<4;frame++)GF_ASSERT(hg_scheduler_step());
    GF_ASSERT(autonomous_calls==4&&movement_calls==4);
    fprintf(stderr,"Original registered player task: %u dispatches, %u movement callbacks\n",autonomous_calls,movement_calls);
    /* A separate ownership fixture exercises the real nonempty-party branch;
     * it is removed before returning to the untouched new-game entry save. */
    Party *party=SaveArray_Party_Get(field->saveData);GF_ASSERT(Party_GetCount(party)==0);
    Pokemon *mon=AllocMonZeroed(HEAP_ID_FIELD2);GF_ASSERT(mon);
    CreateMon(mon,SPECIES_CHIKORITA,5,32,FALSE,0,OT_ID_PLAYER_ID,0);
    GF_ASSERT(Party_AddMon(party,mon));Heap_Free(mon);
    LocalMapObject *follower=FollowMon_InitMapObject(field->mapObjectManager,6,6,field->location->direction,field->location->mapId);
    GF_ASSERT(follower&&follower!=player&&field->followMon.mapObject==follower&&field->followMon.active);
    GF_ASSERT(MapObject_GetManager(follower)==field->mapObjectManager&&MapObject_GetMovement(follower)==48);
    GF_ASSERT(follower->unkD8[1]==1&&follower->unkD8[4]==6&&follower->unkD8[6]==6);
    fprintf(stderr,"Original nonempty-party follower constructor: native object, ARM work initialization, registered task\n");
    autonomous_calls=movement_calls=follower_calls=follower_teardowns=0;
    for(unsigned frame=0;frame<4;frame++)GF_ASSERT(hg_scheduler_step());
    GF_ASSERT(autonomous_calls==8&&movement_calls==8&&follower_calls==4);
    GF_ASSERT(field->followMon.mapObject==follower&&follower->unkD8[1]==1);
    fprintf(stderr,"Original player+follower tasks: %u dispatches, %u original follower callbacks\n",autonomous_calls,follower_calls);
    MapObject_Remove(follower);GF_ASSERT(!MapObject_CheckActive(follower)&&follower_teardowns==1);
    /* Logical destruction clears the slot; the original allocator selects the
     * same first inactive slot without ending the backing-array lifetime. */
    LocalMapObject *recreated=FollowMon_InitMapObject(field->mapObjectManager,6,6,field->location->direction,field->location->mapId);
    GF_ASSERT(recreated==follower&&MapObject_CheckActive(recreated));
    MapObject_Remove(recreated);GF_ASSERT(!MapObject_CheckActive(recreated)&&follower_teardowns==2);
    Party_InitWithMaxSize(party,6);
    GF_ASSERT(!FollowMon_InitMapObject(field->mapObjectManager,6,6,field->location->direction,field->location->mapId));
    callback_loader=NULL;
    PlayerAvatar_DeleteFromMap(field->playerAvatar);field->playerAvatar=NULL;
    GF_ASSERT(!MapObject_CheckActive(player));
    unsigned calls_before=autonomous_calls;GF_ASSERT(hg_scheduler_step()&&autonomous_calls==calls_before);
    MapObjectManager_Delete(field->mapObjectManager);field->mapObjectManager=NULL;
    GF_ASSERT(hg_scheduler_shutdown());
    sub_02053324(field);open_contract=NULL;
}

static unsigned hblank_calls;
static void original_hblank_callback(void *argument){
    GF_ASSERT(argument==&hblank_calls);hblank_calls++;
    GF_ASSERT(!hg_scheduler_step()&&!hg_scheduler_shutdown());
}
static void probe_original_hblank(void){
    GF_ASSERT(hg_scheduler_init());hblank_calls=0;
    HBlankInterruptDisable();GF_ASSERT(!hg_scheduler_irq()->mask&&!hg_scheduler_irq()->hblank_request);
    GF_ASSERT(Main_SetHBlankIntrCB(original_hblank_callback,&hblank_calls));
    GF_ASSERT(!Main_SetHBlankIntrCB(original_hblank_callback,NULL));
    GF_ASSERT(!hg_scheduler_shutdown());
    uint64_t ticks=hg_scheduler_ticks();GF_ASSERT(hg_scheduler_hblank());
    GF_ASSERT(hblank_calls==1&&hg_scheduler_ticks()==ticks);
    GF_ASSERT(hg_native_irq_disable()==0);GF_ASSERT(hg_scheduler_hblank()&&hblank_calls==1);
    GF_ASSERT(hg_native_irq_enable()==0x80&&hblank_calls==2);
    GF_ASSERT(Main_SetHBlankIntrCB(NULL,NULL));
    GF_ASSERT(!gSystem.hBlankIntr&&!gSystem.hBlankIntrArg);
    GF_ASSERT(hg_scheduler_hblank()&&hblank_calls==2);
    GF_ASSERT(hg_scheduler_shutdown());
    fprintf(stderr,"Original HBlank bind/refusal/IRQ-mask/pending-delivery/disable completed\n");
}
void VBlankCB_DmaTasksFramecounter(void);
static void irq_once(SysTask *task,void *data){(*(unsigned *)data)++;SysTask_Destroy(task);}
static unsigned irq_order;
static void irq_ordered(SysTask *task,void *data){GF_ASSERT(*(unsigned *)data==irq_order++);SysTask_Destroy(task);}
static void probe_original_vblank(void){
    GF_ASSERT(hg_scheduler_init());unsigned order[32],vw=0;irq_order=0;gSystem.frameCounter=0;
    for(unsigned i=0;i<32;i++){order[i]=31-i;GF_ASSERT(SysTask_CreateOnVBlankQueue(irq_ordered,order+i,order[i]));}
    GF_ASSERT(gSystem.vblankTaskQueue->limit==32&&gSystem.vblankTaskQueue->activeCount==32);
    SysTask *wait=SysTask_CreateOnVWaitQueue(irq_once,&vw,0);GF_ASSERT(wait);
    GF_ASSERT(hg_scheduler_start_vblank(VBlankCB_DmaTasksFramecounter));
    hg_native_irq_disable_mask(DS_IRQ_VBLANK);GF_ASSERT(!hg_scheduler_wait_vblank()&&!irq_order);
    hg_native_irq_enable_mask(DS_IRQ_VBLANK);hg_native_irq_disable();
    GF_ASSERT(!hg_scheduler_wait_vblank()&&!irq_order);hg_native_irq_enable();
    GF_ASSERT(hg_scheduler_wait_vblank());
    GF_ASSERT(irq_order==32&&!vw&&gSystem.frameCounter==1&&!hg_scheduler_ticks());
    GF_ASSERT(hg_scheduler_irq()->check_flags&DS_IRQ_VBLANK);
    GF_ASSERT(hg_scheduler_stop_vblank());SysTask_Destroy(wait);GF_ASSERT(hg_scheduler_shutdown());
}
static void parent_vblank_sentinel(void *context){(void)context;GF_ASSERT(FALSE);}
static void probe_graphics_failures(FieldSystem *field,unsigned count,const int *ids,unsigned imports,unsigned raw_count) {
    /* Complete external IRQ acquisitions before exact budget comparisons.
     * Keep the display IRQ active during all construction attempts. */
    while(gSystem.vblankTaskQueue->activeCount)GF_ASSERT(hg_scheduler_wait_vblank());
    size_t opaque_size;void *opaque=canonical_manager_storage(field,&opaque_size);
    u8 opaque_before[0x10c];GF_ASSERT(opaque_size==sizeof(opaque_before));
    memcpy(opaque_before,opaque,sizeof(opaque_before));
    size_t region_count=hg_original_test_region_count(parent_resources);
    u32 next=hg_original_test_next(parent_resources);
    u8 *external=Heap_Alloc(HEAP_ID_FIELD1,16);memset(external,0xa5,16);
    size_t texture_bytes[2]={hg_native_texture_pool_used(0),hg_native_texture_pool_used(1)};
    unsigned completed_vwait=hg_original_effects_uploads(parent_resources);
    for(unsigned failure=1;failure<=imports;failure++) {
        HGHeapUsage before[4],after;
        const unsigned heaps[]={HEAP_ID_FIELD1,HEAP_ID_FIELD2,68,69};
        for(unsigned i=0;i<4;i++)GF_ASSERT(hg_heap_usage(heaps[i],before+i));
        unsigned main_tasks=gSystem.mainTaskQueue->activeCount,vwait_tasks=gSystem.vwaitTaskQueue->activeCount;
        hg_original_graphics_fail_import(parent_resources,failure);
        GF_ASSERT(!hg_original_objects_graphics_new(parent_resources,32,
            count,ids,10));
        if(hg_original_graphics_imports(parent_resources)!=failure)
            fprintf(stderr,"Graphics failure point %u reached only %u: %s\n",failure,hg_original_graphics_imports(parent_resources),hg_original_loader_error(parent_resources));
        GF_ASSERT(hg_original_graphics_imports(parent_resources)==failure);
        GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"injected graphics construction failure"));
        for(unsigned i=0;i<4;i++) {
            GF_ASSERT(hg_heap_usage(heaps[i],&after));
            if(after.allocated!=before[i].allocated||after.blocks!=before[i].blocks||after.reserved!=before[i].reserved||after.children!=before[i].children)
                fprintf(stderr,"Rollback %u heap %u: bytes %zu/%zu blocks %zu/%zu reserved %zu/%zu children %u/%u\n",failure,heaps[i],after.allocated,before[i].allocated,after.blocks,before[i].blocks,after.reserved,before[i].reserved,after.children,before[i].children);
            GF_ASSERT(after.allocated==before[i].allocated&&after.blocks==before[i].blocks&&
                after.reserved==before[i].reserved&&after.children==before[i].children);
        }
        GF_ASSERT(!hg_heap_usage(63,&after)&&!hg_heap_usage(64,&after));
        GF_ASSERT(gSystem.mainTaskQueue->activeCount==main_tasks&&gSystem.vwaitTaskQueue->activeCount==vwait_tasks);
        GF_ASSERT(!gSystem.vblankTaskQueue->activeCount);
        GF_ASSERT(hg_native_texture_pool_used(0)==texture_bytes[0]&&hg_native_texture_pool_used(1)==texture_bytes[1]);
        GF_ASSERT(hg_original_effects_uploads(parent_resources)==completed_vwait);
        GF_ASSERT(hg_original_test_region_count(parent_resources)==region_count&&hg_original_test_next(parent_resources)==next);
        GF_ASSERT(!hg_original_test_mapped(parent_resources,next)&&
            !hg_original_test_mapped(parent_resources,0x03102000)&&!hg_original_test_mapped(parent_resources,0x03103000));
        GF_ASSERT(!memcmp(opaque_before,opaque,sizeof(opaque_before)));
        GF_ASSERT(!MapObjectManager_GetFlagsBitsMask(field->mapObjectManager,1)&&!field->mapObjectManager->mapModelNarc);
        for(unsigned i=0;i<16;i++)GF_ASSERT(external[i]==0xa5);
        GF_ASSERT(hg_original_effects_raw_probe(parent_resources,2)==raw_count);
    }
    hg_original_graphics_fail_import(parent_resources,0);Heap_Free(external);
    fprintf(stderr,"Graphics rollback: %u import/return rejection points, external owners preserved, retries admitted\n",imports);
}

/* Validate the reached environment prefix and its own teardown. This is not
 * permission to run whole FieldMap_Exit on a partially initialized parent. */
static void probe_environment_records(FieldSystem *field){
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    GF_ASSERT(field->modelAttributes&&field->fog);
    GF_ASSERT(!hg_original_model_attributes_new(parent_resources)&&!hg_original_fog_new(parent_resources));
    u8 expected[0x48]={0};expected[0x38]=2;expected[0x40]=31;expected[0x45]=0x80;
    GF_ASSERT(!memcmp(field->modelAttributes,expected,sizeof(expected)));
    for(unsigned i=0;i<0x38;i++)GF_ASSERT(!((u8 *)field->fog)[i]);
    /* SDK global state is retained software state, not a rendered-pixel claim. */
    GF_ASSERT(hg_native_video()->global_polygon_attr==0x001f8080u);
    u32 old_model=hg_original_map_native(parent_resources,field->modelAttributes,0x48);
    u32 old_fog=hg_original_map_native(parent_resources,field->fog,0x38);
    GF_ASSERT(old_model&&old_fog&&old_model!=old_fog);
    GF_ASSERT(hg_original_map_native(parent_resources,(u8 *)field->modelAttributes+4,4)==old_model+4);
    GF_ASSERT(!hg_original_model_attributes_configure(parent_resources,(u8 *)field->modelAttributes+4));
    void *foreign=Heap_Alloc(HEAP_ID_FIELD1,0x48);memset(foreign,0x5a,0x48);
    GF_ASSERT(hg_original_map_native(parent_resources,foreign,0x48));
    GF_ASSERT(!hg_original_model_attributes_configure(parent_resources,foreign));
    GF_ASSERT(!hg_original_model_attributes_free(parent_resources,&foreign));
    for(unsigned i=0;i<0x48;i++)GF_ASSERT(((u8 *)foreign)[i]==0x5a);
    Heap_Free(foreign);
    void *wrong=field->modelAttributes;
    GF_ASSERT(!hg_original_fog_free(parent_resources,&wrong)&&wrong==field->modelAttributes);
    canonical_fog_free(&field->fog);canonical_model_attributes_free(&field->modelAttributes);
    GF_ASSERT(!field->fog&&!field->modelAttributes);
    GF_ASSERT(!hg_original_test_mapped(parent_resources,old_model)&&!hg_original_test_mapped(parent_resources,old_model+4));
    GF_ASSERT(!hg_original_test_mapped(parent_resources,old_fog));
    GF_ASSERT(!hg_original_model_attributes_free(parent_resources,&wrong));
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after));
    GF_ASSERT(before.allocated==after.allocated+0x80&&before.blocks==after.blocks+2);
    for(unsigned repeat=0;repeat<3;repeat++){
        field->modelAttributes=canonical_model_attributes_new();canonical_model_attributes_configure(field->modelAttributes);
        field->fog=canonical_fog_new();
        GF_ASSERT(!memcmp(field->modelAttributes,expected,sizeof(expected)));
        GF_ASSERT(hg_original_map_native(parent_resources,field->modelAttributes,0x48)!=old_model);
        canonical_model_attributes_free(&field->modelAttributes);canonical_fog_free(&field->fog);
        HGHeapUsage current;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&current));
        GF_ASSERT(current.allocated==after.allocated&&current.blocks==after.blocks);
    }
    u8 external[0x48]={0};GF_ASSERT(!hg_original_model_attributes_configure(parent_resources,external));
    fprintf(stderr,"Original environment prefix: model/fog bytes, SDK polygon state, stale views and repeated original frees verified\n");
}

/* Explicit ARM32 test representation; pointer fields are numeric DS values. */
typedef struct {
    s32 perspective[5],look[9],distance;u16 angle[4];u8 kind,pad;u16 perspective_angle;
    s32 last[3];u32 target;s32 enable[3];u32 history;
} TestCameraArm;
_Static_assert(sizeof(TestCameraArm)==0x68&&offsetof(TestCameraArm,target)==0x54,"camera test ABI");
static s32 reference_mul(s32 a,s32 b){int64_t v=(int64_t)a*b+2048;return (s32)(v>=0?v/4096:-((-v+4095)/4096));}
static s32 reference_sine(u16 a){return (s32)llroundl(4096*sinl(((a>>4)*2*acosl(-1.0L))/4096));}
static void probe_camera(FieldSystem *field,int exhaustive){
    if(!exhaustive){canonical_camera_delete(field);field->camera=NULL;field->unk28=NULL;return;}
    /* The live object-graphics upload task is persistent. Deliver complete
     * IRQ passes, rather than waiting for that owner's queue to become empty. */
    for(unsigned i=0;i<2;i++)GF_ASSERT(hg_scheduler_wait_vblank());
    TestCameraArm *camera=(void *)field->camera;GF_ASSERT(camera&&field->unk28&&camera->history);
    GF_ASSERT(hg_original_camera_push(parent_resources));
    const DSCamera *render=hg_original_camera_render_state(parent_resources);GF_ASSERT(render);
    double center[]={camera->look[3]/4096.0,camera->look[4]/4096.0,camera->look[5]/4096.0},screen[3];
    GF_ASSERT(ds_raster_project(render,center,256,192,screen));
    GF_ASSERT(fabs(screen[0]-128)<1e-8&&fabs(screen[1]-96)<1e-8);
    GF_ASSERT(!hg_original_camera_new(parent_resources,PlayerAvatar_GetPositionVector(field->playerAvatar),0,1));
    u32 old=hg_original_map_native(parent_resources,camera,0x68),history=camera->history;
    u32 transition=hg_original_map_native(parent_resources,field->unk28,0x34);
    u32 vecs=*(u32 *)((u8 *)hg_original_test_bytes(parent_resources,history,0x28)+0x20);
    canonical_camera_delete(field);
    const u32 *field_words=hg_original_test_bytes(parent_resources,0x03100024,8);
    GF_ASSERT(field_words&&field_words[0]==old&&field_words[1]==transition);
    /* This partial-parent fixture releases native publication separately; the
     * original destructor above leaves the guest numeric fields unchanged. */
    field->camera=NULL;field->unk28=NULL;
    GF_ASSERT(!hg_original_test_mapped(parent_resources,old)&&!hg_original_test_mapped(parent_resources,history)&&!hg_original_test_mapped(parent_resources,vecs));
    GF_ASSERT(!hg_original_camera_delete(parent_resources)&&!hg_original_camera_push(parent_resources));
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    VecFx32 *target=Heap_Alloc(HEAP_ID_FIELD1,sizeof(*target));*target=(VecFx32){4096,8192,12288};
    size_t remaining=before.capacity-before.allocated-before.reserved;
    /* Fail after the Camera allocation, before history publication. */
    void *pressure=Heap_Alloc(HEAP_ID_FIELD1,remaining-12-0x68);
    HGHeapUsage failed_before,failed_after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&failed_before));
    GF_ASSERT(!hg_original_camera_new(parent_resources,target,0,1)&&!hg_original_camera_owner(parent_resources));
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&failed_after)&&failed_after.allocated==failed_before.allocated&&failed_after.blocks==failed_before.blocks);
    Heap_Free(pressure);
    GF_ASSERT(!hg_original_camera_new(parent_resources,target,17,1));
    for(unsigned type=0;type<17;type++){
        GF_ASSERT(hg_original_camera_new(parent_resources,target,type,1));camera=hg_original_camera_owner(parent_resources);
        GF_ASSERT(camera->target==hg_original_map_native(parent_resources,target,12)&&camera->history);
        const u8 *definition=hg_original_test_camera_template(parent_resources,type);const s32 *words=(const void *)definition;
        const u16 *half=(const void *)definition;
        GF_ASSERT(camera->distance==words[0]&&camera->angle[0]==half[2]&&camera->kind==definition[12]);
        GF_ASSERT(camera->perspective_angle==half[7]&&camera->perspective[3]==words[4]&&camera->perspective[4]==words[5]);
        GF_ASSERT(camera->look[3]==target->x+words[6]&&camera->look[4]==target->y+words[7]&&camera->look[5]==target->z+words[8]);
        s32 start[3];memcpy(start,camera->look+3,12);
        for(unsigned frame=1;frame<=10;frame++){
            target->x+=17;target->y+=frame*31;target->z-=23;
            GF_ASSERT(hg_original_camera_update(parent_resources)&&hg_original_camera_push(parent_resources));
            GF_ASSERT(camera->look[3]==start[0]+(s32)(17*frame)&&camera->look[5]==start[2]-(s32)(23*frame));
            /* Original ring begins overriding at read index 6: frame 8 replays
             * frame 2's Y delta; X/Z remain current because only Y is filtered. */
            unsigned sum=frame<=7?frame*(frame+1)/2:28+(frame-6)*(frame-5)/2-1;
            GF_ASSERT(camera->look[4]==start[1]+(s32)(31*sum));
        }
        const u16 angles[][4]={{0,0,0,0},{0,0x4000,0,0},{0x2001,0x4007,0,0},{0xdfff,0xffff,0,0}};
        for(unsigned i=0;i<NELEMS(angles);i++){
            GF_ASSERT(hg_original_test_camera_angles(parent_resources,0x123456,angles[i],i&1));
            s32 sine=reference_sine(angles[i][1]),cosine=reference_sine(angles[i][1]+0x4000);
            s32 cx=reference_sine(angles[i][0]+0x4000),ny=reference_sine((u16)-angles[i][0]);
            GF_ASSERT(camera->look[0]-camera->look[3]==reference_mul(reference_mul(sine,0x123456),cx));
            GF_ASSERT(camera->look[1]-camera->look[4]==reference_mul(ny,0x123456));
            GF_ASSERT(camera->look[2]-camera->look[5]==reference_mul(reference_mul(cosine,0x123456),cx));
        }
        GF_ASSERT(hg_original_camera_delete(parent_resources));
    }
    GF_ASSERT(hg_original_camera_new(parent_resources,target,0,1));camera=hg_original_camera_owner(parent_resources);
    const u8 *preset=hg_original_test_camera_preset(parent_resources);u16 pitch=*(const u16 *)preset;
    const s32 *delta=(const void *)(preset+4);u32 frames=*(const u32 *)(preset+16);GF_ASSERT(frames>0&&frames<=128);
    s32 origin[3];memcpy(origin,camera->look+3,12);u16 initial=camera->angle[0];
    GF_ASSERT(hg_original_test_camera_preset_start(parent_resources));
    for(unsigned frame=1;frame<=frames;frame++){
        GF_ASSERT(hg_original_camera_update(parent_resources)&&hg_original_camera_push(parent_resources));
        GF_ASSERT(camera->angle[0]==(u16)(initial+((s32)pitch-initial)*(s32)frame/(s32)frames));
    }
    for(unsigned i=0;i<3;i++)GF_ASSERT(camera->look[3+i]==origin[i]+delta[i]);
    GF_ASSERT(hg_original_camera_delete(parent_resources));
    /* Map transition: retire old target, recreate on the new owner; stale
     * references reject before ARM dereference and destruction remains legal. */
    GF_ASSERT(hg_original_camera_new(parent_resources,target,0,1));
    Heap_Free(target);GF_ASSERT(!hg_original_camera_push(parent_resources));GF_ASSERT(hg_original_camera_delete(parent_resources));
    target=Heap_Alloc(HEAP_ID_FIELD1,12);*target=(VecFx32){-40960,0,81920};
    GF_ASSERT(hg_original_camera_new(parent_resources,target,0,0));camera=hg_original_camera_owner(parent_resources);
    GF_ASSERT(!camera->history&&camera->target==hg_original_map_native(parent_resources,target,12));
    VecFx32 *next=Heap_Alloc(HEAP_ID_FIELD1,12);*next=(VecFx32){900,800,700};
    GF_ASSERT(hg_original_camera_retarget(parent_resources,next));Heap_Free(target);
    s32 last=camera->look[3];next->x+=99;GF_ASSERT(hg_original_camera_push(parent_resources)&&camera->look[3]==last+99);
    GF_ASSERT(hg_original_camera_delete(parent_resources));Heap_Free(next);
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
    DSCamera math={0};const int32_t eye[]={0,0,0},up[]={0,4096,0},at[]={0,0,-4096},persp[]={2896,2896,4096,4096,40960};
    GF_ASSERT(ds_camera_look_at(&math,eye,up,at)&&ds_camera_perspective(&math,persp));
    const double point[]={1,1,-2};GF_ASSERT(ds_raster_project(&math,point,256,192,screen));GF_ASSERT(fabs(screen[0]-192)<1e-10&&fabs(screen[1]-48)<1e-10);
    const int32_t ortho[]={4096,-4096,-4096,4096,4096,40960};GF_ASSERT(ds_camera_ortho(&math,ortho));
    const double p[]={0.5,0.5,-2};GF_ASSERT(ds_raster_project(&math,p,256,192,screen)&&fabs(screen[0]-192)<1e-10&&fabs(screen[1]-48)<1e-10);
    DSRasterTriangle tri={0};const double xy[3][2]={{-0.5,-0.5},{0.5,-0.5},{0,0.5}};
    for(unsigned i=0;i<3;i++){tri.v[i].x=xy[i][0];tri.v[i].y=xy[i][1];tri.v[i].z=-2;tri.v[i].color=0x7fff;}
    DSRasterMaterial material={.polygon=31<<16};u8 pixels[16*16*4];GF_ASSERT(ds_raster_draw(&tri,1,&material,1,&math,pixels,16,16));
    unsigned lit=0;for(unsigned i=0;i<256;i++)lit+=pixels[4*i]==255;GF_ASSERT(lit>0);
    fprintf(stderr,"Original camera: 17 templates, retained target/history, fixed-point angles, retarget/recreate and renderer projection verified\n");
}

/* The existing save/RTC owner supplies the sole clock; fixtures use its scoped
 * sample exchange, rather than a second field-light clock implementation. */
struct HGSaveRTC { RTCDate date;RTCTime time; };
const struct HGSaveRTC *hg_save_rtc_exchange(const struct HGSaveRTC *);
typedef struct {u32 time;u8 mask,pad;u16 color[4];s16 vector[12];u16 material[4],pad2;} TestLight;
_Static_assert(sizeof(TestLight)==0x30,"original light template");
static unsigned parse_light_fixture(unsigned archive,TestLight out[64]){
    const char *names[]={"area00light.txt","area01light.txt","area02light.txt","dun20_01light.txt","dun20_02light.txt"};
    const u8 *bytes;size_t length;GF_ASSERT(native_resource_file(names[archive],&bytes,&length));
    char *text=malloc(length+1);GF_ASSERT(text);memcpy(text,bytes,length);text[length]=0;
    char *save=NULL,*line=strtok_r(text,"\r\n",&save);unsigned count=0;
    while(line&&strcmp(line,"EOF")){
        GF_ASSERT(count<64);TestLight *t=out+count;memset(t,0,sizeof(*t));GF_ASSERT(sscanf(line,"%u,",&t->time)==1);
        for(unsigned light=0;light<4;light++){
            line=strtok_r(NULL,"\r\n",&save);int enabled,r,g,b,x,y,z;GF_ASSERT(line&&sscanf(line,"%d,%d,%d,%d,%d,%d,%d,",&enabled,&r,&g,&b,&x,&y,&z)==7);
            if(enabled){t->mask|=1u<<light;t->color[light]=r|(g<<5)|(b<<10);t->vector[3*light]=x;t->vector[3*light+1]=y;t->vector[3*light+2]=z;}
        }
        for(unsigned material=0;material<4;material++){
            line=strtok_r(NULL,"\r\n",&save);unsigned r,g,b;GF_ASSERT(line&&sscanf(line,"%u,%u,%u,",&r,&g,&b)==3);t->material[material]=r|(g<<5)|(b<<10);
        }
        count++;line=strtok_r(NULL,"\r\n",&save);
    }
    GF_ASSERT(line&&count);free(text);return count;
}
static void check_active_light(FieldSystem *field,const TestLight *t){
    const s16 *vectors=(const void *)field->modelAttributes;const u16 *colors=(const void *)((u8 *)field->modelAttributes+0x18);
    GF_ASSERT(!memcmp(vectors,t->vector,24)&&!memcmp(colors,t->color,8));
    GF_ASSERT(!memcmp((u8 *)field->modelAttributes+0x20,t->material,8));
}
static void probe_light(FieldSystem *field,int exhaustive){
    GF_ASSERT(field->areaLightManager);canonical_light_free(&field->areaLightManager);
    if(!exhaustive)return;
    struct HGSaveRTC sample={.date={26,10,24,6},.time={0,0,0}};
    const struct HGSaveRTC *previous=hg_save_rtc_exchange(&sample);
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    size_t remaining=before.capacity-before.allocated-before.reserved;
    void *pressure=Heap_Alloc(HEAP_ID_FIELD1,remaining-0x14);u8 preserved[0x48];memcpy(preserved,field->modelAttributes,sizeof(preserved));
    HGHeapUsage fail_before,fail_after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&fail_before));
    GF_ASSERT(!hg_original_light_new(parent_resources,field->modelAttributes,0));
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&fail_after)&&fail_after.allocated==fail_before.allocated&&fail_after.blocks==fail_before.blocks);
    GF_ASSERT(!memcmp(preserved,field->modelAttributes,sizeof(preserved)));Heap_Free(pressure);
    for(unsigned archive=0;archive<5;archive++){
        TestLight expected[64];unsigned count=parse_light_fixture(archive,expected);
        const char *files[]={"area00light.txt","area01light.txt","area02light.txt","dun20_01light.txt","dun20_02light.txt"};
        const u8 *raw;size_t length;GF_ASSERT(native_resource_file(files[archive],&raw,&length));
        if(length>=3&&!memcmp(raw+length-3,"EOF",3)){
            /* Original GetDelim reads beyond a bare EOF. Keep the bounds
             * rejection explicit until original heap backing is established. */
            u8 retained[0x48];memcpy(retained,field->modelAttributes,sizeof(retained));
            GF_ASSERT(!hg_original_light_new(parent_resources,field->modelAttributes,archive));
            GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"original loader instruction/memory/budget boundary"));
            GF_ASSERT(!memcmp(retained,field->modelAttributes,sizeof(retained)));
            GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
            continue;
        }
        sample.time=(RTCTime){0,0,0};field->areaLightManager=canonical_light_new(field->modelAttributes,archive);
        GF_ASSERT(!hg_original_light_new(parent_resources,field->modelAttributes,archive));
        u32 *manager=(void *)field->areaLightManager;GF_ASSERT(manager[0]==count);
        const TestLight *templates=hg_original_test_bytes(parent_resources,manager[1],48*count);GF_ASSERT(templates&&!memcmp(templates,expected,48*count));
        unsigned index=0;while(index<count&&expected[index].time==0)index++;if(index==count)index=0;
        GF_ASSERT(manager[2]==index);check_active_light(field,expected+index);
        sample.time=(RTCTime){23,59,59};
        for(unsigned frame=0;frame<count+2;frame++){
            if(count>1){u32 low=index?expected[index-1].time:0,high=expected[index].time;if(43199>=high||43199<low)index=(index+1)%count;}
            canonical_light_update(field->areaLightManager);GF_ASSERT(manager[2]==index);check_active_light(field,expected+index);
        }
        u32 owner=hg_original_map_native(parent_resources,manager,0x14),table=manager[1];
        canonical_light_free(&field->areaLightManager);GF_ASSERT(!hg_original_test_mapped(parent_resources,owner)&&!hg_original_test_mapped(parent_resources,table));
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
    }
    sample.time=(RTCTime){0,0,0};field->areaLightManager=canonical_light_new(field->modelAttributes,1);
    u32 selected=((u32 *)(void *)field->areaLightManager)[2];
    canonical_model_attributes_free(&field->modelAttributes);
    GF_ASSERT(!hg_original_light_update(parent_resources,field->areaLightManager));
    GF_ASSERT(((u32 *)(void *)field->areaLightManager)[2]==selected);
    field->modelAttributes=canonical_model_attributes_new();canonical_model_attributes_configure(field->modelAttributes);
    GF_ASSERT(!hg_original_light_update(parent_resources,field->areaLightManager));
    canonical_light_free(&field->areaLightManager);
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
    hg_save_rtc_exchange(previous);
    fprintf(stderr,"Original area light: two terminated resources executed; three bare-EOF resources reject with rollback; parser, clock selection, one-step updates and free lifetimes verified\n");
}

static u32 arm_word(const void *bytes,unsigned offset){const u8 *p=(const u8 *)bytes+offset;return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
static void probe_weather(FieldSystem *field,int comprehensive){
    HGHeapUsage before,after;unsigned tasks=hg_original_test_main_tasks(parent_resources),imports=weather_construction_imports;
    GF_ASSERT(tasks&&hg_original_weather_owner(parent_resources)==field->unk4->weatherManager);
    u8 *root=(u8 *)field->unk4->weatherManager;u32 nested=arm_word(root,0),root_g=hg_original_map_native(parent_resources,root,0x18);
    u8 *owner=hg_original_test_bytes(parent_resources,nested,0x10c);GF_ASSERT(owner);
    u32 headers=arm_word(owner,0x18),list=arm_word(owner,0xfc),renderer=nested+0x1c;
    const u8 *sprites=hg_original_test_bytes(parent_resources,list,0x124);GF_ASSERT(sprites&&arm_word(sprites,4)==64&&arm_word(sprites,0x114)==renderer);
    GF_ASSERT(arm_word(sprites,0x110)==list+0x10&&arm_word(sprites,0x10c)==list+0x10);
    GF_ASSERT(arm_word(sprites,0x120)==1&&hg_original_test_bytes(parent_resources,arm_word(sprites,0x118),4));
    const u8 *render=hg_original_test_bytes(parent_resources,renderer,0x98);GF_ASSERT(render&&arm_word(render,0x80)==0xfffff000u&&arm_word(render,0x70)==nested+0xb4);
    const u8 *surface=hg_original_test_bytes(parent_resources,nested+0xb4,0x48);GF_ASSERT(surface&&arm_word(surface,0x10)==1&&arm_word(surface,0x14)==1&&arm_word(surface,8)==0xff000&&arm_word(surface,12)==0xc0000);
    const unsigned members[]={0x39,0x3a,0x37,0x38};
    for(unsigned i=0;i<4;i++){
        const u8 *manager=hg_original_test_bytes(parent_resources,arm_word(owner,8+4*i),0x14),*header=hg_original_test_bytes(parent_resources,headers+0x10*i,0x10),*original;size_t size;
        GF_ASSERT(manager&&header&&arm_word(manager,8)==14&&!arm_word(manager,12)&&arm_word(manager,16)==i);
        GF_ASSERT(native_resource_member(NARC_a_0_6_3,0,members[i],&original,&size)&&size>=4);
        GF_ASSERT(arm_word(header,8)==arm_word(original,0)&&header[12]==1);
        unsigned count=arm_word(header,4);GF_ASSERT(count&&4+count*0x18<=size);
        const void *table=hg_original_test_bytes(parent_resources,arm_word(header,0),count*0x18);
        GF_ASSERT(table&&!memcmp(table,original+4,count*0x18));
    }
    GF_ASSERT(!hg_original_weather_new(parent_resources));
    if(comprehensive)for(unsigned i=0;i<3;i++)GF_ASSERT(hg_original_test_weather_task(parent_resources));
    GF_ASSERT(hg_original_weather_set(parent_resources,0)); /* Original same-weather early return. */
    void *p=root;GF_ASSERT(hg_original_weather_free(parent_resources,&p)&&!p);field->unk4->weatherManager=NULL;
    GF_ASSERT(!hg_original_test_mapped(parent_resources,root_g)&&!hg_original_test_mapped(parent_resources,nested)&&!hg_original_test_mapped(parent_resources,list)&&!hg_original_test_mapped(parent_resources,headers));
    GF_ASSERT(!hg_original_weather_free(parent_resources,&p)&&hg_original_test_main_tasks(parent_resources)==tasks-1);
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    if(comprehensive){
        unsigned at[]={1,2,3,10,25,50,100,150,imports-1,imports};
        for(unsigned i=0;i<NELEMS(at);i++){
            hg_original_owner_fail_import(parent_resources,at[i]);
            GF_ASSERT(!hg_original_weather_new(parent_resources));
            GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"injected owner construction failure"));
            GF_ASSERT(hg_original_test_main_tasks(parent_resources)==tasks-1&&hg_original_test_persistent_graphics_vblank(parent_resources)==1);
            GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.blocks==before.blocks&&after.allocated==before.allocated);
        }
        hg_original_owner_fail_import(parent_resources,0);
    }
    for(unsigned i=0;i<(comprehensive?2:1);i++){
        p=hg_original_weather_new(parent_resources);GF_ASSERT(p&&hg_original_test_main_tasks(parent_resources)==tasks);
        GF_ASSERT(hg_original_test_weather_task(parent_resources)&&hg_original_weather_free(parent_resources,&p));
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.blocks==before.blocks&&after.allocated==before.allocated);
    }
    fprintf(stderr,"Original weather owners: renderer/surface, four managers/header tables, 64 sprite slots, NAN relocation and Main callback/free/retry executed\n");
}

#include "original_2d_oam_tests.inc"

static void probe_field_texture(FieldSystem *field,int comprehensive){
    void *owner=field->unk4->textureManager;GF_ASSERT(owner==hg_original_field_texture_owner(parent_resources));
    u32 guest=hg_original_map_native(parent_resources,owner,0x18c),table=arm_word(owner,0);
    GF_ASSERT(table&&hg_original_test_bytes(parent_resources,table,4));
    GF_ASSERT(!hg_original_field_texture_destroy(parent_resources,owner));
    canonical_field_texture_clear(owner);GF_ASSERT(!hg_original_test_mapped(parent_resources,table));
    GF_ASSERT(arm_word(owner,0)==table); /* Original FreeAllSlots leaves this numeric word. */
    canonical_field_texture_destroy(owner);field->unk4->textureManager=NULL;GF_ASSERT(!hg_original_test_mapped(parent_resources,guest));
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    for(unsigned cycle=0;cycle<(comprehensive?3:1);cycle++){
        owner=hg_original_field_texture_new(parent_resources);GF_ASSERT(owner&&!hg_original_field_texture_new(parent_resources));
        u8 initial[0x18c];memcpy(initial,owner,sizeof(initial));
        if(comprehensive){
            HGHeapUsage allocated;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&allocated));
            hg_original_owner_fail_import(parent_resources,cycle+1);unsigned count;
            GF_ASSERT(!hg_original_field_texture_load(parent_resources,owner,canonical_area_texture(field->areaDataManager),&count));
            GF_ASSERT(!memcmp(initial,owner,sizeof(initial))&&hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==allocated.allocated&&after.blocks==allocated.blocks);
            hg_original_owner_fail_import(parent_resources,0);
        }
        unsigned count;GF_ASSERT(hg_original_field_texture_load(parent_resources,owner,canonical_area_texture(field->areaDataManager),&count));
        u32 table_g=arm_word(owner,0);const u8 *descriptors=hg_original_test_bytes(parent_resources,table_g,4);GF_ASSERT(descriptors&&count<=16);
        unsigned rows=arm_word(descriptors,0);descriptors=hg_original_test_bytes(parent_resources,table_g,4+(size_t)rows*0x34);GF_ASSERT(descriptors);
        const u8 *tex=(const u8 *)canonical_area_texture(field->areaDataManager);HGHeapAllocation backing;
        GF_ASSERT(hg_heap_allocation_info(tex,&backing));size_t prefix=backing.size-(size_t)(tex-(const u8 *)backing.base);
        unsigned dict=tex[14]|(unsigned)tex[15]<<8,pal=arm_word(tex,52);GF_ASSERT(dict+2<=prefix&&pal+2<=prefix);
        DSTextureBank bank={tex,prefix,tex[dict+1],tex[pal+1]};unsigned expected=0;
        for(unsigned row=0;row<rows;row++)for(unsigned index=0;index<bank.textures;index++){
            char name[17];GF_ASSERT(ds_texture_name(&bank,index,0,name));
            if(!memcmp(name,descriptors+4+row*0x34,16)){expected++;break;}
        }
        if(expected>16)expected=16;GF_ASSERT(count==expected);
        unsigned published=0;for(unsigned i=0;i<16;i++){
            u32 resource=arm_word(owner,8+24*i+12);
            if(resource){published++;GF_ASSERT(hg_original_test_bytes(parent_resources,resource,4)&&hg_original_test_bytes(parent_resources,arm_word(owner,8+24*i),1));}
        }
        GF_ASSERT(count==published);fprintf(stderr,"Original field texture selection: %u matching animation slots from %u descriptors\n",count,rows);
        GF_ASSERT(!hg_original_field_texture_load(parent_resources,owner,canonical_area_texture(field->areaDataManager),&count));
        GF_ASSERT(hg_original_field_texture_clear(parent_resources,owner)&&!hg_original_field_texture_clear(parent_resources,owner)&&hg_original_field_texture_destroy(parent_resources,owner));
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
    }
    fprintf(stderr,"Original field texture descriptor load/lookup, publication, rollback and component teardown/reuse checked\n");
}

static void probe_field_windows(FieldSystem *field,int comprehensive){
    FieldDrawMapNameInfo *info=field->unk4->drawMapNameInfo;
    GF_ASSERT(info&&info->bgConfig==field->bgConfig&&!info->task&&!info->onScreen);
    GF_ASSERT(info->window.bgConfig==field->bgConfig&&info->window.bgId==3&&info->window.width==32&&info->window.height==5&&info->window.baseTile==0x197&&info->window.paletteNum==11);
    size_t size;GF_ASSERT(hg_heap_allocation_size(info->window.pixelBuffer,&size)&&size==32*5*32);
    GF_ASSERT(info->mapNameString&&info->mapNameString->maxsize==22&&!info->mapNameString->size);
    GF_ASSERT(info->mapsecMsgData&&info->mapsecMsgData->type==1&&info->mapsecMsgData->file_id==279);
    GF_ASSERT(field->signpost&&!field->signpost->isActive);
    if(comprehensive){
#include "charmap.inc"
        const u8 *bank;size_t bank_size;GF_ASSERT(native_resource_member(HG_RESOURCE_ORIGINAL_MESSAGES,0,279,&bank,&bank_size)&&bank_size>=4);
        unsigned count=bank[0]|(unsigned)bank[1]<<8;
        HGMonString *text=hg_mon_string_new(512,HEAP_ID_FIELD1);
        for(unsigned i=0;i<count;i++){
            const u8 *utf8;size_t length;GF_ASSERT(native_resource_member(NARC_msgdata_msg,279,i,&utf8,&length));
            hg_field_message_read(info->mapsecMsgData->lazy,279,i,HEAP_ID_FIELD1,text);
            unsigned n=0;
            for(size_t k=0;k+1<length;){
                unsigned code=utf8[k++];if(code>=0xc0){unsigned next=code>=0xf0?3:code>=0xe0?2:1;code&=(1u<<(6-next))-1;while(next--){GF_ASSERT(k<length&&(utf8[k]&0xc0)==0x80);code=(code<<6)|(utf8[k++]&63);}}
                unsigned glyph=0;for(unsigned j=0;j<NELEMS(glyph_map);j++)if(glyph_map[j].unicode==code)glyph=glyph_map[j].glyph;
                GF_ASSERT(glyph&&n<text->size&&text->data[n++]==glyph);
            }
            GF_ASSERT(n==text->size&&text->data[n]==0xffff);
        }
        hg_mon_string_delete(text);
    }
    canonical_map_name_free(info);field->unk4->drawMapNameInfo=NULL;hg_field_signpost_delete(field->signpost);field->signpost=NULL;
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before));
    for(unsigned i=0;i<(comprehensive?8:1);i++){
        info=canonical_map_name_new(field->bgConfig);GF_ASSERT(info&&info->window.pixelBuffer&&!info->task);
        u32 guest=hg_original_map_native(parent_resources,info->window.pixelBuffer,5120);GF_ASSERT(guest);
        for(unsigned color=0;color<16;color++){hg_field_window_fill(&info->window,color);for(unsigned k=0;k<5120;k++)GF_ASSERT(((u8 *)info->window.pixelBuffer)[k]==17*color);}canonical_map_name_free(info);
        GF_ASSERT(!hg_original_test_mapped(parent_resources,guest));
        Signpost *sign=hg_field_signpost_new(HEAP_ID_FIELD1);GF_ASSERT(sign&&!sign->isActive&&!sign->window.pixelBuffer);hg_field_signpost_delete(sign);
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.blocks==before.blocks&&after.allocated==before.allocated);
    }
    if(comprehensive){
        u8 *external=Heap_Alloc(HEAP_ID_FIELD1,32);memset(external,0x6d,32);
        u32 guest=hg_original_map_native(parent_resources,external,32);GF_ASSERT(guest&&hg_heap_usage(HEAP_ID_FIELD1,&before));
        for(unsigned i=0;i<3;i++){
            window_fail_lease=1;GF_ASSERT(!canonical_map_name_new(field->bgConfig));window_fail_lease=0;
            GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&after.allocated==before.allocated&&after.blocks==before.blocks);
            GF_ASSERT(hg_original_test_mapped(parent_resources,guest));for(unsigned k=0;k<32;k++)GF_ASSERT(external[k]==0x6d);
            info=canonical_map_name_new(field->bgConfig);GF_ASSERT(info&&info->mapsecMsgData);canonical_map_name_free(info);
        }
        Heap_Free(external);GF_ASSERT(!hg_original_test_mapped(parent_resources,guest));
    }
    HGFieldWindow empty={0};BgConfig absent=*field->bgConfig;absent.bgs[3].tilemapBuffer=NULL;
    hg_field_window_add(&absent,&empty,3,0,0,32,5,11,0x197);GF_ASSERT(!empty.pixelBuffer&&!empty.bgConfig);
    fprintf(stderr,"Original field Window/String/lazy message owners: pixel allocation, encrypted bank279 decode, borrowed BG identity, removal and repeated lifetimes checked\n");
}

static u8 *sound_attribute(unsigned attr,unsigned bytes){
    uint32_t address;GF_ASSERT(hg_original_test_call(parent_resources,"GF_SdatGetAttrPtr",&attr,1,&address));
    u8 *p=hg_original_test_bytes(parent_resources,address,bytes);GF_ASSERT(p);return p;
}
static void probe_field_bgm(FieldSystem *field){
    struct HGSaveRTC sample={.date={26,10,25,0},.time={0,0,0}};
    const struct HGSaveRTC *previous=hg_save_rtc_exchange(&sample);
    SaveVarsFlags *flags=Save_VarsFlags_Get(field->saveData);
    u16 before=FieldBGM_GetOverride(field);int player=PlayerAvatar_GetState(field->playerAvatar);
    BOOL contest=CheckScriptFlag(flags,FLAG_BUG_CONTEST_ACTIVE),surf=CheckScriptFlag(flags,FLAG_UNK_99A);
    ClearScriptFlag(flags,FLAG_BUG_CONTEST_ACTIVE);ClearScriptFlag(flags,FLAG_UNK_99A);
    FieldBGM_SetOverride(field,SEQ_NONE);PlayerAvatar_SetState(field->playerAvatar,PLAYER_STATE_WALKING);
    for(unsigned hour=0;hour<24;hour++){
        sample.time.hour=hour;
        u16 expected=hour<4||hour>=20?MapHeader_GetNightMusicId(field->location->mapId):MapHeader_GetDayMusicId(field->location->mapId);
        GF_ASSERT(FieldBGM_GetForMapHeader(field,field->location->mapId)==expected);
        GF_ASSERT(FieldBGM_GetEffective(field,field->location->mapId)==expected);
    }
    SetScriptFlag(flags,FLAG_BUG_CONTEST_ACTIVE);
    GF_ASSERT(FieldBGM_GetForMapHeader(field,MAP_NATIONAL_PARK)==SEQ_GS_TAIKAIMAE_D5);
    GF_ASSERT(FieldBGM_GetForMapHeader(field,MAP_ROUTE_35_NATIONAL_PARK_POKEATHALON_GATEHOUSE)==SEQ_GS_TAIKAIMAE);
    ClearScriptFlag(flags,FLAG_BUG_CONTEST_ACTIVE);
    FieldBGM_SetOverride(field,SEQ_GS_BICYCLE);
    GF_ASSERT(FieldBGM_GetEffective(field,field->location->mapId)==SEQ_GS_BICYCLE);
    PlayerAvatar_SetState(field->playerAvatar,PLAYER_STATE_SURFING);
    GF_ASSERT(FieldBGM_GetEffective(field,field->location->mapId)==SEQ_GS_NAMINORI);
    SetScriptFlag(flags,FLAG_UNK_99A);
    GF_ASSERT(FieldBGM_GetEffective(field,field->location->mapId)==SEQ_GS_BICYCLE);
    PlayerAvatar_SetState(field->playerAvatar,player);FieldBGM_SetOverride(field,before);
    if(!surf)ClearScriptFlag(flags,FLAG_UNK_99A);if(contest)SetScriptFlag(flags,FLAG_BUG_CONTEST_ACTIVE);
    hg_save_rtc_exchange(previous);
    GF_ASSERT(ds_audio_set_player_channels(parent_audio,7,0x3fff));
    GF_ASSERT(ds_audio_play(parent_audio,DS_AUDIO_BGM,1018)&&ds_audio_tick(parent_audio));
    DSAudio *external=ds_audio_clone(parent_audio);GF_ASSERT(external);
    /* Parent admission completed the original PREPARE sequence. Repetition
     * must follow Nintendo's same-scene suppression without changing owners. */
    u8 *scene=sound_attribute(21,1),*secondary=sound_attribute(22,1);
    const u8 *players=hg_original_test_bytes(parent_resources,hg_original_test_symbol("hg_sound_snd_player_sPlayer"),32*36);
    GF_ASSERT(players);u8 player_before[32*36];memcpy(player_before,players,sizeof(player_before));
    GF_ASSERT(*scene==4&&!*secondary);
    HGHeapUsage usage,after;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&usage));
    for(unsigned i=0;i<4;i++){
        uint32_t result=0xdeadbeef;
        GF_ASSERT(hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_SCENE,SEQ_GS_T_WAKABA,&result));
        GF_ASSERT(*scene==4&&!*secondary&&result==0); /* original same-scene suppression */
        GF_ASSERT(!memcmp(players,player_before,sizeof(player_before)));
        GF_ASSERT(ds_audio_player_channels(parent_audio,7)==0x3fff);
        GF_ASSERT(ds_audio_tick(parent_audio)&&ds_audio_tick(external));
        GF_ASSERT(!memcmp(ds_audio_pcm(parent_audio),ds_audio_pcm(external),DS_AUDIO_FRAMES*4));
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after)&&usage.blocks==after.blocks&&usage.allocated==after.allocated);
    }
    ds_audio_free(external);
    /* Exact original same-scene suppression is tested independently; this is
     * a source-state fixture, never evidence that a sequence has played. */
    *scene=4;uint32_t result=123;
    GF_ASSERT(hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_SCENE,SEQ_GS_T_WAKABA,&result)&&!result);
    *scene=4;
    uint32_t work;GF_ASSERT(hg_original_test_call(parent_resources,"GetSoundDataPointer",NULL,0,&work));
    /* The original SDK sound heap and archive persist across admitted scenes. */
    GF_ASSERT(hg_original_test_bytes(parent_resources,work,0xBEC88));

    const u8 *heap_handle=hg_original_test_bytes(parent_resources,work+0x94,4);
    const u8 *heap_begin=hg_original_test_bytes(parent_resources,work+0x98,16);
    const u8 *heap_end=hg_original_test_bytes(parent_resources,work+0xBEC77,1);
    GF_ASSERT(heap_handle&&heap_begin&&heap_end);

    u32 sdk_heap=(u32)heap_handle[0]|((u32)heap_handle[1]<<8)|
                 ((u32)heap_handle[2]<<16)|((u32)heap_handle[3]<<24);
    GF_ASSERT(sdk_heap==work+0x98);

    u32 sdk_free=0;
    GF_ASSERT(hg_original_test_call(parent_resources,"NNS_SndHeapGetFreeSize",
                                   &sdk_heap,1,&sdk_free));
    GF_ASSERT(sdk_free>0&&sdk_free<0xBEAE0);

    u32 sdk_seq=1018,sdk_param=0;
    GF_ASSERT(hg_original_test_call(parent_resources,"NNS_SndArcGetSeqParam",
                                   &sdk_seq,1,&sdk_param));
    GF_ASSERT(sdk_param);
    const u8 *sdk_record=hg_original_test_bytes(parent_resources,sdk_param,8);
    GF_ASSERT(sdk_record);
    GF_ASSERT(((u32)sdk_record[0]|((u32)sdk_record[1]<<8))==702);
    GF_ASSERT(sdk_record[2]==80&&sdk_record[3]==30&&
              sdk_record[4]==64&&sdk_record[5]==1);
    GF_ASSERT(!hg_original_sound_policy(parent_resources,(enum HGOriginalSoundPolicy)-1,0,NULL));
    GF_ASSERT(!hg_original_sound_policy(parent_resources,HG_SOUND_FIELD_GUARD,2,NULL));
    fprintf(stderr,"Original field BGM: all24 hours, save-flag/map/override/surf priority; original SDK software-state rollback and external PCM preservation checked; archive lookup initialized; active device playback unavailable\n");
}
static void invoke_field_vblank(void){
    gSystem.vBlankIntr(gSystem.vBlankIntrArg);
    if(getenv("HG_TRACE_ORIGINAL"))fprintf(stderr,"HG_ORIGINAL_TRACE {\"event\":\"native_callback_return\",\"symbol\":\"FieldMap_VBlankCallback\"}\n");
}
static void probe_field_publication(FieldSystem *field){
    DSVideo *video=hg_native_video();u8 before[2048];
    memcpy(before,field->bgConfig->bgs[3].tilemapBuffer,sizeof(before));
    u8 *pixels=field->bgConfig->bgs[3].tilemapBuffer;for(unsigned i=0;i<sizeof(before);i++)pixels[i]=(u8)(i*13+7);
    ScheduleBgTilemapBufferTransfer(field->bgConfig,3);
    ScheduleSetBgPosText(field->bgConfig,3,BG_POS_OP_SET_X,37);
    ScheduleSetBgPosText(field->bgConfig,3,BG_POS_OP_SET_Y,19);
    void *storage=sub_0205F1A0(field->mapObjectManager);u8 *pool=canonical_graphics_pool(storage);
    GF_ASSERT(pool);u8 old=pool[3];pool[3]=2;
    unsigned persistent=hg_original_test_persistent_graphics_vblank(parent_resources);
    /* Execute the published original callback, not an approximation of it. */
    GF_ASSERT(gSystem.vBlankIntr==FieldMap_VBlankCallback&&gSystem.vBlankIntrArg==field);
    invoke_field_vblank();
    GF_ASSERT(!pool[3]&&!field->bgConfig->bufferTransferScheduled&&!field->bgConfig->scrollScheduled);
    GF_ASSERT(video->engine[0][0x1c]==37&&video->engine[0][0x1e]==19);
    GF_ASSERT(!memcmp(video->vram+0x40000+0x1000,pixels,sizeof(before)));
    for(unsigned i=0;i<3;i++){pool[3]=2;invoke_field_vblank();GF_ASSERT(!pool[3]);}
    pool[3]=old;
    memcpy(pixels,before,sizeof(before));ScheduleBgTilemapBufferTransfer(field->bgConfig,3);
    ScheduleSetBgPosText(field->bgConfig,3,BG_POS_OP_SET_X,0);ScheduleSetBgPosText(field->bgConfig,3,BG_POS_OP_SET_Y,0);
    invoke_field_vblank();
    GF_ASSERT(hg_original_test_persistent_graphics_vblank(parent_resources)==persistent);
    fprintf(stderr,"Original FieldMap_VBlankCallback: scheduled BG scroll/tilemap publication, empty transfer queue, original OAM apply/reset and object-pool acknowledgement completed five times; no Main/input frame claimed\n");
}

static void probe_real_parent(FieldSystem *field){
    static unsigned parent_cycle;parent_cycle++;
    GF_ASSERT(!field->taskman&&!field->unk60&&!field->mapObjectManager&&!field->playerAvatar);
    GF_ASSERT(hg_scheduler_init());parent_resources=hg_original_resource_context_new();GF_ASSERT(hg_original_sound_bind(parent_resources,parent_audio));GF_ASSERT(!hg_original_sound_bind(parent_resources,parent_audio));callback_loader=parent_resources;gSystem.frameCounter=0;GF_ASSERT(hg_scheduler_start_vblank(VBlankCB_DmaTasksFramecounter));CallFieldTask_NewGame(field);paused_controller_updates=0;canonical_avatar_reads=canonical_id_reads=0;
    DSVideo *video=hg_native_video();
    ds_video_set_display(video,0,0xffffffff);ds_video_set_display(video,1,0xa5a5ffff);
    memset(video->vram+0x40000,0x9a,0x20000);
    video->engine[0][0x50]=0xff;video->engine[0][0x51]=0xff;
    video->engine[1][0x50]=0xff;video->engine[1][0x51]=0xff;
    Main_SetVBlankIntrCB(parent_vblank_sentinel,field);GF_ASSERT(gSystem.vBlankIntr==parent_vblank_sentinel);
    parent_probe=1;
    if(!setjmp(parent_rejected)){
        for(unsigned frame=0;frame<8;frame++)GF_ASSERT(!FieldSystem_Main(field));
        GF_ASSERT(FALSE);
    }
    parent_probe=0;
    fprintf(stderr,"Actual parent init boundary: %s; %u paused original controller updates\n",open_contract,paused_controller_updates);
    {
        unsigned screen_slot=UINT32_MAX;
        GF_ASSERT(hg_original_bottom_screen_select(
            parent_resources,field,field->bottomScreenType,&screen_slot));
        GF_ASSERT(screen_slot==0);
        fprintf(stderr,
            "Original Nintendo bottom-screen selector: type %d -> slot %u\\n",
            field->bottomScreenType,screen_slot);
    }
    /* HG_ORIGINAL_SCREEN_ENVIRONMENT_PROBE */
    {
        u32 callback=hg_original_test_symbol("ov01_021F69C0");
        u32 args[]={callback|1u,16u,10u,4u};
        u32 task=0;
        GF_ASSERT(callback);
        GF_ASSERT(hg_original_test_call(parent_resources,
            "CreateSysTaskAndEnvironment",args,4,&task));
        GF_ASSERT(task>=0x03200000u &&
                  task<0x03200000u+4u*160u &&
                  (task-0x03200000u)%4u==0);
        GF_ASSERT(hg_original_test_call(parent_resources,
            "DestroySysTaskAndEnvironment",&task,1,NULL));
        fprintf(stderr,
            "Original screen environment: created and destroyed guest task %08x\\n",
            task);
    }
    /* HG_ORIGINAL_BOTTOM_SCREEN_DISPATCH_PROBE */
    {
        const u32 guest_field = 0x03100000u;
        GF_ASSERT(!hg_original_test_call(
            parent_resources, "FieldSystem_InitBottomScreen",
            &guest_field, 1, NULL));
        const char *reason = hg_original_loader_error(parent_resources);
        GF_ASSERT(reason && !strcmp(reason, "HandleLoadOverlay"));
        fprintf(stderr,
            "Original bottom-screen dispatcher reached: %s\n", reason);
    }
    GF_ASSERT(open_contract&&!strcmp(open_contract,"FieldSystem_InitBottomScreen: canonical parent contract"));
    u32 sound_status=0,sound_free=0,sound_waiting=0;
    GF_ASSERT(hg_original_test_call(parent_resources,"SND_GetPlayerStatus",NULL,0,&sound_status));
    GF_ASSERT(hg_original_test_call(parent_resources,"SND_CountFreeCommand",NULL,0,&sound_free));
    GF_ASSERT(hg_original_test_call(parent_resources,"SND_CountWaitingCommand",NULL,0,&sound_waiting));
    GF_ASSERT(sound_status==1&&sound_free==256&&!sound_waiting);
    GF_ASSERT(hg_scheduler_irq()->cpu_enabled&&hg_original_test_arm7_irq(parent_resources));
    GF_ASSERT(field->mapLoadType==0&&field->bottomScreenType==1);
    GF_ASSERT(field->unk4->weatherManager==hg_original_weather_owner(parent_resources));
    GF_ASSERT(gSystem.vBlankIntr==FieldMap_VBlankCallback&&gSystem.vBlankIntrArg==field);
    DSAudio *external_owner=parent_audio;DSThumb unrelated={0};
    unrelated.r[0]=0x03110000+4*canonical_object_index(PlayerAvatar_GetMapObject(field->playerAvatar));
    GF_ASSERT(!canonical_loader_object_import(field,"unrelated_import",&unrelated)&&parent_audio==external_owner);
    DSAudioSequenceInfo retained_info;GF_ASSERT(ds_audio_sequence_info(parent_audio,1018,&retained_info));
    probe_field_bgm(field);probe_field_publication(field);
    /* Abort cleanup of the newly published field callback precedes fixture
     * component destruction. The callback was explicitly exercised above;
     * recurring IRQ/Main/input frame scheduling is a separate milestone. */
    Main_SetVBlankIntrCB(NULL,NULL);
    GF_ASSERT(hg_original_test_persistent_graphics_vblank(parent_resources)==1);
    probe_field_texture(field,parent_cycle==1);
    probe_field_windows(field,parent_cycle==1);
    probe_weather(field,parent_cycle==1);
    probe_camera(field,parent_cycle==1);
    probe_light(field,parent_cycle==1);
    canonical_model_attributes_free(&field->modelAttributes);field->modelAttributes=canonical_model_attributes_new();canonical_model_attributes_configure(field->modelAttributes);
    probe_environment_records(field);
    GF_ASSERT(hg_original_test_persistent_graphics_vblank(parent_resources)==1);
    GF_ASSERT(canonical_avatar_reads==2&&canonical_id_reads==1);
    DSThumb invalid_avatar={0};invalid_avatar.r[0]=0x03104004;
    GF_ASSERT(canonical_loader_object_import(field,"PlayerAvatar_GetMapObject",&invalid_avatar)==-1);
    GF_ASSERT(canonical_avatar_reads==2);
    fprintf(stderr,"Original avatar map features: retained effect callback initialized through canonical player identity\n");
    GF_ASSERT(gSystem.vblankTaskQueue->limit==32);
    /* HG_BOTTOM_SCREEN_FRONTIER_DIAGNOSTIC */
    fprintf(stderr,
        "Parent state: paused=%u process=%p parent=%p running=%d\\n",
        paused_controller_updates,
        (void *)field->processManager,
        field->processManager ? (void *)field->processManager->parent : NULL,
        field->runningFieldMap);
    GF_ASSERT(paused_controller_updates==5&&field->processManager->parent&&!field->runningFieldMap);
    OverlayManager *parent=field->processManager->parent;
    GF_ASSERT(parent->template.init==FieldMap_Init&&parent->template.exec==FieldMap_Main&&parent->template.exit==FieldMap_Exit);
    GF_ASSERT(parent->exec_state==1&&parent->proc_state==FIELD_MAP_INIT_STATE_BOTTOM_SCREEN);
    GF_ASSERT(sub_0203DF7C(field)&&!sub_0203DF8C(field)&&!sub_020505C8(field));
    GF_ASSERT(parent->template.ovy_id==FS_OVERLAY_ID_NONE&&hg_overlay_live()==3);
    GF_ASSERT(OverlayManager_GetArgs(parent)==field&&field->unk4&&field->unk4->field3dObjectTaskManager);
    GF_ASSERT(!gSystem.vBlankIntr&&!gSystem.vBlankIntrArg);
    GF_ASSERT((ds_video_display(video,0)&0x3f0f1f0fu)==0x11b08);
    GF_ASSERT(ds_video_display(video,1)==((0xa5a5ffffu&~(0xe087u|0x300010u))|0x10u));
    GF_ASSERT(!video->engine[0][0x50]&&!video->engine[0][0x51]&&!video->engine[1][0x50]&&!video->engine[1][0x51]);
    GF_ASSERT(video->banks[0]==4&&video->banks[2]==128&&video->banks[4]==96&&video->banks[6]==256&&video->banks[8]==3&&video->banks[9]==16);
    GF_ASSERT(field->fieldEffectManager==hg_original_effects_owner(parent_resources));
    GF_ASSERT(!hg_original_effects_new(parent_resources,field,23,HEAP_ID_FIELD1)&&field->fieldEffectManager==hg_original_effects_owner(parent_resources));
    GF_ASSERT(hg_original_effects_reject_alias_free(parent_resources));
    unsigned mask=hg_original_effects_registered(parent_resources);
    fprintf(stderr,"Renderer mask after IRQ delivery: %08x; VBlank active %u; frameCounter %u\n",mask,gSystem.vblankTaskQueue->activeCount,gSystem.frameCounter);
    GF_ASSERT(mask==0x007fffffu);
    GF_ASSERT(gSystem.frameCounter>0&&!hg_scheduler_ticks()&&!hg_original_effects_uploads(parent_resources));
    GF_ASSERT(hg_original_effects_animation_probe(parent_resources));
    unsigned raw_count=hg_original_effects_raw_probe(parent_resources,2);fprintf(stderr,"Raw resources after all renderers: %u\n",raw_count);GF_ASSERT(raw_count==11);
    void *retired_pool=canonical_graphics_pool(sub_0205F1A0(field->mapObjectManager));
    GF_ASSERT(hg_original_objects_graphics_free(parent_resources));
    GF_ASSERT(!hg_original_objects_pool_publish(parent_resources,retired_pool));
    GF_ASSERT(!hg_original_objects_pool(parent_resources,sub_0205F1A0(field->mapObjectManager)));
    GF_ASSERT(!hg_original_objects_graphics_free(parent_resources));
    /* Keep the second session's original pending-IRQ cancellation regression;
     * only the first session drains external acquisitions for exact budgets. */
    if(parent_cycle==1) {
        probe_graphics_failures(field,MapObjectsToPreload_GetCount(field->mapObjectsToPreload)+3,
            MapObjectsToPreload_GetIDs(field->mapObjectsToPreload),hg_original_graphics_imports(parent_resources),raw_count);
        /* A separate preload fixture exercises the resource-owning constructor
         * branch; it does not run Main or change the original progression route. */
        int *preload=Heap_Alloc(HEAP_ID_FIELD1,4*sizeof(int));
        preload[0]=MapObject_GetSpriteID(PlayerAvatar_GetMapObject(field->playerAvatar));
        preload[1]=preload[2]=preload[3]=0xffff;
        int preload_ok=hg_original_objects_graphics_new(parent_resources,32,4,preload,10);
        if(!preload_ok)fprintf(stderr,"Preload constructor sprite %d: %s\n",preload[0],hg_original_loader_error(parent_resources));
        GF_ASSERT(preload_ok&&hg_original_test_graphics_raw_objects(parent_resources)>0);
        unsigned preload_imports=hg_original_graphics_imports(parent_resources);
        GF_ASSERT(hg_original_objects_graphics_free(parent_resources));
        probe_graphics_failures(field,4,preload,preload_imports,raw_count);
        Heap_Free(preload);
        size_t opaque_size;void *opaque=canonical_manager_storage(field,&opaque_size);
        u8 opaque_before[0x10c];GF_ASSERT(opaque_size==sizeof(opaque_before));memcpy(opaque_before,opaque,sizeof(opaque_before));
        NARC *external_narc=NARC_New(NARC_data_mmodel_mmodel,HEAP_ID_FIELD1);
        canonical_manager_narc(field,external_narc);
        GF_ASSERT(!hg_original_objects_graphics_new(parent_resources,32,0,NULL,10));
        GF_ASSERT(field->mapObjectManager->mapModelNarc==external_narc);
        canonical_manager_narc(field,NULL);NARC_Delete(external_narc);
        /* A real rejected acquisition must preserve an externally owned child heap
         * and its live mapped bytes, rather than destroy every heap created nearby. */
        GF_ASSERT(Heap_Create(HEAP_ID_FIELD1,63,128));
        u8 *external_child=Heap_Alloc(63,16);memset(external_child,0x6b,16);
        u32 external_guest=hg_original_map_native(parent_resources,external_child,16);GF_ASSERT(external_guest);
        GF_ASSERT(!hg_original_objects_graphics_new(parent_resources,32,
            MapObjectsToPreload_GetCount(field->mapObjectsToPreload)+3,
            MapObjectsToPreload_GetIDs(field->mapObjectsToPreload),10));
        GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"Heap_Create"));
        HGHeapUsage external_usage;GF_ASSERT(hg_heap_usage(63,&external_usage)&&external_usage.blocks==1&&external_usage.allocated==16);
        GF_ASSERT(hg_original_test_mapped(parent_resources,external_guest));
        for(unsigned i=0;i<16;i++)GF_ASSERT(external_child[i]==0x6b);
        GF_ASSERT(!memcmp(opaque_before,opaque,sizeof(opaque_before)));
        Heap_Free(external_child);GF_ASSERT(!hg_original_test_mapped(parent_resources,external_guest));Heap_Destroy(63);
    }
    /* Reinitialize the actual original graphics owner at the same fixed views. */
    HGHeapUsage graphics_before,graphics_after;
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&graphics_before));
    GF_ASSERT(!hg_original_objects_graphics_new(parent_resources,32,1,NULL,10));
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&graphics_after)&&graphics_before.blocks==graphics_after.blocks&&graphics_before.allocated==graphics_after.allocated);
    GF_ASSERT(hg_original_objects_graphics_new(parent_resources,32,
        MapObjectsToPreload_GetCount(field->mapObjectsToPreload)+3,
        MapObjectsToPreload_GetIDs(field->mapObjectsToPreload),10));
    GF_ASSERT(hg_original_objects_graphics_free(parent_resources));
    unsigned vwait=hg_original_effects_pending_vwait(parent_resources);
    unsigned pending=hg_original_effects_deferred(parent_resources);GF_ASSERT(vwait==14);
    if(parent_cycle==1){
        /* Finish actual display IRQ uploads, then exercise VWait transfer only
         * as a post-rejection resource probe, not an original field frame. */
        while(gSystem.vblankTaskQueue->activeCount)GF_ASSERT(hg_scheduler_wait_vblank());
        GF_ASSERT(hg_scheduler_stop_vblank());
        SysTaskQueue_RunTasks(gSystem.vwaitTaskQueue);
        GF_ASSERT(!hg_original_effects_deferred(parent_resources));
        GF_ASSERT(hg_original_effects_uploads(parent_resources)==vwait);
        GF_ASSERT(hg_original_effects_raw_probe(parent_resources,1)==raw_count);
        fprintf(stderr,"Original effect constructors returned: 23 including renderer 8; joint/color bindings retained; UV animation advanced; %u VWait completions and %u raw VBlank uploads\n",vwait,raw_count);
    }else{
        hg_original_effects_cancel_pending(parent_resources);
        GF_ASSERT(hg_scheduler_stop_vblank());
        GF_ASSERT(!hg_original_effects_deferred(parent_resources)&&!hg_original_effects_uploads(parent_resources));
        GF_ASSERT(hg_original_effects_raw_probe(parent_resources,2)==raw_count);
        fprintf(stderr,"Aborted effect construction: %u queued VBlank/VWait tasks removed before resource release\n",pending);
    }
    hg_original_effects_cancel_pending(parent_resources);
    if(field->unk4->unk18){sub_020556A8(field->unk4->unk18);field->unk4->unk18=NULL;}
    if(field->fieldEffectManager){if(!hg_original_effects_free(parent_resources)){fprintf(stderr,"Original effect destructor boundary: %s\n",hg_original_loader_error(parent_resources));GF_ASSERT(FALSE);}field->fieldEffectManager=NULL;}
    GF_ASSERT(!hg_original_effects_owner(parent_resources)&&!hg_original_effects_free(parent_resources));
    /* Force failure after the constructor's two private allocations: the
     * loader explicitly rejects NARC_New on heap 3. No owner may leak. */
    HGHeapUsage failed_before,failed_after;
    GF_ASSERT(hg_heap_usage(HEAP_ID_3,&failed_before));
    GF_ASSERT(!hg_original_effects_new(parent_resources,field,23,HEAP_ID_3));
    GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"NARC_New"));
    GF_ASSERT(hg_heap_usage(HEAP_ID_3,&failed_after)&&failed_before.blocks==failed_after.blocks&&failed_before.allocated==failed_after.allocated);
    for(unsigned repeat=0;repeat<2;repeat++) {
        GF_ASSERT(hg_original_effects_new(parent_resources,field,23,HEAP_ID_FIELD1));
        /* Borrowed fixed backing ends on native owner release, independently of
         * the graphics subsystem's destructor. A live replacement is rejected. */
        void *work=Heap_Alloc(HEAP_ID_FIELD2,80),*other=Heap_Alloc(HEAP_ID_FIELD2,80);
        u32 fixed=hg_original_object_work(parent_resources,63,work,80);
        GF_ASSERT(fixed==0x03213f00u);
        GF_ASSERT(!hg_original_object_work(parent_resources,63,other,80));
        Heap_Free(work);
        GF_ASSERT(hg_original_object_work(parent_resources,63,other,80)==fixed);
        Heap_Free(other);
        GF_ASSERT(hg_original_effects_free(parent_resources));
    }
    HGHeapUsage effect_heap;GF_ASSERT(!hg_heap_usage(68,&effect_heap)&&!hg_heap_usage(69,&effect_heap));
    /* Partial initialization cannot be passed to original Exit. Retire the real
     * original task manager and field heap explicitly before session retry. */
    HGHeapUsage before_duplicate,after_duplicate;
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&before_duplicate));
    GF_ASSERT(!hg_original_billboards_create(parent_resources,4,HEAP_ID_FIELD1));
    GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"GF_AssertFail"));
    GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&after_duplicate)&&before_duplicate.blocks==after_duplicate.blocks&&before_duplicate.allocated==after_duplicate.allocated);
    GF_ASSERT(hg_original_billboards_draw(parent_resources));BillboardLists_Delete();
    /* Reuse the same original BSS owner after its real Delete clears it. */
    BillboardLists_Create(4,HEAP_ID_FIELD1);
    GF_ASSERT(hg_original_billboards_draw(parent_resources));BillboardLists_Delete();
    GF_ASSERT(video->geometry_initialized&&(video->engine[0][8]&3)==1);
    GF_ASSERT(video->clear_depth==0x7fff&&video->clear_color==0x3f000000);
    GF_ASSERT(video->viewport[0]==0&&video->viewport[1]==0&&video->viewport[2]==255&&video->viewport[3]==191);
    GF_ASSERT(hg_native_texture_pool_active(0)&&hg_native_texture_pool_active(1));
    GF_ASSERT(field->areaDataManager&&field->renderObjManager&&field->mapPropAnimationManager&&field->unkC8&&field->unkCC&&field->unk104);
    GF_ASSERT(field->mapLoadManager==hg_original_map_owner(parent_resources)&&field->dynamicTerrainHeightManager&&field->mapPropManager);
    GF_ASSERT(field->mapObjectsToPreload&&field->mapObjectsToPreload->count<=MAX_MAP_OBJECTS_TO_PRELOAD);
    GF_ASSERT(hg_original_loader_bound_models(parent_resources)==59);
    for(unsigned frame=0;frame<4;frame++){
        GF_ASSERT(hg_original_loader_tick(parent_resources));
        GF_ASSERT(hg_original_loader_active_props(parent_resources)==8);
        GF_ASSERT(hg_original_loader_land_triangles(parent_resources)==260);
    }
    fprintf(stderr,"Original parent LOAD: managers, preload, area Load and InitialLoad completed; four resource ticks\n");
    GF_ASSERT(hg_native_texture_pool_used(0)>0&&hg_native_texture_pool_used(1)>0);
    GF_ASSERT(!hg_original_oam_create(parent_resources,(uint32_t[]){0,124,0,31,0,124,0,31,HEAP_ID_FIELD1}));
    GF_ASSERT(!strcmp(hg_original_loader_error(parent_resources),"GF_AssertFail"));
    if(parent_cycle==2){
        GF_ASSERT(hg_original_loader_texture(parent_resources,0));hg_native_texture_reset(0);
        GF_ASSERT(!hg_original_loader_texture(parent_resources,0));
    }
    GF_ASSERT(hg_original_oam_apply(parent_resources));Thunk_OamManager_Free();
    if(parent_cycle==1){
        /* Field ownership has ended. Use an explicitly bounded new owner on
         * both engines; the original field sub-manager reserves every slot. */
        hg_original_oam_module_init(parent_resources);
        GF_ASSERT(hg_original_oam_create(parent_resources,(u32[]){4,124,1,31,4,124,1,31,HEAP_ID_FIELD1}));
        probe_original_oam(parent_resources);Thunk_OamManager_Free();
    }
    for(unsigned e=0;e<2;e++)for(unsigned at=0;at<0x400;at+=2)GF_ASSERT(video->oam[e][at]==0xc0&&video->oam[e][at+1]==2);
    GF_ASSERT(field->unk120);for(unsigned i=0;i<0x884;i++)GF_ASSERT(!((u8 *)field->unk120)[i]);
    ov02_0224F8F4(field->unk120);field->unk120=NULL;
    GF_ASSERT(hg_original_loader_end(parent_resources));int ended=0;
    for(unsigned i=0;i<8&&!ended;i++){GF_ASSERT(hg_original_loader_tick(parent_resources));GF_ASSERT(hg_original_loader_has_ended(parent_resources,&ended));}GF_ASSERT(ended);
    hg_original_release_pools(parent_resources);field->unkC8=NULL;field->unkCC=NULL;field->mapPropAnimationManager=NULL;
    MapPropOneShotAnimationManager_Free(&field->mapPropOneShotAnimationManager);
    ov01_02204764(field->unk104);field->unk104=NULL;
    MapObjectsToPreload_Free(field->mapObjectsToPreload);field->mapObjectsToPreload=NULL;
    MapPropManager_Free(field->mapPropManager);field->mapPropManager=NULL;
    hg_original_loader_delete(parent_resources);parent_resources=NULL;callback_loader=NULL;
    GF_ASSERT(ds_audio_bgm(parent_audio)==1018&&ds_audio_playing(parent_audio,DS_AUDIO_BGM,1018));
    field->areaDataManager=NULL;field->renderObjManager=NULL;field->mapLoadManager=NULL;field->dynamicTerrainHeightManager=NULL;
    GF_ASSERT(!hg_native_texture_pool_used(0)&&!hg_native_texture_pool_used(1));
    GF_ASSERT(sObjCharTransferTasksManager&&sObjCharTransferTasksManager->max==20);
    GF_ASSERT(sObjCharTransferTasksManager->vramCapacityMain==0x8000&&sObjCharTransferTasksManager->vramCapacitySub==0x4000);
    GF_ASSERT(!sObjCharTransferTasksManager->freeSizeMain&&!sObjCharTransferTasksManager->freeSizeSub);
    GF_ASSERT(sObjPlttTransferTasksManager&&sObjPlttTransferTasksManager->numTasks==20);
    ov01_021E6214();GF_ASSERT(!sObjCharTransferTasksManager&&!sObjPlttTransferTasksManager);
    GF3dRender_DeleteSimpleManager();GF_ASSERT(!hg_native_texture_pool_active(0)&&!hg_native_texture_pool_active(1));
    GF_ASSERT(GF_GetNumPendingVramTransferTasks()==0);
    GF_RunVramTransferTasks();GF_DestroyVramTransferManager();
    Field3dObjectTaskManager_Delete(field->unk4->field3dObjectTaskManager);
    GF_ASSERT(field->bgConfig&&field->bgConfig->heapID==HEAP_ID_FIELD1);
    for(unsigned i=1;i<=3;i++){
        GF_ASSERT(field->bgConfig->bgs[i].tilemapBuffer&&field->bgConfig->bgs[i].bufferSize==2048);
        for(unsigned j=0;j<2048;j++)GF_ASSERT(!((u8 *)field->bgConfig->bgs[i].tilemapBuffer)[j]);
    }
    for(unsigned i=0;i<0x1800;i++)GF_ASSERT(!video->vram[0x40000+i]);
    GF_ASSERT(video->vram[0x40000+0x1800]==0x9a);
    for(unsigned i=0;i<32;i++)GF_ASSERT(!video->vram[0x40000+0x10000+i]&&!video->vram[0x40000+0x14000+i]&&!video->vram[0x40000+0x8000+i]);
    ov01_021E6138(field->bgConfig);
    for(unsigned i=0;i<8;i++)GF_ASSERT(!field->bgConfig->bgs[i].tilemapBuffer);
    Heap_Free(field->bgConfig);field->bgConfig=NULL;
    Heap_Free(field->unk4);field->unk4=NULL;Heap_Destroy(HEAP_ID_FIELD1);
    OverlayManager_Delete(parent);field->processManager->parent=NULL;
    UnloadOverlayByID(FS_OVERLAY_ID(field));
    UnloadOverlayByID(FS_OVERLAY_ID(OVY_2));
    UnloadOverlayByID(FS_OVERLAY_ID(OVY_3));
    GF_ASSERT(!hg_overlay_live());
    for(TaskManager *task=field->taskman;task;task=task->prev)TaskManager_Jump(task,probe_done,NULL);
    GF_ASSERT(FieldSystem_RunTaskFrame(field)&&!field->taskman);
    PlayerAvatar_DeleteFromMap(field->playerAvatar);field->playerAvatar=NULL;
    MapObjectManager_Delete(field->mapObjectManager);field->mapObjectManager=NULL;
    sub_02053324(field);GF_ASSERT(hg_scheduler_shutdown());open_contract=NULL;
}

void SetLCRNGSeed(u32);

#include "original_sound_owner_tests.inc"
#include "original_sound_command_tests.inc"
#include "original_file_tests.inc"
int main(int argc,char **argv) {
    profile_phase("start");
    BOOL probe=argc==2&&!strcmp(argv[1],"--probe");
    BOOL sound_owners=argc==2&&!strcmp(argv[1],"--sound-owners");
    BOOL files=argc==2&&!strcmp(argv[1],"--files");
    BOOL sound_commands=argc==2&&!strcmp(argv[1],"--sound-commands");
    GF_ASSERT(argc==1||probe||sound_owners||files||sound_commands);
    GF_ASSERT(hg_heap_register_root(HEAP_ID_DEFAULT,1024*1024));
    if(sound_owners){probe_sound_owners();return 0;}
    if(files){probe_original_files();return 0;}
    if(sound_commands){probe_original_sound_commands();return 0;}
    GF_ASSERT(hg_heap_register_root(HEAP_ID_3,1024*1024));
    /* Native-only machine adaptation is explicit in generated provenance.
     * There is no protection module or DSProt success-returning provider. */
    GF_ASSERT(hg_overlay_register(FS_OVERLAY_ID(OVY_124)));
    GF_ASSERT(hg_overlay_register(FS_OVERLAY_ID(field)));
    GF_ASSERT(hg_overlay_register(FS_OVERLAY_ID(OVY_2)));
    GF_ASSERT(hg_overlay_register(FS_OVERLAY_ID(OVY_3)));
    GF_ASSERT(hg_overlay_register(FS_OVERLAY_ID(OVY_4)));
    SaveData *save=Heap_Alloc(HEAP_ID_DEFAULT,sizeof(*save));
    hg_save_manager_init_ram(save);Save_SetPositionToPlayerRoom(save);GF_ASSERT(hg_save_manager_validate_ram(save));
    SaveData *before=malloc(sizeof(*before));GF_ASSERT(before);memcpy(before,save,sizeof(*before));
    profile_phase("save_fixture");
    parent_audio=ds_audio_new();GF_ASSERT(parent_audio);
    probe_original_hblank();probe_original_vblank();
    for(unsigned attempt=0;attempt<2;attempt++) {
        UnkStruct_02111868_sub args={.saveData=save};
        OverlayManagerTemplate container={.ovy_id=FS_OVERLAY_ID_NONE};
        OverlayManager *man=OverlayManager_New(&container,&args,HEAP_ID_DEFAULT);
        open_contract=NULL;
        if(setjmp(rejected)) {
            fprintf(stderr,"original constructor blocked: %s\n",open_contract);return 2;
        }
        FieldSystem *field=FieldSystem_New(man);
        profile_phase("original_constructor");
        GF_ASSERT(field==OverlayManager_GetData(man));
        GF_ASSERT(field&&field->processManager&&!field->processManager->parent&&!field->processManager->child);
        GF_ASSERT(!field->processManager->isPaused&&!field->processManager->isDone);
        HGHeapUsage field2,field3,heap89;
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD2,&field2)&&field2.blocks==8);
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD3,&field3)&&!field3.blocks);
        GF_ASSERT(hg_heap_usage(HEAP_ID_89,&heap89)&&!heap89.blocks);
        GF_ASSERT(field2.capacity==0x1c000&&field3.capacity==0x4000&&heap89.capacity==0x570);
        GF_ASSERT(!hg_overlay_live()&&field->saveData==save);
        GF_ASSERT(field->location==LocalFieldData_GetCurrentPosition(Save_LocalFieldData_Get(save)));
        GF_ASSERT(field->mapMatrix&&field->mapEvents&&field->bagCursor&&field->unkA8&&field->unk108);
        GF_ASSERT(field->unk108->mon==NULL&&!field->unk108->isRegistered);
        GF_ASSERT(field->phoneRingManager&&field->phoneRingManager->sys==field);
        GF_ASSERT(field->phoneRingManager->saveData==save);
        GF_ASSERT(field->phoneRingManager->pokegearData==SaveData_Pokegear_Get(save));
        GF_ASSERT(field->phoneRingManager->savingsData==SaveData_GetPhoneCallPersistentState(save));
        GF_ASSERT(!field->phoneRingManager->active&&!field->phoneRingManager->ringing);
        GF_ASSERT(field->phoneRingManager->maxActiveDuration==30&&field->phoneRingManager->unk_varC==10);
        GF_ASSERT(!memcmp(before,save,sizeof(*save)));
        SetLCRNGSeed(0x12345678);
        RunInitScript(field);
        GF_ASSERT(Save_VarsFlags_CheckFlagInArray(Save_VarsFlags_Get(save),FLAG_HIDE_ELMS_LAB_OFFICER));
        GF_ASSERT(Save_VarsFlags_CheckFlagInArray(Save_VarsFlags_Get(save),FLAG_HIDE_VIOLET_KIMONO_GIRL));
        GF_ASSERT(GetScriptVar(Save_VarsFlags_Get(save),VAR_LOTO_NUMBER_LO)!=0);
        HGHeapUsage after_script;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD2,&after_script));
        GF_ASSERT(after_script.blocks==field2.blocks&&after_script.allocated==field2.allocated);
        /* Execute the first original new-game field-entry operation. */
        sub_02052F94(field,field->location);
        GF_ASSERT(field->location->mapId==MAP_NEW_BARK_PLAYER_HOUSE_2F);
        GF_ASSERT(field->location->x==6&&field->location->y==6);
        GF_ASSERT(field->mapEvents->num_warp_events>0);
        GF_ASSERT((u8 *)field->mapEvents->warp_events>=field->mapEvents->event_data);
        GF_ASSERT((u8 *)(field->mapEvents->warp_events+field->mapEvents->num_warp_events)
            <=field->mapEvents->event_data+sizeof(field->mapEvents->event_data));
        /* Exercise actual warp-coordinate selection, then the wild-data branch.
         * These are resource/lifetime checks, not gameplay progression. */
        Location entrance=*field->location;entrance.warpId=0;
        u16 warp_x=field->mapEvents->warp_events[0].x,warp_z=field->mapEvents->warp_events[0].z;
        sub_02052F94(field,&entrance);
        GF_ASSERT(field->location->x==warp_x&&field->location->y==warp_z);
        Location route={.mapId=MAP_ROUTE_29,.warpId=-1};
        sub_02052F94(field,&route);
        GF_ASSERT(MapHeader_HasWildEncounters(field->location->mapId));
        GF_ASSERT(field->mapEvents->wildEncounters.encounterRate_walking>0);
        Save_SetPositionToPlayerRoom(save);sub_02052F94(field,field->location);
        GF_ASSERT(!field->unk60);sub_02053284(field);
        GF_ASSERT(field->unk60&&field->mapLoadMode&&field->mapLoadMode->unk_4==0xc4000&&!field->terrainAttributes);
        GF_ASSERT(field->mapMatrix->width&&field->mapMatrix->height);
        GF_ASSERT(field->mapMatrix->data.headers[0]==field->location->mapId);
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD2,&after_script)&&after_script.blocks==field2.blocks);
        /* Original normal MapLoadMode packs {0,64,12} into unk_4. Guest
         * module BSS/code/stack are bootstrap storage, outside the field heap. */
        Heap_Create(HEAP_ID_3,HEAP_ID_FIELD1,field->mapLoadMode->unk_4);
        profile_phase("script_events_matrix");
        HGOriginalLoader *loader=hg_original_loader_new(field->mapMatrix,0);
        int32_t unavailable=123;GF_ASSERT(!hg_original_loader_terrain(loader,field->unk60,1,6*65536,6*65536,0,&unavailable)&&unavailable==123);
        GF_ASSERT(hg_original_loader_prepare_area(loader,field->location->mapId));
        const uint8_t *area_bytes,*texture_bytes;size_t area_size,texture_size;
        GF_ASSERT(native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(field->location->mapId),&area_bytes,&area_size)&&area_size==8);
        unsigned texture_id=area_bytes[2]|(unsigned)area_bytes[3]<<8;
        GF_ASSERT(native_resource_member(NARC_a_0_4_4,0,texture_id,&texture_bytes,&texture_size));
        DSTextureBank archive_bank;GF_ASSERT(ds_texture_bank(texture_bytes,texture_size,&archive_bank));
        GF_ASSERT(hg_original_loader_texture_count(loader)>0);
        GF_ASSERT(hg_original_loader_texture(loader,0)->bytes==archive_bank.bytes);
        GF_ASSERT(hg_original_loader_texture_shrinks(loader)>=2);
        for(unsigned t=0;t<hg_original_loader_texture_count(loader);t++) {
            const DSTextureBank *bank=hg_original_loader_texture(loader,t);GF_ASSERT(bank&&bank->textures);
            char tex[17],pal[17]="";DSTexture texture;unsigned rgba,sampled=0;
            GF_ASSERT(ds_texture_name(bank,0,0,tex));
            for(unsigned j=0;j<(bank->palettes?bank->palettes:1);j++) {
                if(bank->palettes)GF_ASSERT(ds_texture_name(bank,j,1,pal));
                if(ds_texture_find(bank,tex,pal,&texture)&&ds_texture_sample(&texture,0,0,0,&rgba)){sampled=1;break;}
            }
            GF_ASSERT(sampled);
        }
        GF_ASSERT(!hg_original_loader_error(loader));
        GF_ASSERT(hg_original_loader_bound_models(loader)>0);
        unsigned sampled_model=0;
        for(unsigned model=0;model<550&&!sampled_model;model++)for(unsigned mat=0;mat<32;mat++) {
            DSModelMaterial material;DSTexture texture;u32 rgba;
            if(hg_original_loader_model_material(loader,model,mat,&material,&texture)&&
                ds_texture_sample(&texture,0,0,material.texture_params,&rgba)){sampled_model=1;break;}
        }
        GF_ASSERT(sampled_model);
        double first_uv[6],next_uv[6];GF_ASSERT(hg_original_loader_animation_sample(loader,first_uv));
        hg_original_loader_tick_animations(loader);GF_ASSERT(hg_original_loader_animation_sample(loader,next_uv));
        GF_ASSERT(memcmp(first_uv,next_uv,sizeof(first_uv)));
        fprintf(stderr,"Original area Load complete: %u models bound\n",hg_original_loader_bound_models(loader));
        GF_ASSERT(hg_original_loader_initial_load(loader,field->location->x,field->location->y));
        GF_ASSERT(hg_original_loader_active_props(loader)>0);
        /* Regression for the enlarged image crossing the old fixed scratch
         * 0x0204B000: real terrain queries must preserve that image backing. */
        GF_ASSERT(hg_original_test_scratch_address()>=0x02000000u+hg_original_test_image_size());
        GF_ASSERT(hg_original_test_image_size()>0x4b304);
        u8 collision_before[0x304];void *old_scratch=hg_original_test_bytes(loader,0x0204b000,sizeof(collision_before));
        GF_ASSERT(old_scratch);memcpy(collision_before,old_scratch,sizeof(collision_before));
        int32_t attribute,height;
        if(!hg_original_loader_terrain(loader,field->unk60,0,6,6,0,&attribute)){fprintf(stderr,"attribute query: %s\n",hg_original_loader_error(loader));GF_ASSERT(FALSE);}
        if(!hg_original_loader_terrain(loader,field->unk60,1,6*65536,6*65536,0,&height)){fprintf(stderr,"height query: %s\n",hg_original_loader_error(loader));GF_ASSERT(FALSE);}
        const u8 *land_bytes;size_t land_size;HGLandChunk land;
        GF_ASSERT(native_resource_member(NARC_fielddata_landdata_land_data,0,MapMatrix_GetMapModelNo(0,field->mapMatrix),&land_bytes,&land_size)&&hg_land_decode(land_bytes,land_size,&land));
        GF_ASSERT(attribute==(land.attributes[2*(6+6*32)]|land.attributes[2*(6+6*32)+1]<<8));
        int32_t expected_height=-1;GF_ASSERT(hg_collision_height(land.collision,land.collision_size,0,0,(6-16)*65536,(6-16)*65536,&expected_height)==1&&height==expected_height);
        fprintf(stderr,"Original canonical terrain dispatch: attribute %x, height %d\n",attribute,height);
        int32_t transported_height=123;unsigned surface_kind=99,behavior=99;
        GF_ASSERT(hg_original_loader_height(loader,field->unk60,6*65536,6*65536,0,&transported_height,&surface_kind));
        GF_ASSERT(transported_height==height&&surface_kind==1);
        GF_ASSERT(hg_original_loader_behavior(loader,field->unk60,6,6,&behavior)&&behavior==(attribute&255));
        GF_ASSERT(!memcmp(collision_before,old_scratch,sizeof(collision_before)));
        fprintf(stderr,"Guest image/scratch regression: original bytes at old 0x0204B000 survive actual terrain queries; scratch=%08x image=%zu\n",hg_original_test_scratch_address(),hg_original_test_image_size());
        unsigned triangles=hg_original_loader_land_triangles(loader);GF_ASSERT(triangles>0);
        for(unsigned frame=0;frame<4;frame++) {
            GF_ASSERT(hg_original_loader_tick(loader));hg_original_loader_tick_animations(loader);
            GF_ASSERT(hg_original_loader_land_triangles(loader)==triangles);
            unsigned drawn=0;GF_ASSERT(!hg_original_loader_render_probe(loader,&drawn));
            GF_ASSERT(drawn==triangles);GF_ASSERT(!strcmp(hg_original_loader_error(loader),"ov01_021F3A3C"));
        }
        fprintf(stderr,"Original InitialLoad and four Tick frames: %u props, %u land triangles\n",hg_original_loader_active_props(loader),triangles);
        sub_02053324(field);
        profile_phase("area_loader_frames");
        probe_object_entry(field,loader);
        profile_phase("object_callbacks");
        if(!hg_original_loader_end(loader)){fprintf(stderr,"original End blocked: %s\n",hg_original_loader_error(loader));GF_ASSERT(FALSE);}
        GF_ASSERT(!hg_original_loader_land_triangles(loader)&&!hg_original_loader_active_props(loader));
        GF_ASSERT(!hg_original_loader_texture(loader,0)&&!hg_original_loader_end(loader));
        int ended=0;GF_ASSERT(hg_original_loader_has_ended(loader,&ended)&&ended);
        for(unsigned frame=0;frame<2;frame++){
            GF_ASSERT(hg_original_loader_tick(loader));
            ended=0;GF_ASSERT(hg_original_loader_has_ended(loader,&ended)&&ended);
        }
        fprintf(stderr,"Original teardown polling: End, HasEnded and two retained-owner Tick/HasEnded frames\n");
        unsigned retired=123;GF_ASSERT(!hg_original_loader_render_probe(loader,&retired)&&!retired);
        unavailable=123;GF_ASSERT(!hg_original_loader_terrain(loader,field->unk60,0,6,6,0,&unavailable)&&unavailable==123);
        transported_height=123;surface_kind=99;behavior=99;
        GF_ASSERT(!hg_original_loader_height(loader,field->unk60,6*65536,6*65536,0,&transported_height,&surface_kind));
        GF_ASSERT(transported_height==123&&surface_kind==99);
        GF_ASSERT(!hg_original_loader_behavior(loader,field->unk60,6,6,&behavior)&&behavior==99);
        hg_original_loader_delete(loader);
        HGHeapUsage loader_heap;GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD1,&loader_heap)&&!loader_heap.blocks);
        Heap_Destroy(HEAP_ID_FIELD1);
        GF_ASSERT(!field->unk60&&!field->mapLoadMode);
        profile_phase("loader_teardown");
        probe_real_parent(field);
        profile_phase("parent_load_cleanup");
        memcpy(before,save,sizeof(*before));
        FieldSystem_Delete(man);GF_ASSERT(!OverlayManager_GetData(man));
        OverlayManager_Delete(man);
        profile_phase("original_destructor");
        HGHeapUsage root;GF_ASSERT(hg_heap_usage(HEAP_ID_DEFAULT,&root)&&root.blocks==1&&!root.reserved);
        GF_ASSERT(hg_heap_usage(HEAP_ID_3,&root)&&!root.blocks&&!root.reserved);
        GF_ASSERT(!hg_heap_usage(HEAP_ID_FIELD2,&root)&&!hg_heap_usage(HEAP_ID_FIELD3,&root)&&!hg_heap_usage(HEAP_ID_89,&root));
        GF_ASSERT(!memcmp(before,save,sizeof(*save))&&hg_save_manager_validate_ram(save));
    }
    GF_ASSERT(hg_overlay_unregister(FS_OVERLAY_ID(OVY_124))&&!hg_overlay_live());
    free(before);hg_save_manager_release(save);
    Heap_Destroy(HEAP_ID_3);Heap_Destroy(HEAP_ID_DEFAULT);
    puts("original FieldSystem_New/native-adapted Init/RunInitScript/event+matrix load/Delete complete twice on the complete original save owner; FieldMap_Init entered; Main/Exit remain unexecuted");
    if(probe)fprintf(stderr,"P0 remains blocked at real FieldMap resource/ABI initialization\n");
    ds_audio_free(parent_audio);parent_audio=NULL;
    return probe?2:0;
}
