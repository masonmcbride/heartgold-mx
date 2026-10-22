#include "compat/save_c_records.h"
#include "compat/save_manager.h"
#include "battle_runtime.h"
#include "battle_view.h"
#include "bag.h"
#include "ds_save_flash.h"
#include "game_name.h"
#include <unistd.h>
static char error[256];
#define CHECK(x) do {if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
static int publish(HGBattle *battle,SaveData *save){return hg_battle_commit_storage(battle,hg_story_save_party(save),hg_story_save_profile(save),hg_story_save_bag(save),hg_story_save_dex(save),hg_story_save_stats(save),SaveArray_Get(save,SAVE_PCSTORAGE));}
static void failed_capture(void){
    SaveData *save=malloc(sizeof(*save));CHECK(save);hg_save_manager_init_ram(save);CHECK(hg_name_encode("Ethan",hg_story_save_profile(save)->name,7));
    Party *party=hg_story_save_party(save),opponent;Party_InitWithMaxSize(&opponent,6);Pokemon mon;CreateMon(&mon,152,50,31,TRUE,123,OT_ID_PRESET,42);CHECK(Party_AddMon(party,&mon));
    CreateMon(&mon,249,3,31,TRUE,124,OT_ID_PRESET,42);
    /* Keep a low catch rate without Whirlwind ending the test battle. */
    for(unsigned i=0;i<4;i++){u32 move=i?0:150,pp=i?0:40;SetMonData(&mon,MON_DATA_MOVE1+i,&move);SetMonData(&mon,MON_DATA_MOVE1_PP+i,&pp);}
    CHECK(Party_AddMon(&opponent,&mon));Bag *bag=hg_story_save_bag(save);CHECK(Bag_AddItem(bag,ITEM_POKE_BALL,5,HEAP_ID_FIELD2));
    HGBattleSetup setup={.storage=SaveArray_Get(save,SAVE_PCSTORAGE),.party=party,.opponent=&opponent,.profile=hg_story_save_profile(save),.bag=bag,.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.seed=1};
    HGBattle *battle=hg_battle_new_setup(&setup,error,sizeof(error));HGBattleView *view=hg_battle_view_new();CHECK(battle&&view);u8 *before=malloc(512*192*4),*after=malloc(512*192*4);CHECK(before&&after);
    unsigned throws=0,restored=0,saw_ball=0,tick=0;int state=HG_BATTLE_RUNNING;
    for(;tick<20000&&state!=HG_BATTLE_FINISHED;tick++){
        state=hg_battle_tick(battle);CHECK(state!=HG_BATTLE_FAILED);
        if(hg_battle_capture_phase(battle)>0&&hg_battle_capture_phase(battle)<100)saw_ball=1;
        if(state==HG_BATTLE_MESSAGE)CHECK(hg_battle_input(battle,1));
        else if(state==HG_BATTLE_COMMAND){
            CHECK(!hg_battle_capture_phase(battle));CHECK(hg_battle_view_render(view,battle,state,0,throws?after:before,512,192));
            if(throws){CHECK(saw_ball);for(unsigned y=20;y<96;y++)CHECK(!memcmp(before+4*(y*512+160),after+4*(y*512+160),80*4));restored++;}
            if(throws<2){CHECK(hg_battle_input(battle,2));throws++;saw_ball=0;}else CHECK(hg_battle_input(battle,4));
        }
    }
    if(!(tick<20000&&restored>=2&&throws==2))fprintf(stderr,"Breakout: tick=%u state=%d throws=%u restored=%u phase=%u text=%s\n",tick,state,throws,restored,hg_battle_capture_phase(battle),hg_battle_message(battle));
    CHECK(tick<20000&&restored>=2&&throws==2);CHECK(publish(battle,save));CHECK(Party_GetCount(party)==1&&Bag_GetQuantity(bag,ITEM_POKE_BALL,HEAP_ID_FIELD2)==3);
    free(before);free(after);hg_battle_view_free(view);hg_battle_free(battle);hg_save_manager_bind(NULL);free(save);
}
int main(void){
    failed_capture();
    for(unsigned scenario=0;scenario<3;scenario++){
        SaveData *save=malloc(sizeof(*save)),*before=malloc(sizeof(*before)),*loaded=malloc(sizeof(*loaded));CHECK(save&&before&&loaded);
        hg_save_manager_init_ram(save);CHECK(hg_name_encode("Ethan",hg_story_save_profile(save)->name,7));
        Party *party=hg_story_save_party(save);PCStorage *storage=SaveArray_Get(save,SAVE_PCSTORAGE);
        Pokemon mon;CreateMon(&mon,152,15,31,TRUE,123,OT_ID_PRESET,42);
        unsigned party_count=scenario?6:1;
        for(unsigned i=0;i<party_count;i++)CHECK(Party_AddMon(party,&mon));
        if(scenario==2)for(unsigned i=0;i<30;i++)CHECK(PCStorage_PlaceMonInBoxFirstEmptySlot(storage,0,Mon_GetBoxMon(&mon)));
        Bag *bag=hg_story_save_bag(save);CHECK(Bag_AddItem(bag,ITEM_POKE_BALL,20,HEAP_ID_FIELD2));
        Party opponent;Party_InitWithMaxSize(&opponent,6);CreateMon(&mon,10,3,31,TRUE,124,OT_ID_PRESET,42);
        u32 hp=1,status=3;SetMonData(&mon,MON_DATA_HP,&hp);SetMonData(&mon,MON_DATA_STATUS,&status);CHECK(Party_AddMon(&opponent,&mon));
        if(scenario==1)Pokedex_SetMonCaughtFlag(hg_story_save_dex(save),&mon);
        *before=*save;
        HGBattleSetup setup={.storage=storage,.party=party,.opponent=&opponent,.profile=hg_story_save_profile(save),.bag=bag,.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.seed=1};
        HGBattle *battle=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(battle&&hg_battle_can_throw(battle));
        unsigned ticks=0,balls=0,naming=0;int state=HG_BATTLE_RUNNING;
        while(state!=HG_BATTLE_FINISHED&&ticks++<20000){
            state=hg_battle_tick(battle);if(state==HG_BATTLE_FAILED)snprintf(error,sizeof(error),"%s",hg_battle_error(battle));CHECK(state!=HG_BATTLE_FAILED);
            if(state==HG_BATTLE_MESSAGE)CHECK(hg_battle_input(battle,1));
            if(state==HG_BATTLE_COMMAND){CHECK(hg_battle_input(battle,2));balls++;}
            if(state==HG_BATTLE_NICKNAME){CHECK(!hg_battle_input(battle,1));CHECK(hg_battle_input(battle,255));naming++;}
            CHECK(!memcmp(save,before,sizeof(*save)));
        }
        CHECK(ticks<20000&&balls&&naming==1);
        storage->curBox=2;CHECK(!publish(battle,save));CHECK(Party_GetCount(party)==party_count&&Bag_GetQuantity(bag,ITEM_POKE_BALL,HEAP_ID_FIELD2)==20);
        storage->curBox=0;CHECK(!hg_battle_commit(battle,party,hg_story_save_profile(save),bag,hg_story_save_dex(save),hg_story_save_stats(save)));CHECK(publish(battle,save));CHECK(!publish(battle,save));
        CHECK(Bag_GetQuantity(bag,ITEM_POKE_BALL,HEAP_ID_FIELD2)==20-balls);
        CHECK(Party_GetCount(party)==(scenario?6:2));
        if(scenario){unsigned box=scenario==2?1:0;CHECK(GetBoxMonData(&storage->boxes[box].mons[0],MON_DATA_SPECIES,NULL)==10);CHECK(storage->curBox==(int)box);CHECK(storage->boxModifiedFlag&(1u<<box));}
        else CHECK(GetMonData(Party_GetMonByIndex(party,1),MON_DATA_SPECIES,NULL)==10);
        char path[]="/tmp/hg-capture-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));
        CHECK(ds_save_flash_open(path));CHECK(hg_save_manager_write(save,error,sizeof(error)));ds_save_flash_close();
        CHECK(ds_save_flash_open(path));CHECK(hg_save_manager_load(loaded,error,sizeof(error)));ds_save_flash_close();
        CHECK(!memcmp(SaveArray_Get(loaded,SAVE_PCSTORAGE),storage,sizeof(*storage)));CHECK(!memcmp(hg_story_save_party(loaded),party,sizeof(*party)));CHECK(!memcmp(hg_story_save_bag(loaded),bag,sizeof(*bag)));
        CHECK(!unlink(path));hg_battle_free(battle);hg_save_manager_bind(NULL);free(save);free(before);free(loaded);
    }
    puts("Original ordinary capture: party/PC/next-box insertion, explicit original name, atomic/stale publication, ball use and durable reopen pass");
}
