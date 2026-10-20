#include "compat/motion.h"
#include "compat/battle.h"
#include "field_app.h"
#include "battle_runtime.h"
#include "field_player.h"
#include "field_data.h"
#include "metatile_behavior.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
int main(void){
    char error[256]={0};HGFieldApp *app=hg_field_app_new(33,error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);
    HGStoryState *story=hg_field_app_story(app);FieldSystem field={0};hg_story_bind_field(story,&field);Party *party=hg_story_save_party(field.saveData);
    Pokemon mon;PlayerProfile *profile=hg_story_save_profile(field.saveData);CreateMon(&mon,155,5,31,FALSE,0,OT_ID_PRESET,profile->id);SetMonData(&mon,MON_DATA_OT_NAME,profile->name);SetMonData(&mon,MON_DATA_OT_GENDER,&profile->gender);CHECK(Party_AddMon(party,&mon));
    CHECK(Bag_AddItem(hg_story_save_bag(field.saveData),ITEM_POTION,5,HEAP_ID_FIELD1));
    CHECK(hg_field_app_inspect(app,1));char menu[4096];hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"empty"));
    int before_x,before_z,after_x,after_z;CHECK(hg_field_app_player(app,&before_x,&before_z));
    hg_field_app_input(app,0x10);for(unsigned hold=0;hold<120;hold++){hg_field_app_step(app);hg_field_app_input(app,0x10);}hg_field_app_clear_input(app);
    hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"Medicine")&&strstr(menu,"Potion x5"));
    for(unsigned i=0;i<20;i++)hg_field_app_step(app);CHECK(hg_field_app_player(app,&after_x,&after_z)&&before_x==after_x&&before_z==after_z);
    CHECK(hg_field_app_inspect(app,2));hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"CYNDAQUIL")&&strstr(menu,"Tackle")&&strstr(menu,"HP "));
    CHECK(Party_AddMon(party,&mon));
    hg_field_app_input(app,0x80);for(unsigned hold=0;hold<121;hold++){hg_field_app_step(app);hg_field_app_input(app,0x80);}
    hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"2/2"));
    hg_field_app_input(app,0);hg_field_app_input(app,0x80);hg_field_app_step(app);
    hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"1/2"));
    hg_field_app_clear_input(app);Party_InitWithMaxSize(party,6);CHECK(Party_AddMon(party,&mon));
    CHECK(hg_field_app_inspect(app,0));CHECK(!hg_field_app_inspect_kind(app));
    unsigned exp=GetMonData(Party_GetMonByIndex(party,0),MON_DATA_EXPERIENCE,NULL);int sx,sz;CHECK(hg_field_app_player(app,&sx,&sz));printf("Route spawn %d,%d\n",sx,sz);
    int gx=-1,gz=-1;
    for(int r=0;r<48&&gx<0;r++)for(int z=sz-r;z<=sz+r&&gx<0;z++)for(int x=sx-r;x<=sx+r;x++){
        if(abs(x-sx)+abs(z-sz)!=r)continue;uint16_t a,b;
        if(hg_field_attribute(33,x,z,&a)&&hg_field_attribute(33,x+1,z,&b)&&MetatileBehavior_IsTallGrass(a&255)&&MetatileBehavior_IsTallGrass(b&255)){gx=x;gz=z;break;}
    }
    CHECK(gx>=0);printf("Grass pair %d,%d\n",gx,gz);hg_test_walk_to(app,gx,gz);hg_test_settle(app);
    uint8_t *pixels=malloc(1024*384*4);CHECK(pixels);DSCamera camera;hg_field_app_camera(app,&camera,1024,384);unsigned battles=0,frames=0;
    for(unsigned steps=0;steps<300&&battles<1;steps++){
        hg_field_app_input(app,steps&1?0x20:0x10);hg_field_app_step(app);hg_field_app_clear_input(app);
        int seen=0;
        for(unsigned tick=0;tick<3000;tick++){
            hg_field_app_step(app);
            if(hg_field_app_battle_active(app)){
                seen=1;int state=hg_field_app_battle_state(app);
                if(state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES||state==HG_BATTLE_MESSAGE){CHECK(hg_field_app_render(app,&camera,pixels,1024,384));frames++;
                    if(state==HG_BATTLE_MOVES&&frames<8){FILE *file=fopen("build/evidence/wild-battle.ppm","wb");if(file){fprintf(file,"P6\n1024 384\n255\n");for(unsigned i=0;i<1024*384;i++)fwrite(pixels+4*i,1,3,file);fclose(file);}}
                    hg_field_app_interact(app);}
            }else if(tick>16){if(seen)battles++;break;}
            if(hg_field_app_error(app)[0])fprintf(stderr,"Field battle: %s\n",hg_field_app_error(app));CHECK(!hg_field_app_error(app)[0]);
        }
    }
    CHECK(battles&&frames);CHECK(hg_field_app_story(app)==story&&hg_field_app_map(app)==33);
    CHECK(GetMonData(Party_GetMonByIndex(party,0),MON_DATA_EXPERIENCE,NULL)>exp);
    CHECK(hg_field_app_inspect(app,2));hg_field_app_inspect_text(app,menu,sizeof(menu));
    char exp_text[64];snprintf(exp_text,sizeof(exp_text),"EXP %u",GetMonData(Party_GetMonByIndex(party,0),MON_DATA_EXPERIENCE,NULL));CHECK(strstr(menu,exp_text));
    CHECK(hg_field_app_render(app,&camera,pixels,1024,384));
    FILE *panel=fopen("build/evidence/party-panel.ppm","wb");if(panel){fprintf(panel,"P6\n1024 384\n255\n");for(unsigned i=0;i<1024*384;i++)fwrite(pixels+4*i,1,3,panel);fclose(panel);}
    CHECK(hg_field_app_inspect(app,1));hg_field_app_input(app,0x10);hg_field_app_step(app);hg_field_app_clear_input(app);hg_field_app_inspect_text(app,menu,sizeof(menu));CHECK(strstr(menu,"Potion x5"));
    CHECK(hg_field_app_inspect(app,0));
    CHECK(hg_field_app_render(app,&camera,pixels,1024,384));free(pixels);hg_field_app_free(app);
    printf("Grass walk -> visible battle (%u input frames) -> original EXP -> retained field session passed\n",frames);return 0;
}
