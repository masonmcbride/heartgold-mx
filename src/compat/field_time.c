#include "compat/field_time.h"
#include "compat/save_manager.h"
#include "compat/resources.h"
#include "compat/phone.h"
#include "ds_audio.h"
#undef POKEHEARTGOLD_FIELD_TYPES_DEF_H
#include "field_types_def.h"
#include "save_local_field_data.h"
#include "map_header.h"
#include "save_pokegear.h"
#include "gear_phone.h"
#include "friend_group.h"
#include "unk_0202C730.h"
#include "unk_02031B0C.h"
#include "player_data.h"
#include "sys_vars.h"
#include "sys_flags.h"
#include "constants/trainer_data.h"
SaveVarsFlags *hg_story_save_vars(SaveData *);
u16 Pokedex_CountNationalDexOwned(Pokedex *);
#include "bug_contest_internal.h"
#include "compat/bug_contest.h"
#include "field_time_types.inc"
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <setjmp.h>
#define SAFARI_ENCOUNTER_SLOTS_LAND 0
#define NUM_SAFARI_ENCOUNTER_TYPES 5
#define TIMEOFDAY_WILD_MORN 0
#define TIMEOFDAY_WILD_MAX 3
typedef int TimeOfDayWildParam;
typedef struct GearPhoneRingManager GearPhoneRingManager;
struct HGFieldTime { GearPhoneRingManager *phone;BugContest *contest;int forced; };
static HGFieldTime *running_time;
static DSAudio *ring_audio;
static int ring_audio_failed;
static const HGTimeSample *host_sample;
static SysInfo *host_owner;
static jmp_buf time_trap;
static const char *time_error;
static void *time_allocations[128];
static unsigned time_allocation_count;
static void unsupported(const char *message) { time_error=message;longjmp(time_trap,1); }
static void *time_allocate(enum HeapID heap,u32 size) {
    (void)heap;
    if(time_allocation_count==NELEMS(time_allocations))unsupported("Field-time allocation limit");
    void *p=malloc(size);if(!p)unsupported("Out of memory updating field time");
    time_allocations[time_allocation_count++]=p;return p;
}
static void time_free(void *p) {
    if(!p)return;
    for(unsigned i=0;i<time_allocation_count;i++)if(time_allocations[i]==p){
        free(p);time_allocations[i]=time_allocations[--time_allocation_count];return;
    }
    unsupported("Unowned field-time allocation");
}
#define Heap_Alloc time_allocate
#define Heap_AllocAtEnd time_allocate
#define Heap_Free time_free
typedef struct { struct { u8 month,day; } birthday; } OSOwnerInfo;
static s64 OS_GetOwnerRtcOffset(void) { return host_sample->rtc_offset; }
static void OS_GetMacAddress(u8 *out) { memcpy(out,host_sample->identity,6); }
static void OS_GetOwnerInfo(OSOwnerInfo *out) { out->birthday.month=host_owner->birth_month;out->birthday.day=host_owner->birth_day; }
/* The existing native frame/audio service owns the ring task's cadence. */
static void GearPhone_ToggleRinging(GearPhoneRingManager *phone,BOOL toggle) {
    if(phone->ringing!=!!toggle)phone->gearRing.counter=0;
    phone->ringing=!!toggle;phone->gearRing.task=NULL;
}
static PhoneBook *AllocAndReadPhoneBook(enum HeapID heap) {
    unsigned count;const PhoneBookEntry *entries=hg_phone_entries(&count);
    PhoneBook *book=Heap_Alloc(heap,sizeof(*book));book->count=count;
    book->entries=Heap_Alloc(heap,count*sizeof(*entries));memcpy(book->entries,entries,count*sizeof(*entries));return book;
}
static void StartMapSceneScript(FieldSystem *field,u16 script,void *object) {
    (void)field;GF_ASSERT(running_time&&script==2047&&!object);running_time->forced=1;
}
static BOOL IsSEPlaying(unsigned id) { return ds_audio_playing(ring_audio,DS_AUDIO_EFFECT,id); }
static void PlaySE_SetPitch(unsigned id,int pitch) { GF_ASSERT(!pitch);if(!ds_audio_play(ring_audio,DS_AUDIO_EFFECT,id))ring_audio_failed=1; }
static void *GfGfxLoader_LoadFromNarc(int archive,int member,BOOL compressed,enum HeapID heap,BOOL at_end) {
    (void)at_end;
    const u8 *bytes;size_t size;
    if(compressed||archive!=NARC_a_2_3_0||!native_resource_member(archive,0,member,&bytes,&size))unsupported("Missing Safari encounter resource");
    void *copy=Heap_Alloc(heap,size);memcpy(copy,bytes,size);return copy;
}
void sub_0206759C(SaveData *,int);
u32 sub_020674BC(SaveData *);
#define SaveArray_Party_Get hg_story_save_party
#define Save_VarsFlags_Get hg_story_save_vars
#define Save_Pokedex_Get hg_story_save_dex
#include "save_c_records_decls.inc"
#include "field_time_decls.inc"
#include "field_time_kernel.inc"

HGFieldTime *hg_field_time_new(void) { return calloc(1,sizeof(HGFieldTime)); }
void hg_field_time_free(HGFieldTime *state) { if(state){free(state->phone);free(state);} }
int hg_field_time_sample(HGTimeSample *sample,char *error,size_t size) {
    static uuid_t identity;static int identity_loaded;
    struct timespec timeout={1,0};time_t now=time(NULL);struct tm local;
    if(!sample||now==(time_t)-1||!localtime_r(&now,&local)||local.tm_year<100||local.tm_year>=200||(!identity_loaded&&gethostuuid(identity,&timeout))) {
        if(error&&size)snprintf(error,size,"Cannot sample the host clock and console identity");return 0;
    }
    identity_loaded=1;
    sample->rtc.date=(RTCDate){local.tm_year-100,local.tm_mon+1,local.tm_mday,local.tm_wday};
    sample->rtc.time=(RTCTime){local.tm_hour,local.tm_min,local.tm_sec};
    memcpy(sample->identity,identity,6);sample->identity[0]=(sample->identity[0]|2)&~1;
    sample->rtc_offset=local.tm_gmtoff;return 1;
}
static void enter_sample(SaveData *save,const HGTimeSample *sample) {
    host_sample=sample;host_owner=SaveArray_Get(save,SAVE_SYSINFO);
}
void hg_field_time_claim(SaveData *save,const HGTimeSample *sample) {
    enter_sample(save,sample);Save_SysInfo_InitFromSystem(host_owner);host_sample=NULL;host_owner=NULL;
}
static int run_time(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,int continuing,int map,char *error,size_t size) {
    if(error&&size)error[0]=0;
    if(!state||!save||!sample)return 0;
    /* Both bytes and runtime phone counters publish together. On failure the
     * caller retains the old owner and pending RTC baseline. */
    SaveData *candidate=malloc(sizeof(*candidate));if(!candidate){if(error&&size)snprintf(error,size,"Out of memory updating original field time");return 0;}*candidate=*save;
    GearPhoneRingManager *previous_phone=state->phone;
    GearPhoneRingManager previous={0};if(state->phone)previous=*state->phone;
    BugContest previous_contest={0};if(state->contest)previous_contest=*state->contest;
    FieldSystem field={0};field.saveData=candidate;field.bugContest=state->contest;
    const HGSaveRTC *previous_rtc=hg_save_rtc_exchange(&sample->rtc);
    enter_sample(candidate,sample);SaveData *previous_save=hg_save_manager_exchange(candidate);time_error=NULL;time_allocation_count=0;
    int previous_forced=state->forced;running_time=state;
    unsigned previous_button_mode=gSystem.buttonMode;u32 previous_rng=GetLCRNGSeed();
    int ok=0;
    if(!setjmp(time_trap)) {
        if(Save_VarsFlags_CheckBugContestFlag(hg_story_save_vars(candidate))&&!state->contest)unsupported("Active Bug-Catching Contest needs its original runtime before time can resume");
        if(!state->phone)state->phone=GearPhoneRingManager_New(HEAP_ID_FIELD2,&field);
        GearPhoneRingManager *phone=state->phone;
        phone->saveData=candidate;phone->pokegearData=SaveData_Pokegear_Get(candidate);
        phone->savingsData=SaveData_GetPhoneCallPersistentState(candidate);phone->sys=&field;
        field.phoneRingManager=phone;
        if(continuing&1) {
            Options_SetButtonModeOnMain(candidate,0);
            if(!Save_SysInfo_MacAddressIsMine(host_owner)||!Save_SysInfo_RTCOffsetIsMine(host_owner)) {
                SysInfoRTC_HandleContinueOnNewConsole(Save_SysInfo_RTC_Get(candidate));
                Save_BerryPotRTC_Init(Save_BerryPotRTC_Get(candidate));
                Save_SysInfo_InitFromSystem(host_owner);
                Party_ResetAllShayminToLandForm(SaveArray_Party_Get(candidate));
            }
        }
        if(state->contest)hg_bug_contest_rebind(state->contest,candidate);
        FieldSystem_StartBugContestTimer(&field);
        if(continuing&(4|8))sub_02093070(&field);
        if(continuing&8)sub_020930C4(&field);
        if(continuing&2) {
            Location *position=LocalFieldData_GetCurrentPosition(Save_LocalFieldData_Get(candidate));
            int previous_map=position->mapId;position->mapId=map;
            ov02_02251F20(phone);position->mapId=previous_map;
        }
        ok=memcmp(save,candidate,sizeof(*save))?HG_FIELD_TIME_CHANGED:HG_FIELD_TIME_UNCHANGED;
        *save=*candidate;
    }
    if(!ok){state->forced=previous_forced;state->phone=previous_phone;if(state->phone)*state->phone=previous;gSystem.buttonMode=previous_button_mode;SetLCRNGSeed(previous_rng);}
    if(state->contest){if(!ok)*state->contest=previous_contest;hg_bug_contest_rebind(state->contest,save);}
    for(unsigned i=0;i<time_allocation_count;i++)if(!ok||time_allocations[i]!=state->phone)free(time_allocations[i]);
    time_allocation_count=0;
    if(state->phone){state->phone->saveData=NULL;state->phone->pokegearData=NULL;state->phone->savingsData=NULL;state->phone->sys=NULL;}
    hg_save_manager_exchange(previous_save);hg_save_rtc_exchange(previous_rtc);host_sample=NULL;host_owner=NULL;running_time=NULL;
    free(candidate);
    if(!ok&&error&&size)snprintf(error,size,"%s",time_error?time_error:"Cannot update original field time");
    return ok;
}
int hg_field_time_update(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,char *error,size_t size) { return run_time(state,save,sample,0,-1,error,size); }
int hg_field_time_continue(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,char *error,size_t size) { return run_time(state,save,sample,1,-1,error,size); }

void hg_field_time_attach_contest(HGFieldTime *state,BugContest *contest) { if(state)state->contest=contest; }

int hg_field_time_poll(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,unsigned map,char *error,size_t size) { return run_time(state,save,sample,2,map,error,size); }
int hg_field_time_pending(const HGFieldTime *state,HGIncomingCall *call) {
    const GearPhoneRingManager *p=state?state->phone:NULL;if(!p||!p->active)return 0;
    if(call)*call=(HGIncomingCall){p->callerId,p->isScriptedCall,p->callScriptID,p->callTriggerID,state->forced};return 1;
}
void hg_field_time_answered(HGFieldTime *state) { if(state&&state->phone){gearRingingManagerReset(state->phone);state->forced=0;} }
int hg_field_time_ring_tick(HGFieldTime *state,DSAudio *audio) {
    if(!state||!state->phone||!state->phone->active||!state->phone->ringing)return 1;
    ring_audio=audio;ring_audio_failed=0;SysTask_RingGearPhone(NULL,state->phone);ring_audio=NULL;return !ring_audio_failed;
}

int hg_field_time_pc_full(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,char *error,size_t size) { return run_time(state,save,sample,4,-1,error,size); }

int hg_field_time_after_battle(HGFieldTime *state,SaveData *save,const HGTimeSample *sample,char *error,size_t size){return run_time(state,save,sample,8,-1,error,size);}

unsigned hg_field_time_of_day(FieldSystem *field){return Field_GetTimeOfDay(field);}

void hg_field_time_set_call_trigger(SaveData *save,unsigned trigger){
    GF_ASSERT(save&&trigger<13);GearPhoneRingManager phone={0};phone.savingsData=SaveData_GetPhoneCallPersistentState(save);sub_02092E14(&phone,(u8)trigger,FALSE);
}
