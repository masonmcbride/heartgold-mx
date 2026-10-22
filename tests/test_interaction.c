#include "compat/motion.h"
#include "field_events.h"
#include "field_app.h"
#include "motion_guest.h"
#include "ds_resources.h"
#include "compat/resources.h"
#include "compat/story.h"
#include "compat/pokemon.h"
#include "bag.h"
#include "constants/phone_contacts.h"
#include "save_pokegear.h"
#include <string.h>
#include <sys/stat.h>
#include "game_stats.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
static void acceptance_frame(HGFieldApp *app,unsigned species,const char *stage) {
    const char *folder=getenv("HG_ACCEPT_EVIDENCE");
    uint8_t pixels[512*384*4];DSCamera camera;hg_field_app_camera(app,&camera,512,384);CHECK(hg_field_app_render(app,&camera,pixels,512,384));
    if(!strcmp(stage,"mr-pokemon-black"))for(unsigned i=0;i<512*384;i++)CHECK(!pixels[4*i]&&!pixels[4*i+1]&&!pixels[4*i+2]);
    if(!strcmp(stage,"menu")||!strcmp(stage,"choice")) {
        unsigned highlight=0;for(unsigned i=0;i<512*384;i++)highlight+=pixels[4*i]==184&&pixels[4*i+1]==216&&pixels[4*i+2]==240;CHECK(highlight>1000);
    }
    if(!folder)return;
    char path[4096];CHECK(snprintf(path,sizeof(path),"%s/starter-%u-%s.ppm",folder,species,stage)>0);FILE *f=fopen(path,"wb");CHECK(f);CHECK(fprintf(f,"P6\n512 384\n255\n")>0);
    for(unsigned i=0;i<512*384;i++)CHECK(fwrite(pixels+4*i,1,3,f)==3);CHECK(!fclose(f));
}
/* Observe rendered pages, rather than only checking the eventual gift flags. */
static void mom_gift_page(HGFieldApp *app,void *context) {
    unsigned *seen=context;const char *page=hg_field_app_dialogue_page(app);
    if(strstr(page,"Bag to carry"))*seen|=1;
    if(strstr(page,"Trainer Card"))*seen|=2;
    if(strstr(page,"save a record"))*seen|=4;
    if(strstr(page,"also rather helpful"))*seen|=8;
    if(strstr(page,"received")&&strstr(page,"Pokégear"))*seen|=16;
    if(strstr(page,"received the Bag")){*seen|=32;if(getenv("HG_ACCEPT_EVIDENCE"))acceptance_frame(app,0,"bag-unlock");}
    if(strstr(page,"received your")&&strstr(page,"Trainer Card")){*seen|=64;if(getenv("HG_ACCEPT_EVIDENCE"))acceptance_frame(app,0,"trainer-card-unlock");}
    if(strstr(page,"Save button unlocked")){*seen|=128;if(getenv("HG_ACCEPT_EVIDENCE"))acceptance_frame(app,0,"save-unlock");}
    if(strstr(page,"Options button unlocked")){*seen|=256;if(getenv("HG_ACCEPT_EVIDENCE"))acceptance_frame(app,0,"options-unlock");}
}
static void close_conversation(HGFieldApp *app) {
    unsigned pages=0,ticks=0;
    while(hg_field_app_interaction_busy(app)&&ticks++<2048) {
        if(hg_field_app_dialogue(app)[0]){hg_field_app_interact(app);pages++;}hg_field_app_step(app);
    }
    CHECK(pages&&pages<64&&ticks<2048&&!hg_field_app_dialogue(app)[0]);
}
static void checkpoint(HGFieldApp *app,unsigned species,unsigned stage){
    mkdir("build/checkpoints",0755);char path[256],error[256];snprintf(path,sizeof(path),"build/checkpoints/%u-%u.hgcheckpoint",species,stage);
    if(!hg_field_app_checkpoint_write(app,path,stage,error,sizeof(error))){fprintf(stderr,"Checkpoint %s: %s\n",path,error);CHECK(0);}
    HGFieldApp *copy=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!copy)fprintf(stderr,"Resume %s: %s\n",path,error);CHECK(copy);
    int x,z,cx,cz;CHECK(hg_field_app_player(app,&x,&z)&&hg_field_app_player(copy,&cx,&cz)&&x==cx&&z==cz);CHECK(hg_field_app_map(app)==hg_field_app_map(copy));
    FieldSystem a={0},b={0};hg_story_bind_field(hg_field_app_story(app),&a);hg_story_bind_field(hg_field_app_story(copy),&b);
    CHECK(!memcmp(hg_story_save_party(a.saveData),hg_story_save_party(b.saveData),sizeof(Party)));CHECK(!memcmp(hg_story_save_bag(a.saveData),hg_story_save_bag(b.saveData),sizeof(Bag)));
    CHECK(hg_field_app_running_shoes(app)==hg_field_app_running_shoes(copy));hg_field_app_free(copy);
}
int main(void) {
    HGEventView view;CHECK(hg_event_load(60,&view));
    CHECK(view.background_count==5&&view.object_count==10&&view.warp_count==5&&view.coordinate_count==4);
    unsigned valid=0,missing=0;
    for(unsigned i=0;i<491;i++) {
        const uint8_t *bytes;size_t size;CHECK(native_resource_member(NARC_fielddata_eventdata_zone_event,0,i,&bytes,&size));
        if(!size){missing++;continue;}
        HGEventView decoded;CHECK(hg_event_decode(bytes,size,&decoded));valid++;
        for(size_t n=0;n<size;n++)CHECK(!hg_event_decode(bytes,n,&decoded));
    }
    CHECK(valid==464&&missing==27);
    LocalMapObject object={0};MapObjectManager manager={0};FieldSystem field={0};PlayerSaveData save;
    manager.objects=&object;manager.objectCount=1;manager.fieldSystem=&field;object.manager=&manager;object.movement=1;
    MapObject_SetFlagsBits(&object,MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
    const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
    for(unsigned i=0;i<4;i++)for(unsigned d=0;d<4;d++) {
        const uint8_t *event=view.backgrounds+20*i;
        int x=(int)(event[4]|(unsigned)event[5]<<8),z=(int)(event[8]|(unsigned)event[9]<<8);
        MapObject_SetCurrentX(&object,x-dx[d]);MapObject_SetCurrentZ(&object,z-dz[d]);
        PlayerAvatar_SetFacingDirection(avatar,d);unsigned script=0;
        CHECK(hg_motion_background_event(avatar,event,1,&script));
        CHECK(script==(unsigned)(event[0]|event[1]<<8));
        uint8_t directed[20];memcpy(directed,event,20);
        for(unsigned rule=0;rule<7;rule++) {
            directed[16]=rule;directed[17]=0;
            CHECK(hg_motion_background_event(avatar,directed,1,&script));
            int expected=rule==4||(d==0&&(rule==0||rule==6))||(d==1&&(rule==3||rule==6))||
                (d==2&&(rule==2||rule==5))||(d==3&&(rule==1||rule==5));
            CHECK(script==(expected?(unsigned)(event[0]|event[1]<<8):65535));
        }
        MapObject_SetCurrentX(&object,x+2);CHECK(hg_motion_background_event(avatar,event,1,&script)&&script==65535);
    }
    /* Execute the original coordinate selector against the actual lab trigger
     * table. Rectangle upper edges are exclusive; original story variables
     * choose entry 12 initially and entry 4 in the later lab scene. */
    HGStoryState *trigger_story=hg_story_new();CHECK(trigger_story);hg_story_bind_field(trigger_story,&field);
    HGEventView lab;CHECK(hg_event_load(61,&lab)&&lab.coordinate_count==2);
    for(unsigned scene=0;scene<3;scene++) {
        CHECK(hg_story_var_set(trigger_story,VAR_SCENE_ELMS_LAB,(uint16_t)scene));
        for(int z=8;z<=13;z++)for(int x=1;x<=8;x++) {
            MapObject_SetCurrentX(&object,x);MapObject_SetCurrentZ(&object,z);unsigned script=777;
            CHECK(hg_motion_coordinate_event(&field,avatar,lab.coordinates,lab.coordinate_count,&script));
            unsigned expected=x>=3&&x<7?(scene==0&&z==10?12:scene==1&&z==11?4:65535):65535;CHECK(script==expected);
        }
    }
    uint8_t invalid_coord[16];memcpy(invalid_coord,lab.coordinates,16);invalid_coord[14]=0;invalid_coord[15]=0x80;
    unsigned unchanged_script=777;CHECK(!hg_motion_coordinate_event(&field,avatar,invalid_coord,1,&unchanged_script)&&unchanged_script==777);
    CHECK(!hg_motion_coordinate_event(&field,avatar,lab.coordinates,101,&unchanged_script)&&unchanged_script==777);
    hg_story_free(trigger_story);field.saveData=NULL;
    char message[512];CHECK(hg_sign_script(14,message,sizeof(message)));CHECK(!strcmp(message,"Elm Pokémon Lab"));
    CHECK(hg_sign_script(15,message,sizeof(message)));CHECK(!strcmp(message,"New Bark Town\nWinds of a New Beginning"));
    for(unsigned id=1;id<=18;id++) {
        strcpy(message,"unchanged");int accepted=hg_sign_script(id,message,sizeof(message));
        CHECK(accepted==(id==8||id==14||id==15));if(!accepted)CHECK(!strcmp(message,"unchanged"));
    }
    CHECK(hg_sign_script(8,message,sizeof(message))&&!strcmp(message,"Ethan’s House"));
    strcpy(message,"unchanged");CHECK(!hg_sign_script(9,message,sizeof(message))&&!strcmp(message,"unchanged"));
    CHECK(!hg_sign_script(14,message,3)&&!strcmp(message,"unchanged"));CHECK(!hg_sign_script(0,message,sizeof(message)));
    HGFieldMessageEffects effects={.face_player=777,.select_sound=888};
    CHECK(hg_field_message(62,2,message,sizeof(message),&effects));
    CHECK(!strcmp(message,"When I grow up, I’m going to help\nmy dad!\rI’m going to be a great\nPokémon Professor!"));
    CHECK(effects.face_player==1&&effects.select_sound==1500);
    CHECK(hg_field_message(65,1,message,sizeof(message),&effects));CHECK(strstr(message,"PIKACHU is an evolved Pokémon.")&&effects.face_player==1);
    strcpy(message,"unchanged");effects=(HGFieldMessageEffects){777,888};
    CHECK(!hg_field_message(63,1,message,sizeof(message),&effects)&&!strcmp(message,"unchanged")&&effects.face_player==777&&effects.select_sound==888);
    CHECK(!hg_field_message(62,2,message,3,&effects)&&!strcmp(message,"unchanged")&&effects.face_player==777&&effects.select_sound==888);
    CHECK(!hg_field_message(540,1,message,sizeof(message),&effects));
    PlayerAvatar_FreeToHeap(avatar);
    char error[256];HGFieldApp *app=hg_field_app_new(60,error,sizeof(error));CHECK(app);
    const unsigned route[]={0x20,0x20,0x80,0x80,0x20};
    for(unsigned i=0;i<5;i++) {
        hg_field_app_input(app,route[i]);hg_field_app_step(app);hg_field_app_input(app,0);
        for(unsigned j=0;j<7;j++)hg_field_app_step(app);
    }
    int px,pz;CHECK(hg_field_app_player(app,&px,&pz));
    CHECK(px==686&&pz==400);
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(!strcmp(hg_field_app_dialogue(app),"New Bark Town\nWinds of a New Beginning"));
    DSCamera camera;hg_field_app_camera(app,&camera,320,240);unsigned char shown[320*240*4],closed[sizeof(shown)];
    CHECK(hg_field_app_render(app,&camera,shown,320,240));
    hg_field_app_input(app,0x10);for(unsigned i=0;i<16;i++)hg_field_app_step(app);
    int frozenx,frozenz;CHECK(hg_field_app_player(app,&frozenx,&frozenz)&&frozenx==px&&frozenz==pz);
    hg_field_app_interact(app);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(!hg_field_app_dialogue(app)[0]);CHECK(hg_field_app_render(app,&camera,closed,320,240));
    CHECK(memcmp(shown,closed,sizeof(shown)));hg_field_app_free(app);
    /* Actual NPC selection, presentation facing and page acknowledgment. No
     * story flag/profile variable is supplied to make a conditional script run. */
    app=hg_field_app_new(65,error,sizeof(error));CHECK(app);
    for(unsigned i=0;i<3;i++) {
        hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);
        for(unsigned tick=1;tick<8;tick++)hg_field_app_step(app);
    }
    CHECK(hg_field_app_player(app,&px,&pz)&&px==4&&pz==5);
    hg_field_app_input(app,0x10);hg_field_app_step(app);hg_field_app_clear_input(app);
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(!strcmp(hg_field_app_dialogue_page(app),"PIKACHU is an evolved Pokémon."));
    CHECK(strstr(hg_field_app_dialogue(app),"Pokémon evolution."));
    hg_field_app_camera(app,&camera,320,240);CHECK(hg_field_app_render(app,&camera,shown,320,240));
    unsigned pages=1;
    while(hg_field_app_dialogue(app)[0]&&pages<16) {
        hg_field_app_input(app,0x80);for(unsigned i=0;i<16;i++)hg_field_app_step(app);
        CHECK(hg_field_app_player(app,&frozenx,&frozenz)&&frozenx==px&&frozenz==pz);
        hg_field_app_interact(app);hg_field_app_step(app);pages++;
        if(hg_field_app_dialogue(app)[0])CHECK(hg_field_app_dialogue_page(app)[0]&&!strchr(hg_field_app_dialogue_page(app),'\r'));
    }
    CHECK(pages>2&&pages<16&&!hg_field_app_dialogue(app)[0]&&!hg_field_app_dialogue_page(app)[0]);
    CHECK(hg_field_app_render(app,&camera,closed,320,240)&&memcmp(shown,closed,sizeof(shown)));
    hg_field_app_free(app);
    /* One continuous owner: town -> house -> Mom -> town woman. Routes are
     * discovered from actual actor occupancy rather than handwritten keys. */
    app=hg_field_app_new(60,error,sizeof(error));CHECK(app);HGStoryState *story=hg_field_app_story(app);CHECK(story);
    hg_test_walk_to(app,695,397);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);
    hg_test_travel(app,63,32);
    CHECK(hg_field_app_map(app)==63&&hg_field_app_story(app)==story);hg_test_approach(app,2);
    unsigned long long revision=hg_story_revision(story);unsigned steps=hg_field_app_steps(app);
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(!strcmp(hg_field_app_dialogue(app),"Professor Elm has been waiting for\nyou, dear. Please, go and help him."));
    CHECK(hg_story_revision(story)==revision);close_conversation(app);CHECK(hg_story_revision(story)==revision+1&&hg_field_app_steps(app)==steps);
    hg_test_walk_to(app,3,10);hg_field_app_input(app,0x80);hg_field_app_step(app);hg_field_app_clear_input(app);
    hg_test_travel(app,60,32);
    CHECK(hg_field_app_map(app)==60&&hg_field_app_story(app)==story);uint16_t scene;CHECK(hg_story_var_get(story,VAR_SCENE_NEW_BARK_TOWN_OW,&scene)&&scene==0);
    hg_test_approach(app,2);hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(strstr(hg_field_app_dialogue(app),"I like your Bag!"));close_conversation(app);hg_field_app_free(app);
    app=hg_field_app_new(62,error,sizeof(error));CHECK(app);hg_test_approach(app,2);
    hg_field_app_interact(app);hg_field_app_step(app);CHECK(strstr(hg_field_app_dialogue(app),"Pokémon Professor!"));
    close_conversation(app);hg_field_app_free(app);
    const unsigned talk_maps[]={60,61,60},talk_scripts[]={16,1,1};
    const char *talk_text[]={"Yo, Ethan!","Professor Elm: If a wild Pokémon","So this is the famous Elm"};
    for(unsigned t=0;t<3;t++) {
        app=hg_field_app_new(talk_maps[t],error,sizeof(error));CHECK(app);hg_test_approach(app,talk_scripts[t]);
        int startx,startz;CHECK(hg_field_app_player(app,&startx,&startz));
        unsigned long long before=hg_story_revision(hg_field_app_story(app));
        hg_field_app_interact(app);hg_field_app_step(app);
        if(!strstr(hg_field_app_dialogue(app),talk_text[t]))fprintf(stderr,"Conversation %u: %s\n",t,hg_field_app_dialogue(app));
        CHECK(strstr(hg_field_app_dialogue(app),talk_text[t]));
        CHECK(hg_story_revision(hg_field_app_story(app))==before);
        close_conversation(app);CHECK(hg_story_revision(hg_field_app_story(app))==before+1);
        CHECK(hg_field_app_player(app,&px,&pz));CHECK(px==startx&&pz==startz+(t==2?4:0));hg_field_app_free(app);
    }
    /* The reachable west approach is exercised above. Native integration
     * fixtures exercise the north/south script branches without claiming
     * those adjacent tiles are reachable through the town's collision map. */
    for(unsigned approach=0;approach<2;approach++) {
        HGFieldPlayer *player=hg_field_player_new();CHECK(player);
        if(approach){CHECK(hg_field_player_move(player,0x80));for(unsigned i=0;i<8;i++)CHECK(hg_field_player_tick(player));}
        int startx,startz;double baseline[3];CHECK(hg_field_player_position(player,&startx,&startz,baseline));
        unsigned before_steps=hg_field_player_steps(player);story=hg_field_player_story(player);unsigned long long before=hg_story_revision(story);
        HGFieldActor rival;CHECK(hg_field_player_actor(player,0,&rival)&&rival.script==1);
        CHECK(hg_field_player_start_script(player,1,(int)rival.id));
        unsigned ticks=0;int first=0,second=0,jump=0,status=HG_SCRIPT_RUNNING;
        while(hg_field_player_interaction_busy(player)&&ticks++<2048) {
            CHECK(!hg_field_player_move(player,0x10));status=hg_field_player_interaction_tick(player,message,sizeof(message));
            CHECK(status!=HG_SCRIPT_ERROR);
            if(status==HG_SCRIPT_MESSAGE){first|=strstr(message,"famous Elm")!=NULL;second|=strstr(message,"What are you staring at?")!=NULL;CHECK(hg_field_player_interaction_ack(player));}
            double world[3];CHECK(hg_field_player_position(player,NULL,NULL,world));jump|=world[1]>baseline[1]+1;
            if(hg_field_player_interaction_busy(player))CHECK(hg_story_revision(story)==before);
        }
        CHECK(ticks<2048&&first&&second&&jump&&status==HG_SCRIPT_DONE&&hg_story_revision(story)==before+1);
        CHECK(hg_field_player_position(player,&px,&pz,NULL)&&px==startx-(approach==1)&&pz==startz+(approach==1?5:4));
        CHECK(hg_field_player_steps(player)==before_steps);CHECK(hg_field_player_actor(player,0,&rival)&&rival.x==682&&rival.z==391&&rival.direction==3);
        hg_field_player_free(player);
    }
    /* Acceptance begins at the original bedroom spawn, without running shoes.
     * All story advancement below comes from original automatic scripts. */
    const unsigned expected[]={152,155,158};
    for(unsigned starter_choice=0;starter_choice<3;starter_choice++) {
    HGFieldApp *arrival=hg_field_app_new_game(error,sizeof(error));CHECK(arrival);
    CHECK(hg_field_app_map(arrival)==64&&hg_field_app_player(arrival,&px,&pz)&&px==6&&pz==6);
    CHECK(!hg_field_app_running_shoes(arrival));
    acceptance_frame(arrival,expected[starter_choice],"bedroom");
    HGStoryState *arrival_story=hg_field_app_story(arrival);FieldSystem opening_field={0};hg_story_bind_field(arrival_story,&opening_field);
    CHECK(PlayerProfile_GetMoney(hg_story_save_profile(opening_field.saveData))==3000);
    hg_test_portal(arrival,63);
    CHECK(hg_field_app_map(arrival)==63&&hg_field_app_story(arrival)==arrival_story);
    unsigned mom_pages=0;CHECK(hg_test_settle_observe(arrival,1,mom_gift_page,&mom_pages)>0);
    CHECK((mom_pages&15)==15);CHECK((mom_pages&(32|64|128|256))==(32|64|128|256));int opening_flag;
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_BAG,&opening_flag)&&opening_flag);
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_TRAINER_CARD,&opening_flag)&&opening_flag);
    hg_test_walk_to(arrival,3,10);hg_field_app_input(arrival,0x80);for(unsigned hold=0;hold<30;hold++){hg_field_app_step(arrival);hg_field_app_input(arrival,0x80);}hg_field_app_input(arrival,0);hg_field_app_clear_input(arrival);
    CHECK(hg_field_app_map(arrival)==60&&hg_field_app_story(arrival)==arrival_story);
    hg_test_settle(arrival);
    hg_test_walk_to(arrival,684,394);
    hg_field_app_input(arrival,0x40);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    hg_test_travel(arrival,61,32);
    CHECK(hg_field_app_map(arrival)==61&&hg_field_app_story(arrival)==arrival_story);
    for(unsigned step=0;step<4;step++) {
        hg_field_app_input(arrival,0x40);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
        for(unsigned tick=1;tick<8;tick++)hg_field_app_step(arrival);
    }
    CHECK(hg_field_app_interaction_busy(arrival));unsigned arrival_ticks=0,emote_pixels=0;
    while(hg_field_app_interaction_busy(arrival)&&arrival_ticks++<2048) {
        if(hg_field_app_dialogue(arrival)[0])hg_field_app_interact(arrival);
        hg_field_app_step(arrival);
        if(arrival_ticks%5==0&&!hg_field_app_dialogue(arrival)[0]) {
            uint8_t frame[320*240*4];DSCamera view_camera;hg_field_app_camera(arrival,&view_camera,320,240);
            CHECK(hg_field_app_render(arrival,&view_camera,frame,320,240));
            for(unsigned i=0;i<320*240;i++)emote_pixels+=frame[4*i]==255&&frame[4*i+1]==224&&frame[4*i+2]==64;
        }
    }
    CHECK(arrival_ticks<2048&&emote_pixels);
    int arrival_flag;CHECK(hg_story_flag_get(arrival_story,FLAG_ELMS_LAB_PREVENT_PLAYER_ESCAPE,&arrival_flag)&&arrival_flag);
    CHECK(hg_field_app_player(arrival,&px,&pz)&&px==4&&pz==7);
    hg_test_walk_to(arrival,8,5);hg_field_app_input(arrival,0x40);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    unsigned long long choice_before=hg_story_revision(arrival_story);
    hg_field_app_interact(arrival);hg_field_app_step(arrival);
    unsigned chosen=0,menu=0,starter_ticks=0;
    while(hg_field_app_interaction_busy(arrival)&&starter_ticks++<4096) {
        CHECK(!strstr(hg_field_app_dialogue(arrival),"isn’t available yet"));
        if(hg_field_app_starter_pending(arrival)){acceptance_frame(arrival,expected[starter_choice],"choice");chosen++;for(unsigned i=0;i<starter_choice;i++){hg_field_app_input(arrival,0x80);for(unsigned hold=0;hold<30;hold++){hg_field_app_step(arrival);hg_field_app_input(arrival,0x80);}hg_field_app_input(arrival,0);}hg_field_app_interact(arrival);}
        else if(hg_field_app_menu_pending(arrival)){acceptance_frame(arrival,expected[starter_choice],"menu");menu++;hg_field_app_interact(arrival);}
        else if(hg_field_app_dialogue(arrival)[0])hg_field_app_interact(arrival);
        hg_field_app_step(arrival);
    }
    CHECK(starter_ticks<4096&&chosen==1&&menu==1);
    CHECK(hg_story_revision(arrival_story)==choice_before+1);
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_STARTER,&arrival_flag)&&arrival_flag);
    CHECK(hg_story_flag_get(arrival_story,FLAG_ELMS_LAB_PREVENT_PLAYER_ESCAPE,&arrival_flag)&&!arrival_flag);
    unsigned species;int fx,fz;
    CHECK(hg_field_app_follower(arrival,&species,&fx,&fz)&&species==expected[starter_choice]);
    FieldSystem saved_field={0};hg_story_bind_field(arrival_story,&saved_field);
    Party *saved_party=hg_story_save_party(saved_field.saveData);CHECK(Party_GetCount(saved_party)==1);
    CHECK(GetMonData(Party_GetMonByIndex(saved_party,0),MON_DATA_SPECIES,NULL)==species);
    CHECK(Pokedex_CheckMonCaughtFlag(hg_story_save_dex(saved_field.saveData),species));
    acceptance_frame(arrival,species,"lab");
    hg_test_walk_to(arrival,4,14);
    hg_field_app_input(arrival,0x80);for(unsigned hold=0;hold<30;hold++){hg_field_app_step(arrival);hg_field_app_input(arrival,0x80);}hg_field_app_input(arrival,0);hg_field_app_clear_input(arrival);
    CHECK(hg_field_app_map(arrival)==60&&hg_field_app_story(arrival)==arrival_story);
    CHECK(hg_field_app_follower(arrival,&species,&fx,&fz)&&species==expected[starter_choice]);
    CHECK(hg_field_app_player(arrival,&px,&pz)&&fx==px&&fz==pz);
    CHECK(hg_test_settle(arrival)>0); /* Original friend/Marill frame event. */
    uint16_t town_scene;CHECK(hg_story_var_get(arrival_story,VAR_SCENE_NEW_BARK_TOWN_OW,&town_scene)&&town_scene==2);
    for(unsigned step=0;step<2;step++) {
        hg_field_app_input(arrival,0x80);for(unsigned hold=0;hold<30;hold++){hg_field_app_step(arrival);hg_field_app_input(arrival,0x80);}hg_field_app_input(arrival,0);hg_field_app_clear_input(arrival);
        for(unsigned tick=0;tick<16;tick++)hg_field_app_step(arrival);
    }
    CHECK(hg_field_app_player(arrival,&px,&pz)&&hg_field_app_follower(arrival,&species,&fx,&fz));
    if(fx!=px||fz!=pz-1)fprintf(stderr,"Following outside: player %d,%d follower %d,%d\n",px,pz,fx,fz);
    CHECK(fx==px&&fz==pz-1);
    uint16_t lab_scene;CHECK(hg_story_var_get(arrival_story,VAR_SCENE_ELMS_LAB,&lab_scene)&&lab_scene==2);
    CHECK(Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_POTION,HEAP_ID_FIELD1)==5);
    acceptance_frame(arrival,species,"outside");
    int outside_z=pz;hg_field_app_input(arrival,0x40);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    for(unsigned tick=0;tick<16;tick++)hg_field_app_step(arrival);
    CHECK(hg_field_app_player(arrival,&px,&pz)&&pz==outside_z-1);
    hg_test_walk_to(arrival,684,394);hg_field_app_input(arrival,0x40);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    hg_test_travel(arrival,61,32);
    CHECK(hg_field_app_map(arrival)==61&&hg_field_app_follower(arrival,&species,&fx,&fz)&&species==expected[starter_choice]);
    CHECK(Party_GetCount(saved_party)==1&&Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_POTION,HEAP_ID_FIELD1)==5);
    hg_test_portal(arrival,60);hg_test_settle(arrival);
    hg_test_portal(arrival,63);hg_test_settle(arrival);hg_test_approach(arrival,2);
    hg_field_app_interact(arrival);hg_field_app_step(arrival);mom_pages=0;
    CHECK(hg_test_settle_observe(arrival,starter_choice%2,mom_gift_page,&mom_pages)>0);CHECK(mom_pages&16);
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_POKEGEAR,&arrival_flag)&&arrival_flag);
    CHECK(Party_GetCount(saved_party)==1&&Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_POTION,HEAP_ID_FIELD1)==5);
    hg_test_portal(arrival,60);hg_test_settle(arrival);CHECK(hg_field_app_story(arrival)==arrival_story);
    /* Original mandatory farewell: actual prop frames, Elm walks out, gives
     * his number, returns through the door, and clears the original gate. */
    CHECK(hg_story_phone_registered(arrival_story,PHONE_CONTACT_MOTHER));
    CHECK(!hg_story_phone_registered(arrival_story,PHONE_CONTACT_PROF__ELM));
    hg_test_walk_to(arrival,677,398);
    hg_field_app_input(arrival,0x20);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    for(unsigned tick=0;tick<16;tick++)hg_field_app_step(arrival);
    CHECK(hg_test_settle(arrival)>0);
    CHECK(!hg_field_app_error(arrival)[0]);
    CHECK(hg_story_phone_registered(arrival_story,PHONE_CONTACT_PROF__ELM));
    uint16_t farewell;CHECK(hg_story_var_get(arrival_story,VAR_SCENE_NEW_BARK_WEST_EXIT,&farewell)&&farewell==1);
    CHECK(Party_GetCount(saved_party)==1&&Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_POTION,HEAP_ID_FIELD1)==5);
    acceptance_frame(arrival,species,"elm-farewell");
    hg_test_walk_to(arrival,672,398);unsigned border_steps=hg_field_app_steps(arrival);
    unsigned long long border_revision=hg_story_revision(arrival_story);
    hg_field_app_input(arrival,0x20);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    for(unsigned tick=0;tick<16;tick++)hg_field_app_step(arrival);
    CHECK(hg_field_app_map(arrival)==33&&hg_field_app_player(arrival,&px,&pz)&&px==671&&pz==398);
    CHECK(hg_field_app_steps(arrival)==border_steps+1&&hg_story_revision(arrival_story)>border_revision);
    CHECK(hg_field_app_story(arrival)==arrival_story&&hg_story_phone_registered(arrival_story,PHONE_CONTACT_PROF__ELM));
    CHECK(Party_GetCount(saved_party)==1&&Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_POTION,HEAP_ID_FIELD1)==5);
    acceptance_frame(arrival,species,"route-29");checkpoint(arrival,species,1);
    hg_test_approach(arrival,std_apricorn_tree);
    hg_field_app_interact(arrival);hg_field_app_step(arrival);CHECK(hg_test_settle(arrival)>0);
    CHECK(!hg_field_app_error(arrival)[0]);
    acceptance_frame(arrival,species,"apricorn-tree");
    hg_test_portal(arrival,67);hg_test_settle(arrival);
    hg_test_walk_to(arrival,565,398);hg_test_settle(arrival);
    uint16_t guide_scene;CHECK(hg_story_var_get(arrival_story,VAR_SCENE_CHERRYGROVE_CITY_OW,&guide_scene)&&guide_scene==1);
    CHECK(hg_field_app_running_shoes(arrival));
    CHECK(hg_story_flag_get(arrival_story,FLAG_HIDE_CHERRYGROVE_GUIDE_GENT,&arrival_flag)&&arrival_flag);
    acceptance_frame(arrival,species,"cherrygrove");
    hg_test_portal(arrival,34);hg_test_settle(arrival);
    CHECK(hg_story_var_get(arrival_story,VAR_SCENE_CHERRYGROVE_CITY_OW,&guide_scene)&&guide_scene==2);
    CHECK(Pokegear_GetRegisteredCardsArray(hg_story_save_pokegear(saved_field.saveData))&GEARCARD_MAP);
    CHECK(hg_field_app_running_shoes(arrival));
    acceptance_frame(arrival,species,"route-30");
    Pokemon *resting=Party_GetMonByIndex(saved_party,0);u32 damaged=1,condition=8;u8 empty_pp=0;
    hg_test_portal(arrival,143);
    /* Damage the healing fixture indoors, after real grass encounters on the trip. */
    SetMonData(resting,MON_DATA_HP,&damaged);SetMonData(resting,MON_DATA_STATUS,&condition);SetMonData(resting,MON_DATA_MOVE1_PP,&empty_pp);
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_APRICORN_BOX,&arrival_flag)&&arrival_flag);
    CHECK(Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_APRICORN_BOX,HEAP_ID_FIELD1)==1);
    unsigned rest_ticks=0;int fade_out=0,black=0,fade_in=0;
    for(;rest_ticks<8192;rest_ticks++) {
        hg_field_app_step(arrival);int brightness=hg_field_app_brightness(arrival);
        if(brightness<0&&brightness> -16&&!fade_out){acceptance_frame(arrival,species,"mr-pokemon-fade-out");fade_out=1;}
        if(brightness== -16&&!black){acceptance_frame(arrival,species,"mr-pokemon-black");black=1;}
        if(black&&brightness<0&&brightness> -16&&!fade_in){acceptance_frame(arrival,species,"mr-pokemon-fade-in");fade_in=1;}
        if(strstr(hg_field_app_dialogue(arrival),"isn’t available yet"))fprintf(stderr,"House: %s\n",hg_field_app_error(arrival));
        CHECK(!strstr(hg_field_app_dialogue(arrival),"isn’t available yet"));
        if(hg_field_app_dialogue(arrival)[0])hg_field_app_interact(arrival);
        if(!hg_field_app_interaction_busy(arrival)&&!hg_field_app_dialogue(arrival)[0]&&rest_ticks>1)break;
    }
    CHECK(rest_ticks<8192&&fade_out&&black&&fade_in&&!hg_field_app_brightness(arrival));
    CHECK(GetMonData(resting,MON_DATA_HP,NULL)==GetMonData(resting,MON_DATA_MAX_HP,NULL));
    CHECK(!GetMonData(resting,MON_DATA_STATUS,NULL)&&GetMonData(resting,MON_DATA_MOVE1_PP,NULL)>0);
    CHECK(!hg_field_app_error(arrival)[0]);
    CHECK(Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_MYSTERY_EGG,HEAP_ID_FIELD1)==1);
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_POKEDEX,&arrival_flag)&&arrival_flag);
    CHECK(Pokedex_IsEnabled(hg_story_save_dex(saved_field.saveData)));
    CHECK(hg_story_phone_registered(arrival_story,PHONE_CONTACT_PROF__OAK));
    CHECK(Save_SysInfo_GetMysteryGiftActive(hg_story_save_sys_info(saved_field.saveData)));
    CHECK(hg_story_blackout_spawn(saved_field.saveData)==2);
    acceptance_frame(arrival,species,"mr-pokemon");
    hg_test_portal(arrival,34);
    unsigned phone_ticks=0,phone_messages=0;int phone_seen=0;int phone_x,phone_z;CHECK(hg_field_app_player(arrival,&phone_x,&phone_z));
    for(;phone_ticks<4096;phone_ticks++) {
        hg_field_app_step(arrival);
        CHECK(!strstr(hg_field_app_dialogue(arrival),"isn’t available yet"));
        if(hg_field_app_phone_active(arrival)) {
            if(hg_field_app_dialogue(arrival)[0]&&!phone_seen){CHECK(strstr(hg_field_app_dialogue(arrival),"Ethan")&&strstr(hg_field_app_dialogue(arrival),"disaster"));acceptance_frame(arrival,species,"elm-phone");phone_seen=1;}
        }
        if(hg_field_app_dialogue(arrival)[0]){phone_messages++;hg_field_app_interact(arrival);}
        if(phone_seen&&!hg_field_app_interaction_busy(arrival)&&!hg_field_app_dialogue(arrival)[0])break;
    }
    CHECK(phone_ticks<4096&&phone_seen&&phone_messages&&!hg_field_app_phone_active(arrival));
    CHECK(!hg_field_app_error(arrival)[0]&&!hg_field_app_brightness(arrival));
    CHECK(hg_story_flag_get(arrival_story,FLAG_GOT_ELMS_PANIC_CALL,&arrival_flag)&&arrival_flag);
    CHECK(Bag_GetQuantity(hg_story_save_bag(saved_field.saveData),ITEM_MYSTERY_EGG,HEAP_ID_FIELD1)==1);
    int returned_x,returned_z;CHECK(hg_field_app_player(arrival,&returned_x,&returned_z)&&returned_x==phone_x&&returned_z==phone_z);
    acceptance_frame(arrival,species,"elm-phone-return");
    CHECK(hg_field_app_running_shoes(arrival));
    CHECK(hg_field_app_story(arrival)==arrival_story&&hg_story_phone_registered(arrival_story,PHONE_CONTACT_PROF__ELM));

    hg_test_portal(arrival,67);hg_test_settle(arrival);
    hg_test_walk_to(arrival,574,398);hg_test_settle(arrival);checkpoint(arrival,species,2);unsigned long long rival_revision=hg_story_revision(arrival_story);
    hg_field_app_input(arrival,0x10);hg_field_app_step(arrival);hg_field_app_clear_input(arrival);
    unsigned before_rival_exp=GetMonData(Party_GetMonByIndex(saved_party,0),MON_DATA_EXPERIENCE,NULL),before_rival_count=GameStats_GetCapped(hg_story_save_stats(saved_field.saveData),GAME_STAT_TRAINER_BATTLES);
    unsigned rival_seen=0,rival_done=0;for(unsigned tick=0;tick<16000;tick++){
        hg_field_app_step(arrival);
        if(hg_field_app_error(arrival)[0])fprintf(stderr,"Rival: %s\n",hg_field_app_error(arrival));CHECK(!hg_field_app_error(arrival)[0]);
        if(hg_field_app_battle_active(arrival)){
            CHECK(hg_field_app_battle_trainer(arrival));CHECK(hg_story_revision(arrival_story)==rival_revision);CHECK(GetMonData(Party_GetMonByIndex(saved_party,0),MON_DATA_EXPERIENCE,NULL)==before_rival_exp);CHECK(GameStats_GetCapped(hg_story_save_stats(saved_field.saveData),GAME_STAT_TRAINER_BATTLES)==before_rival_count);if(!rival_seen){char blocked[256];CHECK(!hg_field_app_checkpoint_write(arrival,"build/checkpoints/busy.hgcheckpoint",2,blocked,sizeof(blocked)));acceptance_frame(arrival,species,"rival-battle");rival_seen=1;}
            hg_test_battle_input(arrival);
        }else if(hg_field_app_dialogue(arrival)[0])hg_field_app_interact(arrival);
        if(rival_seen&&!hg_field_app_interaction_busy(arrival)&&!hg_field_app_dialogue(arrival)[0]){rival_done=1;break;}
    }
    CHECK(rival_seen&&rival_done&&hg_field_app_story(arrival)==arrival_story);CHECK(GameStats_GetCapped(hg_story_save_stats(saved_field.saveData),GAME_STAT_TRAINER_BATTLES)==before_rival_count+1);
    CHECK(hg_story_flag_get(arrival_story,FLAG_HIDE_CHERRYGROVE_RIVAL,&arrival_flag)&&arrival_flag);
    uint16_t rival_outcome;CHECK(hg_story_var_get(arrival_story,VAR_BATTLE_RESULT,&rival_outcome)&&(rival_outcome==1||rival_outcome==2));
    CHECK(hg_story_revision(arrival_story)>rival_revision);CHECK(hg_field_app_follower(arrival,NULL,NULL,NULL));
    acceptance_frame(arrival,species,"rival-return");checkpoint(arrival,species,3);
    printf("Starter %u: visible rival outcome %u, original cutscene return and checkpoint round trips passed\n",species,rival_outcome);
    hg_field_app_free(arrival);
    }
    /* Full original introduction from its real coordinate-trigger row. */
    HGFieldPlayer *intro=hg_field_player_new_map(61);CHECK(intro);
    for(unsigned step=0;step<4;step++) {
        CHECK(hg_field_player_move(intro,0x40));for(unsigned tick=0;tick<8;tick++)CHECK(hg_field_player_tick(intro));
    }
    CHECK(hg_field_player_position(intro,&px,&pz,NULL)&&px==4&&pz==10);
    if(!hg_field_player_start_script(intro,12,-1))fprintf(stderr,"Intro: %s\n",hg_field_player_interaction_error(intro));
    CHECK(hg_field_player_interaction_busy(intro));
    char intro_message[HG_SCRIPT_TEXT_CAPACITY];unsigned intro_ticks=0,intro_messages=0;int intro_emote=0;
    while(hg_field_player_interaction_busy(intro)&&intro_ticks++<2048) {
        int status=hg_field_player_interaction_tick(intro,intro_message,sizeof(intro_message));
        CHECK(status!=HG_SCRIPT_ERROR);
        for(unsigned actor=0;actor<hg_field_player_actor_counts(intro).active;actor++) {
            HGFieldActor info;CHECK(hg_field_player_actor(intro,actor,&info));intro_emote|=info.emote;
        }
        if(status==HG_SCRIPT_MESSAGE){intro_messages++;CHECK(hg_field_player_interaction_ack(intro));}
    }
    CHECK(intro_ticks<2048&&intro_emote&&intro_messages==4);
    int intro_flag;CHECK(hg_story_flag_get(hg_field_player_story(intro),FLAG_ELMS_LAB_PREVENT_PLAYER_ESCAPE,&intro_flag)&&intro_flag);
    CHECK(hg_field_player_position(intro,&px,&pz,NULL)&&px==4&&pz==7);hg_field_player_free(intro);
    /* An unsupported later Elm branch explains the limitation visibly and
     * leaves the original story candidate unpublished. */
    app=hg_field_app_new(61,error,sizeof(error));CHECK(app);hg_test_approach(app,1);story=hg_field_app_story(app);
    CHECK(hg_story_flag_set(story,FLAG_SYS_HATCHED_TOGEPI_EGG,1));revision=hg_story_revision(story);
    hg_field_app_interact(app);hg_field_app_step(app);CHECK(strstr(hg_field_app_dialogue(app),"isn’t available yet"));
    close_conversation(app);CHECK(hg_story_revision(story)==revision);hg_field_app_free(app);
    printf("Original events: %u validated / %u unresolved; facing selection and original sign bytecode/message dispatch passed\n",valid,missing);return 0;
}
