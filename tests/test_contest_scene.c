#include "compat/bug_contest.h"
#include "compat/field_time.h"
#include "compat/save_manager.h"
#include "field_app.h"
#include "field_player.h"
#include "field_events.h"
#include "save_local_field_data.h"
#include "compat/motion.h"
#include "field_types_def.h"
#include "constants/flags.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);} } while(0)
#define H(x) ((x)&255),(((x)>>8)&255)
static char error[256];
static void finish(HGFieldPlayer *player) {
    unsigned ticks=0;int state=HG_SCRIPT_RUNNING;char message[4096];
    while(state!=HG_SCRIPT_DONE&&ticks++<256){state=hg_field_player_interaction_tick(player,message,sizeof(message));if(state==HG_SCRIPT_ERROR)snprintf(error,sizeof(error),"%s",hg_field_player_interaction_error(player));CHECK(state!=HG_SCRIPT_ERROR);if(state==HG_SCRIPT_MESSAGE)CHECK(hg_field_player_interaction_ack(player));}CHECK(ticks<256);
}
static void travel_owner(const HGStoryState *story) {
    HGEventView events;CHECK(hg_event_load(63,&events)&&events.warp_count>1);
    const u8 *warp=events.warps+12;
    int wx=warp[0]+256*warp[1],wz=warp[2]+256*warp[3];
    const unsigned keys[]={0x40,0x80,0x20,0x10};
    for(int oz=-1;oz<=1;oz++)for(int ox=-1;ox<=1;ox++)for(unsigned d=0;d<4;d++) {
        HGFieldLocation at={63,-1,wx+ox,wz+oz,(int)d};
        HGStoryState *fixture=hg_story_clone(story);CHECK(fixture);
        FieldSystem source={0};hg_story_bind_field(fixture,&source);
        *LocalFieldData_GetCurrentPosition(SaveArray_Get(source.saveData,SAVE_LOCAL_FIELD_DATA))=(Location){63,-1,at.x,at.z,at.direction};
        HGFieldPlayer *player=hg_field_player_resume(fixture,&at,1);hg_story_free(fixture);if(!player)continue;
        HGWarpDestination destination;
        if(!hg_field_player_probe_transition(player,keys[d],&destination)||destination.map!=64){hg_field_player_free(player);continue;}
        const u8 entry[]={H(785),0,H(2),H(30),H(FLAG_UNK_996),H(2)};
        CHECK(hg_field_player_start_bytes(player,entry,sizeof(entry),246));finish(player);
        CHECK(hg_field_player_probe_transition(player,keys[d],&destination));
        CHECK(hg_field_player_transition(player,&destination));
        FieldSystem live={0};hg_field_player_bind_runtime(player,&live);
        BugContest *contest=hg_field_player_contest(player);
        CHECK(contest&&contest->saveData==live.saveData&&contest->party_cur==hg_story_save_party(live.saveData));
        CHECK(contest->sport_balls==20&&Party_GetCount(contest->party_cur)==1);
        const u8 leave[]={H(785),1,H(0),H(31),H(FLAG_UNK_996),H(2)};
        CHECK(hg_field_player_start_bytes(player,leave,sizeof(leave),246));finish(player);
        hg_field_player_bind_runtime(player,&live);CHECK(Party_GetCount(hg_story_save_party(live.saveData))==2);
        hg_field_player_free(player);return;
    }
    CHECK(!"No original stair approach admitted for contest owner scenario");
}
int main(void) {
    HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-3.hgcheckpoint",error,sizeof(error));CHECK(app);
    HGStoryState *story=hg_field_app_story(app);FieldSystem field={0};hg_story_bind_field(story,&field);
    Pokemon mon;CreateMon(&mon,155,15,31,TRUE,123,OT_ID_PRESET,42);Party *party=hg_story_save_party(field.saveData);CHECK(Party_AddMon(party,&mon));
    travel_owner(story);
    int x,z;CHECK(hg_field_app_player(app,&x,&z));HGFieldLocation location={67,-1,x,z,0};
    const u8 entry[]={H(785),0,H(2),H(30),H(FLAG_UNK_996),H(2)};
    HGFieldPlayer *player=hg_field_player_resume(story,&location,1);CHECK(player);
    CHECK(hg_field_player_start_bytes(player,entry,sizeof(entry),246));CHECK(!hg_field_player_contest(player));
    hg_field_player_free(player);CHECK(Party_GetCount(party)==2);
    player=hg_field_player_resume(story,&location,1);CHECK(player&&hg_field_player_start_bytes(player,entry,sizeof(entry),246));
    finish(player);BugContest *contest=hg_field_player_contest(player);CHECK(contest);
    FieldSystem live={0};hg_field_player_bind_runtime(player,&live);CHECK(live.bugContest==contest&&Party_GetCount(hg_story_save_party(live.saveData))==1);
    HGWarpDestination destination={.map=67,.warp=-1,.x=x,.z=z,.direction=0};
    /* No warp was selected: rejection must preserve the live contest owner. */
    CHECK(!hg_field_player_transition(player,&destination));
    contest=hg_field_player_contest(player);CHECK(contest);
    hg_field_player_bind_runtime(player,&live);
    CHECK(contest->saveData==live.saveData&&contest->party_cur==hg_story_save_party(live.saveData));
    CHECK(Party_GetCount(contest->party_cur)==1);
    HGStoryState *candidate=hg_story_clone(hg_field_player_story(player));CHECK(candidate&&!hg_field_player_prepare_save(player,candidate));hg_story_free(candidate);
    HGTimeSample sample={.rtc={{26,10,21,3},{12,0,0}},.identity={2,3,4,5,6,7}};
    SysInfo *sys=SaveArray_Get(live.saveData,SAVE_SYSINFO);sys->rtc_info.date=sample.rtc.date;sys->rtc_info.time=sample.rtc.time;sys->rtc_info.days_since_nitro_epoch=RTC_ConvertDateToDay(&sample.rtc.date);
    HGFieldTime *clock=hg_field_time_new();CHECK(clock);hg_field_time_attach_contest(clock,contest);
    sample.rtc.time.minute=19;CHECK(hg_field_time_update(clock,live.saveData,&sample,error,sizeof(error))&&contest->elapsed_time==19);
    sample.rtc.time.minute=20;CHECK(hg_field_time_update(clock,live.saveData,&sample,error,sizeof(error))&&contest->elapsed_time==20);
    hg_field_time_attach_contest(clock,NULL);hg_field_time_free(clock);
    CreateMon(&mon,123,30,31,TRUE,124,OT_ID_PRESET,42);CHECK(hg_bug_contest_keep(contest,&mon,1));hg_story_touch(hg_field_player_story(player));
    const u8 judging[]={H(787),H(0x8000),H(0x8001),H(0x8002),H(786),0,H(788),4,H(0x8003),H(789),5,H(785),1,H(0),H(31),H(FLAG_UNK_996),H(2)};
    CHECK(hg_field_player_start_bytes(player,judging,sizeof(judging),246));finish(player);
    CHECK(!hg_field_player_contest(player)&&hg_field_player_pc_full_event(player));
    hg_field_player_bind_runtime(player,&live);CHECK(Party_GetCount(hg_story_save_party(live.saveData))==3);
    CHECK(GetMonData(Party_GetMonByIndex(hg_story_save_party(live.saveData),2),MON_DATA_SPECIES,NULL)==123);
    CHECK(hg_field_player_prepare_save(player,(candidate=hg_story_clone(hg_field_player_story(player)))));hg_story_free(candidate);
    hg_field_player_free(player);hg_field_app_free(app);
    puts("Original contest scene: private/cancelled entry, published party narrowing, shared minute timer, judging/text/exit, capture ownership and save inhibition pass");
}
