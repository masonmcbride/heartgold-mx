#include "compat/field_time.h"
#include "compat/phone.h"
#include "compat/save_manager.h"
#include "field_app.h"
#include "field_player.h"
#include "save_pokegear.h"
#include "sys_vars.h"
#include "constants/phone_constants.h"
#include "constants/phone_contacts.h"
#include "constants/phone_scripts.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);} } while(0)
static char error[256];
int main(void) {
    HGTimeSample sample={.rtc={{26,10,21,3},{12,0,0}},.identity={2,3,4,5,6,7}};
    const HGSaveRTC *previous=hg_save_rtc_exchange(&sample.rtc);
    HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-3.hgcheckpoint",error,sizeof(error));CHECK(app);
    HGStoryState *story=hg_field_app_story(app);FieldSystem field={0};hg_story_bind_field(story,&field);
    SysInfo *sys=SaveArray_Get(field.saveData,SAVE_SYSINFO);
    sys->rtc_info.date=sample.rtc.date;sys->rtc_info.time=sample.rtc.time;sys->rtc_info.days_since_nitro_epoch=RTC_ConvertDateToDay(&sample.rtc.date);
    PhoneCallPersistentState *calls=SaveData_GetPhoneCallPersistentState(field.saveData);
    PhoneCallPersistentState_SetCallTriggerFlag(calls,CALL_TRIGGER_BAOBA_NEXT_TEST);
    HGFieldTime *clock=hg_field_time_new();CHECK(clock);
    CHECK(hg_field_time_poll(clock,field.saveData,&sample,67,error,sizeof(error)));
    sample.rtc.time.minute=1;CHECK(hg_field_time_poll(clock,field.saveData,&sample,67,error,sizeof(error)));
    sample.rtc.time.minute=11;CHECK(hg_field_time_poll(clock,field.saveData,&sample,67,error,sizeof(error)));
    HGIncomingCall call;CHECK(hg_field_time_pending(clock,&call));
    CHECK(call.caller==PHONE_CONTACT_BAOBA&&call.kind==3&&call.script==PHONE_SCRIPT_142&&call.trigger==CALL_TRIGGER_BAOBA_NEXT_TEST&&call.forced);
    CHECK(PhoneCallPersistentState_CheckCallTriggerFlag(calls,call.trigger));
    int x,z;CHECK(hg_field_app_player(app,&x,&z));
    HGFieldLocation location={67,-1,x,z,0};
    HGFieldPlayer *player=hg_field_player_resume(story,&location,1);CHECK(player);
    uint16_t args[]={call.caller,call.kind,call.script,call.trigger};
    CHECK(hg_field_player_start_script_args(player,2047,-1,args,4));
    /* Preflight executed the complete original call privately. */
    CHECK(PhoneCallPersistentState_CheckCallTriggerFlag(calls,call.trigger));
    unsigned messages=0,ticks=0;int status=HG_SCRIPT_RUNNING;char text[4096];
    while(status!=HG_SCRIPT_DONE&&ticks++<2048) {
        status=hg_field_player_interaction_tick(player,text,sizeof(text));
        if(status==HG_SCRIPT_ERROR)snprintf(error,sizeof(error),"%s",hg_field_player_interaction_error(player));
        CHECK(status!=HG_SCRIPT_ERROR);
        if(status==HG_SCRIPT_MESSAGE){messages++;CHECK(hg_field_player_interaction_ack(player));}
    }
    CHECK(ticks<2048&&messages);
    FieldSystem result={0};hg_story_bind_field(hg_field_player_story(player),&result);
    CHECK(!PhoneCallPersistentState_CheckCallTriggerFlag(SaveData_GetPhoneCallPersistentState(result.saveData),call.trigger));
    CHECK(Save_VarsFlags_GetVar4057(SaveArray_Get(result.saveData,SAVE_FLAGS))==4);
    calls=SaveData_GetPhoneCallPersistentState(result.saveData);
    u8 areas[]={0,1};PhoneCallPersistentState_SafariZoneArrangement_Set(calls,areas,2);
    PhoneCallPersistentState_SetCallTriggerFlag(calls,CALL_TRIGGER_BAOBA_NEW_POKEMON);
    hg_story_touch(hg_field_player_story(player));
    uint16_t area_call[]={PHONE_CONTACT_BAOBA,3,PHONE_SCRIPT_NONE,CALL_TRIGGER_BAOBA_NEW_POKEMON};
    CHECK(hg_field_player_start_script_args(player,2047,-1,area_call,4));
    int seen_area=0;status=HG_SCRIPT_RUNNING;ticks=0;
    while(status!=HG_SCRIPT_DONE&&ticks++<2048){
        status=hg_field_player_interaction_tick(player,text,sizeof(text));
        if(status==HG_SCRIPT_ERROR)snprintf(error,sizeof(error),"%s",hg_field_player_interaction_error(player));CHECK(status!=HG_SCRIPT_ERROR);
        if(status==HG_SCRIPT_MESSAGE){seen_area|=strstr(text,"Plains")!=NULL;CHECK(hg_field_player_interaction_ack(player));}
    }
    CHECK(ticks<2048&&seen_area&&!PhoneCallPersistentState_CheckCallTriggerFlag(calls,CALL_TRIGGER_BAOBA_NEW_POKEMON));
    for(unsigned species=1;species<=50;species++){
        Pokemon mon;CreateMon(&mon,species,5,31,TRUE,123,OT_ID_PRESET,42);
        Pokedex_SetMonCaughtFlag(hg_story_save_dex(result.saveData),&mon);
    }
    PhoneCallPersistentState_ClearCallTriggerFlag(calls,CALL_TRIGGER_OAK_DEX_PROGRESS);
    CHECK(hg_field_time_after_battle(clock,result.saveData,&sample,error,sizeof(error)));
    CHECK(PhoneCallPersistentState_CheckCallTriggerFlag(calls,CALL_TRIGGER_OAK_DEX_PROGRESS));
    hg_field_time_answered(clock);CHECK(!hg_field_time_pending(clock,NULL));
    hg_field_player_free(player);hg_field_time_free(clock);hg_field_app_free(app);hg_save_rtc_exchange(previous);
    puts("Original incoming call: timed selection, forced request, original standard script/fades/audio/application, private preflight and trigger/quest publication pass");
}
