#include "ds_audio_output.h"
#include "map_header.h"
#include "constants/sndseq.h"
#include "field_app.h"
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
struct HGFieldApp {
    FieldSystem runtime;DSFrameClock frame_clock;
    int initialized,new_game,exploring;
    HGFieldView *view,*player_sprite,*zone_view;
    HGFieldPlayer *player;HGFont *font;
    char dialogue[HG_SCRIPT_TEXT_CAPACITY],dialogue_page[512];size_t dialogue_cursor;int interact_pending;
    uint16_t held_keys,pending_keys,input_keys;unsigned move_delay,walk_serial,frame;
    unsigned long long revision;
    double visual[3],from[3];
    unsigned map,choice_index;int choice_active;
    DSAudioOutput *audio_output;unsigned audio_map;int audio_started;
    HGBattle *battle;HGBattleView *battle_view;HGEncounterState encounters;
    int battle_state;unsigned battle_choice;
    HGFieldMenu *field_menu;
    int stop;
    char error[256];
};
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
    snprintf(app->error,sizeof(app->error),"%s",error);
    snprintf(app->dialogue,sizeof(app->dialogue),"This interaction needs a game feature\nthat isn’t available yet.");
    app->dialogue_cursor=0;next_dialogue_page(app);app->revision++;
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
    HGBattleSetup setup={.party=hg_story_save_party(field.saveData),.opponent=&opponent,.profile=hg_story_save_profile(field.saveData),.bag=hg_story_save_bag(field.saveData),.options=hg_story_save_options(field.saveData),.pokedex=hg_story_save_dex(field.saveData),.stats=hg_story_save_stats(field.saveData),.seed=app->encounters.rng,.audio=hg_field_player_audio(app->player)};
    app->battle=hg_battle_new_setup(&setup,app->error,sizeof(app->error));if(!app->battle)return;
    app->battle_view=hg_battle_view_new();if(!app->battle_view){hg_battle_free(app->battle);app->battle=NULL;snprintf(app->error,sizeof(app->error),"Cannot create battle presentation");return;}
    app->battle_state=HG_BATTLE_RUNNING;app->battle_choice=0;app->held_keys=app->pending_keys=0;app->interact_pending=0;app->revision++;
    if(setup.audio&&!ds_audio_play(setup.audio,DS_AUDIO_BGM,SEQ_GS_VS_NORAPOKE))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(setup.audio));
}
static void battle_frame(HGFieldApp *app){
    int old=app->battle_state;
    if(app->battle_state==HG_BATTLE_COMMAND||app->battle_state==HG_BATTLE_MOVES){
        unsigned count=app->battle_state==HG_BATTLE_COMMAND?2:4;
        if(app->pending_keys&(0x40|0x20))app->battle_choice=(app->battle_choice+count-1)%count;
        else if(app->pending_keys&(0x80|0x10))app->battle_choice=(app->battle_choice+1)%count;
    }
    if(app->battle_state==HG_BATTLE_MOVES){for(unsigned i=0;i<4&&!hg_battle_move_allowed(app->battle,app->battle_choice);i++)app->battle_choice=(app->battle_choice+1)%4;}
    if(app->interact_pending){unsigned choice=app->battle_state==HG_BATTLE_COMMAND?(app->battle_choice?4:1):app->battle_state==HG_BATTLE_MOVES?app->battle_choice+1:1;hg_battle_input(app->battle,choice);}
    app->held_keys=app->pending_keys=0;app->interact_pending=0;
    app->battle_state=hg_battle_tick(app->battle);if(app->battle_state!=old)app->battle_choice=0;app->revision++;
    if(app->battle_state==HG_BATTLE_FINISHED||app->battle_state==HG_BATTLE_FAILED){
        FieldSystem field={0};hg_field_player_bind_runtime(app->player,&field);
        if(app->battle_state==HG_BATTLE_FAILED)snprintf(app->error,sizeof(app->error),"%s",hg_battle_error(app->battle));
        else if(!hg_battle_commit(app->battle,hg_story_save_party(field.saveData),hg_story_save_profile(field.saveData),hg_story_save_bag(field.saveData),hg_story_save_dex(field.saveData),hg_story_save_stats(field.saveData)))snprintf(app->error,sizeof(app->error),"Battle result could not be published");
        else hg_story_touch(hg_field_player_story(app->player));
        hg_battle_free(app->battle);hg_battle_view_free(app->battle_view);app->battle=NULL;app->battle_view=NULL;
        app->encounters.inhibit_steps=0;app->audio_started=0;app->move_delay=8;
        if(app->error[0]){snprintf(app->dialogue,sizeof(app->dialogue),"Battle stopped: %s",app->error);app->dialogue_cursor=0;next_dialogue_page(app);}
    }
}
static int scene_init(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);
    app->view=hg_field_view_load(app->map,app->error,sizeof(app->error));if(app->view && hg_transition_supported(app->map)) {
        app->player=app->exploring?hg_field_player_new_explore(app->map):app->new_game?hg_field_player_new_game():hg_field_player_new_map(app->map);app->player_sprite=hg_player_sprite_load(app->error,sizeof(app->error));
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
    if(app->battle){battle_frame(app);goto audio_step;}
    if(hg_field_menu_kind(app->field_menu)){if(app->pending_keys){hg_field_menu_input(app->field_menu,app->pending_keys);app->revision++;}app->held_keys=app->pending_keys=0;app->interact_pending=0;goto audio_step;}
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
        if(app->choice_active!=2){app->choice_index=1;app->choice_active=2;app->revision++;}
        if(app->pending_keys&0xf0){app->choice_index^=1;app->revision++;}
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
            HGFieldView *next=hg_field_view_load((unsigned)destination.map,app->error,sizeof(app->error));
            if(next&&hg_field_player_transition(app->player,&destination)) {
                hg_field_view_free(app->view);app->view=next;app->map=(unsigned)destination.map;
                app->held_keys=app->pending_keys=0;app->move_delay=0;app->dialogue[0]=0;app->revision++;
                hg_field_player_position(app->player,NULL,NULL,app->visual);memcpy(app->from,app->visual,sizeof(app->from));
            } else { hg_field_view_free(next);app->move_delay=8; }
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
            int started=hg_field_player_coordinate_begin(app->player);
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
    if(hg_field_player_interaction_busy(app->player)&&!app->dialogue[0]) {
        int status=hg_field_player_interaction_tick(app->player,app->dialogue,sizeof(app->dialogue));
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
        if(status==HG_SCRIPT_MESSAGE){app->dialogue_cursor=0;next_dialogue_page(app);app->revision++;}
        else if(status==HG_SCRIPT_DONE)app->revision++;
        else if(status==HG_SCRIPT_ERROR)interaction_unavailable(app);
    }
    /* Initialize the presentation in the frame the original task requests it,
     * so the first rendered menu already has its real default selection. */
    if(hg_field_player_starter_pending(app->player)&&app->choice_active!=1){app->choice_index=0;app->choice_active=1;}
    if(hg_field_player_menu_pending(app->player)&&app->choice_active!=2){app->choice_index=1;app->choice_active=2;}
    if(app->player) {
        static const unsigned idle[]={1,5,9,13};
        static const unsigned stride[][2]={{2,4},{6,8},{10,12},{14,16}};
        unsigned facing=(unsigned)hg_field_player_facing(app->player),frame=idle[facing];
        if(hg_field_player_walking(app->player)) {
            double distance=fabs(app->visual[0]-app->from[0])+fabs(app->visual[2]-app->from[2]);
            if(hg_field_player_running(app->player))frame=idle[facing]+16;
            if(distance<=8)frame=stride[facing][app->walk_serial&1];
            if(hg_field_player_running(app->player)&&distance<=8)frame+=16;
        }
        if(frame!=app->frame){app->frame=frame;hg_player_sprite_frame(app->player_sprite,frame);app->revision++;}
    }
audio_step:;
    DSAudio *audio=hg_field_player_audio(app->player);
    if(audio) {
        if(!app->battle&&(!app->audio_started||app->audio_map!=app->map)) {
            if(!ds_audio_play(audio,DS_AUDIO_BGM,MapHeader_GetDayMusicId(app->map)))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
            app->audio_map=app->map;app->audio_started=1;
        }
        if(ds_audio_last_epoch(audio)<=audio_epoch&&!ds_audio_tick(audio))snprintf(app->error,sizeof(app->error),"%s",ds_audio_error(audio));
        ds_audio_output_push(app->audio_output,ds_audio_pcm(audio),DS_AUDIO_FRAMES);
    }
    return FALSE;
}
static int scene_exit(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);hg_battle_free(app->battle);hg_battle_view_free(app->battle_view);app->battle=NULL;app->battle_view=NULL;ds_audio_output_free(app->audio_output);app->audio_output=NULL;hg_field_view_free(app->zone_view);app->zone_view=NULL;hg_field_view_free(app->view);app->view=NULL;hg_field_menu_free(app->field_menu);app->field_menu=NULL;hg_field_player_free(app->player);app->player=NULL;hg_field_view_free(app->player_sprite);app->player_sprite=NULL;hg_font_free(app->font);app->font=NULL;return TRUE;
}
static BOOL run_scene_application(TaskManager *task) {
    HGFieldApp *app=TaskManager_GetEnvironment(task);u32 *state=TaskManager_GetStatePtr(task);
    if(*state==0) {
        static const OverlayManagerTemplate template={scene_init,scene_exec,scene_exit,FS_OVERLAY_ID_NONE};
        *state=1;CallApplicationAsTask(task,&template,app);return FALSE;
    }
    return TRUE;
}
static HGFieldApp *app_new(unsigned map,int new_game,char *error,size_t size) {
    if(error && size)error[0]=0;
    HGFieldApp *app=calloc(1,sizeof(*app));if(!app) { if(error && size)snprintf(error,size,"Out of memory");return NULL; }
    app->map=map;app->new_game=new_game==1;app->exploring=new_game==2;FieldSystem_CreateTask(&app->runtime,run_scene_application,app);
    /* First frame calls a child task; second launches/initializes its app. */
    hg_field_tasks_tick(&app->runtime);hg_field_tasks_tick(&app->runtime);GF_ASSERT(app->initialized);
    if(!app->view) { if(error && size)snprintf(error,size,"%s",app->error);hg_field_app_free(app);return NULL; }
    return app;
}
HGFieldApp *hg_field_app_new(unsigned map,char *error,size_t size) { return app_new(map,0,error,size); }
HGFieldApp *hg_field_app_new_game(char *error,size_t size) { HGFieldLocation start=hg_field_new_game_location();return app_new((unsigned)start.map,1,error,size); }
void hg_field_app_free(HGFieldApp *app) {
    if(!app)return;app->stop=1;
    /* Exec completes, then exit, then the original waiting task unwinds its
     * child and parent. Never forcibly free a task with an unknown environment. */
    for(unsigned i=0;i<5 && !hg_field_tasks_idle(&app->runtime);i++)hg_field_tasks_tick(&app->runtime);
    GF_ASSERT(hg_field_tasks_idle(&app->runtime));free(app);
}
void hg_field_app_step(HGFieldApp *app) { if(app)hg_field_tasks_tick(&app->runtime); }
unsigned long long hg_field_app_ticks(const HGFieldApp *app) { return app?app->runtime.ticks:0; }
const HGFieldView *hg_field_app_view(const HGFieldApp *app) { return app?app->view:NULL; }
int hg_field_app_render(HGFieldApp *app,const DSCamera *camera,uint8_t *pixels,unsigned width,unsigned height) {
    if(app&&app->battle)return hg_battle_view_render(app->battle_view,app->battle,app->battle_state,app->battle_choice,pixels,width,height);
    if(!app || !camera || !pixels)return 0;
    int ok;
    if(app->player) {
        if(!hg_field_player_present_props(app->player,app->view))return 0;
        HGFieldBillboard actors[65]={0};unsigned count=0,total=hg_field_player_actor_counts(app->player).active;
        for(unsigned i=0;i<total;i++) { HGFieldActor actor;if(!hg_field_player_actor(app->player,i,&actor))return 0;if(!actor.hidden)actors[count++]=actor.billboard; }
        actors[count].sprite=app->player_sprite;memcpy(actors[count].world,app->visual,sizeof(app->visual));
        count++;const HGFieldView *props=hg_field_player_props(app->player);
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
    if(ok&&app->player&&hg_field_player_phone_active(app->player)) {
        for(size_t i=0;i<(size_t)width*height;i++){pixels[4*i]=24;pixels[4*i+1]=48;pixels[4*i+2]=72;pixels[4*i+3]=255;}
        if(app->font&&width>=256&&height>=80) {
            uint8_t header[256*64*4];for(unsigned i=0;i<256*64;i++){header[4*i]=48;header[4*i+1]=88;header[4*i+2]=112;header[4*i+3]=255;}
            hg_font_draw(app->font,"Pokégear · Phone",header,256,64,10,8,236,0xf7f9ec,0x182c40);
            hg_font_draw(app->font,hg_field_player_phone_contact(app->player),header,256,64,10,32,236,0xf7f9ec,0x182c40);
            unsigned scale=width>=512&&height>=160?2:1,left=(width-256*scale)/2;
            for(unsigned y=0;y<64*scale;y++)for(unsigned x=0;x<256*scale;x++)memcpy(pixels+4*((size_t)(y+8)*width+left+x),header+4*((y/scale)*256+x/scale),4);
        }
    }
    if(ok&&app->player) {
        /* Field brightness belongs to the scene. An application draws its
         * modal surface above it while original field tasks wait. */
        if(hg_field_player_starter_pending(app->player))ok=ds_brightness_apply(pixels,(size_t)width*height,hg_field_player_brightness(app->player));
        if((hg_field_player_starter_pending(app->player)||hg_field_player_menu_pending(app->player))&&app->font&&width>=256&&height>=112) {
            uint8_t panel[256*112*4];for(unsigned i=0;i<256*112;i++){panel[4*i]=247;panel[4*i+1]=249;panel[4*i+2]=236;panel[4*i+3]=255;}
            hg_font_draw(app->font,hg_field_player_menu_pending(app->player)?"Choose an answer":"Choose your first Pokémon",panel,256,112,8,6,240,0x202830,0xc0c8c0);
            int menu=hg_field_player_menu_pending(app->player);
            for(unsigned i=0;i<(menu?2u:3u);i++) {
                HGStarterOption option={0};if(!menu&&!hg_field_player_starter_option(app->player,i,&option))return 0;
                if(i==app->choice_index)for(unsigned y=26+20*i;y<46+20*i;y++)for(unsigned x=6;x<250;x++) {
                    uint8_t *pixel=panel+4*(y*256+x);pixel[0]=184;pixel[1]=216;pixel[2]=240;
                }
                char row[96];if(menu)snprintf(row,sizeof(row),"  %s%s",i?"No":"Yes",hg_field_player_menu_choice_supported(app->player,i)?"":" (unavailable)");
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

void hg_field_app_interact(HGFieldApp *app) { if(app&&app->player){app->held_keys=app->pending_keys=0;app->interact_pending=1;} }
const char *hg_field_app_dialogue(const HGFieldApp *app) { return app&&app->battle&&app->battle_state==HG_BATTLE_MESSAGE?hg_battle_message(app->battle):app?app->dialogue:""; }
const char *hg_field_app_dialogue_page(const HGFieldApp *app) { return app?app->dialogue_page:""; }
int hg_field_app_interaction_busy(const HGFieldApp *app) { return app&&(hg_field_menu_kind(app->field_menu)||app->battle||app->dialogue[0]||hg_field_player_interaction_busy(app->player)); }

unsigned hg_field_app_advance(HGFieldApp *app,uint64_t monotonic_ns) {
    if(!app)return 0;unsigned count=ds_frame_clock_advance(&app->frame_clock,monotonic_ns);
    for(unsigned i=0;i<count;i++)hg_field_app_step(app);return count;
}
void hg_field_app_resume(HGFieldApp *app) {
    if(app){app->frame_clock=(DSFrameClock){0};hg_field_app_clear_input(app);}
}
unsigned hg_field_app_steps(const HGFieldApp *app) { return app?hg_field_player_steps(app->player):0; }
unsigned hg_field_app_map(const HGFieldApp *app) { return app?app->map:UINT32_MAX; }
HGStoryState *hg_field_app_story(HGFieldApp *app) { return app?hg_field_player_story(app->player):NULL; }

int hg_field_app_starter_pending(const HGFieldApp *app) { return app&&hg_field_player_starter_pending(app->player); }
int hg_field_app_menu_pending(const HGFieldApp *app) { return app&&hg_field_player_menu_pending(app->player); }
int hg_field_app_menu_choice_supported(HGFieldApp *app,unsigned choice) { return app&&hg_field_player_menu_choice_supported(app->player,choice); }

int hg_field_app_follower(const HGFieldApp *app,unsigned *species,int *x,int *z) { return app&&hg_field_player_follower(app->player,species,x,z); }
const char *hg_field_app_error(const HGFieldApp *app) { return app?app->error:"No field application"; }

int hg_field_app_running_shoes(const HGFieldApp *app) { return app&&hg_field_player_running_shoes(app->player); }

HGFieldApp *hg_field_app_new_explore(unsigned map,char *error,size_t size) { return app_new(map,2,error,size); }
int hg_field_app_is_exploring(const HGFieldApp *app) { return app&&app->exploring; }

int hg_field_app_brightness(const HGFieldApp *app) { return app?hg_field_player_brightness(app->player):0; }

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
    if(app->battle_state==HG_BATTLE_COMMAND){if(choice>=2)return 0;}
    else if(app->battle_state==HG_BATTLE_MOVES){if(choice>=4||!hg_battle_move_allowed(app->battle,choice))return 0;}
    else return 0;
    app->battle_choice=choice;app->interact_pending=1;app->held_keys=app->pending_keys=0;return 1;
}

int hg_field_app_inspect(HGFieldApp *app,int kind){
    if(!app||kind<0||kind>2)return 0;
    if(hg_field_menu_kind(app->field_menu)==kind)kind=0;
    if(!kind){hg_field_menu_open(app->field_menu,0);hg_field_app_clear_input(app);app->revision++;return 1;}
    if(!app->player||app->exploring||app->battle||app->dialogue[0]||hg_field_player_interaction_busy(app->player)||hg_field_player_walking(app->player))return 0;
    if(!app->field_menu){FieldSystem field={0};hg_field_player_bind_runtime(app->player,&field);app->field_menu=hg_field_menu_new(hg_story_save_bag(field.saveData),hg_story_save_party(field.saveData));}
    if(!app->field_menu)return 0;hg_field_menu_open(app->field_menu,kind);hg_field_app_clear_input(app);app->revision++;return hg_field_menu_kind(app->field_menu)==kind;
}
int hg_field_app_inspect_kind(const HGFieldApp *app){return app?hg_field_menu_kind(app->field_menu):0;}
void hg_field_app_inspect_text(HGFieldApp *app,char *out,size_t size){hg_field_menu_text(app?app->field_menu:NULL,out,size);}
