#include "ds_audio.h"
#include "compat/motion.h"
#include "compat/stats.h"
#include "compat/follow.h"
#include "motion_guest.h"
#include "field_transition.h"
#include "ds_thumb.h"
#include "metatile_behavior.h"
#include "field_data.h"
#include "constants/sndseq.h"
#include "field_guest.inc"
_Static_assert(sizeof(hg_field_guest_image)<=0x8000,"Guest image overlaps marshaled field state");
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define TASK_HANDLE 0x03060000u
#define TASK_ENV 0x0200b200u
#define CAMERA_HANDLE 0x03060004u
#define FADE_WORK 0x0200b000u
#define FADE_ENV 0x0200b100u
#define STATIC_HEIGHT 0x02110000u
#define DISPATCH_ADDRESS 0x02008200u
#define HANDLE 0x03000000u
#define FIELD_ADDRESS 0x02008000u
#define ATTRIBUTE_ADDRESS 0x02009000u
#define MANAGER_HANDLE 0x03010000u
#define OBJECT_BASE 0x03020000u
#define LOADER_HANDLE 0x03000004u
#define AI_ADDRESS 0x02008700u
#define MOVEMENT_ADDRESS 0x02008600u
#define SAVE_HANDLE 0x03030000u
#define STATS_HANDLE 0x03030004u
#define EFFECT_HANDLE 0x03040000u
#define EFFECT_MODEL 0x03040004u
#define EFFECT_ENV 0x02008800u
#define PLAYER_SAVE_HANDLE 0x03030008u
/* Only the original normal-field dispatch offsets are marshaled. No native
 * FieldSystem or object is cast to the DS serialized layout. */
typedef struct { PlayerAvatar *avatar; LocalMapObject *object;
    MapObjectManager *manager; HGFieldResources *field; VecFx32 *vector; HGFieldTile selected; const uint8_t *backgrounds; unsigned background_count; int selected_valid; int static_environment;
    uint32_t *facing_handle;HGMotionEffect *effect;int effect_update;
    HGMotionSession *session;uint16_t input_keys;
    int *location_output;const uint8_t *door_list;HGMotionTask *task;
    HGMotionFade *fade;FieldSystem *story_field;const uint8_t *coordinates;unsigned coordinate_count;
} MotionBoundary;
static int store(DSThumb *cpu,uint32_t address,uint32_t value,unsigned width) {
    if(address<0x02000000u+sizeof(hg_field_guest_image) ||
       (width!=1 && width!=2 && width!=4) || address%width)return 0;
    uint8_t *p=ds_translate(&cpu->memory,address,width);if(!p)return 0;
    for(unsigned i=0;i<width;i++)p[i]=(uint8_t)(value>>(8*i));return 1;
}
static int load_word(DSThumb *cpu,uint32_t address,uint32_t *value) {
    if(address&3)return 0;const uint8_t *p=ds_translate(&cpu->memory,address,4);if(!p)return 0;
    *value=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;return 1;
}
static int boundary(DSThumb *cpu, void *opaque) {
    MotionBoundary *b=opaque;const char *name=NULL;
    for(size_t i=0;i<sizeof(hg_field_guest_imports)/sizeof(*hg_field_guest_imports);i++)
        if(cpu->r[15]==hg_field_guest_imports[i].address) { name=hg_field_guest_imports[i].name;break; }
    if(cpu->r[15]==STATIC_HEIGHT && b->field && b->static_environment) {
        /* Static BDHC only; the original dynamic-platform selector is not run.
         * Walking queries intermediate positions as well as tile centers. */
        if(cpu->r[0]!=FIELD_ADDRESS || (int32_t)cpu->r[2]<0 || (int32_t)cpu->r[3]<0)return -1;
        HGFieldTile terrain;
        if(!hg_field_resources_tile(b->field,(int32_t)cpu->r[2]/65536,(int32_t)cpu->r[3]/65536,&terrain)||!hg_transition_supported(terrain.map_header))return -1;
        int32_t height=(int32_t)cpu->r[1];
        uint32_t status;
        if(!load_word(cpu,cpu->r[13],&status))return -1;
        int surface=hg_field_resources_height_at(b->field,(int32_t)cpu->r[2],(int32_t)cpu->r[3],0,(int32_t)cpu->r[1],&height);
        if(surface!=0&&surface!=1)return -1; /* Dynamic platforms remain unsupported. */
        /* No BDHC surface is an original status, used by offstage event actors.
         * sub_02061248 leaves their explicit event/script height unchanged. */
        if(status && !store(cpu,status,(uint32_t)surface,1))return -1;
        cpu->r[0]=(uint32_t)height;return 1;
    }
    if(b->task&&name) {
        HGMotionTask *task=b->task;
        if(!strcmp(name,"TaskManager_GetFieldSystem")&&cpu->r[0]==TASK_HANDLE){cpu->r[0]=FIELD_ADDRESS;return 1;}
        if(!strcmp(name,"TaskManager_GetEnvironment")&&cpu->r[0]==TASK_HANDLE){cpu->r[0]=TASK_ENV;return 1;}
        if(!strcmp(name,"Heap_AllocAtEnd")&&cpu->r[1]==sizeof(task->environment)){cpu->r[0]=TASK_ENV;return 1;}
        if(!strcmp(name,"Heap_Free")&&cpu->r[0]==TASK_ENV){task->active=0;return 1;}
        if(!strcmp(name,"TaskManager_Call")&&cpu->r[0]==TASK_HANDLE&&cpu->r[1]==(HG_FIELD_GUEST_ov01_02205B14|1)&&cpu->r[2]==TASK_ENV){task->callback=cpu->r[1]&~1u;task->active=1;return 1;}
        if((!strcmp(name,"FollowMon_IsActive")||!strcmp(name,"FollowMon_GetMapObject"))&&cpu->r[0]==FIELD_ADDRESS){
            LocalMapObject *follower=MapObjectManager_GetFirstActiveObjectByID(b->manager,obj_partner_poke);
            if(!strcmp(name,"FollowMon_IsActive")){cpu->r[0]=follower!=NULL;return 1;}
            if(!follower)return -1;
            for(unsigned i=0;i<b->manager->objectCount;i++)if(b->manager->objects+i==follower){cpu->r[0]=OBJECT_BASE+4*i;return 1;}
            return -1;
        }
        if(!strcmp(name,"ov01_0220329C")&&cpu->r[1]==3){
            unsigned index=(cpu->r[0]-OBJECT_BASE)/4;
            if(cpu->r[0]<OBJECT_BASE||(cpu->r[0]&3)||index>=b->manager->objectCount||MapObject_GetID(b->manager->objects+index)!=obj_partner_poke)return -1;
            task->retract=1;return 1; /* Host sprite presentation, original task owns the wait. */
        }
        if(!strcmp(name,"ov01_021F771C")&&cpu->r[0]==MANAGER_HANDLE){cpu->r[0]=CAMERA_HANDLE;return 1;}
        if(!strcmp(name,"sub_02023E78")&&cpu->r[0]==CAMERA_HANDLE){
            /* Original resets the camera's temporary handoff scale to unity.
             * This host camera stays centered on the player throughout. */
            uint32_t v;for(unsigned i=0;i<3;i++)if(!load_word(cpu,cpu->r[1]+4*i,&v)||v!=4096)return -1;
            return 1;
        }
    }
    if(b->door_list&&name) {
        if(!strcmp(name,"ov01_021FB904")){cpu->r[0]=0x03050000;return 1;}
        if(!strcmp(name,"NARC_ReadWholeMember")) {
            if(cpu->r[0]!=0x03050000||cpu->r[1]||cpu->r[2]<0x02008000)return -1;
            void *out=ds_translate(&cpu->memory,cpu->r[2],24);if(!out)return -1;memcpy(out,b->door_list,24);return 1;
        }
    }
    if(b->fade&&name) {
        if(!strcmp(name,"Heap_Alloc")) {
            if(cpu->r[1]!=sizeof(b->fade->environment))return -1;
            cpu->r[0]=FADE_ENV;return 1;
        }
        if(!strcmp(name,"Heap_Free"))return cpu->r[0]==FADE_ENV?1:-1;
        if(!strcmp(name,"SetMasterBrightness")) {
            int value=(int32_t)cpu->r[1];if(cpu->r[0]!=0||value< -16||value>16)return -1;
            b->fade->brightness=value;return 1;
        }
        if(!strcmp(name,"_s32_div_f")) {
            int32_t a=(int32_t)cpu->r[0],d=(int32_t)cpu->r[1];if(!d||(a==INT32_MIN&&d==-1))return -1;
            cpu->r[0]=(uint32_t)(a/d);cpu->r[1]=(uint32_t)(a%d);return 1;
        }
    }
    if(!name)return 0;
    if(b->effect&&name) {
        HGMotionEffect *effect=b->effect;
        if(!strcmp(name,"ov01_02200540")) {
            if(!b->object||cpu->r[0]!=HANDLE||cpu->r[1]!=0||cpu->r[2]!=1||effect->active)return -1;
            memset(effect,0,sizeof(*effect));effect->active=1;
            /* Scoped SDK allocation initializes the original update env. */
            effect->state[10]=6*4096;effect->state[16]=EFFECT_MODEL;
            VecFx32 v;MapObject_CopyPositionVector(b->object,&v);effect->position[0]=v.x;effect->position[1]=v.y;effect->position[2]=v.z;
            cpu->r[0]=EFFECT_HANDLE;return 1;
        }
        if(!strcmp(name,"sub_02023EA4")) {
            if(cpu->r[0]!=EFFECT_MODEL||cpu->r[1]>1||!effect->active)return -1;
            effect->visible=(int)cpu->r[1];return 1;
        }
        if(!strcmp(name,"sub_02068DB8")||!strcmp(name,"sub_02068DA8")) {
            if(cpu->r[0]!=EFFECT_HANDLE||!effect->active)return -1;
            for(unsigned i=0;i<3;i++) {
                if(!strcmp(name,"sub_02068DB8")){if(!store(cpu,cpu->r[1]+4*i,(uint32_t)effect->position[i],4))return -1;}
                else {uint32_t value;if(!load_word(cpu,cpu->r[1]+4*i,&value))return -1;effect->position[i]=(int32_t)value;}
            }
            return 1;
        }
        if(!strcmp(name,"ov01_022003F4")) {
            if(cpu->r[0]!=EFFECT_HANDLE||!effect->active)return -1;cpu->r[0]=effect->state[5]!=0;return 1;
        }
        if(!strcmp(name,"sub_02068B48")) {
            if(cpu->r[0]!=EFFECT_HANDLE||!effect->active||!effect->state[5])return -1;
            effect->active=effect->visible=0;return 1;
        }
    }
    if(b->story_field&&!strcmp(name,"FieldSystem_VarGet")&&cpu->r[0]==FIELD_ADDRESS) {
        unsigned var=cpu->r[1];if(var>=0x4170)return -1;
        cpu->r[0]=FieldSystem_VarGet(b->story_field,(u16)var);return 1;
    }
    if(b->field&&b->object&&!strcmp(name,"PlaySE")&&(cpu->r[0]==SEQ_SE_DP_DANSA||cpu->r[0]==SEQ_SE_DP_SUTYA2)) {
        MapObjectManager *manager=MapObject_GetManager(b->object);
        FieldSystem *field=manager?manager->fieldSystem:NULL;
        if(field&&field->native_audio&&!ds_audio_play(field->native_audio,DS_AUDIO_EFFECT,cpu->r[0]))return -1;
        return 1;
    }
    if(b->session && b->session->stats && !strcmp(name,"Save_GameStats_Get") && cpu->r[0]==SAVE_HANDLE) {
        /* Native save-owner lookup only; original SaveArray_Get is not run. */
        cpu->r[0]=STATS_HANDLE;return 1;
    }
    if(b->session && b->session->stats && !strcmp(name,"GameStats_Inc") && cpu->r[0]==STATS_HANDLE) {
        if(cpu->r[1]>1)return -1;
        cpu->r[0]=GameStats_Inc(b->session->stats,(int)cpu->r[1]);return 1;
    }
    if(b->avatar && !strcmp(name,"PlayerSaveData_CheckRunningShoes") && cpu->r[0]==PLAYER_SAVE_HANDLE) {
        cpu->r[0]=PlayerSaveData_CheckRunningShoes(PlayerAvatar_GetPlayerSaveData(b->avatar));return 1;
    }
    if(!strcmp(name,"Gymmick_CheckCollision") && b->static_environment && cpu->r[0]==FIELD_ADDRESS) {
        /* This admitted environment has no active gym gimmick. No other
         * environment is silently treated as having no collision provider. */
        cpu->r[0]=0;return 1;
    }
    if(b->field && !strcmp(name,"ov01_021F654C")) {
        if(cpu->r[0]!=LOADER_HANDLE)return -1;
        b->selected_valid=hg_field_resources_tile(b->field,(int32_t)cpu->r[1],(int32_t)cpu->r[2],&b->selected);
        if(b->selected_valid && !store(cpu,cpu->r[3],0,1))return -1;
        cpu->r[0]=b->selected_valid;return 1;
    }
    if(b->field && !strcmp(name,"ov01_021F65E4")) {
        if(cpu->r[0]!=LOADER_HANDLE || cpu->r[1]!=0 || !b->selected_valid)return -1;
        void *attributes=ds_translate(&cpu->memory,ATTRIBUTE_ADDRESS,2048);if(!attributes)return -1;
        memcpy(attributes,b->selected.land.attributes,2048);cpu->r[0]=ATTRIBUTE_ADDRESS;return 1;
    }
#define TILE_PREDICATE(n) if(!strcmp(name,#n)){if(cpu->r[0]>255)return -1;cpu->r[0]=n((uint8_t)cpu->r[0]);return 1;}
    TILE_PREDICATE(MetatileBehavior_IsJumpNorth)
    TILE_PREDICATE(MetatileBehavior_IsJumpSouth)
    TILE_PREDICATE(MetatileBehavior_IsJumpWest)
    TILE_PREDICATE(MetatileBehavior_IsJumpEast)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceNorth)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceSouth)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceWest)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceEast)
    TILE_PREDICATE(MetatileBehavior_IsDoor)
    TILE_PREDICATE(MetatileBehavior_IsSurfableWater)
    TILE_PREDICATE(sub_0205B700)
    TILE_PREDICATE(MetatileBehavior_IsSnow)
    TILE_PREDICATE(MetatileBehavior_IsVeryTallGrass)
    TILE_PREDICATE(MetatileBehavior_IsMud)
    TILE_PREDICATE(sub_0205BA54)
    TILE_PREDICATE(sub_0205B78C)
    TILE_PREDICATE(sub_0205BA30)
    TILE_PREDICATE(sub_0205BA24)
    TILE_PREDICATE(MetatileBehavior_IsRockClimbNorthSouth)
    TILE_PREDICATE(MetatileBehavior_IsRockClimbEastWest)
    TILE_PREDICATE(sub_0205B8F4)
    TILE_PREDICATE(sub_0205B918)
    TILE_PREDICATE(sub_0205B93C)
    TILE_PREDICATE(sub_0205B960)
    if(!strcmp(name,"GetMetatileBehavior_None")){cpu->r[0]=GetMetatileBehavior_None();return 1;}
    if(b->manager) {
        unsigned count=MapObjectManager_GetObjectCount(b->manager);
        LocalMapObject *objects=MapObjectManager_GetObjects2(b->manager);
        if(!strcmp(name,"MapObjectManager_GetFirstObjectWithXAndZ")&&cpu->r[0]==MANAGER_HANDLE) {
            LocalMapObject *found=MapObjectManager_GetFirstObjectWithXAndZ(b->manager,cpu->r[1],cpu->r[2],(BOOL)cpu->r[3]);
            if(!found){cpu->r[0]=0;return 1;}
            for(unsigned i=0;i<count;i++)if(objects+i==found){cpu->r[0]=found==b->object?HANDLE:OBJECT_BASE+4*i;return 1;}
            return -1;
        }
        if(!strcmp(name,"MapObjectManager_GetObjects2")&&cpu->r[0]==MANAGER_HANDLE) {
            cpu->r[0]=objects==b->object?HANDLE:OBJECT_BASE;return 1;
        }
        if(!strcmp(name,"MapObjectManager_GetFirstActiveObjectWithMovement1")&&cpu->r[0]==MANAGER_HANDLE) {
            LocalMapObject *found=MapObjectManager_GetFirstActiveObjectWithMovement1(b->manager);
            if(!found){cpu->r[0]=0;return 1;}
            for(unsigned i=0;i<count;i++)if(objects+i==found){cpu->r[0]=found==b->object?HANDLE:OBJECT_BASE+4*i;return 1;}
            return -1;
        }
        if(!strcmp(name,"MapObjectManager_GetObjectCount")&&cpu->r[0]==MANAGER_HANDLE) {
            cpu->r[0]=count;return 1;
        }
        if(!strcmp(name,"MapObjectArray_NextObject")) {
            if(cpu->r[0]&3)return -1;
            uint8_t *p=ds_translate(&cpu->memory,cpu->r[0],4);if(!p)return -1;
            uint32_t handle=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
            unsigned index=count;
            for(unsigned i=0;i<count;i++)if(handle==(objects+i==b->object?HANDLE:OBJECT_BASE+4*i)){index=i;break;}
            if(index==count)return -1;
            LocalMapObject *next=objects+index;MapObjectArray_NextObject(&next);
            uint32_t next_handle=next==b->object?HANDLE:OBJECT_BASE+4*(index+1);
            return store(cpu,cpu->r[0],next_handle,4)?1:-1;
        }
    }
    LocalMapObject *object=cpu->r[0]==HANDLE?b->object:NULL;
    if(!object && b->manager && cpu->r[0]>=OBJECT_BASE && !((cpu->r[0]-OBJECT_BASE)&3)) {
        unsigned index=(cpu->r[0]-OBJECT_BASE)/4;
        if(index<MapObjectManager_GetObjectCount(b->manager))object=MapObjectManager_GetObjects2(b->manager)+index;
    }
    if(object && b->manager && !strcmp(name,"MapObject_GetManager")){cpu->r[0]=MANAGER_HANDLE;return 1;}
    if(object && !strcmp(name,"MapObject_GetFlagsBitsMask")){cpu->r[0]=MapObject_GetFlagsBitsMask(object,cpu->r[1]);return 1;}
    if(object && !strcmp(name,"sub_0205F370")) {
        if(object!=b->object||cpu->r[1]>16)return -1;
        memcpy(ds_translate(&cpu->memory,AI_ADDRESS,16),sub_0205F370(object,cpu->r[1]),16);cpu->r[0]=AI_ADDRESS;return 1;
    }
    if(object && !strcmp(name,"sub_0205F394")){if(object!=b->object)return -1;cpu->r[0]=AI_ADDRESS;return 1;}
    if(object && !strcmp(name,"sub_0205F3C0")) {
        if(cpu->r[1]>16)return -1;
        memcpy(ds_translate(&cpu->memory,MOVEMENT_ADDRESS,16),sub_0205F3C0(object,(int)cpu->r[1]),16);
        cpu->r[0]=MOVEMENT_ADDRESS;return 1;
    }
    if(object && !strcmp(name,"sub_0205F3E4")){cpu->r[0]=MOVEMENT_ADDRESS;return 1;}
    if(object && !strcmp(name,"MapObject_IncrementMovementStep")){MapObject_IncrementMovementStep(object);return 1;}
    if(object && !strcmp(name,"sub_0205F484")) {
        /* Native renderer completion callback, not an original sprite/VRAM
         * callback. Unknown or absent callback owners are not silently skipped. */
        if(!object->unkC8)return -1;
        memcpy(sub_0205F3E4(object),ds_translate(&cpu->memory,MOVEMENT_ADDRESS,16),16);
        sub_0205F484(object);return 1;
    }
    if(object && !strcmp(name,"MapObject_TestFlagsBits")){cpu->r[0]=MapObject_TestFlagsBits(object,cpu->r[1]);return 1;}
    if(!strcmp(name,"FieldSystem_GetPlayerAvatar")&&b->avatar&&cpu->r[0]==FIELD_ADDRESS){cpu->r[0]=HANDLE;return 1;}
    if(!strcmp(name,"PlayerAvatar_GetSpriteByStateAndGender")){
        if(cpu->r[0]>14||cpu->r[1]>1)return -1;
        cpu->r[0]=PlayerAvatar_GetSpriteByStateAndGender(cpu->r[0],cpu->r[1]);return 1;
    }
    /* DS graphics teardown/recreation is replaced by the host's sprite owner.
     * Resource admission happens before running the private avatar transition. */
    if(object&&!strcmp(name,"ov01_021FA930")){MapObject_SetSpriteID(object,cpu->r[1]);return 1;}
    if(cpu->r[0]!=HANDLE && !object)return -1;
    if(b->avatar && !strcmp(name,"PlayerAvatar_GetMapObject")){
        LocalMapObject *player=PlayerAvatar_GetMapObject(b->avatar);
        if(player==b->object){cpu->r[0]=HANDLE;return 1;}
        if(!b->manager)return -1;
        for(unsigned i=0;i<b->manager->objectCount;i++)if(b->manager->objects+i==player){cpu->r[0]=OBJECT_BASE+4*i;return 1;}
        return -1;
    }
    if(b->avatar && !strcmp(name,"PlayerAvatar_GetPlayerSaveData")){cpu->r[0]=PLAYER_SAVE_HANDLE;return 1;}
    if(b->avatar && !strcmp(name,"PlayerAvatar_SetUnk28Unk2C")) {
        PlayerAvatar_SetUnk28Unk2C(b->avatar,(int32_t)cpu->r[1],(int32_t)cpu->r[2]);return 1;
    }
    if(b->avatar && !strcmp(name,"PlayerAvatar_ClearFlag6")){PlayerAvatar_ClearFlag6(b->avatar);return 1;}
    if(b->avatar && !strcmp(name,"PlayerAvatar_SetFlag6")){PlayerAvatar_SetFlag6(b->avatar);return 1;}
    if(b->avatar && !strcmp(name,"PlayerAvatar_SetUnk8")){PlayerAvatar_SetUnk8(b->avatar,cpu->r[1]);return 1;}
    if(b->field && b->object && !strcmp(name,"MapObject_GetFieldSystem")) {cpu->r[0]=FIELD_ADDRESS;return 1;}
#define GET_AVATAR(n) if(!strcmp(name,#n)&&b->avatar){cpu->r[0]=(uint32_t)n(b->avatar);return 1;}
    GET_AVATAR(PlayerAvatar_GetTransitionFlags)
    GET_AVATAR(PlayerAvatar_GetGender)
    GET_AVATAR(PlayerAvatar_GetUnk34)
#define SET_AVATAR(n) if(!strcmp(name,#n)&&b->avatar){n(b->avatar,cpu->r[1]);return 1;}
    SET_AVATAR(PlayerAvatar_SetTransitionFlags)
    SET_AVATAR(PlayerAvatar_SetState)
    SET_AVATAR(PlayerAvatar_SetUnk34)
#undef SET_AVATAR
    if(b->avatar&&!strcmp(name,"PlayerAvatar_ClearUnk24ClearFlag2")){PlayerAvatar_ClearUnk24ClearFlag2(b->avatar);return 1;}
    GET_AVATAR(PlayerAvatar_GetFacingDirection)
    GET_AVATAR(PlayerAvatar_GetXCoord)
    GET_AVATAR(PlayerAvatar_GetZCoord)
    GET_AVATAR(PlayerAvatar_GetState)
    GET_AVATAR(PlayerAvatar_GetPreviousXCoord)
    GET_AVATAR(PlayerAvatar_GetPreviousZCoord)
    GET_AVATAR(PlayerAvatar_GetPlayerMoveState)
    GET_AVATAR(PlayerAvatar_CheckFlag6)
    GET_AVATAR(PlayerAvatar_GetNextFacingDirection)
    GET_AVATAR(PlayerAvatar_GetUnk28)
    GET_AVATAR(PlayerAvatar_GetUnk2C)
#define GET_OBJECT(n) if(!strcmp(name,#n)&&object){cpu->r[0]=(uint32_t)n(object);return 1;}
    if(object && !strcmp(name,"MapObject_CopyPositionVector")) {
        VecFx32 v;MapObject_CopyPositionVector(object,&v);
        if(!store(cpu,cpu->r[1],(uint32_t)v.x,4)||!store(cpu,cpu->r[1]+4,(uint32_t)v.y,4)||
           !store(cpu,cpu->r[1]+8,(uint32_t)v.z,4))return -1;
        return 1;
    }
    if(object && !strcmp(name,"MapObject_GetParam")) {
        if(cpu->r[1]>2)return -1;
        cpu->r[0]=MapObject_GetParam(object,cpu->r[1]);return 1;
    }
    GET_OBJECT(MapObject_GetID)
    GET_OBJECT(MapObject_GetInitialX)
    GET_OBJECT(MapObject_GetInitialZ)
    GET_OBJECT(MapObject_GetXRange)
    GET_OBJECT(MapObject_GetYRange)
    GET_OBJECT(sub_02069E14)
    GET_OBJECT(sub_02069EAC)
    GET_OBJECT(sub_02069EC0)
    GET_OBJECT(sub_02069ED4)
    GET_OBJECT(MapObject_CheckFlag28)
    GET_OBJECT(MapObject_GetSpriteID)
    GET_OBJECT(MapObject_CheckFlag19Disabled)
    GET_OBJECT(MapObject_GetPositionVectorYCoordUInt)
    GET_OBJECT(sub_0205F8D0)
    GET_OBJECT(sub_0205F504)
    GET_OBJECT(MapObject_GetPreviousXCoord)
    GET_OBJECT(MapObject_GetPreviousZCoord)
    GET_OBJECT(MapObject_GetXCoord)
    GET_OBJECT(MapObject_GetYCoord)
    GET_OBJECT(MapObject_GetZCoord)
    GET_OBJECT(MapObject_GetMovementCommand)
    GET_OBJECT(MapObject_GetMovementStep)
    GET_OBJECT(MapObject_GetFacingDirection)
    GET_OBJECT(MapObject_CheckVisible)
    GET_OBJECT(MapObject_CheckIgnoreHeights)
    GET_OBJECT(MapObject_CheckFlag29)
    if(object && !strcmp(name,"MapObject_SetPositionFromXYZAndDirection")) {
        uint32_t facing;if(!load_word(cpu,cpu->r[13],&facing)||facing>3)return -1;
        MapObject_SetPositionFromXYZAndDirection(object,(int32_t)cpu->r[1],(int32_t)cpu->r[2],(int32_t)cpu->r[3],facing);return 1;
    }
    if(object && !strcmp(name,"MapObject_SetPositionFromVectorAndDirection")) {
        uint32_t x,y,z;if(!load_word(cpu,cpu->r[1],&x)||!load_word(cpu,cpu->r[1]+4,&y)||!load_word(cpu,cpu->r[1]+8,&z)||cpu->r[2]>3)return -1;
        VecFx32 v={(int32_t)x,(int32_t)y,(int32_t)z};MapObject_SetPositionFromVectorAndDirection(object,&v,cpu->r[2]);return 1;
    }
    if(object && !strcmp(name,"MapObject_SetPositionVector")) {
        uint32_t x,y,z;
        if(!load_word(cpu,cpu->r[1],&x)||!load_word(cpu,cpu->r[1]+4,&y)||!load_word(cpu,cpu->r[1]+8,&z))return -1;
        VecFx32 v={(int32_t)x,(int32_t)y,(int32_t)z};MapObject_SetPositionVector(object,&v);return 1;
    }
    if(object && !strcmp(name,"MapObject_SetFacingVector")) {
        uint32_t x,y,z;
        if(!load_word(cpu,cpu->r[1],&x)||!load_word(cpu,cpu->r[1]+4,&y)||!load_word(cpu,cpu->r[1]+8,&z))return -1;
        VecFx32 v={(int32_t)x,(int32_t)y,(int32_t)z};MapObject_SetFacingVector(object,&v);return 1;
    }
#define SET_OBJECT(n) if(!strcmp(name,#n)&&object){n(object,(int32_t)cpu->r[1]);return 1;}
    SET_OBJECT(MapObject_SetPreviousX)
    SET_OBJECT(MapObject_SetPreviousY)
    SET_OBJECT(MapObject_SetPreviousZ)
    SET_OBJECT(MapObject_AddCurrentX)
    SET_OBJECT(MapObject_AddCurrentY)
    SET_OBJECT(MapObject_AddCurrentZ)
    SET_OBJECT(MapObject_SetCurrentY)
    SET_OBJECT(MapObject_SetOrQueueFacing)
    SET_OBJECT(MapObject_SetFacingDirection)
    SET_OBJECT(MapObject_SetFlagsBits)
    SET_OBJECT(MapObject_ClearFlagsBits)
    SET_OBJECT(MapObject_SetMovementCommand)
    SET_OBJECT(MapObject_SetMovementStep)
    SET_OBJECT(sub_0205F328)
    SET_OBJECT(MapObject_ClearFlag18)
    SET_OBJECT(sub_02069E28)
    SET_OBJECT(sub_02069E50)
    SET_OBJECT(sub_02069E84)
    SET_OBJECT(sub_02069DC8)
#define OBJECT_ACTION(n) if(!strcmp(name,#n)&&object){n(object);return 1;}
    OBJECT_ACTION(MapObject_UnpauseMovement)
    OBJECT_ACTION(MapObject_PauseMovement)
    OBJECT_ACTION(MapObject_ClearSingleMovement)
    OBJECT_ACTION(MapObject_SetSingleMovement)
    OBJECT_ACTION(MapObject_ClearEndMovement)
    return -1;
}
static int run(MotionBoundary *b,uint32_t entry,uint32_t arg0,uint32_t arg1,uint32_t arg2,uint32_t arg3,uint32_t *result) {
    uint8_t *ram=calloc(1,65536);if(!ram)return 0;
    memcpy(ram,hg_field_guest_image,sizeof(hg_field_guest_image));
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,65536};
    cpu.code_start=0x02000000;cpu.code_size=HG_FIELD_GUEST_CODE_SIZE;
    cpu.return_address=0x02200001;cpu.r[14]=cpu.return_address;
    cpu.r[13]=0x0200f000;cpu.r[15]=entry;cpu.r[0]=arg0;cpu.r[1]=arg1;cpu.r[2]=arg2;cpu.r[3]=arg3;
    if(b->object)memcpy(ram+AI_ADDRESS-0x02000000u,sub_0205F394(b->object),16);
    if(b->object)memcpy(ram+MOVEMENT_ADDRESS-0x02000000u,sub_0205F3E4(b->object),16);
    if(b->field) {
        if(b->manager)store(&cpu,FIELD_ADDRESS+0x3c,MANAGER_HANDLE,4);
        store(&cpu,FIELD_ADDRESS+0x40,HANDLE,4);
        store(&cpu,FIELD_ADDRESS+0x2c,LOADER_HANDLE,4);
        store(&cpu,FIELD_ADDRESS+0x60,DISPATCH_ADDRESS,4);
        store(&cpu,DISPATCH_ADDRESS,b->static_environment?STATIC_HEIGHT+1:HG_FIELD_GUEST_sub_02054654+1,4);
        store(&cpu,DISPATCH_ADDRESS+4,HG_FIELD_GUEST_sub_020547D8+1,4);
    }
    if(b->task) {
        memcpy(ram+TASK_ENV-0x02000000u,b->task->environment,sizeof(b->task->environment));
        store(&cpu,FIELD_ADDRESS+0x10,TASK_HANDLE,4);
        LocalMapObject *follower=MapObjectManager_GetFirstActiveObjectByID(b->manager,obj_partner_poke);
        for(unsigned i=0;follower&&i<b->manager->objectCount;i++)if(b->manager->objects+i==follower)store(&cpu,FIELD_ADDRESS+0xe4,OBJECT_BASE+4*i,4);
    }
    if(b->session) {
        store(&cpu,FIELD_ADDRESS+0xc,SAVE_HANDLE,4);
        memcpy(ram+FIELD_ADDRESS-0x02000000u+0xe4,b->session->follower,32);
        store(&cpu,0x0200f000,b->input_keys,4); /* Fifth AAPCS argument. */
    }
    if(b->backgrounds) {
        if(b->background_count>100){free(ram);return 0;}
        memcpy(ram+0xa000,b->backgrounds,20*b->background_count);
        cpu.r[0]=FIELD_ADDRESS;store(&cpu,FIELD_ADDRESS+0x40,HANDLE,4);
    }
    if(b->coordinates) {
        if(b->coordinate_count>100){free(ram);return 0;}
        memcpy(ram+0xa000,b->coordinates,16*b->coordinate_count);
        store(&cpu,FIELD_ADDRESS+0x40,HANDLE,4);
    }
    if(b->vector) {
        cpu.r[0]=arg1;cpu.r[1]=arg2;cpu.r[2]=0x02008400;
        store(&cpu,0x02008404,(uint32_t)b->vector->y,4);
    }
    if(b->effect_update)memcpy(ram+EFFECT_ENV-0x02000000u,b->effect->state,sizeof(b->effect->state));
    if(b->fade){memcpy(ram+FADE_WORK-0x02000000u,b->fade->work,sizeof(b->fade->work));memcpy(ram+FADE_ENV-0x02000000u,b->fade->environment,sizeof(b->fade->environment));}
    cpu.boundary=boundary;cpu.boundary_user=b;
    DSThumbResult status=ds_thumb_run(&cpu,10000);
    int ok=status==DS_THUMB_RETURN && cpu.r[13]==0x0200f000;
    if(!ok) {
        const char *name="internal";
        for(size_t i=0;i<sizeof(hg_field_guest_imports)/sizeof(*hg_field_guest_imports);i++)
            if(cpu.r[15]==hg_field_guest_imports[i].address)name=hg_field_guest_imports[i].name;
        fprintf(stderr,"Motion guest failed: status=%d pc=%08x instruction=%04x provider=%s\n",
                status,cpu.r[15],cpu.fault_instruction,name);
        fprintf(stderr,"Guest fault address=%08x r0=%08x r1=%08x r2=%08x r4=%08x r6=%08x\n",cpu.fault_address,cpu.r[0],cpu.r[1],cpu.r[2],cpu.r[4],cpu.r[6]);
    }
    if(ok&&b->task)memcpy(b->task->environment,ram+TASK_ENV-0x02000000u,sizeof(b->task->environment));
    if(ok&&b->fade){memcpy(b->fade->work,ram+FADE_WORK-0x02000000u,sizeof(b->fade->work));memcpy(b->fade->environment,ram+FADE_ENV-0x02000000u,sizeof(b->fade->environment));}
    if(ok&&b->vector) {
        uint32_t x,y,z;
        ok=load_word(&cpu,0x02008400,&x)&&load_word(&cpu,0x02008404,&y)&&load_word(&cpu,0x02008408,&z);
        if(ok)*b->vector=(VecFx32){(int32_t)x,(int32_t)y,(int32_t)z};
    }
    if(ok&&b->effect_update)memcpy(b->effect->state,ram+EFFECT_ENV-0x02000000u,sizeof(b->effect->state));
    if(ok&&b->facing_handle)ok=load_word(&cpu,0x02008620,b->facing_handle);
    if(ok&&b->object)memcpy(sub_0205F394(b->object),ram+AI_ADDRESS-0x02000000u,16);
    if(ok&&b->object)memcpy(sub_0205F3E4(b->object),ram+MOVEMENT_ADDRESS-0x02000000u,16);
    if(ok&&b->session)memcpy(b->session->follower,ram+FIELD_ADDRESS-0x02000000u+0xe4,32);
    if(ok&&b->location_output){
        int output[5];for(unsigned i=0;i<5;i++){uint32_t value;if(!load_word(&cpu,0x0200a000+4*i,&value)){ok=0;break;}output[i]=(int32_t)value;}
        if(ok)memcpy(b->location_output,output,sizeof(output));
    }
    if(ok&&result)*result=cpu.r[0];free(ram);return ok;
}
int hg_motion_direction(PlayerAvatar *avatar,uint16_t keys,int *direction) {
    if(!avatar||!direction)return 0;
    MotionBoundary b={.avatar=avatar};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_0205DDD4,HANDLE,0,keys,0,&result))return 0;
    if(result!=UINT32_MAX&&result>3)return 0;
    *direction=(int32_t)result;return 1;
}
int hg_motion_clear_held(LocalMapObject *object) {
    if(!object)return 0;LocalMapObject saved=*object;MotionBoundary b={.object=object};
    if(run(&b,HG_FIELD_GUEST_MapObject_ClearHeldMovement,HANDLE,0,0,0,NULL))return 1;
    *object=saved;return 0;
}
int hg_motion_sync_previous(LocalMapObject *object) {
    if(!object)return 0;LocalMapObject saved=*object;MotionBoundary b={.object=object};
    if(run(&b,HG_FIELD_GUEST_sub_02060F78,HANDLE,0,0,0,NULL))return 1;
    *object=saved;return 0;
}
int hg_motion_update_height(HGFieldResources *field,LocalMapObject *object) {
    if(!field||!object)return 0;LocalMapObject saved=*object;
    MotionBoundary b={.object=object,.field=field,.static_environment=1};
    if(run(&b,HG_FIELD_GUEST_sub_02061070,HANDLE,0,0,0,NULL))return 1;
    *object=saved;return 0;
}
int hg_motion_settle(HGFieldResources *field,LocalMapObject *object) {
    if(!field||!object)return 0;LocalMapObject saved=*object;
    MotionBoundary b={.object=object,.field=field,.static_environment=1};uint32_t result;
    int ok=run(&b,HG_FIELD_GUEST_sub_02061070,HANDLE,0,0,0,&result);
    if(ok&&(result==1||(result==0&&MapObject_CheckIgnoreHeights(&saved))))return 1;
    *object=saved;return 0;
}
int hg_motion_grid_step(LocalMapObject *object,unsigned direction) {
    if(!object||direction>3)return 0;
    LocalMapObject saved=*object;MotionBoundary b={.object=object};
    if(run(&b,HG_FIELD_GUEST_sub_02060F24,HANDLE,direction,0,0,NULL))return 1;
    *object=saved;return 0;
}

int hg_motion_tile(HGFieldResources *field,int x,int z,HGMotionTile *out) {
    if(!field||!out)return 0;
    HGFieldTile tile;if(!hg_field_resources_tile(field,x,z,&tile))return 0;
    MotionBoundary b={.field=field};uint32_t wall,behavior;
    if(!run(&b,HG_FIELD_GUEST_sub_020548C0,FIELD_ADDRESS,(uint32_t)x,(uint32_t)z,0,&wall) ||
       !run(&b,HG_FIELD_GUEST_GetMetatileBehavior,FIELD_ADDRESS,(uint32_t)x,(uint32_t)z,0,&behavior))return 0;
    if(wall>1 || behavior>255)return 0;
    *out=(HGMotionTile){.wall=(int)wall,.behavior=(uint8_t)behavior};return 1;
}
int hg_motion_can_surf(PlayerAvatar *avatar,unsigned standing,unsigned facing,int *out) {
    if(!avatar||!out||standing>255||facing>255)return 0;
    MotionBoundary b={.avatar=avatar,.object=PlayerAvatar_GetMapObject(avatar)};uint32_t result;
    if(!b.object||!run(&b,HG_FIELD_GUEST_Field_PlayerCanSurfOnTile,HANDLE,standing,facing,0,&result)||result>1)return 0;
    *out=(int)result;return 1;
}
int hg_motion_rock_climb(unsigned facing,unsigned direction,int *out) {
    if(facing>255||direction>3||!out)return 0;MotionBoundary b={0};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_MetatileBehavior_IsRockClimbInDirection,facing,direction,0,0,&result)||result>1)return 0;
    *out=(int)result;return 1;
}
int hg_motion_terrain_barrier(HGFieldResources *field,LocalMapObject *object,unsigned direction,int *blocked) {
    if(!field||!object||!blocked||direction>3)return 0;
    LocalMapObject candidate=*object;
    HGMotionTile current,target;
    if(!hg_motion_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&current) ||
       !hg_motion_grid_step(&candidate,direction) ||
       !hg_motion_tile(field,MapObject_GetXCoord(&candidate),MapObject_GetZCoord(&candidate),&target))return 0;
    /* Refresh the original object's cached current behavior on a private copy. */
    sub_0205F4FC(&candidate,current.behavior);
    MotionBoundary b={.field=field,.object=&candidate};uint32_t directional;
    if(!run(&b,HG_FIELD_GUEST_sub_02060DEC,HANDLE,MapObject_GetXCoord(&candidate),
            MapObject_GetZCoord(&candidate),direction,&directional)||directional>1)return 0;
    *blocked=target.wall || directional;return 1;
}

int hg_motion_occupied(LocalMapObject *object,int x,int y,int z,int *occupied) {
    if(!object||!occupied)return 0;
    MapObjectManager *manager=MapObject_GetManager(object);
    if(!manager||!MapObjectManager_GetObjectCount(manager)||!MapObjectManager_GetObjects2(manager))return 0;
    unsigned count=MapObjectManager_GetObjectCount(manager);
    /* The bridge admits a bounded native object array, never arbitrary pointers. */
    if(count>1024)return 0;
    int found=0;for(unsigned i=0;i<count;i++)if(MapObjectManager_GetObjects2(manager)+i==object)found=1;
    if(!found)return 0;
    MotionBoundary b={.object=object,.manager=manager};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_02060BFC,HANDLE,(uint32_t)x,(uint32_t)y,(uint32_t)z,&result)||result>1)return 0;
    *occupied=(int)result;return 1;
}

int hg_motion_player_collision(HGFieldResources *field,PlayerAvatar *avatar,unsigned direction,unsigned *mask) {
    if(!field||!avatar||!mask||direction>3)return 0;
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);if(!object)return 0;
    MapObjectManager *manager=MapObject_GetManager(object);
    if(!manager||!MapObjectManager_GetObjectCount(manager)||MapObjectManager_GetObjectCount(manager)>1024)return 0;
    int found=0;LocalMapObject *objects=MapObjectManager_GetObjects2(manager);if(!objects)return 0;
    for(unsigned i=0;i<MapObjectManager_GetObjectCount(manager);i++)if(objects+i==object)found=1;
    if(!found || PlayerAvatar_GetState(avatar)>2)return 0;
    LocalMapObject saved=*object,candidate=*object;
    HGFieldTile current,target;
    if(!hg_field_resources_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&current) ||
       !hg_motion_grid_step(&candidate,direction) ||
       !hg_field_resources_tile(field,MapObject_GetXCoord(&candidate),MapObject_GetZCoord(&candidate),&target) ||
       !hg_transition_supported(current.map_header)||!hg_transition_supported(target.map_header))return 0;
    /* The static-height boundary cannot resolve a tile without BDHC support.
     * Reject it before guest dispatch rather than faulting during collision. */
    VecFx32 position;MapObject_CopyPositionVector(object,&position);int32_t target_height;
    if(hg_field_resources_height(field,MapObject_GetXCoord(&candidate),MapObject_GetZCoord(&candidate),0,position.y,&target_height)!=1)return 0;
    sub_0205F4FC(object,(uint8_t)current.attribute);
    MotionBoundary b={.avatar=avatar,.object=object,.manager=manager,.field=field,.static_environment=1};
    uint32_t result;int ok=run(&b,HG_FIELD_GUEST_sub_0205DA34,HANDLE,HANDLE,direction,0,&result);
    *object=saved;
    if(!ok || (result&~0x2fu))return 0;
    *mask=result;return 1;
}

int hg_motion_walk_step(HGFieldResources *field,PlayerAvatar *avatar,uint16_t keys,HGMotionMove *out) {
    if(!field||!avatar||!out||PlayerAvatar_GetState(avatar)!=0)return 0;
    int direction;if(!hg_motion_direction(avatar,keys,&direction))return 0;
    HGMotionMove move={.direction=direction};
    if(direction<0){*out=move;return 1;}
    if(!hg_motion_player_collision(field,avatar,(unsigned)direction,&move.collision_mask))return 0;
    if(move.collision_mask){*out=move;return 1;}
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar),candidate=*object;
    if(!hg_motion_grid_step(&candidate,(unsigned)direction))return 0;
    int x=MapObject_GetXCoord(&candidate),z=MapObject_GetZCoord(&candidate);VecFx32 vector;
    MapObject_CopyPositionVector(object,&vector);
    if(hg_field_resources_height(field,x,z,0,vector.y,&vector.y)!=1)return 0;
    MotionBoundary b={.vector=&vector};
    if(!run(&b,HG_FIELD_GUEST_sub_020611C8,HANDLE,(uint32_t)x,(uint32_t)z,0,NULL))return 0;
    MapObject_SetPositionVector(&candidate,&vector);
    MapObject_SetCurrentY(&candidate,(vector.y>>3)/4096);
    MapObject_SetFacingDirectionDirect(&candidate,(unsigned)direction);
    MapObject_SetNextFacingDirection(&candidate,(unsigned)direction);
    *object=candidate;move.moved=1;*out=move;return 1;
}

int hg_motion_walk_begin(HGFieldResources *field,PlayerAvatar *avatar,uint16_t keys,HGMotionMove *out) {
    if(!field||!avatar||!out||PlayerAvatar_GetState(avatar)!=0)return 0;
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);
    if(!object||!object->unkC8||MapObject_GetMovementCommand(object)!=255)return 0;
    int direction;if(!hg_motion_direction(avatar,keys,&direction))return 0;
    HGMotionMove move={.direction=direction};
    if(direction<0){*out=move;return 1;}
    if(!hg_motion_player_collision(field,avatar,(unsigned)direction,&move.collision_mask))return 0;
    if(!move.collision_mask) {
        /* Original normal-walk branch sub_0205D4B4 selects base 0x0c.
         * The native admission shell does not execute that full controller. */
        MotionBoundary selection={0};uint32_t command;
        if(!run(&selection,HG_FIELD_GUEST_sub_0206234C,(uint32_t)direction,12,0,0,&command)||command<12||command>15)return 0;
        MapObject_SetMovementCommand(object,command);
        MapObject_SetMovementStep(object,0);MapObject_ClearFlagsBits(object,0x20);
        move.moved=1;
    }
    *out=move;return 1;
}
int hg_motion_script_command(unsigned command) {
    /* Regular facing, walking, in-place, jumping and delay families all use
     * the original command table and shared position/height services. */
    return command<=66||command==69||command==70||command==71||command==72||command==75||(command>=76&&command<=83)||(command>=88&&command<=91)||command==100||command==102||command==104;
}
static int command_tick(HGFieldResources *field,LocalMapObject *object,int *complete,int scripted,HGMotionEffect *effect,int defer_reset) {
    if(!field||!object||!complete||!object->unkC8)return 0;
    HGFieldTile tile;
    if(!hg_field_resources_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&tile)||!hg_transition_supported(tile.map_header))return 0;
    unsigned command=MapObject_GetMovementCommand(object);
    if(!(scripted?hg_motion_script_command(command):((command>=12&&command<=15)||(command>=88&&command<=91)||(command>=56&&command<=59)))||MapObject_GetMovementStep(object)>(command==100?2u:1u))return 0;
    if(command==75&&!effect)return 0;
    LocalMapObject saved=*object;HGMotionEffect saved_effect={0};if(effect)saved_effect=*effect;
    MotionBoundary b={.object=object,.field=field,.static_environment=1,.effect=effect};uint32_t result;
    if(effect&&effect->active) {
        b.effect_update=1;
        if(!run(&b,HG_FIELD_GUEST_ov01_02200480,EFFECT_HANDLE,EFFECT_ENV,0,0,NULL)){*object=saved;*effect=saved_effect;return 0;}
        b.effect_update=0;
    }
    if(!run(&b,defer_reset?HG_FIELD_GUEST_sub_02062400:HG_FIELD_GUEST_sub_02062428,HANDLE,0,0,0,&result)||(!defer_reset&&result>1)){*object=saved;if(effect)*effect=saved_effect;return 0;}
    if(defer_reset)result=MapObject_TestFlagsBits(object,0x20);
    if(!defer_reset&&result && (saved.flags&0x10) && !run(&b,HG_FIELD_GUEST_MapObject_ClearHeldMovement,HANDLE,0,0,0,NULL)) {
        *object=saved;if(effect)*effect=saved_effect;return 0;
    }
    *complete=(int)result;return 1;
}
int hg_motion_walk_tick(HGFieldResources *field,LocalMapObject *object,int *complete) { return command_tick(field,object,complete,0,NULL,0); }
int hg_motion_script_tick(HGFieldResources *field,LocalMapObject *object,int *complete) { return command_tick(field,object,complete,1,NULL,0); }

int hg_motion_locomotion_begin(HGFieldResources *field,PlayerAvatar *avatar,uint16_t keys,HGMotionSession *session,HGMotionMove *out) {
    if(!field||!avatar||!session||!session->stats||!out||PlayerAvatar_GetState(avatar)!=0)return 0;
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);
    if(!object||!object->unkC8||MapObject_GetMovementCommand(object)!=255)return 0;
    int direction;if(!hg_motion_direction(avatar,keys,&direction))return 0;
    PlayerAvatar saved_avatar=*avatar;
    MotionBoundary b={.avatar=avatar,.object=object,.field=field,.manager=MapObject_GetManager(object),
        .static_environment=1,.session=session,.input_keys=keys};
    /* Original entry point resolves direction before recording the new axes. */
    if(!run(&b,HG_FIELD_GUEST_sub_0205CC4C,HANDLE,0,0,keys,NULL)){*avatar=saved_avatar;return 0;}
    HGMotionMove move={.direction=direction};
    if(direction<0){*out=move;return 1;}
    if(!hg_motion_player_collision(field,avatar,(unsigned)direction,&move.collision_mask)){*avatar=saved_avatar;return 0;}
    /* The original controller selects ledges as well as walking/running and
     * publishes the command to the avatar and follower queue together. */
    if(move.collision_mask && !(move.collision_mask&0x4)){*out=move;return 1;}
    LocalMapObject saved_object=*object;HGMotionSession saved_session=*session;
    HGFieldTile current;
    if(!hg_field_resources_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&current)) {
        *avatar=saved_avatar;return 0;
    }
    sub_0205F4FC(object,(uint8_t)current.attribute);
    unsigned size=GameStats_sizeof();uint8_t *saved_stats=malloc(size);
    if(!saved_stats){*object=saved_object;*avatar=saved_avatar;return 0;}memcpy(saved_stats,session->stats,size);
    int ok=run(&b,HG_FIELD_GUEST_sub_0205D4B4,HANDLE,HANDLE,(uint32_t)direction,0,NULL);
    unsigned command=MapObject_GetMovementCommand(object);
    if(!ok||!((command>=12&&command<=15)||(command>=88&&command<=91)||(command>=56&&command<=59))) {
        *object=saved_object;*avatar=saved_avatar;*session=saved_session;
        memcpy(session->stats,saved_stats,size);free(saved_stats);return 0;
    }
    free(saved_stats);move.moved=1;*out=move;return 1;
}

int hg_motion_background_event(PlayerAvatar *avatar,const uint8_t *backgrounds,unsigned count,unsigned *script) {
    if(!avatar||!backgrounds||!script||count>100)return 0;
    /* This path admits ordinary/visible background events. Hidden items require original
     * saved flag state and are deliberately excluded by the caller. */
    for(unsigned i=0;i<count;i++)if((backgrounds[20*i+2]|(unsigned)backgrounds[20*i+3]<<8)==2)return 0;
    MotionBoundary b={.avatar=avatar,.backgrounds=backgrounds,.background_count=count};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_GetInteractedBackgroundEventScript,FIELD_ADDRESS,0x0200a000,count,0,&result)||result>65535)return 0;
    *script=result;return 1;
}
int hg_motion_coordinate_event(FieldSystem *field,PlayerAvatar *avatar,const uint8_t *bytes,unsigned count,unsigned *script) {
    if(!field||!field->saveData||!avatar||!bytes||!script||count>100)return 0;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *p=bytes+16*i;unsigned var=p[14]|(unsigned)p[15]<<8;
        if(var>=0x4170)return 0; /* No live ScriptEnvironment special variables. */
    }
    MotionBoundary b={.avatar=avatar,.story_field=field,.coordinates=bytes,.coordinate_count=count};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_0203DE04,FIELD_ADDRESS,0x0200a000,count,0,&result)||result>65535)return 0;
    *script=result;return 1;
}
int hg_motion_facing_object(HGFieldResources *field,PlayerAvatar *avatar,LocalMapObject **out) {
    if(!field||!avatar||!out)return 0;
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);if(!object)return 0;
    MapObjectManager *manager=MapObject_GetManager(object);if(!manager)return 0;
    unsigned count=MapObjectManager_GetObjectCount(manager);LocalMapObject *objects=MapObjectManager_GetObjects2(manager);
    if(!count||count>1024||!objects||PlayerAvatar_GetFacingDirection(avatar)>3)return 0;
    unsigned member=0;for(unsigned i=0;i<count;i++)member|=objects+i==object;if(!member)return 0;
    uint32_t handle,result;MotionBoundary boundary={.object=object,.avatar=avatar,.manager=manager,.field=field,.facing_handle=&handle};
    if(!run(&boundary,HG_FIELD_GUEST_sub_0203DC64,FIELD_ADDRESS,0x02008620,0,0,&result)||result>1)return 0;
    if(!result){*out=NULL;return 1;}
    if(handle==HANDLE){*out=object;return 1;}
    if(handle<OBJECT_BASE||((handle-OBJECT_BASE)&3)||(handle-OBJECT_BASE)/4>=count)return 0;
    *out=objects+(handle-OBJECT_BASE)/4;return 1;
}
unsigned hg_field_guest_mom_spawn(void) {
    MotionBoundary boundary={0};uint32_t result;
    GF_ASSERT(run(&boundary,HG_FIELD_GUEST_GetMomSpawnId,0,0,0,0,&result)&&result<=65535);
    return result;
}

int hg_motion_script_effect_tick(HGFieldResources *field,LocalMapObject *object,HGMotionEffect *effect,int *complete) { return command_tick(field,object,complete,1,effect,0); }

int hg_motion_location_compatible(unsigned location,int *out) {
    if(location>65535||!out)return 0;MotionBoundary b={0};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_LocationIsDiamondPearlCompatible,location,0,0,0,&result)||result>1)return 0;
    *out=(int)result;return 1;
}
int hg_motion_map_location(unsigned type,unsigned offset,unsigned *out) {
    if(type>=3||offset>65535||!out)return 0;MotionBoundary b={0};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_02017FE4,type,offset,0,0,&result)||result>65535)return 0;
    *out=result;return 1;
}

int hg_motion_follow_tick(HGFieldResources *field,PlayerAvatar *avatar,LocalMapObject *follower,HGMotionSession *session,int init,unsigned movement) {
    if(!field||!avatar||!follower||!session||(movement!=48&&movement!=55&&movement!=56))return 0;
    LocalMapObject saved=*follower;HGMotionSession before=*session;
    MotionBoundary b={.field=field,.avatar=avatar,.object=follower,.manager=follower->manager,.session=session,.static_environment=1};
    uint32_t entry=init==2?HG_FIELD_GUEST_sub_020659CC:init?HG_FIELD_GUEST_sub_020658D4:movement==48?HG_FIELD_GUEST_sub_02065900:movement==55?HG_FIELD_GUEST_sub_02065938:HG_FIELD_GUEST_sub_02065968;
    if(!run(&b,entry,HANDLE,0,0,0,NULL)){*follower=saved;*session=before;return 0;}return 1;
}

/* Original follower sprite arc reads the movement controller's own phase and
 * frame counters; presentation needs no second animation clock. */
int hg_motion_follow_height(LocalMapObject *object,int32_t *height) {
    if(!object||!height)return 0;
    MotionBoundary b={.object=object};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_ov01_022054E0,HANDLE,0,0,0,&result))return 0;
    *height=(int32_t)result;return 1;
}

int hg_motion_follow_position(HGFieldResources *field,PlayerAvatar *avatar,LocalMapObject *follower,unsigned direction,unsigned facing) {
 if(!field||!avatar||!follower||direction>3||facing>3)return 0;
 MotionBoundary b={.field=field,.avatar=avatar,.object=PlayerAvatar_GetMapObject(avatar),.manager=follower->manager,.static_environment=1};
 for(unsigned i=0;i<b.manager->objectCount;i++)if(b.manager->objects+i==follower)
  return run(&b,HG_FIELD_GUEST_ov01_02205720,HANDLE,OBJECT_BASE+4*i,direction,facing,NULL);
 return 0;
}

BOOL MapObject_IsMovementPaused(LocalMapObject *object) {
 MotionBoundary b={.object=object};uint32_t result;GF_ASSERT(object&&run(&b,HG_FIELD_GUEST_MapObject_IsMovementPaused,HANDLE,0,0,0,&result)&&result<=1);return (BOOL)result;
}

int hg_motion_player_tick(HGFieldResources *field,LocalMapObject *object,int *complete) { return command_tick(field,object,complete,0,NULL,1); }

int hg_motion_apricorn_sprite(unsigned kind,int available,unsigned *out) {
    if(kind>=8||(available!=0&&available!=1)||!out)return 0;MotionBoundary b={0};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_020640A4,kind,(unsigned)available,0,0,&result))return 0;*out=result;return 1;
}

static int fade_run(HGMotionFade *fade,unsigned *result) {
    MotionBoundary boundary={.fade=fade};return run(&boundary,fade->type?HG_FIELD_GUEST_FadeFunc_01:HG_FIELD_GUEST_FadeFunc_00,FADE_WORK,0,0,0,result);
}
int hg_motion_fade_begin(HGMotionFade *fade,unsigned type,unsigned color,unsigned steps,unsigned speed) {
    if(!fade||fade->active||type>1||(color!=0&&color!=32767)||!steps||steps>3600||!speed||speed>60)return 0;
    HGMotionFade next={.active=1,.type=type};
    /* Original FadeWork: type, steps, frames/step, phase, LCD, allocation,
     * task fields, heap, RGB555 color, direction and brightness selector. */
    next.work[0]=(uint8_t)type;next.work[4]=(uint8_t)steps;next.work[5]=(uint8_t)(steps>>8);next.work[8]=(uint8_t)speed;
    next.work[36]=(uint8_t)color;next.work[37]=(uint8_t)(color>>8);
    unsigned result;if(!fade_run(&next,&result)||result)return 0;*fade=next;return 1;
}
int hg_motion_fade_tick(HGMotionFade *fade) {
    if(!fade||!fade->active)return 1;HGMotionFade next=*fade;unsigned result;
    if(!fade_run(&next,&result)||result>1)return 0;
    if(result)next.active=0;*fade=next;return 1;
}

int hg_motion_position_center(int x,int z,VecFx32 *position) {
    if(!position||x<0||z<0)return 0;
    MotionBoundary b={.vector=position};
    return run(&b,HG_FIELD_GUEST_sub_020611C8,HANDLE,(uint32_t)x,(uint32_t)z,0,NULL);
}

int hg_field_guest_spawn(unsigned spawn,int special,int output[5]) {
    if(!output||!spawn||spawn>30||(special!=0&&special!=1))return 0;
    int result[5];MotionBoundary b={.location_output=result};
    if(!run(&b,special?HG_FIELD_GUEST_GetSpecialSpawnWarpData:HG_FIELD_GUEST_GetDeathWarpData,spawn,0x0200a000,0,0,NULL))return 0;
    memcpy(output,result,sizeof(result));return 1;
}

int hg_motion_avatar_transition(PlayerAvatar *avatar) {
    const unsigned supported=(1u<<0)|(1u<<8)|(1u<<10)|(1u<<11);
    if(!avatar||!avatar->mapObject||(PlayerAvatar_GetTransitionFlags(avatar)&~supported)||PlayerAvatar_GetUnk34(avatar))return 0;
    PlayerAvatar candidate=*avatar;LocalMapObject object=*avatar->mapObject;PlayerSaveData save;
    candidate.mapObject=&object;
    if(avatar->playerSaveData){save=*avatar->playerSaveData;candidate.playerSaveData=&save;}
    MotionBoundary b={.avatar=&candidate,.object=&object};
    if(!run(&b,HG_FIELD_GUEST_Field_PlayerAvatar_ApplyTransitionFlags,HANDLE,0,0,0,NULL))return 0;
    *avatar->mapObject=object;if(avatar->playerSaveData)*avatar->playerSaveData=save;
    candidate.mapObject=avatar->mapObject;candidate.playerSaveData=avatar->playerSaveData;*avatar=candidate;return 1;
}

int hg_motion_door_sound(const uint8_t list[24],int opening,unsigned *sound) {
    if(!list||!sound||(opening!=0&&opening!=1)||list[4]<1||list[4]>4)return 0;
    MotionBoundary b={.door_list=list};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_GetDoorSE,FIELD_ADDRESS,0,(unsigned)opening,0,&result))return 0;
    *sound=result;return 1;
}

int hg_motion_handoff_begin(FieldSystem *field,HGMotionTask *task){
    if(!field||!task||task->active)return 0;
    HGMotionTask candidate={0};MotionBoundary b={.field=field->native_scene_resources,.avatar=field->playerAvatar,.object=PlayerAvatar_GetMapObject(field->playerAvatar),.manager=field->mapObjectManager,.task=&candidate,.static_environment=1};
    if(!run(&b,HG_FIELD_GUEST_ov01_02205AEC,FIELD_ADDRESS,0,0,0,NULL)||!candidate.active)return 0;
    *task=candidate;return 1;
}
int hg_motion_task_tick(FieldSystem *field,HGMotionTask *task){
    if(!task||!task->active)return 1;
    if(!field||task->callback!=HG_FIELD_GUEST_ov01_02205B14)return 0;
    MotionBoundary b={.field=field->native_scene_resources,.avatar=field->playerAvatar,.object=PlayerAvatar_GetMapObject(field->playerAvatar),.manager=field->mapObjectManager,.task=task,.static_environment=1};uint32_t result;
    if(!run(&b,task->callback,TASK_HANDLE,0,0,0,&result)||result>1)return 0;
    if(result)task->active=0;return 1;
}
