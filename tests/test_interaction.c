#include "compat/motion.h"
#include "field_events.h"
#include "field_app.h"
#include "motion_guest.h"
#include "ds_resources.h"
#include "compat/resources.h"
#include "compat/story.h"
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
static void close_conversation(HGFieldApp *app) {
    unsigned pages=0,ticks=0;
    while(hg_field_app_interaction_busy(app)&&ticks++<2048) {
        if(hg_field_app_dialogue(app)[0]){hg_field_app_interact(app);pages++;}hg_field_app_step(app);
    }
    CHECK(pages&&pages<64&&ticks<2048&&!hg_field_app_dialogue(app)[0]);
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
    CHECK(hg_field_app_map(app)==63&&hg_field_app_story(app)==story);hg_test_approach(app,2);
    unsigned long long revision=hg_story_revision(story);unsigned steps=hg_field_app_steps(app);
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(!strcmp(hg_field_app_dialogue(app),"Professor Elm has been waiting for\nyou, dear. Please, go and help him."));
    CHECK(hg_story_revision(story)==revision);close_conversation(app);CHECK(hg_story_revision(story)==revision+1&&hg_field_app_steps(app)==steps);
    CHECK(hg_story_var_set(story,VAR_SCENE_NEW_BARK_TOWN_OW,1));
    hg_test_walk_to(app,3,10);hg_field_app_input(app,0x80);hg_field_app_step(app);hg_field_app_clear_input(app);
    CHECK(hg_field_app_map(app)==60&&hg_field_app_story(app)==story);uint16_t scene;CHECK(hg_story_var_get(story,VAR_SCENE_NEW_BARK_TOWN_OW,&scene)&&scene==1);
    hg_test_approach(app,2);hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(strstr(hg_field_app_dialogue(app),"adorable"));close_conversation(app);hg_field_app_free(app);
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
    /* An unsupported later Elm branch explains the limitation visibly and
     * leaves the original story candidate unpublished. */
    app=hg_field_app_new(61,error,sizeof(error));CHECK(app);hg_test_approach(app,1);story=hg_field_app_story(app);
    CHECK(hg_story_flag_set(story,FLAG_SYS_HATCHED_TOGEPI_EGG,1));revision=hg_story_revision(story);
    hg_field_app_interact(app);hg_field_app_step(app);CHECK(strstr(hg_field_app_dialogue(app),"isn’t available yet"));
    close_conversation(app);CHECK(hg_story_revision(story)==revision);hg_field_app_free(app);
    printf("Original events: %u validated / %u unresolved; facing selection and original sign bytecode/message dispatch passed\n",valid,missing);return 0;
}
