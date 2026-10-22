#include "compat/motion.h"
#include "compat/save_manager.h"
#include "compat/battle.h"
#include "field_app.h"
#include "field_player.h"
#include "field_data.h"
#include "metatile_behavior.h"
#include <unistd.h>
static char error[256];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
#include "field_routes.h"
int main(int argc,char **argv){
    unsigned stage=argc>1?(unsigned)strtoul(argv[1],NULL,10):1;unsigned species=argc>2?(unsigned)strtoul(argv[2],NULL,10):152;
    char checkpoint[128];snprintf(checkpoint,sizeof(checkpoint),"build/checkpoints/%u-%u.hgcheckpoint",species,stage);
    HGFieldApp *app=hg_field_app_checkpoint_load(checkpoint,error,sizeof(error));CHECK(app);
    hg_test_prepare_defeat(app);
    FieldSystem live={0};hg_story_bind_field(hg_field_app_story(app),&live);
    Party *party=hg_story_save_party(live.saveData);Pokemon *lead=Party_GetMonByIndex(party,0);
    unsigned money=hg_story_save_profile(live.saveData)->money;
    unsigned target=hg_story_blackout_spawn(live.saveData)==1?63:69;
    unsigned battles=0,ticks=0;u8 *pixels=malloc(512*384*4);CHECK(pixels);DSCamera camera;hg_field_app_camera(app,&camera,512,384);
    for(unsigned step=0;step<300&&!battles;step++){
        hg_field_app_input(app,step&1?0x20:0x10);hg_field_app_step(app);hg_field_app_clear_input(app);
        for(unsigned frame=0;frame<20000;frame++){
            hg_field_app_step(app);ticks++;
            int state=hg_field_app_battle_state(app);
            if(hg_field_app_battle_active(app)){
                battles=1;
                if(state==HG_BATTLE_COMMAND)CHECK(hg_field_app_battle_choose(app,0));
                else if(state==HG_BATTLE_MOVES)CHECK(hg_field_app_battle_choose(app,1));
                else if(state==HG_BATTLE_MESSAGE)hg_field_app_interact(app);
            }else if(battles){
                hg_field_app_camera(app,&camera,512,384);CHECK(hg_field_app_render(app,&camera,pixels,512,384));
                if(hg_field_app_interaction_busy(app))hg_field_app_interact(app);
                else if(hg_field_app_map(app)==target)break;
            }else if(frame>16)break;
            if(hg_field_app_error(app)[0])snprintf(error,sizeof(error),"%s",hg_field_app_error(app));CHECK(!hg_field_app_error(app)[0]);
        }
    }
    CHECK(battles&&ticks<20000&&hg_field_app_map(app)==target&&!hg_field_app_interaction_busy(app));
    hg_story_bind_field(hg_field_app_story(app),&live);party=hg_story_save_party(live.saveData);lead=Party_GetMonByIndex(party,0);
    CHECK(GetMonData(lead,MON_DATA_HP,NULL)==GetMonData(lead,MON_DATA_MAX_HP,NULL)&&!GetMonData(lead,MON_DATA_STATUS,NULL));
    CHECK(hg_story_save_profile(live.saveData)->money<money);
    if(target==69){hg_test_approach(app,1);hg_field_app_interact(app);hg_field_app_step(app);CHECK(hg_test_settle_choice(app,0)>0);hg_story_bind_field(hg_field_app_story(app),&live);party=hg_story_save_party(live.saveData);}
    char path[]="/tmp/hg-blackout-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));
    CHECK(hg_field_app_save(app,path,error,sizeof(error)));
    HGFieldApp *continued=hg_field_app_continue(path,error,sizeof(error));CHECK(continued&&hg_field_app_map(continued)==target);
    FieldSystem restored={0};hg_story_bind_field(hg_field_app_story(continued),&restored);
    CHECK(hg_story_save_profile(restored.saveData)->money==hg_story_save_profile(live.saveData)->money);
    CHECK(!memcmp(hg_story_save_party(restored.saveData),party,sizeof(*party)));
    hg_field_app_free(continued);CHECK(!unlink(path));
    CHECK(hg_field_app_render(app,&camera,pixels,512,384));free(pixels);hg_field_app_free(app);
    puts("Original wild defeat -> private blackout -> original destination/healing/recovery script -> playable field passes");
}
