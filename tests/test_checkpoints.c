#include "compat/battle.h"
#include "field_app.h"
#include "field_player.h"
#include "constants/vars.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
static void rival(HGFieldApp *app,unsigned expected){
    hg_field_app_input(app,0x10);hg_field_app_step(app);hg_field_app_clear_input(app);unsigned seen=0,done=0;
    for(unsigned tick=0;tick<16000;tick++){
        hg_field_app_step(app);if(hg_field_app_error(app)[0])fprintf(stderr,"Checkpoint rival: %s\n",hg_field_app_error(app));CHECK(!hg_field_app_error(app)[0]);
        if(hg_field_app_battle_active(app)){seen++;CHECK(hg_field_app_battle_trainer(app));hg_test_battle_input(app);}
        else if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);
        if(seen&&!hg_field_app_interaction_busy(app)&&!hg_field_app_dialogue(app)[0]){done=1;break;}
    }
    CHECK(seen&&done);uint16_t outcome;CHECK(hg_story_var_get(hg_field_app_story(app),VAR_BATTLE_RESULT,&outcome));if(outcome!=expected)fprintf(stderr,"Rival outcome %u, expected %u\n",outcome,expected);CHECK(outcome==expected);
    int hidden;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_HIDE_CHERRYGROVE_RIVAL,&hidden)&&hidden);CHECK(hg_field_app_follower(app,NULL,NULL,NULL));
}
int main(void){
    char path[256],error[256];const unsigned starters[]={152,155,158};
    for(unsigned i=0;i<3;i++){
        snprintf(path,sizeof(path),"build/checkpoints/%u-1.hgcheckpoint",starters[i]);HGFieldApp *route=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(route);hg_test_portal(route,67);hg_test_settle(route);hg_test_walk_to(route,565,398);hg_test_settle(route);CHECK(hg_field_app_running_shoes(route));uint16_t tour;CHECK(hg_story_var_get(hg_field_app_story(route),VAR_SCENE_CHERRYGROVE_CITY_OW,&tour)&&tour==1);hg_field_app_free(route);

        snprintf(path,sizeof(path),"build/checkpoints/%u-2.hgcheckpoint",starters[i]);HGFieldApp *app=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);rival(app,1);hg_field_app_free(app);
        snprintf(path,sizeof(path),"build/checkpoints/%u-3.hgcheckpoint",starters[i]);app=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(app);hg_test_walk_to(app,574,398);hg_test_settle(app);hg_field_app_input(app,0x10);for(unsigned tick=0;tick<60;tick++)hg_field_app_step(app);hg_field_app_clear_input(app);CHECK(!hg_field_app_battle_active(app)&&!hg_field_app_interaction_busy(app));hg_field_app_free(app);
    }
    HGFieldApp *loss=hg_field_app_checkpoint_load("build/checkpoints/152-2.hgcheckpoint",error,sizeof(error));CHECK(loss);FieldSystem field={0};hg_story_bind_field(hg_field_app_story(loss),&field);Party *party=hg_story_save_party(field.saveData);
    Pokemon *mon=Party_GetMonByIndex(party,0);u32 hp=1;SetMonData(mon,MON_DATA_HP,&hp);MonSetMoveInSlot(mon,MOVE_GROWL,0);rival(loss,2);CHECK(GetMonData(mon,MON_DATA_HP,NULL)==GetMonData(mon,MON_DATA_MAX_HP,NULL));CHECK(GetMonData(mon,MON_DATA_MOVE1_PP,NULL)>0);hg_field_app_free(loss);
    FILE *file=fopen("build/checkpoints/152-2.hgcheckpoint","rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long size=ftell(file);CHECK(size>40);rewind(file);uint8_t *bytes=malloc((size_t)size);CHECK(bytes&&fread(bytes,(size_t)size,1,file)==1);fclose(file);
    bytes[size-1]^=1;file=fopen("build/checkpoints/damaged.hgcheckpoint","wb");CHECK(file&&fwrite(bytes,(size_t)size,1,file)==1);fclose(file);free(bytes);
    CHECK(!hg_field_app_checkpoint_load("build/checkpoints/damaged.hgcheckpoint",error,sizeof(error)));CHECK(error[0]);
    puts("Loaded checkpoints: all starters battle rival, post-rival does not retrigger, loss heals/resumes, damaged state rejected");return 0;
}
