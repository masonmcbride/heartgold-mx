#include "compat/motion.h"
#include "compat/pokemon.h"
#include "compat/field_queries.h"
#include "compat/field_time.h"
#include "field_app.h"
#include "field_data.h"
#include <unistd.h>
static char error[256];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
#include "field_routes.h"
static void observe(HGFieldApp *app,void *unused){(void)unused;DSCamera camera;u8 pixels[256*192*4];hg_field_app_camera(app,&camera,256,192);CHECK(hg_field_app_render(app,&camera,pixels,256,192));}
static void scenario(unsigned species,unsigned variant){
    char checkpoint[128];snprintf(checkpoint,sizeof(checkpoint),"build/checkpoints/%u-3.hgcheckpoint",species);
    HGFieldApp *app=hg_field_app_checkpoint_load(checkpoint,error,sizeof(error));CHECK(app);
    hg_test_portal(app,69);hg_test_travel(app,69,80);CHECK(hg_field_app_map(app)==69);hg_test_settle(app);hg_test_approach(app,1);
    FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);CHECK(hg_field_trainer_stars(&field)==0);
    Party *party=hg_story_save_party(field.saveData);Pokemon *mon=Party_GetMonByIndex(party,0);
    /* Deliberate party fixtures exercise healing, refusal and follower changes;
     * the route, dialogue, healing and result publication are real gameplay. */
    if(variant==2){Pokemon second=*mon;CHECK(Party_AddMon(party,&second));}
    u32 hp=variant==2?0:1,status=8,pp=0;SetMonData(mon,MON_DATA_HP,&hp);SetMonData(mon,MON_DATA_STATUS,&status);SetMonData(mon,MON_DATA_MOVE1_PP,&pp);
    if(variant==3){u32 pokerus=0x11;SetMonData(mon,MON_DATA_POKERUS,&pokerus);}
    hg_story_touch(hg_field_app_story(app));Party before=*party;
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(hg_test_settle_observe(app,variant==1?1:0,observe,NULL)>0);
    CHECK(!hg_field_app_error(app)[0]&&!hg_field_app_interaction_busy(app));
    hg_story_bind_field(hg_field_app_story(app),&field);party=hg_story_save_party(field.saveData);mon=Party_GetMonByIndex(party,0);
    if(variant==1)CHECK(!memcmp(party,&before,sizeof(before)));
    else {
        CHECK(GetMonData(mon,MON_DATA_HP,NULL)==GetMonData(mon,MON_DATA_MAX_HP,NULL));
        CHECK(!GetMonData(mon,MON_DATA_STATUS,NULL));CHECK(GetMonData(mon,MON_DATA_MOVE1_PP,NULL)==GetMonData(mon,MON_DATA_MOVE1_MAX_PP,NULL));
        if(variant==3){int told=0;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_WAS_TOLD_ABOUT_POKERUS,&told)&&told);}
    }
    if(!variant){
        char path[]="/tmp/hg-nurse-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));
        CHECK(hg_field_app_save(app,path,error,sizeof(error)));HGFieldApp *continued=hg_field_app_continue(path,error,sizeof(error));CHECK(continued&&hg_field_app_map(continued)==69);
        FieldSystem restored={0};hg_story_bind_field(hg_field_app_story(continued),&restored);CHECK(!memcmp(party,hg_story_save_party(restored.saveData),sizeof(*party)));
        hg_test_approach(continued,1);hg_field_app_interact(continued);hg_field_app_step(continued);CHECK(hg_test_settle_choice(continued,0)>0);
        CHECK(!hg_field_app_error(continued)[0]);hg_field_app_free(continued);CHECK(!unlink(path));
    }
    hg_field_app_free(app);
}
static void queries(void){
    HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-3.hgcheckpoint",error,sizeof(error));CHECK(app);
    HGStoryState *fixture=hg_story_clone(hg_field_app_story(app));CHECK(fixture);FieldSystem field={0};hg_story_bind_field(fixture,&field);
    CHECK(hg_field_trainer_stars(&field)==0);
    FieldSystem_FlagSet(&field,FLAG_GAME_CLEAR);CHECK(hg_field_trainer_stars(&field)==1);
    FieldSystem_FlagSet(&field,0xf1);FieldSystem_FlagSet(&field,0x184);CHECK(hg_field_trainer_stars(&field)==3);
    u16 streak=100;memcpy(SaveArray_Get(field.saveData,SAVE_UNK_19),&streak,sizeof(streak));CHECK(hg_field_trainer_stars(&field)==4);
    Pokedex *dex=hg_story_save_dex(field.saveData);memset(dex->caughtSpecies,255,sizeof(dex->caughtSpecies));CHECK(hg_field_trainer_stars(&field)==4);memset(dex->seenSpecies,255,sizeof(dex->seenSpecies));CHECK(hg_field_trainer_stars(&field)==5);
    hg_story_free(fixture);hg_field_app_free(app);
}
int main(void){
    queries();
    for(unsigned i=0;i<3;i++)scenario((unsigned[]){152,155,158}[i],0);
    scenario(152,1);scenario(152,2);scenario(152,3);
    puts("Original Nurse Joy: all starters, decline, HP/status/PP, fainted lead restoration, Pokerus and durable Save/Continue pass");
}
