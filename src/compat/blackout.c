#include "compat/blackout.h"
#include "compat/save_manager.h"
#include "compat/field_tasks.h"
#include "compat/resources.h"
#include "ds_audio.h"
#include "field_transition.h"
#include "motion_guest.h"
#undef POKEHEARTGOLD_FIELD_TYPES_DEF_H
#include "field_types_def.h"
#include "save_local_field_data.h"
#include "constants/maps.h"
#include "constants/std_script.h"
#include "game_name.h"
struct HGBlackout {
    FieldSystem field;SaveData *save;Location location;DSAudio *audio;
    unsigned sound_ticks,display_ticks,display_phase,script;
    int brightness,waiting,acknowledged,restored,failed;
    char message[1024],error[192];
};
_Static_assert(offsetof(HGBlackout,field)==0,"Blackout field must remain first");
_Static_assert(sizeof(Location)==5*sizeof(int),"Original spawn records require five 32-bit fields");
static HGBlackout *owner(FieldSystem *field){return (HGBlackout *)(void *)field;}
static void failed(HGBlackout *n,const char *reason){n->failed=1;snprintf(n->error,sizeof(n->error),"Blackout: %s",reason);}
static void death_spawn(unsigned id,Location *out){int location[5];GF_ASSERT(hg_field_guest_spawn(id,0,location));memcpy(out,location,sizeof(location));}
static void special_spawn(unsigned id,Location *out){int location[5];GF_ASSERT(hg_field_guest_spawn(id,1,location));memcpy(out,location,sizeof(location));}
static void warp(TaskManager *task,const Location *location){
    HGBlackout *n=owner(TaskManager_GetFieldSystem(task));
    HGFieldLocation to={location->mapId,location->warpId,location->x,location->y,location->direction};
    if(!hg_transition_relocate_record(SaveArray_Get(n->save,SAVE_LOCAL_FIELD_DATA),&to)){failed(n,"destination records unavailable");return;}
    n->location=*LocalFieldData_GetCurrentPosition(SaveArray_Get(n->save,SAVE_LOCAL_FIELD_DATA));
}
void Save_VarsFlags_ClearHaveFollowerFlag(SaveVarsFlags *);
void Save_VarsFlags_SetFollowerTrainerNum(SaveVarsFlags *,u16);
static void clear_follower(FieldSystem *field){SaveVarsFlags *flags=SaveArray_Get(field->saveData,SAVE_FLAGS);Save_VarsFlags_ClearHaveFollowerFlag(flags);Save_VarsFlags_SetFollowerTrainerNum(flags,0);}
static HGBlackout *running_blackout;
static void fade_sound(int volume,int frames){HGBlackout *n=running_blackout;GF_ASSERT(n&&volume==0&&frames==20);n->sound_ticks=frames;ds_audio_fade(n->audio,volume,frames);}
static unsigned sound_timer(void){GF_ASSERT(running_blackout);return running_blackout->sound_ticks;}
static void stop_music(void){ds_audio_stop(running_blackout->audio,DS_AUDIO_BGM,0);}
static void brightness(int value,unsigned planes,unsigned screens){(void)planes;(void)screens;GF_ASSERT(running_blackout&&(value==-16||value==0));running_blackout->brightness=value;}
static BOOL display_task(TaskManager *task){
    HGBlackout *n=owner(TaskManager_GetFieldSystem(task));
    if(n->display_phase==0){n->brightness=-16+(int)++n->display_ticks*2;if(n->display_ticks==8){n->display_phase=1;n->display_ticks=0;n->waiting=1;}}
    else if(n->display_phase==1&&n->acknowledged){n->waiting=0;n->display_phase=2;}
    else if(n->display_phase==2){n->brightness=-(int)++n->display_ticks*2;if(n->display_ticks==8)return TRUE;}
    return FALSE;
}
static void draw(FieldSystem *field,TaskManager *task){
    HGBlackout *n=owner(field);const u8 *text;size_t length;unsigned id=n->location.mapId==MAP_NEW_BARK_PLAYER_HOUSE_1F?4:3;
    char name[64];const char token[]="{STRVAR_1 3, 0, 0}";
    if(!native_resource_member(NARC_msgdata_msg,203,id,&text,&length)||!length||text[length-1]||!hg_name_decode(hg_story_save_profile(n->save)->name,8,name,sizeof(name))){failed(n,"message resource unavailable");return;}
    const char *at=strstr((const char *)text,token);
    if(!at||strchr(at+sizeof(token)-1,'{')||memchr(text,'{',(size_t)(at-(const char *)text))){failed(n,"unsupported blackout message control");return;}
    int written=snprintf(n->message,sizeof(n->message),"%.*s%s%s",(int)(at-(const char *)text),text,name,at+sizeof(token)-1);
    if(written<0||(size_t)written>=sizeof(n->message)){failed(n,"blackout message too long");return;}
    TaskManager_Call(task,display_task,n);
}
static void restore(TaskManager *task){owner(TaskManager_GetFieldSystem(task))->restored=1;}
static void queue(TaskManager *task,unsigned script,void *object,void *args){HGBlackout *n=owner(TaskManager_GetFieldSystem(task));GF_ASSERT(!object&&!args&&n->restored&&!n->script&&(script==std_whited_out_to_mom||script==std_whited_out_to_pokecenter));n->script=script;}
#define GetDeathWarpData death_spawn
#define GetSpecialSpawnWarpData special_spawn
#define sub_020537A8 warp
#define FieldSystem_ClearFollowingTrainer clear_follower
#define Save_LocalFieldData_Get(save) ((LocalFieldData *)SaveArray_Get((save),SAVE_LOCAL_FIELD_DATA))
#define SaveArray_Party_Get(save) ((Party *)SaveArray_Get((save),SAVE_PARTY))
#define GF_SndStartFadeOutBGM fade_sound
#define GF_SndGetFadeTimer sound_timer
#define FieldBGM_Stop stop_music
#define SetBlendBrightness brightness
#define Blackout_DrawMessage draw
#define CallTask_RestoreOverworld restore
#define QueueScript queue
#define GetMomSpawnId hg_field_guest_mom_spawn
#define GXBlendPlaneMask unsigned
#define GX_BLEND_PLANEMASK_BD 32
#define GX_BLEND_PLANEMASK_OBJ 16
#define GX_BLEND_PLANEMASK_BG3 8
#define GX_BLEND_PLANEMASK_BG2 4
#define GX_BLEND_PLANEMASK_BG1 2
#define GX_BLEND_PLANEMASK_BG0 1
#define SCREEN_MASK_MAIN 1
#define SCREEN_MASK_SUB 2
#include "blackout_kernel.inc"
HGBlackout *hg_blackout_new(const SaveData *save,DSAudio *audio,char *error,size_t size){
    if(error&&size)error[0]=0;
    if(!save||!hg_save_manager_validate_ram(save)){if(error&&size)snprintf(error,size,"Invalid blackout save");return NULL;}
    const LocalFieldData *local=SaveArray_Const_Get(save,SAVE_LOCAL_FIELD_DATA);
    unsigned spawn=LocalFieldData_GetBlackoutSpawn((LocalFieldData *)local);
    int destination[5];if(!hg_field_guest_spawn(spawn,0,destination)){if(error&&size)snprintf(error,size,"Invalid blackout spawn %u",spawn);return NULL;}
    HGBlackout *n=calloc(1,sizeof(*n));if(!n)return NULL;n->save=malloc(sizeof(*save));if(!n->save){free(n);return NULL;}
    *n->save=*save;n->field.saveData=n->save;n->audio=audio;n->location=*LocalFieldData_GetCurrentPosition(SaveArray_Get(n->save,SAVE_LOCAL_FIELD_DATA));n->field.location=&n->location;
    FieldSystem_CreateTask(&n->field,Task_Blackout,NULL);return n;
}
int hg_blackout_tick(HGBlackout *n){if(!n||n->failed)return HG_BLACKOUT_FAILED;if(n->sound_ticks)n->sound_ticks--;HGBlackout *previous=running_blackout;running_blackout=n;hg_field_tasks_tick(&n->field);running_blackout=previous;return n->failed?HG_BLACKOUT_FAILED:n->waiting?HG_BLACKOUT_MESSAGE:hg_field_tasks_idle(&n->field)?HG_BLACKOUT_DONE:HG_BLACKOUT_RUNNING;}
static BOOL cancel_task(TaskManager *task){(void)task;return TRUE;}
void hg_blackout_free(HGBlackout *n){if(!n)return;for(TaskManager *task=n->field.taskman;task;task=task->prev)task->func=cancel_task;if(n->field.taskman)hg_field_tasks_tick(&n->field);free(n->save);free(n);}
int hg_blackout_ack(HGBlackout *n){if(!n||!n->waiting)return 0;n->acknowledged=1;return 1;}
const SaveData *hg_blackout_save(const HGBlackout *n){return n?n->save:NULL;}
unsigned hg_blackout_script(const HGBlackout *n){return n?n->script:0;}
const char *hg_blackout_message(const HGBlackout *n){return n?n->message:"";}
const char *hg_blackout_error(const HGBlackout *n){return n?n->error:"No blackout owner";}
int hg_blackout_brightness(const HGBlackout *n){return n?n->brightness:0;}

int hg_blackout_waiting(const HGBlackout *n){return n&&n->waiting;}
