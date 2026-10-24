#include "compat/mart.h"
#include "ds_audio_output.h"
#include "ds_save_flash.h"
#include "constants/flags.h"
#include "map_header.h"
#include "constants/sndseq.h"
#include "field_app.h"
#include <unistd.h>
#include <stdlib.h>
#include "field_player.h"
#include "battle_view.h"
#include "field_menu.h"
#include "compat/pokemon.h"
#include "field_data.h"
#include "game_font.h"
#include "ds_frame_clock.h"
#include "ds_brightness.h"
#include <math.h>
#include "compat/field_tasks.h"
#include "compat/motion.h"
#include "compat/save_manager.h"
#include "save_local_field_data.h"
#include "compat/play_time.h"
#include "compat/field_time.h"
#include "compat/phone.h"
#include "compat/bug_contest.h"
#include "compat/blackout.h"
#include "constants/std_script.h"
#include "player_data.h"
#include <time.h>
struct HGFieldApp {
    FieldSystem runtime;DSFrameClock frame_clock;HGPlayTime play_time;HGFieldTime *field_time;
    s64 field_time_second;int field_time_sampled,battle_notifications;
    int initialized,new_game,exploring;
    HGFieldView *view,*player_sprite,*zone_view;unsigned player_sprite_id;
    HGFieldPlayer *player;HGFont *font;
    char dialogue[HG_SCRIPT_TEXT_CAPACITY],dialogue_page[512];size_t dialogue_cursor;int interact_pending;
    uint16_t held_keys,pending_keys,input_keys;unsigned move_delay,walk_serial,frame;
    unsigned long long revision;
    double visual[3],from[3];
    unsigned map,choice_index;int choice_active;
    DSAudioOutput *audio_output;unsigned audio_map;int audio_started;
    HGBlackout *blackout;
    HGBattle *battle;HGBattleView *battle_view;HGEncounterState encounters;
    int battle_state,script_battle;unsigned battle_choice;FieldSystem battle_field;
    HGFieldMenu *field_menu;Pokemon contest_catch;int catch_choice;
    HGStoryState *resume_story;HGFieldLocation resume_location;int resume_shoes;
    /* Staged travel: open a supported entrance prop, then fade out, swap and fade in.
     * The destination is prepared before the fade so a refused load leaves the
     * current scene untouched instead of fading back to an unchanged map. */
    HGWarpDestination pending_warp;HGFieldView *pending_view;int transition_phase;
    int stop;char *save_path;
    char error[256];
};
static void update_play_time(HGFieldApp *app) {
    if(!app||!app->player||app->exploring)return;
    struct timespec now;GF_ASSERT(clock_gettime(CLOCK_MONOTONIC,&now)==0);
    FieldSystem field={0};hg_story_bind_field(hg_field_player_story(app->player),&field);
    hg_play_time_update(&app->play_time,Save_PlayerData_GetIGTAddr(field.saveData),
        (uint64_t)now.tv_sec*1000000000ULL+now.tv_nsec);
}
static int update_field_time(HGFieldApp *app) {
    if(!app||!app->field_time||!app->player||app->exploring)return 1;
    /* A script's private candidate must not replace newer daily/phone state.
     * Wait for the shared field transaction to finish, then consume all elapsed
     * time together before another interaction starts. */
    hg_field_time_attach_contest(app->field_time,hg_field_player_contest(app->player));
    if(hg_field_app_interaction_busy(app)||hg_field_player_walking(app->player))return 1;
    HGTimeSample sample;if(!hg_field_time_sample(&sample,app->error,sizeof(app->error)))return 0;
    s64 second=RTC_ConvertDateTimeToSecond(&sample.rtc.date,&sample.rtc.time);
    if(app->field_time_sampled&&second==app->field_time_second&&!hg_field_player_pc_full_event(app->player)&&!app->battle_notifications)return 1;
    HGStoryState *story=hg_field_player_story(app->player);FieldSystem field={0};hg_story_bind_field(story,&field);
    u32 previous_rng=GetLCRNGSeed();SetLCRNGSeed(hg_story_rng_get(story));
    if(app->battle_notifications){
        int status=hg_field_time_after_battle(app->field_time,field.saveData,&sample,app->error,sizeof(app->error));
        if(!status){SetLCRNGSeed(previous_rng);return 0;}
        if(status==HG_FIELD_TIME_CHANGED)hg_story_touch(story);
        app->battle_notifications=0;
    }
    if(hg_field_player_pc_full_event(app->player)) {
        int status=hg_field_time_pc_full(app->field_time,field.saveData,&sample,app->error,sizeof(app->error));
        if(!status){SetLCRNGSeed(previous_rng);return 0;}
        if(status==HG_FIELD_TIME_CHANGED)hg_story_touch(story);
        hg_field_player_pc_full_ack(app->player);
    }
    int has_gear=0;hg_story_flag_get(story,FLAG_GOT_POKEGEAR,&has_gear);
    int ok=has_gear?hg_field_time_poll(app->field_time,field.saveData,&sample,app->map,app->error,sizeof(app->error)):
        hg_field_time_update(app->field_time,field.saveData,&sample,app->error,sizeof(app->error));
    u32 next_rng=GetLCRNGSeed();SetLCRNGSeed(previous_rng);
    if(ok){hg_story_rng_set(story,next_rng);if(ok==HG_FIELD_TIME_CHANGED)hg_story_touch(story);app->field_time_second=second;app->field_time_sampled=1;}
    return ok;
}
static int answer_incoming(HGFieldApp *app) {
    HGIncomingCall call;if(!app||!app->player||!hg_field_time_pending(app->field_time,&call)||hg_field_app_interaction_busy(app)||hg_field_player_walking(app->player))return 0;
    uint16_t args[]={call.caller,call.kind,call.script,call.trigger};
    if(!hg_field_player_start_script_args(app->player,2047,-1,args,4)) {
        snprintf(app->error,sizeof(app->error),"%s",hg_field_player_interaction_error(app->player));return 0;
    }
    hg_field_time_answered(app->field_time);app->revision++;return 1;
}
static void next_dialogue_page(HGFieldApp *app) {
    const char *start=app->dialogue+app->dialogue_cursor;
    size_t consumed=hg_font_page_end(app->font,start,236,2),length=consumed;
    if(!length){app->dialogue[0]=app->dialogue_page[0]=0;return;}
    memcpy(app->dialogue_page,start,length);app->dialogue_page[length]=0;
    while(length&&(app->dialogue_page[length-1]=='\r'||app->dialogue_page[length-1]=='\n'))app->dialogue_page[--length]=0;
    app->dialogue_cursor+=consumed;
}
static void interaction_unavailable(HGFieldApp *app) {
    const char *error=hg_field_player_interaction_error(app->player);if(!error[0])return;
    if(!app->error[0])snprintf(app->error,sizeof(app->error),"%s",error);
    snprintf(app->dialogue,sizeof(app->dialogue),"This interaction needs a game feature\nthat isn’t available yet.");
    app->dialogue_cursor=0;next_dialogue_page(app);app->revision++;
}
static int launch_battle(HGFieldApp *app,HGBattleSetup *setup){
    app->battle=hg_battle_new_setup(setup,app->error,sizeof(app->error));if(!app->battle)return 0;
    app->battle_view=hg_battle_view_new();if(!app->battle_view){hg_battle_free(app->battle);app->battle=NULL;snprintf(app->error,sizeof(app->error),"Cannot create battle presentation");return 0;}
    app->battle_state=HG_BATTLE_RUNNING;app->battle_choice=0;hg_field_app_clear_input(app);app->interact_pending=0;app->revision++;
    if(setup->audio&&!ds_audio_play(setup->audio,DS_AUDIO_BGM,setup->trainer_id?SEQ_GS_VS_RIVAL:SEQ_GS_VS_NORAPOKE))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(setup->audio));return 1;
}
static void begin_script_battle(HGFieldApp *app){
    unsigned trainer;FieldSystem field={0};if(!app->player||app->battle||app->dialogue[0]||!hg_field_player_battle_request(app->player,&trainer,&field))return;
    HGBattleSetup setup={.party=hg_story_save_party(field.saveData),.profile=hg_story_save_profile(field.saveData),.bag=hg_story_save_bag(field.saveData),.options=hg_story_save_options(field.saveData),.pokedex=hg_story_save_dex(field.saveData),.stats=hg_story_save_stats(field.saveData),.seed=hg_story_rng_get(hg_field_player_story(app->player)),.trainer_id=trainer,.allow_loss=trainer?1:0,.audio=field.native_audio};
    app->script_battle=1;app->battle_field=field;
    if(!trainer)setup.tutorial_field=&app->battle_field;
    if(!launch_battle(app,&setup)){hg_field_player_battle_complete(app->player,0);app->script_battle=0;}
}
static void begin_wild_battle(HGFieldApp *app){
    if(app->exploring||app->battle||!app->player||app->dialogue[0]||hg_field_player_interaction_busy(app->player))return;
    int x,z;uint16_t attribute;if(!hg_field_player_position(app->player,&x,&z,NULL)||!hg_field_attribute(app->map,x,z,&attribute))return;
    FieldSystem field={0};hg_field_player_bind_runtime(app->player,&field);HGStoryState *story=hg_field_player_story(app->player);
    app->encounters.rng=hg_story_rng_get(story);Party opponent;
    int found=hg_encounter_step(&app->encounters,&field,app->map,attribute&255,1,&opponent,app->error,sizeof(app->error));
    hg_story_rng_set(story,app->encounters.rng);
    if(found<0){snprintf(app->dialogue,sizeof(app->dialogue),"Encounter stopped: %s",app->error);app->dialogue_cursor=0;next_dialogue_page(app);app->revision++;return;}
    if(!found)return;
    HGBattleSetup setup={.field=&field,.storage=SaveArray_Get(field.saveData,SAVE_PCSTORAGE),.party=hg_story_save_party(field.saveData),.opponent=&opponent,.profile=hg_story_save_profile(field.saveData),.bag=hg_story_save_bag(field.saveData),.options=hg_story_save_options(field.saveData),.pokedex=hg_story_save_dex(field.saveData),.stats=hg_story_save_stats(field.saveData),.seed=app->encounters.rng,.audio=hg_field_player_audio(app->player),.contest=field.bugContest};
    app->script_battle=0;app->battle_field=field;launch_battle(app,&setup);
}
static int begin_blackout(HGFieldApp *app){
    if(hg_field_player_contest(app->player)){snprintf(app->error,sizeof(app->error),"Contest defeat recovery is unavailable");return 0;}
    HGStoryState *candidate=hg_story_clone(hg_field_player_story(app->player));
    if(!candidate||!hg_field_player_prepare_relocation(app->player,candidate)){hg_story_free(candidate);snprintf(app->error,sizeof(app->error),"Cannot prepare loss recovery");return 0;}
    FieldSystem field={0};hg_story_bind_field(candidate,&field);
    int ok=hg_battle_commit_storage(app->battle,hg_story_save_party(field.saveData),hg_story_save_profile(field.saveData),hg_story_save_bag(field.saveData),hg_story_save_dex(field.saveData),hg_story_save_stats(field.saveData),SaveArray_Get(field.saveData,SAVE_PCSTORAGE));
    if(ok)app->blackout=hg_blackout_new(field.saveData,hg_field_player_audio(app->player),app->error,sizeof(app->error));
    hg_story_free(candidate);if(!app->blackout&&!app->error[0])snprintf(app->error,sizeof(app->error),"Cannot start loss recovery");return app->blackout!=NULL;
}
static void blackout_frame(HGFieldApp *app){
    if(app->interact_pending)hg_blackout_ack(app->blackout);
    hg_field_app_clear_input(app);app->interact_pending=0;app->revision++;
    int status=hg_blackout_tick(app->blackout);
    if(status==HG_BLACKOUT_FAILED)snprintf(app->error,sizeof(app->error),"%s",hg_blackout_error(app->blackout));
    else if(status!=HG_BLACKOUT_DONE)return;
    if(status==HG_BLACKOUT_DONE){
        HGStoryState *story=hg_story_from_save_records(hg_blackout_save(app->blackout));FieldSystem field={0};if(story)hg_story_bind_field(story,&field);
        Location *position=story?LocalFieldData_GetCurrentPosition(SaveArray_Get(field.saveData,SAVE_LOCAL_FIELD_DATA)):NULL;
        HGFieldLocation at=position?(HGFieldLocation){position->mapId,position->warpId,position->x,position->y,position->direction}:(HGFieldLocation){0};
        HGFieldPlayer *player=story?hg_field_player_resume_checked(story,&at,hg_field_player_running_shoes(app->player),app->error,sizeof(app->error)):NULL;
        HGFieldView *view=player?hg_field_view_load((unsigned)at.map,app->error,sizeof(app->error)):NULL;
        if(player){
            FieldSystem previous={0},next={0};hg_story_bind_field(hg_field_player_story(app->player),&previous);hg_story_bind_field(hg_field_player_story(player),&next);
            *Save_PlayerData_GetIGTAddr(next.saveData)=*Save_PlayerData_GetIGTAddr(previous.saveData);
            hg_story_rng_set(hg_field_player_story(player),hg_story_rng_get(hg_field_player_story(app->player)));
        }
        if(!view||!hg_field_player_start_script(player,hg_blackout_script(app->blackout),-1)){
            if(!app->error[0])snprintf(app->error,sizeof(app->error),"Blackout scene: %s",player?hg_field_player_interaction_error(player):"cannot rebuild destination");
            hg_field_view_free(view);hg_field_player_free(player);
        }else{
            hg_field_menu_free(app->field_menu);app->field_menu=NULL;
            hg_blackout_free(app->blackout);app->blackout=NULL;
            hg_field_player_free(app->player);hg_field_view_free(app->view);app->player=player;app->view=view;app->map=at.map;
            hg_field_player_position(player,NULL,NULL,app->visual);memcpy(app->from,app->visual,sizeof(app->from));app->audio_started=0;
        }
        hg_story_free(story);
    }
    if(app->blackout){hg_blackout_free(app->blackout);app->blackout=NULL;ds_audio_fade(hg_field_player_audio(app->player),127,0);}
    if(app->error[0]){snprintf(app->dialogue,sizeof(app->dialogue),"Recovery stopped: %s",app->error);app->dialogue_cursor=0;next_dialogue_page(app);}
}
static void battle_frame(HGFieldApp *app){
    int old=app->battle_state;
    unsigned count=hg_battle_menu_count(app->battle,app->battle_state);
    if(count){
        if(app->pending_keys&(0x40|0x20))app->battle_choice=(app->battle_choice+count-1)%count;
        else if(app->pending_keys&(0x80|0x10))app->battle_choice=(app->battle_choice+1)%count;
    }
    if(app->battle_state==HG_BATTLE_MOVES){for(unsigned i=0;i<4&&!hg_battle_move_allowed(app->battle,app->battle_choice);i++)app->battle_choice=(app->battle_choice+1)%4;}
    if(app->interact_pending){unsigned code=app->battle_choice==UINT32_MAX?255:count?hg_battle_choice_code(app->battle,app->battle_state,app->battle_choice):app->battle_state==HG_BATTLE_NICKNAME?255:1;if(code)hg_battle_input(app->battle,code);}
    app->held_keys=app->pending_keys=0;app->interact_pending=0;
    app->battle_state=hg_battle_tick(app->battle);if(app->battle_state!=old)app->battle_choice=0;app->revision++;
    if(app->battle_state==HG_BATTLE_FINISHED||app->battle_state==HG_BATTLE_FAILED){
        FieldSystem field=app->battle_field;
        if(app->battle_state==HG_BATTLE_FAILED)snprintf(app->error,sizeof(app->error),"%s",hg_battle_error(app->battle));
        else if(!app->script_battle&&hg_battle_result(app->battle)==2){begin_blackout(app);}
        else if(!hg_battle_commit_storage(app->battle,hg_story_save_party(field.saveData),hg_story_save_profile(field.saveData),hg_story_save_bag(field.saveData),hg_story_save_dex(field.saveData),hg_story_save_stats(field.saveData),SaveArray_Get(field.saveData,SAVE_PCSTORAGE)))snprintf(app->error,sizeof(app->error),"Battle result could not be published");
        else if(app->script_battle){if(!hg_field_player_battle_complete(app->player,hg_battle_result(app->battle)))snprintf(app->error,sizeof(app->error),"Cannot resume battle script");}
        else {
            BugContest *contest=hg_field_player_contest(app->player);
            if(!contest)app->battle_notifications=1;
            if(contest&&hg_battle_contest_catch(app->battle,&app->contest_catch,sizeof(app->contest_catch))) {
                if(contest->caught_poke){app->catch_choice=1;app->choice_active=3;app->choice_index=0;}
                else hg_bug_contest_keep(contest,&app->contest_catch,1);
            }
            hg_story_touch(hg_field_player_story(app->player));
        }
        if(app->script_battle&&app->error[0])hg_field_player_battle_complete(app->player,0);app->script_battle=0;
        hg_battle_free(app->battle);hg_battle_view_free(app->battle_view);app->battle=NULL;app->battle_view=NULL;
        app->encounters.inhibit_steps=0;app->audio_started=0;app->move_delay=8;
        if(app->error[0]){snprintf(app->dialogue,sizeof(app->dialogue),"Battle stopped: %s",app->error);app->dialogue_cursor=0;next_dialogue_page(app);}
    }
}
/* Native transition staging retains original prop playback and fade kernels.
 * Entrance doors open visibly before the field fades; the destination map is
 * installed, then the field fades back in. Doors use original GetDoorSE; other
 * travel retains SEQ_SE_DP_KAIDAN2. Native fade
 * steps use the same six-step brightness ramp as the original starter sequence. */
#define HG_TRANSITION_FADE_STEPS 6
#define HG_TRANSITION_FADE_SPEED 1
static void transition_cancel(HGFieldApp *app) {
    hg_field_player_door_end(app->player);
    hg_field_view_free(app->pending_view);app->pending_view=NULL;
    app->pending_warp=(HGWarpDestination){0};app->transition_phase=0;
}
static void transition_frame(HGFieldApp *app) {
    if(!app->player) { transition_cancel(app);return; }
    if(app->transition_phase==3) {
        int done=hg_field_player_door_tick(app->player);
        if(done<0){snprintf(app->error,sizeof(app->error),"Door animation failed during travel");transition_cancel(app);return;}
        if(done) {
            if(!hg_field_player_fade_begin(app->player,0,0,HG_TRANSITION_FADE_STEPS,HG_TRANSITION_FADE_SPEED)){snprintf(app->error,sizeof(app->error),"Door exit fade failed");transition_cancel(app);return;}
            app->transition_phase=1;
        }
        app->revision++;return;
    }
    if(!hg_field_player_fade_tick(app->player)) { /* Rejected tick: abandon travel. */
        snprintf(app->error,sizeof(app->error),"Field fade failed during travel");
        transition_cancel(app);return;
    }
    if(app->transition_phase==1 && !hg_field_player_fade_active(app->player)) {
        /* Phase one complete: install the prepared destination scene, then fade in. */
        if(!hg_field_player_transition(app->player,&app->pending_warp)) {
            const char *reason=hg_field_player_interaction_error(app->player);
            snprintf(app->error,sizeof(app->error),"%s",reason[0]?reason:"Field travel could not complete");
            transition_cancel(app);return;
        }
        hg_field_view_free(app->view);app->view=app->pending_view;app->pending_view=NULL;
        app->map=(unsigned)app->pending_warp.map;
        hg_field_player_position(app->player,NULL,NULL,app->visual);memcpy(app->from,app->visual,sizeof(app->from));
        if(!hg_field_player_fade_begin(app->player,1,0,HG_TRANSITION_FADE_STEPS,HG_TRANSITION_FADE_SPEED)) {
            snprintf(app->error,sizeof(app->error),"Field fade-in could not start");app->transition_phase=0;app->revision++;return;
        }
        app->transition_phase=2;app->revision++;return;
    }
    if(app->transition_phase==2&&hg_field_player_exit_active(app->player)) {
        int done=hg_field_player_exit_tick(app->player);
        if(done<0){snprintf(app->error,sizeof(app->error),"Destination door sequence failed");transition_cancel(app);return;}
        unsigned frame=5;
        if(hg_field_player_exit_active(app->player)==3) {
            double position[3];hg_field_player_position(app->player,NULL,NULL,position);
            if(fabs(position[0]-app->from[0])+fabs(position[2]-app->from[2])<=8)frame=6;
        }
        if(hg_avatar_sprite_frames(app->player_sprite)==12)frame=4;
        app->frame=frame;hg_player_sprite_frame(app->player_sprite,frame);
        if(!done){app->revision++;return;}
    }
    if(app->transition_phase==2 && !hg_field_player_fade_active(app->player)) {
        app->transition_phase=0;app->pending_warp=(HGWarpDestination){0};
        app->move_delay=4;app->revision++;return;
    }
    app->revision++;
}
static int transition_begin(HGFieldApp *app,const HGWarpDestination *destination) {
    if(!app->player||app->transition_phase||!destination)return 0;
    HGFieldView *next=hg_field_view_load((unsigned)destination->map,app->error,sizeof(app->error));
    if(!next)return 0;
    int door=hg_field_player_door_begin(app->player,(unsigned)destination->direction);
    if(door<0){hg_field_view_free(next);snprintf(app->error,sizeof(app->error),"Entrance door animation is unsupported");return 0;}
    if(!door&&!hg_field_player_fade_begin(app->player,0,0,HG_TRANSITION_FADE_STEPS,HG_TRANSITION_FADE_SPEED)){hg_field_view_free(next);return 0;}
    app->pending_view=next;app->pending_warp=*destination;app->transition_phase=door?3:1;
    if(door){app->frame=1;hg_player_sprite_frame(app->player_sprite,app->frame);}
    DSAudio *audio=hg_field_player_audio(app->player);
    if(!door&&audio&&!ds_audio_play(audio,DS_AUDIO_EFFECT,SEQ_SE_DP_KAIDAN2))
        snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
    hg_field_app_clear_input(app);app->interact_pending=0;app->revision++;return 1;
}
static int scene_init(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);
    app->view=hg_field_view_load(app->map,app->error,sizeof(app->error));if(app->view && hg_transition_supported(app->map)) {
        app->player=app->resume_story?hg_field_player_resume(app->resume_story,&app->resume_location,app->resume_shoes):app->exploring?hg_field_player_new_explore(app->map):app->new_game?hg_field_player_new_game():hg_field_player_new_map(app->map);app->player_sprite_id=hg_field_player_sprite(app->player);app->player_sprite=hg_avatar_sprite_load(app->player_sprite_id,hg_field_player_facing(app->player),app->error,sizeof(app->error));
        if(!app->player||!app->player_sprite){
            if(!app->error[0])snprintf(app->error,sizeof(app->error),"Field gameplay is unavailable for map %u",app->map);
            hg_field_menu_free(app->field_menu);app->field_menu=NULL;hg_field_player_free(app->player);app->player=NULL;hg_field_view_free(app->player_sprite);app->player_sprite=NULL;
            if(app->new_game){hg_field_view_free(app->view);app->view=NULL;}
        }
    }
    if(app->player) {
        hg_field_player_position(app->player,NULL,NULL,app->visual);
        memcpy(app->from,app->visual,sizeof(app->from));
        app->font=hg_font_new();
        if(!app->font){snprintf(app->error,sizeof(app->error),"Cannot initialize original dialogue font");hg_field_view_free(app->view);app->view=NULL;}
        app->frame=1+4*(unsigned)hg_field_player_facing(app->player);hg_player_sprite_frame(app->player_sprite,app->frame);
    }
    app->initialized=1;return TRUE;
}
static int scene_exec(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);if(app->stop)return TRUE;
    uint64_t audio_epoch=ds_audio_epoch();
    BugContest *contest=hg_field_player_contest(app->player);
    if(contest&&contest->elapsed_time>=20&&!hg_field_app_interaction_busy(app)&&!hg_field_player_walking(app->player)) {
        if(!hg_field_player_start_script(app->player,std_bug_contest_time_up,-1))interaction_unavailable(app);
    }
    if(app->catch_choice) {
        if(app->pending_keys&0xf0)app->choice_index^=1;
        if(app->interact_pending){hg_bug_contest_keep(hg_field_player_contest(app->player),&app->contest_catch,app->choice_index==1);app->catch_choice=0;app->choice_active=0;hg_story_touch(hg_field_player_story(app->player));}
        hg_field_app_clear_input(app);app->interact_pending=0;app->revision++;goto audio_step;
    }
    HGIncomingCall incoming;
    if(hg_field_time_pending(app->field_time,&incoming)&&incoming.forced&&!hg_field_app_interaction_busy(app))answer_incoming(app);
    if(!hg_field_time_ring_tick(app->field_time,hg_field_player_audio(app->player)))snprintf(app->error,sizeof(app->error),"Cannot play incoming phone ring");
    HGShop *shop=hg_field_player_shop(app->player);
    if(hg_shop_open(shop)){
        hg_shop_input(shop,app->pending_keys,app->interact_pending,0);app->held_keys=app->pending_keys=0;app->interact_pending=0;
        char message[sizeof(app->dialogue)];int status=hg_field_player_interaction_tick(app->player,message,sizeof(message));
        if(status==HG_SCRIPT_ERROR)interaction_unavailable(app);
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;goto audio_step;
    }
    if(app->blackout){blackout_frame(app);goto audio_step;}
    if(app->battle){battle_frame(app);goto audio_step;}
    if(hg_field_menu_kind(app->field_menu)){if(app->pending_keys){hg_field_menu_input(app->field_menu,app->pending_keys);app->revision++;}if(app->interact_pending){hg_field_menu_confirm(app->field_menu);app->revision++;}if(hg_field_menu_take_changed(app->field_menu))hg_story_touch(hg_field_player_story(app->player));app->held_keys=app->pending_keys=0;app->interact_pending=0;goto audio_step;}
    /* A staged door/stair travel owns the frame until both fade phases finish.
     * Original input is ignored and held axis history is cleared meanwhile. */
    if(app->transition_phase) {
        transition_frame(app);
        /* Keep the original presentation and audio tasks running beneath the fade. */
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
        hg_field_app_clear_input(app);app->interact_pending=0;
        goto audio_step;
    }
    if(app->move_delay)app->move_delay--;
    if(app->player&&!app->dialogue[0]&&!hg_field_player_interaction_busy(app->player)&&!hg_field_player_walking(app->player)) {
        int started=hg_field_player_frame_begin(app->player);
        if(started)app->held_keys=app->pending_keys=0;
        if(started<0)interaction_unavailable(app);
    }
    if(app->player&&hg_field_player_starter_pending(app->player)) {
        if(!app->choice_active){app->choice_index=0;app->choice_active=1;app->revision++;}
        uint16_t keys=app->pending_keys;
        if(keys&(0x40|0x20)){app->choice_index=(app->choice_index+2)%3;app->revision++;}
        else if(keys&(0x80|0x10)){app->choice_index=(app->choice_index+1)%3;app->revision++;}
        app->held_keys=app->pending_keys=0;
        if(app->interact_pending){hg_field_player_choose_starter(app->player,app->choice_index);app->interact_pending=0;app->choice_active=0;app->revision++;}
    }
    if(app->player&&hg_field_player_menu_pending(app->player)) {
        if(app->choice_active!=2){app->choice_index=hg_field_player_menu_count(app->player)==2?1:0;app->choice_active=2;app->revision++;}
        unsigned count=hg_field_player_menu_count(app->player);
        if(app->pending_keys&(0x40|0x20)){app->choice_index=(app->choice_index+count-1)%count;app->revision++;}
        else if(app->pending_keys&(0x80|0x10)){app->choice_index=(app->choice_index+1)%count;app->revision++;}
        app->held_keys=app->pending_keys=0;
        if(app->interact_pending){
            if(hg_field_player_menu_choose(app->player,app->choice_index))app->choice_active=0;
            app->interact_pending=0;app->revision++;
        }
    }
    if(app->player && !(app->held_keys&0xf0) && !(app->pending_keys&0xf0) && !hg_field_player_walking(app->player))
        hg_field_player_move(app->player,0); /* Original idle axis-history update. */
    if(app->player && ((app->held_keys|app->pending_keys)&0xf0) && !app->dialogue[0] && !hg_field_player_interaction_busy(app->player) && !app->interact_pending && !app->move_delay && !hg_field_player_walking(app->player)) {
        memcpy(app->from,app->visual,sizeof(app->from));
        uint16_t keys=app->held_keys|app->pending_keys;app->pending_keys=0;
        HGWarpDestination destination;
        if(hg_field_player_probe_transition(app->player,keys,&destination)) {
            /* Stage the original fade-out -> scene swap -> fade-in. The ready
             * destination view is prepared up front so a refused travel leaves
             * the current scene untouched rather than fading back unchanged. */
            if(!transition_begin(app,&destination)) { app->move_delay=8; }
        } else if(hg_field_player_move(app->player,keys)) {
            int map=hg_field_player_pending_map(app->player);
            if(map>=0) {
                app->zone_view=hg_field_view_load((unsigned)map,app->error,sizeof(app->error));
                if(!app->zone_view){hg_field_player_cancel_step(app->player);interaction_unavailable(app);}
            }
            app->walk_serial++;
        } else {app->move_delay=8;if(hg_field_player_interaction_error(app->player)[0])interaction_unavailable(app);}
    }
    if(hg_field_player_walking(app->player)) {
        int advanced=hg_field_player_tick(app->player);
        if(app->zone_view&&(advanced<0||!hg_field_player_walking(app->player))) {
            int zone_map=hg_field_player_pending_map(app->player);
            if(advanced>0&&hg_field_player_commit_zone(app->player)) {
                hg_field_view_free(app->view);app->view=app->zone_view;app->zone_view=NULL;
                app->map=(unsigned)zone_map;app->revision++;app->held_keys=app->pending_keys=0;
            } else {
                hg_field_view_free(app->zone_view);app->zone_view=NULL;hg_field_player_cancel_step(app->player);interaction_unavailable(app);advanced=-1;
            }
        }
        if(advanced>0&&!hg_field_player_walking(app->player)) {
            HGWarpDestination destination;
            int started=hg_field_player_coordinate_begin(app->player);
            if(!started&&hg_field_player_probe_step_transition(app->player,&destination)){
                started=1;if(!transition_begin(app,&destination))interaction_unavailable(app);
            }
            if(started)app->held_keys=app->pending_keys=0;
            if(started<0)interaction_unavailable(app);
            if(!started)begin_wild_battle(app);
        }
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
    }
    else if(!hg_field_player_interaction_busy(app->player)) { int changed=hg_field_player_tick(app->player);if(changed<0)interaction_unavailable(app);else if(changed)app->revision++; }
    if(app->interact_pending && !hg_field_player_walking(app->player)) {
        app->interact_pending=0;app->held_keys=app->pending_keys=0;
        if(app->dialogue[0]) {
            /* Original render_text.c:TextPrinter_Continue plays this cue on
             * accepted advance input, including the last page. */
            DSAudio *audio=hg_field_player_audio(app->player);
            if(!ds_audio_play(audio,DS_AUDIO_EFFECT,SEQ_SE_DP_SELECT))
                snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
            if(app->dialogue[app->dialogue_cursor])next_dialogue_page(app);
            else {app->dialogue[0]=app->dialogue_page[0]=0;hg_field_player_interaction_ack(app->player);}
            app->revision++;
        } else if(!hg_field_player_interaction_busy(app->player)&&!hg_field_player_interaction_begin(app->player))interaction_unavailable(app);
    }
    if(hg_field_player_interaction_busy(app->player)&&app->dialogue[0]) {
        char retained[sizeof(app->dialogue)];
        int status=hg_field_player_interaction_tick(app->player,retained,sizeof(retained));
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
        if(status==HG_SCRIPT_ERROR)interaction_unavailable(app);
    } else if(hg_field_player_interaction_busy(app->player)) {
        int status=hg_field_player_interaction_tick(app->player,app->dialogue,sizeof(app->dialogue));
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
        if(status==HG_SCRIPT_MESSAGE){app->dialogue_cursor=0;next_dialogue_page(app);app->revision++;}
        else if(status==HG_SCRIPT_DONE)app->revision++;
        else if(status==HG_SCRIPT_ERROR)interaction_unavailable(app);
    }
    begin_script_battle(app);
    /* Initialize the presentation in the frame the original task requests it,
     * so the first rendered menu already has its real default selection. */
    if(hg_field_player_starter_pending(app->player)&&app->choice_active!=1){app->choice_index=0;app->choice_active=1;}
    if(hg_field_player_menu_pending(app->player)&&app->choice_active!=2){app->choice_index=hg_field_player_menu_count(app->player)==2?1:0;app->choice_active=2;}
    if(app->player) {
        unsigned sprite=hg_field_player_sprite(app->player);
        if(sprite!=app->player_sprite_id) {
            HGFieldView *replacement=hg_avatar_sprite_load(sprite,hg_field_player_facing(app->player),app->error,sizeof(app->error));
            if(replacement){hg_field_view_free(app->player_sprite);app->player_sprite=replacement;app->player_sprite_id=sprite;app->frame=0;app->revision++;}
        }
        static const unsigned idle[]={1,5,9,13};
        static const unsigned stride[][2]={{2,4},{6,8},{10,12},{14,16}};
        unsigned facing=(unsigned)hg_field_player_facing(app->player),frame=idle[facing];
        if(hg_field_player_walking(app->player)) {
            double distance=fabs(app->visual[0]-app->from[0])+fabs(app->visual[2]-app->from[2]);
            if(hg_field_player_running(app->player))frame=idle[facing]+16;
            if(distance<=8)frame=stride[facing][app->walk_serial&1];
            if(hg_field_player_running(app->player)&&distance<=8)frame+=16;
        }
        if(hg_avatar_sprite_frames(app->player_sprite)==12)frame=1+3*facing;
        if(frame!=app->frame){if(!hg_player_sprite_frame(app->player_sprite,frame))snprintf(app->error,sizeof(app->error),"Cannot present avatar frame %u",frame);else {app->frame=frame;app->revision++;}}
    }
audio_step:;
    DSAudio *audio=hg_field_player_audio(app->player);
    if(audio) {
        if(!app->battle&&(!app->audio_started||app->audio_map!=app->map)) {
            if(app->audio_map!=app->map)ds_audio_discard_saved_bgm(audio);
            if(!ds_audio_restore_bgm(audio)&&!ds_audio_play(audio,DS_AUDIO_BGM,MapHeader_GetDayMusicId(app->map)))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
            app->audio_map=app->map;app->audio_started=1;
        }
        if(ds_audio_last_epoch(audio)<=audio_epoch&&!ds_audio_tick(audio))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
        ds_audio_output_push(app->audio_output,ds_audio_pcm(audio),DS_AUDIO_FRAMES);
    }
    return FALSE;
}
static int scene_exit(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);hg_blackout_free(app->blackout);app->blackout=NULL;hg_battle_free(app->battle);hg_battle_view_free(app->battle_view);app->battle=NULL;app->battle_view=NULL;ds_audio_output_free(app->audio_output);app->audio_output=NULL;hg_field_view_free(app->pending_view);app->pending_view=NULL;app->transition_phase=0;hg_field_view_free(app->zone_view);app->zone_view=NULL;hg_field_view_free(app->view);app->view=NULL;hg_field_menu_free(app->field_menu);app->field_menu=NULL;hg_field_player_free(app->player);app->player=NULL;hg_field_view_free(app->player_sprite);app->player_sprite=NULL;hg_font_free(app->font);app->font=NULL;return TRUE;
}
static BOOL run_scene_application(TaskManager *task) {
    HGFieldApp *app=TaskManager_GetEnvironment(task);u32 *state=TaskManager_GetStatePtr(task);
    if(*state==0) {
        static const OverlayManagerTemplate template={scene_init,scene_exec,scene_exit,FS_OVERLAY_ID_NONE};
        *state=1;CallApplicationAsTask(task,&template,app);return FALSE;
    }
    return TRUE;
}
static HGFieldApp *app_new_state(unsigned map,int new_game,const HGStoryState *story,const HGFieldLocation *where,int shoes,char *error,size_t size) {
    if(error && size)error[0]=0;
    HGFieldApp *app=calloc(1,sizeof(*app));if(!app) { if(error && size)snprintf(error,size,"Out of memory");return NULL; }
    if(story){app->resume_story=hg_story_clone(story);if(!app->resume_story){hg_story_free(app->resume_story);free(app->save_path);free(app);return NULL;}app->resume_location=*where;app->resume_shoes=shoes;}
    app->map=map;app->new_game=new_game==1;app->exploring=new_game==2;FieldSystem_CreateTask(&app->runtime,run_scene_application,app);
    /* First frame calls a child task; second launches/initializes its app. */
    hg_field_tasks_tick(&app->runtime);hg_field_tasks_tick(&app->runtime);GF_ASSERT(app->initialized);
    if(!app->view||(story&&!app->player)) { if(error && size)snprintf(error,size,"%s",app->error);hg_field_app_free(app);return NULL; }
    if(app->player&&!app->exploring) {
        app->field_time=hg_field_time_new();
        if(!app->field_time){if(error&&size)snprintf(error,size,"Cannot allocate the original field-time owner");hg_field_app_free(app);return NULL;}
        if(app->new_game){
            HGTimeSample sample;
            if(!hg_field_time_sample(&sample,error,size)){hg_field_app_free(app);return NULL;}
            FieldSystem field={0};hg_story_bind_field(hg_field_player_story(app->player),&field);
            hg_field_time_claim(field.saveData,&sample);
        }
    }
    update_play_time(app);
    return app;
}
static HGFieldApp *app_new(unsigned map,int game,char *error,size_t size){return app_new_state(map,game,NULL,NULL,0,error,size);}
HGFieldApp *hg_field_app_new(unsigned map,char *error,size_t size) { return app_new(map,0,error,size); }
HGFieldApp *hg_field_app_new_game(char *error,size_t size) { HGFieldLocation start=hg_field_new_game_location();return app_new((unsigned)start.map,1,error,size); }
void hg_field_app_free(HGFieldApp *app) {
    if(!app)return;app->stop=1;
    /* Exec completes, then exit, then the original waiting task unwinds its
     * child and parent. Never forcibly free a task with an unknown environment. */
    for(unsigned i=0;i<5 && !hg_field_tasks_idle(&app->runtime);i++)hg_field_tasks_tick(&app->runtime);
    GF_ASSERT(hg_field_tasks_idle(&app->runtime));hg_story_free(app->resume_story);hg_field_time_free(app->field_time);free(app->save_path);free(app);
}
void hg_field_app_step(HGFieldApp *app) { if(app){update_play_time(app);if(update_field_time(app))hg_field_tasks_tick(&app->runtime);} }
unsigned long long hg_field_app_ticks(const HGFieldApp *app) { return app?app->runtime.ticks:0; }
const HGFieldView *hg_field_app_view(const HGFieldApp *app) { return app?app->view:NULL; }
int hg_field_app_render(HGFieldApp *app,const DSCamera *camera,uint8_t *pixels,unsigned width,unsigned height) {
    if(app&&app->blackout&&pixels&&width&&height){
        for(size_t i=0;i<(size_t)width*height;i++){pixels[4*i]=pixels[4*i+1]=pixels[4*i+2]=255;pixels[4*i+3]=255;}
        if(app->font&&width>=256&&height>=192){
            u8 panel[256*192*4];memset(panel,255,sizeof(panel));hg_font_draw(app->font,hg_blackout_message(app->blackout),panel,256,192,24,40,208,0x202020,0xffffff);
            unsigned scale=width>=512&&height>=384?2:1,left=(width-256*scale)/2,top=(height-192*scale)/2;
            for(unsigned y=0;y<192*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
        }
        return ds_brightness_apply(pixels,(size_t)width*height,hg_blackout_brightness(app->blackout));
    }
    if(app&&app->battle)return hg_battle_view_render(app->battle_view,app->battle,app->battle_state,app->battle_choice,pixels,width,height);
    if(!app || !camera || !pixels)return 0;
    int ok;
    if(app->player) {
        if(!hg_field_player_present_props(app->player,app->view))return 0;
        HGFieldBillboard actors[65]={0};unsigned count=0,total=hg_field_player_actor_counts(app->player).active;
        for(unsigned i=0;i<total;i++) { HGFieldActor actor;if(!hg_field_player_actor(app->player,i,&actor))return 0;if(!actor.hidden)actors[count++]=actor.billboard; }
        if(!hg_field_player_hidden(app->player)){actors[count].sprite=app->player_sprite;memcpy(actors[count].world,app->visual,sizeof(app->visual));count++;}const HGFieldView *props=hg_field_player_props(app->player);
        if(props)actors[count++]=(HGFieldBillboard){.sprite=props,.fixed=1};
        ok=hg_field_view_render_actors(app->view,actors,count,camera,pixels,width,height);
    } else ok=hg_field_view_render(app->view,camera,pixels,width,height);
    /* Native glyph presentation of the original emote update. The original
     * NNS effect model and graphics task controller are not executed. */
    if(ok&&app->player&&app->font)for(unsigned i=0;i<hg_field_player_actor_counts(app->player).active;i++) {
        HGFieldActor actor;if(!hg_field_player_actor(app->player,i,&actor)||!actor.emote)continue;
        double world[]={actor.billboard.world[0],actor.billboard.world[1]+38+actor.emote_offset,actor.billboard.world[2]},screen[3];
        if(hg_field_view_project(app->view,world,camera,width,height,screen)&&screen[0]>=0&&screen[0]<width&&screen[1]>=0&&screen[1]<height) {
            /* Original font expansion is bounded to DS-sized surfaces. Draw
             * locally, then composite into the larger native viewport. */
            uint8_t glyph[16*16*4]={0};hg_font_draw(app->font,"!",glyph,16,16,0,0,16,0xffe040,0x202830);
            int left=(int)screen[0]-3,top=(int)screen[1]-8;
            for(int y=0;y<16;y++)for(int x=0;x<16;x++)if(glyph[4*(y*16+x)+3]&&left+x>=0&&top+y>=0&&(unsigned)(left+x)<width&&(unsigned)(top+y)<height)
                memcpy(pixels+4*((size_t)(top+y)*width+(unsigned)(left+x)),glyph+4*(y*16+x),4);
        }
    }
    if(ok&&app->player&&(hg_field_player_phone_active(app->player)||hg_field_time_pending(app->field_time,NULL))) {
        for(size_t i=0;i<(size_t)width*height;i++){pixels[4*i]=24;pixels[4*i+1]=48;pixels[4*i+2]=72;pixels[4*i+3]=255;}
        if(app->font&&width>=256&&height>=80) {
            uint8_t header[256*64*4];for(unsigned i=0;i<256*64;i++){header[4*i]=48;header[4*i+1]=88;header[4*i+2]=112;header[4*i+3]=255;}
            hg_font_draw(app->font,"Pokégear · Phone",header,256,64,10,8,236,0xf7f9ec,0x182c40);
            hg_font_draw(app->font,hg_field_player_phone_active(app->player)?hg_field_player_phone_contact(app->player):"Incoming call - press Space",header,256,64,10,32,236,0xf7f9ec,0x182c40);
            unsigned scale=width>=512&&height>=160?2:1,left=(width-256*scale)/2;
            for(unsigned y=0;y<64*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(y+8)*width+left+x),header+4*((y/scale)*256+x/scale),4);
        }
    }
    if(ok&&app->player) {
        /* Field brightness belongs to the scene. An application draws its
         * modal surface above it while original field tasks wait. */
        if(hg_field_player_starter_pending(app->player))ok=ds_brightness_apply(pixels,(size_t)width*height,hg_field_player_brightness(app->player));
        if((hg_field_player_starter_pending(app->player)||hg_field_player_menu_pending(app->player)||app->catch_choice)&&app->font&&width>=256&&height>=112) {
            uint8_t panel[256*112*4];for(unsigned i=0;i<256*112;i++){panel[4*i]=247;panel[4*i+1]=249;panel[4*i+2]=236;panel[4*i+3]=255;}
            hg_font_draw(app->font,app->catch_choice?"Which Pokemon will you keep?":hg_field_player_menu_pending(app->player)?"Choose an answer":"Choose your first Pokémon",panel,256,112,8,6,240,0x202830,0xc0c8c0);
            int menu=hg_field_player_menu_pending(app->player)||app->catch_choice;
            for(unsigned i=0;i<(hg_field_player_menu_pending(app->player)?hg_field_player_menu_count(app->player):menu?2u:3u);i++) {
                HGStarterOption option={0};if(!menu&&!hg_field_player_starter_option(app->player,i,&option))return 0;
                if(i==app->choice_index)for(unsigned y=26+20*i;y<46+20*i;y++)for(unsigned x=6;x<250;x++) {
                    uint8_t *pixel=panel+4*(y*256+x);pixel[0]=184;pixel[1]=216;pixel[2]=240;
                }
                char row[96];if(app->catch_choice){Pokemon *mon=i?&app->contest_catch:hg_field_player_contest(app->player)->mon;snprintf(row,sizeof(row),"  %s",hg_pokemon_species_name(GetMonData(mon,MON_DATA_SPECIES,NULL)));}
                else if(menu)snprintf(row,sizeof(row),"  %s%s",hg_field_player_menu_label(app->player,i),hg_field_player_menu_choice_supported(app->player,i)?"":" (unavailable)");
                else snprintf(row,sizeof(row),"  %s  Lv%u  HP %u",option.name,option.level,option.hp);
                hg_font_draw(app->font,row,panel,256,112,8,28+20*i,240,0x202830,0xc0c8c0);
            }
            hg_font_draw(app->font,"W/S: choose   Enter: confirm",panel,256,112,8,94,240,0x405060,0xc0c8c0);
            unsigned scale=width>=512&&height>=224?2:1,left=(width-256*scale)/2,top=(height-112*scale)/2;
            for(unsigned y=0;y<112*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
        }
    }
    if(ok && app->dialogue[0] && app->font && width>=256 && height>=80) {
        unsigned char panel[256*64*4];
        for(unsigned y=0;y<64;y++)for(unsigned x=0;x<256;x++) {
            unsigned char *p=panel+4*(y*256+x);int edge=x<2||x>=254||y<2||y>=62;
            p[0]=edge?48:247;p[1]=edge?64:249;p[2]=edge?80:236;p[3]=255;
        }
        hg_font_draw(app->font,app->dialogue_page,panel,256,64,10,8,236,0x202830,0xc0c8c0);
        hg_font_draw(app->font,app->dialogue[app->dialogue_cursor]?"Enter / Space for next":"Enter / Space to close",panel,256,64,10,44,236,0x405060,0xc0c8c0);
        unsigned scale=width>=512&&height>=160?2:1;
        unsigned left=(width-256*scale)/2,top=height-64*scale-8;
        for(unsigned y=0;y<64*scale;y++)for(unsigned x=0;x<256*scale;x++)
            memcpy(pixels+4*((top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
    }
    if(ok&&app->player&&!hg_field_player_phone_active(app->player)&&!hg_field_player_starter_pending(app->player))ok=ds_brightness_apply(pixels,(size_t)width*height,hg_field_player_brightness(app->player));
    if(ok&&hg_field_app_name_pending(app)&&app->font&&width>=256&&height>=128){
        uint8_t panel[256*128*4];for(unsigned i=0;i<256*128;i++){panel[4*i]=247;panel[4*i+1]=249;panel[4*i+2]=236;panel[4*i+3]=255;}
        hg_font_draw(app->font,"What is your rival's name?",panel,256,128,8,8,240,0x202830,0xc0c8c0);
        hg_font_draw(app->font,hg_field_app_name_text(app),panel,256,128,8,40,240,0x202830,0xc0c8c0);
        hg_font_draw(app->font,"Type up to 7 characters",panel,256,128,8,72,240,0x405060,0xc0c8c0);
        hg_font_draw(app->font,"Enter: confirm   Esc: retry",panel,256,128,8,100,240,0x405060,0xc0c8c0);
        unsigned scale=width>=512&&height>=256?2:1,left=(width-256*scale)/2,top=(height-128*scale)/2;
        for(unsigned y=0;y<128*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
    }
    HGShop *shop=hg_field_player_shop(app->player);
    if(ok&&hg_shop_open(shop)&&app->font&&width>=256&&height>=384){
        u8 panel[256*384*4];memset(panel,245,sizeof(panel));for(unsigned i=0;i<256*384;i++)panel[4*i+3]=255;
        char text[2048];hg_shop_text(shop,text,sizeof(text));hg_font_draw(app->font,text,panel,256,384,12,12,232,0x182838,0);
        unsigned scale=height/384;if(scale>width/256)scale=width/256;unsigned left=width-256*scale,top=(height-384*scale)/2;
        for(unsigned y=0;y<384*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
    }
    if(ok&&hg_field_menu_kind(app->field_menu))ok=hg_field_menu_render(app->field_menu,pixels,width,height);
    return ok;
}

void hg_field_app_input(HGFieldApp *app,uint16_t keys) {
    if(!app)return;keys&=0xf2;
    if(!(app->held_keys&0xf0))app->move_delay=0;
    /* Only directional edges queue an action; a short Shift press alone must
     * not start a move or turn a later walk into an unintended run. */
    /* Menu consumption clears held_keys, but a physical key stays down until
     * input reports its release. Do not turn each simulation tick into a press. */
    uint16_t pressed=(keys&~app->input_keys)&0xf0;
    if(pressed)app->pending_keys|=pressed|(keys&2);app->held_keys=keys;app->input_keys=keys;
}
void hg_field_app_clear_input(HGFieldApp *app) { if(app)app->input_keys=app->held_keys=app->pending_keys=0; }
int hg_field_app_player(const HGFieldApp *app,int *x,int *z) { return app&&hg_field_player_position(app->player,x,z,NULL); }
void hg_field_app_camera(const HGFieldApp *app,DSCamera *camera,unsigned w,unsigned h) {
    if(!app)return;
    if(app->player)hg_field_view_focus(app->view,app->visual,camera,w,h);
    else hg_field_view_camera(app->view,camera,w,h);
}

unsigned long long hg_field_app_revision(const HGFieldApp *app) { return app?app->revision:0; }
int hg_field_app_presentation(const HGFieldApp *app,double world[3],unsigned *frame) {
    if(!app||!app->player)return 0;
    if(world)memcpy(world,app->visual,sizeof(app->visual));if(frame)*frame=app->frame;return 1;
}

void hg_field_app_interact(HGFieldApp *app) {
    if(answer_incoming(app))return;
    if(app&&app->player){
        app->held_keys=0;
        /* A direction and confirmation can arrive in the same native frame.
         * Keep the queued selection for menus; discard field walking input. */
        if(!hg_field_menu_kind(app->field_menu)&&!hg_shop_open(hg_field_player_shop(app->player))&&!app->battle&&!app->catch_choice&&!hg_field_player_menu_pending(app->player)&&!hg_field_player_starter_pending(app->player))app->pending_keys=0;
        app->interact_pending=1;
    }
}
const char *hg_field_app_dialogue(const HGFieldApp *app) { return app&&hg_blackout_waiting(app->blackout)?hg_blackout_message(app->blackout):app&&app->battle&&app->battle_state==HG_BATTLE_MESSAGE?hg_battle_message(app->battle):app?app->dialogue:""; }
const char *hg_field_app_dialogue_page(const HGFieldApp *app) { return app&&hg_blackout_waiting(app->blackout)?hg_blackout_message(app->blackout):app?app->dialogue_page:""; }
int hg_field_app_interaction_busy(const HGFieldApp *app) { return app&&(app->blackout||app->catch_choice||app->transition_phase||hg_field_menu_kind(app->field_menu)||app->battle||app->dialogue[0]||hg_field_player_interaction_busy(app->player)); }

unsigned hg_field_app_advance(HGFieldApp *app,uint64_t monotonic_ns) {
    if(!app)return 0;unsigned count=ds_frame_clock_advance(&app->frame_clock,monotonic_ns);
    for(unsigned i=0;i<count;i++)hg_field_app_step(app);return count;
}
void hg_field_app_resume(HGFieldApp *app) {
    if(app){app->frame_clock=(DSFrameClock){0};hg_field_app_clear_input(app);}
}
unsigned hg_field_app_steps(const HGFieldApp *app) { return app?hg_field_player_steps(app->player):0; }
unsigned hg_field_app_map(const HGFieldApp *app) { return app?app->map:UINT32_MAX; }
int hg_field_app_transition_active(const HGFieldApp *app) { return app&&app->transition_phase; }
HGStoryState *hg_field_app_story(HGFieldApp *app) { return app?hg_field_player_story(app->player):NULL; }

int hg_field_app_starter_pending(const HGFieldApp *app) { return app&&hg_field_player_starter_pending(app->player); }
int hg_field_app_menu_pending(const HGFieldApp *app) { return app&&hg_field_player_menu_pending(app->player); }
int hg_field_app_menu_choice_supported(HGFieldApp *app,unsigned choice) { return app&&hg_field_player_menu_choice_supported(app->player,choice); }

int hg_field_app_follower(const HGFieldApp *app,unsigned *species,int *x,int *z) { return app&&hg_field_player_follower(app->player,species,x,z); }
const char *hg_field_app_error(const HGFieldApp *app) { return app?app->error:"No field application"; }

int hg_field_app_running_shoes(const HGFieldApp *app) { return app&&hg_field_player_running_shoes(app->player); }

HGFieldApp *hg_field_app_new_explore(unsigned map,char *error,size_t size) { return app_new(map,2,error,size); }
int hg_field_app_is_exploring(const HGFieldApp *app) { return app&&app->exploring; }

int hg_field_app_brightness(const HGFieldApp *app) { return app?app->blackout?hg_blackout_brightness(app->blackout):hg_field_player_brightness(app->player):0; }

int hg_field_app_phone_active(const HGFieldApp *app) { return app&&hg_field_player_phone_active(app->player); }

int hg_field_app_audio_enable(HGFieldApp *app,int enabled,char *error,size_t size) {
    if(!app)return 0;
    if(!enabled){ds_audio_output_free(app->audio_output);app->audio_output=NULL;return 1;}
    if(!app->audio_output)app->audio_output=ds_audio_output_new(error,size);
    return app->audio_output!=NULL;
}

uint64_t hg_field_app_audio_frames(HGFieldApp *app){return app?ds_audio_output_frames(app->audio_output):0;}

int hg_field_app_battle_active(const HGFieldApp *app){return app&&app->battle!=NULL;}
int hg_field_app_battle_state(const HGFieldApp *app){return app&&app->battle?app->battle_state:HG_BATTLE_FINISHED;}

int hg_field_app_battle_mon(const HGFieldApp *app,unsigned side,HGBattleMonView *mon){return app&&app->battle&&hg_battle_mon(app->battle,side,mon);}
int hg_field_app_battle_choose(HGFieldApp *app,unsigned choice){
    if(!app||!app->battle)return 0;
    if(app->battle_state==HG_BATTLE_NICKNAME){if(choice)return 0;}
    else if(!hg_battle_choice_code(app->battle,app->battle_state,choice))return 0;
    app->battle_choice=choice;app->interact_pending=1;app->held_keys=app->pending_keys=0;return 1;
}

static int field_party_swap(void *owner,unsigned first,unsigned second){return hg_field_player_swap_party(owner,first,second);}
int hg_field_app_inspect(HGFieldApp *app,int kind){
    if(!app||kind<0||kind>2)return 0;
    if(hg_field_menu_kind(app->field_menu)==kind)kind=0;
    if(!kind){hg_field_menu_open(app->field_menu,0);hg_field_app_clear_input(app);app->revision++;return 1;}
    if(!app->player||app->exploring||app->blackout||app->battle||app->dialogue[0]||hg_field_player_interaction_busy(app->player)||hg_field_player_walking(app->player))return 0;
    if(!app->field_menu){FieldSystem field={0};hg_field_player_bind_runtime(app->player,&field);app->field_menu=hg_field_menu_new(field.saveData,MapHeader_GetMapSec(app->map));}
    if(!app->field_menu)return 0;hg_field_menu_set_party_swap(app->field_menu,field_party_swap,app->player);hg_field_menu_open(app->field_menu,kind);hg_field_app_clear_input(app);app->revision++;return hg_field_menu_kind(app->field_menu)==kind;
}
int hg_field_app_inspect_kind(const HGFieldApp *app){return app?hg_field_menu_kind(app->field_menu):0;}
void hg_field_app_inspect_text(HGFieldApp *app,char *out,size_t size){hg_field_menu_text(app?app->field_menu:NULL,out,size);}

int hg_field_app_battle_trainer(const HGFieldApp *app){return app&&app->battle&&app->script_battle&&!hg_battle_tutorial(app->battle);}

typedef struct {uint32_t magic,version,size,checksum,stage,map,x,z,direction,shoes;} HGCheckpointHeader;
static uint32_t checkpoint_hash(const void *bytes,size_t size,uint32_t hash){const uint8_t *p=bytes;for(size_t i=0;i<size;i++)hash=(hash^p[i])*16777619u;return hash;}
static int checkpoint_map(unsigned stage,unsigned map){return stage==1||stage==4||stage==5?map==33:(stage==2||stage==3)&&map==67;}
int hg_field_app_checkpoint_write(HGFieldApp *app,const char *path,unsigned stage,char *error,size_t size){
    if(error&&size)error[0]=0;int x,z;
    if(!app||!path||app->exploring||!app->player||hg_field_app_interaction_busy(app)||hg_field_player_walking(app->player)||!hg_field_app_player(app,&x,&z)||!checkpoint_map(stage,app->map)||!hg_story_checkpoint_stage(hg_field_app_story(app),stage)){
        if(error&&size)snprintf(error,size,"Checkpoint needs a completed opening-route stage and an idle field");return 0;}
    size_t length=hg_story_snapshot_size();uint8_t *bytes=malloc(length);if(!bytes)return 0;
    HGCheckpointHeader h={.magic=0x50434748,.version=3,.size=(uint32_t)length,.stage=stage,.map=app->map,.x=(uint32_t)x,.z=(uint32_t)z,.direction=(unsigned)hg_field_player_facing(app->player),.shoes=hg_field_app_running_shoes(app)};
    HGStoryState *snapshot=hg_story_clone(hg_field_app_story(app));
    if(!snapshot||!hg_field_player_prepare_save(app->player,snapshot)){hg_story_free(snapshot);free(bytes);if(error&&size)snprintf(error,size,"Wait for all field objects to stop before saving");return 0;}
    hg_story_snapshot_export(snapshot,bytes,length);hg_story_free(snapshot);h.checksum=checkpoint_hash(bytes,length,checkpoint_hash(&h,sizeof(h),2166136261u));
    char temporary[4096];int count=snprintf(temporary,sizeof(temporary),"%s.tmp",path);int ok=0;FILE *file=count>0&&(size_t)count<sizeof(temporary)?fopen(temporary,"wb"):NULL;
    if(file){ok=fwrite(&h,sizeof(h),1,file)==1&&fwrite(bytes,length,1,file)==1;if(fclose(file))ok=0;if(ok)ok=rename(temporary,path)==0;}free(bytes);
    if(!ok&&error&&size)snprintf(error,size,"Could not write checkpoint");return ok;
}
HGFieldApp *hg_field_app_checkpoint_load(const char *path,char *error,size_t size){
    if(error&&size)error[0]=0;FILE *file=path?fopen(path,"rb"):NULL;HGCheckpointHeader h={0};HGStoryState *story=NULL;HGFieldApp *app=NULL;
    if(!file)goto invalid;
    if(fread(&h,sizeof(h),1,file)!=1||h.magic!=0x50434748||h.version!=3||h.size!=hg_story_snapshot_size()||h.direction>3||h.shoes>1||h.x>2047||h.z>2047||!checkpoint_map(h.stage,h.map))goto close;
    uint8_t *bytes=malloc(h.size);if(!bytes)goto close;int valid=fread(bytes,h.size,1,file)==1&&fgetc(file)==EOF;uint32_t expected=h.checksum;h.checksum=0;
    valid=valid&&expected==checkpoint_hash(bytes,h.size,checkpoint_hash(&h,sizeof(h),2166136261u));if(valid)story=hg_story_snapshot_import(bytes,h.size);free(bytes);
    if(story&&hg_story_checkpoint_stage(story,h.stage)){
        HGFieldLocation where={(int)h.map,-1,(int)h.x,(int)h.z,(int)h.direction};app=app_new_state(h.map,0,story,&where,(int)h.shoes,error,size);
    }
    hg_story_free(story);
close:fclose(file);
invalid:if(!app&&error&&size&&!error[0])snprintf(error,size,"Checkpoint is incompatible, damaged, or missing its required story state");return app;
}

int hg_field_app_name_pending(const HGFieldApp *p) { return p?hg_field_player_name_pending(p->player):0; }

const char * hg_field_app_name_text(const HGFieldApp *p) { return p?hg_field_player_name_text(p->player):""; }

int hg_field_app_name_edit(HGFieldApp *p,const char *text) { return p?hg_field_player_name_edit(p->player,text):0; }

int hg_field_app_name_submit(HGFieldApp *p,const char *text,int cancel) { return p?hg_field_player_name_submit(p->player,text,cancel):0; }

int hg_field_app_battle_tutorial(const HGFieldApp *app){return app&&hg_battle_tutorial(app->battle);}
unsigned hg_field_app_capture_phase(const HGFieldApp *app){return app?hg_battle_capture_phase(app->battle):0;}

const char *hg_field_app_save_path(const HGFieldApp *app) { return app&&app->save_path?app->save_path:""; }
int hg_field_app_save(HGFieldApp *app,const char *path,char *error,size_t size) {
    if(error&&size)error[0]=0;
    if(!app||!app->player||app->exploring||!path||!*path||hg_field_app_interaction_busy(app)) {
        if(error&&size)snprintf(error,size,"Save requires an idle story field");return 0;
    }
    if(hg_field_player_contest(app->player)){if(error&&size)snprintf(error,size,"Save is unavailable during the Bug-Catching Contest");return 0;}
    if(app->save_path&&strcmp(app->save_path,path)) {
        if(error&&size)snprintf(error,size,"Use the current save file to preserve its extra records");return 0;
    }
    update_play_time(app);
    if(!update_field_time(app)){if(error&&size)snprintf(error,size,"%s",app->error);return 0;}
    HGStoryState *live=hg_field_app_story(app),*candidate=hg_story_clone(live);
    char *owned_path=strdup(path);unsigned long long revision=hg_story_revision(live);
    if(!candidate||!owned_path||!hg_field_player_prepare_save(app->player,candidate)) {
        hg_story_free(candidate);free(owned_path);
        if(error&&size)snprintf(error,size,"Wait for all field objects to stop before saving");return 0;
    }
    FieldSystem field={0};hg_story_bind_field(candidate,&field);
    int ok=ds_save_flash_open(path);
    if(ok){ok=hg_save_manager_write(field.saveData,error,size);ds_save_flash_close();}
    else if(error&&size)snprintf(error,size,"%s",ds_save_flash_last_error());
    if(ok)ok=hg_story_commit(live,candidate,revision);
    hg_story_free(candidate);
    if(ok){free(app->save_path);app->save_path=owned_path;}else free(owned_path);
    return ok;
}
HGFieldApp *hg_field_app_continue(const char *path,char *error,size_t size) {
    unsigned previous_button_mode=gSystem.buttonMode;
    if(error&&size)error[0]=0;
    SaveData *save=malloc(sizeof(*save));if(!save)return NULL;
    if(!path||!ds_save_flash_open_existing(path)) {
        if(error&&size)snprintf(error,size,"%s",ds_save_flash_last_error());free(save);return NULL;
    }
    int ok=hg_save_manager_load(save,error,size);ds_save_flash_close();
    HGFieldTime *field_time=ok?hg_field_time_new():NULL;HGTimeSample sample;
    u32 previous_rng=GetLCRNGSeed();SetLCRNGSeed(arc4random());
    if(ok)ok=field_time&&hg_field_time_sample(&sample,error,size)&&hg_field_time_continue(field_time,save,&sample,error,size);
    u32 continued_rng=GetLCRNGSeed();SetLCRNGSeed(previous_rng);
    HGStoryState *story=ok?hg_story_from_save_records(save):NULL;
    if(story)hg_story_rng_set(story,continued_rng);
    HGFieldApp *app=NULL;
    if(story) {
        LocalFieldData *local=SaveArray_Get(save,SAVE_LOCAL_FIELD_DATA);
        Location *location=LocalFieldData_GetCurrentPosition(local);
        HGFieldLocation where={location->mapId,location->warpId,location->x,location->y,location->direction};
        int shoes=PlayerSaveData_CheckRunningShoes(LocalFieldData_GetPlayer(local));
        app=app_new_state((unsigned)where.map,0,story,&where,shoes,error,size);
        if(app&&!hg_field_player_restore_save_objects(app->player)){hg_field_app_free(app);app=NULL;}
        if(app){app->save_path=strdup(path);if(!app->save_path){hg_field_app_free(app);app=NULL;}}
    }
    if(app){hg_field_time_free(app->field_time);app->field_time=field_time;field_time=NULL;
        app->field_time_second=RTC_ConvertDateTimeToSecond(&sample.rtc.date,&sample.rtc.time);app->field_time_sampled=1;}
    hg_field_time_free(field_time);hg_story_free(story);free(save);
    if(!app)gSystem.buttonMode=previous_button_mode;
    if(!app&&error&&size&&!error[0])snprintf(error,size,"Cannot reconstruct the saved field with current runtime services");
    return app;
}

int hg_field_app_save_unlocked(HGFieldApp *app) {
    int unlocked=0;return app&&!app->exploring&&hg_story_flag_get(hg_field_app_story(app),FLAG_GOT_SAVE_BUTTON,&unlocked)&&unlocked;
}

unsigned hg_field_app_battle_command_count(const HGFieldApp *app){return app&&app->battle?hg_battle_command_count(app->battle):0;}

struct HGShop *hg_field_app_shop(HGFieldApp *app){return app?hg_field_player_shop(app->player):NULL;}
void hg_field_app_shop_cancel(HGFieldApp *app){if(app)hg_shop_input(hg_field_player_shop(app->player),0,0,1);}
unsigned hg_field_app_menu_count(const HGFieldApp *app){return app?hg_field_player_menu_count(app->player):0;}

int hg_field_app_battle_cancel(HGFieldApp *app){
    if(!app||!hg_battle_menu_cancellable(app->battle))return 0;
    app->battle_choice=UINT32_MAX;app->interact_pending=1;app->held_keys=app->pending_keys=0;return 1;
}

HGFieldApp *hg_field_app_continue_copy(const char *source,const char *directory,const char *name,char *error,size_t size){
    if(error&&size)error[0]=0;
    char path[4096];
    if(!source||!directory||!name||snprintf(path,sizeof(path),"%s/%s-XXXXXX.sav",directory,name)>=(int)sizeof(path))return NULL;
    FILE *input=fopen(source,"rb");if(!input){if(error&&size)snprintf(error,size,"Generate the Violet arrival checkpoint first");return NULL;}
    int fd=mkstemps(path,4);FILE *output=fd<0?NULL:fdopen(fd,"wb");
    if(!output){if(fd>=0){close(fd);unlink(path);}fclose(input);if(error&&size)snprintf(error,size,"Cannot create a separate team save");return NULL;}
    unsigned char bytes[8192];size_t length;int ok=1;
    while((length=fread(bytes,1,sizeof(bytes),input)))if(fwrite(bytes,1,length,output)!=length){ok=0;break;}
    if(ferror(input))ok=0;
    if(fclose(input))ok=0;if(fclose(output))ok=0;
    HGFieldApp *app=ok?hg_field_app_continue(path,error,size):NULL;
    if(!app){unlink(path);if(error&&size&&!error[0])snprintf(error,size,"Cannot copy the checkpoint save");}
    return app;
}
