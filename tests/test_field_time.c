#include "compat/field_time.h"
#include "compat/save_manager.h"
#include "friend_group.h"
#include "save_pokegear.h"
#include "save_trainer_card.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "sys_vars.h"
#include "constants/phone_constants.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);} } while(0)
static char error[256];
static SaveData *new_save(const HGTimeSample *sample) {
    SaveData *save=malloc(sizeof(*save));CHECK(save);
    const HGSaveRTC *previous=hg_save_rtc_exchange(&sample->rtc);
    hg_save_manager_init_ram(save);hg_save_rtc_exchange(previous);
    hg_field_time_claim(save,sample);return save;
}
int main(void) {
    HGTimeSample sample={.rtc={{24,2,28,3},{23,59,45}},.identity={2,4,6,8,10,12},.rtc_offset=0};
    SaveData *save=new_save(&sample);HGFieldTime *time=hg_field_time_new();CHECK(time);
    SysInfo *sys=SaveArray_Get(save,SAVE_SYSINFO);
    FieldSystem greeting={.saveData=save};
    for(unsigned hour=0;hour<24;hour++){
        sys->rtc_info.time.hour=hour;unsigned expected=hour<4?4:hour<10?0:hour<17?1:hour<20?2:3;
        CHECK(hg_field_time_of_day(&greeting)==expected);
    }
    sys->rtc_info.time.hour=23;
    SaveVarsFlags *flags=SaveArray_Get(save,SAVE_FLAGS);
    Save_VarsFlags_SetFlagInArray(flags,DAILY_FLAG_BASE);
    SAV_FRIEND_GRP *friends=SaveArray_Get(save,SAVE_FRIEND_GROUP);
    for(unsigned i=0;i<6;i++)friends->groups[i].unk_28=i+1;
    PLAYERDATA *player=SaveArray_Get(save,SAVE_PLAYERDATA);player->profile.johtoBadges=1;
    struct SaveTrainerCard *card=SaveArray_Get(save,SAVE_TRAINER_CARD);
    int *shine=TrainerCard_GetBadgeShininessArr(card);CHECK(shine[0]==140);
    Pokemon mon;CreateMon(&mon,152,5,32,TRUE,123,OT_ID_PRESET,42);
    u8 pokerus=0x13;SetMonData(&mon,MON_DATA_POKERUS,&pokerus);
    Party *party=SaveArray_Get(save,SAVE_PARTY);CHECK(Party_AddMon(party,&mon));
    u8 *apricorn=SaveArray_Get(save,SAVE_APRICORN_BOX);apricorn[9]=1;apricorn[14]=7;
    PokeathlonSave *ath=SaveArray_Get(save,SAVE_POKEATHLON);ath->unk_B7C=31;
    HGTimeSample next=sample;next.rtc.date=(RTCDate){24,2,29,4};next.rtc.time=(RTCTime){0,1,1};
    CHECK(hg_field_time_continue(time,save,&next,error,sizeof(error)));
    sys=SaveArray_Get(save,SAVE_SYSINFO);CHECK(sys->rtc_info.penaltyInMinutes==0);
    CHECK(sys->rtc_info.days_since_nitro_epoch==8825&&sys->rtc_info.time.minute==1);
    CHECK(!Save_VarsFlags_CheckFlagInArray(flags,DAILY_FLAG_BASE));
    CHECK(shine[0]==130&&shine[1]==140);
    CHECK(GetMonData(Party_GetMonByIndex(party,0),MON_DATA_POKERUS,NULL)==0x12);
    CHECK(apricorn[14]==17&&ath->unk_B7C==0);
    for(unsigned i=0;i<6;i++)CHECK(friends->groups[i].unk_28==(u32)((i+1)*1812433253u+1));
    RoamerSaveData *roamer=SaveArray_Get(save,SAVE_ROAMER);
    CHECK(roamer->rand[0]==friends->groups[1].unk_28&&roamer->rand[1]==roamer->rand[0]);
    /* Repeating a sample cannot age a record twice. Subminute changes retain
     * the original minute baseline until enough seconds have accumulated. */
    SaveData *copy=malloc(sizeof(*copy));CHECK(copy);*copy=*save;
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error))&&!memcmp(copy,save,sizeof(*save)));
    next.rtc.time.second=59;CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(sys->rtc_info.time.second==1);
    next.rtc.time.minute=2;next.rtc.time.second=1;
    sys->rtc_info.penaltyInMinutes=2;
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error))&&sys->rtc_info.penaltyInMinutes==1);
    /* The original console-change branch resets berry/RTC ownership and applies
     * a 24-hour penalty without aging the pending day on the new console. */
    Pokemon shaymin;CreateMon(&shaymin,492,30,32,TRUE,456,OT_ID_PRESET,42);
    u32 sky=1;SetMonData(&shaymin,MON_DATA_FORM,&sky);CHECK(Party_AddMon(party,&shaymin));
    next.identity[1]^=0x80;next.rtc.date=(RTCDate){24,3,2,6};
    u32 friend_seed=friends->groups[1].unk_28;
    CHECK(hg_field_time_continue(time,save,&next,error,sizeof(error)));
    CHECK(sys->rtc_info.penaltyInMinutes==1440&&friends->groups[1].unk_28==friend_seed);
    CHECK(!memcmp(sys->mac_address,next.identity,6));
    CHECK(GetMonData(Party_GetMonByIndex(party,1),MON_DATA_FORM,NULL)==0);
    /* RTC-offset ownership changes use the same original penalty branch. */
    next.rtc_offset=3600;CHECK(hg_field_time_continue(time,save,&next,error,sizeof(error)));
    CHECK(sys->rtc_offset==3600&&sys->rtc_info.penaltyInMinutes==1440);
    SAVE_MISC_DATA *misc=SaveArray_Get(save,SAVE_MISC);
    CHECK(!memcmp(&misc->berry_datetime.date,&next.rtc.date,sizeof(RTCDate)));
    /* Backwards host time moves the original baseline without negative aging. */
    next.rtc.date=(RTCDate){24,3,1,5};
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(sys->rtc_info.date.day==1&&friends->groups[1].unk_28==friend_seed);
    /* Exercise Safari's actual encounter resource comparison and aging path. */
    SafariZone *safari=SaveArray_Get(save,SAVE_SAFARI_ZONE);safari->objectUnlockLevel=1;
    safari->area_sets[0].areaLevels[0]=9;
    next.rtc.date=(RTCDate){24,3,2,6};
    sys->rtc_info.penaltyInMinutes=0;
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(safari->area_sets[0].areaLevels[0]==10);
    /* A late resource failure follows earlier daily mutations. The private
     * transaction must retain every old byte/RNG and allow a correct retry. */
    next.rtc.date.day=3;next.rtc.date.week=0;
    Save_VarsFlags_SetFlagInArray(flags,DAILY_FLAG_BASE);
    u8 area=safari->area_sets[0].areas[0].area_no;safari->area_sets[0].areas[0].area_no=255;
    *copy=*save;u32 rng=GetLCRNGSeed();
    CHECK(!hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(strstr(error,"Safari")&&!memcmp(copy,save,sizeof(*save))&&GetLCRNGSeed()==rng);
    safari->area_sets[0].areas[0].area_no=area;
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(safari->area_sets[0].areaLevels[0]==11&&!Save_VarsFlags_CheckFlagInArray(flags,DAILY_FLAG_BASE));
    /* Active contests are an explicit admission failure while their runtime
     * is absent, rather than silently dropping elapsed contest time. */
    Save_VarsFlags_SetFlagInArray(flags,FLAG_UNK_996);*copy=*save;
    next.rtc.time.minute++;
    CHECK(!hg_field_time_update(time,save,&next,error,sizeof(error))&&!memcmp(copy,save,sizeof(*save)));
    CHECK(strstr(error,"Contest"));Save_VarsFlags_ClearFlagInArray(flags,FLAG_UNK_996);
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    /* A second session proves the evening form reset, full penalty expiry and
     * phone trigger production through actual elapsed minutes/days. */
    HGTimeSample evening=sample;evening.rtc.date=(RTCDate){24,3,5,2};evening.rtc.time=(RTCTime){19,59,0};
    SaveData *other=new_save(&evening);HGFieldTime *other_time=hg_field_time_new();CHECK(other_time);
    PLAYERDATA *other_player=SaveArray_Get(other,SAVE_PLAYERDATA);other_player->options.buttonMode=1;
    Party *other_party=SaveArray_Get(other,SAVE_PARTY);CHECK(Party_AddMon(other_party,&shaymin));
    CHECK(hg_field_time_continue(other_time,other,&evening,error,sizeof(error))&&gSystem.buttonMode==3);
    evening.rtc.time=(RTCTime){20,0,0};
    CHECK(hg_field_time_update(other_time,other,&evening,error,sizeof(error)));
    CHECK(GetMonData(Party_GetMonByIndex(other_party,0),MON_DATA_FORM,NULL)==0);
    SysInfo *other_sys=SaveArray_Get(other,SAVE_SYSINFO);
    u8 *other_apricorn=SaveArray_Get(other,SAVE_APRICORN_BOX);other_apricorn[9]=1;other_apricorn[14]=1;
    SafariZone *other_safari=SaveArray_Get(other,SAVE_SAFARI_ZONE);other_safari->objectUnlockLevel=1;other_safari->area_sets[0].areaLevels[0]=9;
    evening.identity[2]^=1;evening.rtc.date=(RTCDate){24,3,6,3};
    CHECK(hg_field_time_continue(other_time,other,&evening,error,sizeof(error)));
    CHECK(other_sys->rtc_info.penaltyInMinutes==1440);
    evening.rtc.date=(RTCDate){24,3,7,4};
    CHECK(hg_field_time_update(other_time,other,&evening,error,sizeof(error)));
    CHECK(other_sys->rtc_info.penaltyInMinutes==0&&other_apricorn[14]==1&&other_safari->area_sets[0].areaLevels[0]==9);
    evening.rtc.date=(RTCDate){24,3,8,5};
    CHECK(hg_field_time_update(other_time,other,&evening,error,sizeof(error)));
    CHECK(other_apricorn[14]==11&&other_safari->area_sets[0].areaLevels[0]==10);
    SaveVarsFlags *other_flags=SaveArray_Get(other,SAVE_FLAGS);CHECK(SetScriptVar(other_flags,VAR_UNK_4057,3));
    other_player->igt.hours=4;evening.rtc.time.minute=1;
    CHECK(hg_field_time_update(other_time,other,&evening,error,sizeof(error)));
    CHECK(PhoneCallPersistentState_CheckCallTriggerFlag(SaveData_GetPhoneCallPersistentState(other),CALL_TRIGGER_BAOBA_NEXT_TEST));
    CHECK(hg_field_time_update(time,save,&next,error,sizeof(error)));
    CHECK(sys->rtc_info.date.day==3&&other_sys->rtc_info.date.day==8);
    hg_field_time_free(other_time);free(other);
    hg_field_time_free(time);free(copy);hg_save_manager_bind(NULL);free(save);
    puts("Original offline time: leap-day daily updates, party/flags/badges/RNG/Apricorns, minutes, console penalty, rollback and Safari resources pass");
}
