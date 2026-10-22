#include "compat/save_c_records.h"
#include "compat/save_manager.h"
#include "battle_runtime.h"
#include "battle_view.h"
#include "bag.h"
#include "game_name.h"
#include "ds_save_flash.h"
#include <unistd.h>
static char error[256];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
static int next(HGBattle *b){for(unsigned i=0;i<3000;i++){int state=hg_battle_tick(b);if(state==HG_BATTLE_FAILED)snprintf(error,sizeof(error),"%s",hg_battle_error(b));CHECK(state!=HG_BATTLE_FAILED);if(state==HG_BATTLE_MESSAGE){CHECK(hg_battle_input(b,1));continue;}if(state!=HG_BATTLE_RUNNING)return state;}CHECK(0);return -1;}
static unsigned item_row(HGBattle *b,const char *name){for(unsigned i=0;i<hg_battle_menu_count(b,HG_BATTLE_BAG);i++){char label[128];hg_battle_menu_label(b,HG_BATTLE_BAG,i,label,sizeof(label));if(!strncmp(label,name,strlen(name)))return i;}CHECK(0);return 0;}
static void bag(HGBattle *b,const char *name){CHECK(hg_battle_input(b,5));CHECK(next(b)==HG_BATTLE_BAG);CHECK(hg_battle_input(b,item_row(b,name)+1));CHECK(next(b)==HG_BATTLE_ITEM_TARGET);}
static void render(HGBattle *b,int state){HGBattleView *view=hg_battle_view_new();u8 *pixels=malloc(512*192*4);CHECK(view&&pixels&&hg_battle_view_render(view,b,state,0,pixels,512,192));free(pixels);hg_battle_view_free(view);}
static void forced_replacement(void){
    SaveData *save=malloc(sizeof(*save)),*before=malloc(sizeof(*before));CHECK(save&&before);hg_save_manager_init_ram(save);Party *party=hg_story_save_party(save),opponent;Party_InitWithMaxSize(&opponent,6);Pokemon mon;
    const unsigned species[]={152,179,163,194,74,92};for(unsigned i=0;i<6;i++){CreateMon(&mon,species[i],11,31,TRUE,100+i,OT_ID_PRESET,42);CHECK(Party_AddMon(party,&mon));}
    u32 hp=1;SetMonData(Party_GetMonByIndex(party,0),MON_DATA_HP,&hp);CreateMon(&mon,74,30,31,TRUE,500,OT_ID_PRESET,42);
    for(unsigned i=0;i<4;i++){u32 move=i?0:33,pp=i?0:35;SetMonData(&mon,MON_DATA_MOVE1+i,&move);SetMonData(&mon,MON_DATA_MOVE1_PP+i,&pp);}CHECK(Party_AddMon(&opponent,&mon));*before=*save;
    HGBattleSetup setup={.storage=SaveArray_Get(save,SAVE_PCSTORAGE),.party=party,.opponent=&opponent,.profile=hg_story_save_profile(save),.bag=hg_story_save_bag(save),.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.seed=1};
    HGBattle *b=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(b&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_MOVES);CHECK(hg_battle_input(b,2));int state=next(b);CHECK(state==HG_BATTLE_YES_NO);render(b,state);CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_PARTY);render(b,HG_BATTLE_PARTY);
    CHECK(!hg_battle_menu_cancellable(b)&&!hg_battle_input(b,255)&&!hg_battle_input(b,1));CHECK(hg_battle_input(b,2)&&next(b)==HG_BATTLE_COMMAND);HGBattleMonView active;CHECK(hg_battle_mon(b,0,&active)&&active.species==179&&active.hp);CHECK(!GetMonData(Party_GetMonByIndex((Party *)hg_battle_party(b),0),MON_DATA_HP,NULL));CHECK(!memcmp(save,before,sizeof(*save)));
    hg_battle_free(b);hg_save_manager_bind(NULL);free(save);free(before);
}
static void falkner_team(void){
    SaveData *save=malloc(sizeof(*save));CHECK(save);hg_save_manager_init_ram(save);CHECK(hg_name_encode("Ethan",hg_story_save_profile(save)->name,7));Party *party=hg_story_save_party(save);Pokemon mon;
    /* First-Gym feature roster at/below Falkner's level-13 ace. */
    const unsigned species[]={74,179,152,163,194,92},levels[]={13,12,11,11,11,11};
    for(unsigned i=0;i<6;i++){CreateMon(&mon,species[i],levels[i],31,TRUE,100+i,OT_ID_PRESET,42);CHECK(Party_AddMon(party,&mon));}
    HGBattleSetup setup={.storage=SaveArray_Get(save,SAVE_PCSTORAGE),.party=party,.trainer_id=20,.profile=hg_story_save_profile(save),.bag=hg_story_save_bag(save),.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.seed=1};
    HGBattle *b=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(b);unsigned money=PlayerProfile_GetMoney(hg_story_save_profile(save));int state=HG_BATTLE_RUNNING;unsigned seen=0,turns=0;
    while(state!=HG_BATTLE_FINISHED&&turns++<500){
        state=next(b);HGBattleMonView enemy,active;CHECK(hg_battle_mon(b,1,&enemy));if(enemy.species==16)seen|=1;if(enemy.species==17)seen|=2;
        if(state==HG_BATTLE_COMMAND)CHECK(hg_battle_input(b,1));
        else if(state==HG_BATTLE_MOVES){CHECK(hg_battle_mon(b,0,&active));unsigned slot=0;for(unsigned i=0;i<4;i++)if(active.moves[i]==88&&hg_battle_move_allowed(b,i)){slot=i;break;}while(slot<4&&!hg_battle_move_allowed(b,slot))slot++;CHECK(slot<4&&hg_battle_input(b,slot+1));}
        else if(state==HG_BATTLE_YES_NO)CHECK(hg_battle_input(b,255));
        else if(state==HG_BATTLE_PARTY){unsigned row=0;while(row<hg_battle_menu_count(b,state)&&!hg_battle_choice_code(b,state,row))row++;CHECK(row<hg_battle_menu_count(b,state)&&hg_battle_input(b,row+1));}
        else CHECK(state==HG_BATTLE_FINISHED);
    }
    CHECK(turns<500&&seen==3&&hg_battle_result(b)==1&&hg_battle_ai_calls(b)>0);CHECK(hg_battle_commit_storage(b,party,hg_story_save_profile(save),hg_story_save_bag(save),hg_story_save_dex(save),hg_story_save_stats(save),SaveArray_Get(save,SAVE_PCSTORAGE)));
    CHECK(PlayerProfile_GetMoney(hg_story_save_profile(save))>money);
    hg_battle_free(b);
    setup.trainer_id=21;b=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(b&&Party_GetCount(hg_battle_opponent(b))==3); /* Original odd-sized, padded party member. */
    hg_battle_free(b);hg_save_manager_bind(NULL);free(save);
}
int main(void){
    forced_replacement();
    falkner_team();
    SaveData *save=malloc(sizeof(*save)),*before=malloc(sizeof(*before));CHECK(save&&before);hg_save_manager_init_ram(save);
    /* Diverse early-Violet feature roster; separate from progression saves.
     * Original level-up learnsets, abilities and stats populate all six. */
    const unsigned species[]={152,179,163,194,74,92},levels[]={11,11,10,10,11,10};Party *party=hg_story_save_party(save);Pokemon mon;
    for(unsigned i=0;i<6;i++){CreateMon(&mon,species[i],levels[i],31,TRUE,100+i,OT_ID_PRESET,42);CHECK(Party_AddMon(party,&mon));}
    Pokemon *lead=Party_GetMonByIndex(party,0);u32 hp=GetMonData(lead,MON_DATA_MAX_HP,NULL)-5,zero=0,status=64;SetMonData(lead,MON_DATA_STATUS,&status);SetMonData(lead,MON_DATA_HP,&hp);SetMonData(lead,MON_DATA_MOVE1_PP,&zero);SetMonData(Party_GetMonByIndex(party,2),MON_DATA_HP,&zero);
    Party opponent;Party_InitWithMaxSize(&opponent,6);CreateMon(&mon,129,3,31,TRUE,555,OT_ID_PRESET,42);CHECK(Party_AddMon(&opponent,&mon));
    Bag *items=hg_story_save_bag(save);CHECK(Bag_AddItem(items,ITEM_POTION,3,HEAP_ID_FIELD2));CHECK(Bag_AddItem(items,ITEM_ETHER,1,HEAP_ID_FIELD2));CHECK(Bag_AddItem(items,ITEM_PARLYZ_HEAL,1,HEAP_ID_FIELD2));CHECK(Bag_AddItem(items,ITEM_REVIVE,1,HEAP_ID_FIELD2));
    *before=*save;HGBattleSetup setup={.storage=SaveArray_Get(save,SAVE_PCSTORAGE),.party=party,.opponent=&opponent,.profile=hg_story_save_profile(save),.bag=items,.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.seed=1};
    HGBattle *b=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(b&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_command_count(b)==5);HGBattleMonView enemy,active;CHECK(hg_battle_mon(b,1,&enemy));unsigned pp=enemy.pp[0];
    CHECK(hg_battle_input(b,5)&&next(b)==HG_BATTLE_BAG);render(b,HG_BATTLE_BAG);CHECK(hg_battle_input(b,255)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,1,&enemy)&&enemy.pp[0]==pp);
    bag(b,"Potion");render(b,HG_BATTLE_ITEM_TARGET);CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,0,&active)&&active.hp==active.max_hp);CHECK(hg_battle_mon(b,1,&enemy)&&enemy.pp[0]==pp-1);pp=enemy.pp[0];
    bag(b,"Potion");CHECK(!hg_battle_input(b,1));CHECK(strstr(hg_battle_menu_hint(b,HG_BATTLE_ITEM_TARGET,0),"effect"));CHECK(hg_battle_input(b,255)&&next(b)==HG_BATTLE_BAG);CHECK(hg_battle_input(b,255)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,1,&enemy)&&enemy.pp[0]==pp);
    bag(b,"Parlyz Heal");CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,0,&active)&&!active.status);
    bag(b,"Ether");CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_ITEM_MOVE);render(b,HG_BATTLE_ITEM_MOVE);CHECK(hg_battle_input(b,1)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,0,&active)&&active.pp[0]==10);
    bag(b,"Revive");CHECK(hg_battle_input(b,3)&&next(b)==HG_BATTLE_COMMAND);CHECK(GetMonData(Party_GetMonByIndex((Party *)hg_battle_party(b),2),MON_DATA_HP,NULL)>0);
    CHECK(hg_battle_input(b,3)&&next(b)==HG_BATTLE_PARTY);render(b,HG_BATTLE_PARTY);CHECK(!hg_battle_input(b,1));CHECK(hg_battle_input(b,255)&&next(b)==HG_BATTLE_COMMAND);
    CHECK(hg_battle_input(b,3)&&next(b)==HG_BATTLE_PARTY);CHECK(hg_battle_input(b,2)&&next(b)==HG_BATTLE_COMMAND);CHECK(hg_battle_mon(b,0,&active)&&active.species==179);
    CHECK(!memcmp(save,before,sizeof(*save)));CHECK(hg_battle_input(b,4)&&next(b)==HG_BATTLE_FINISHED);CHECK(hg_battle_commit_storage(b,party,hg_story_save_profile(save),items,hg_story_save_dex(save),hg_story_save_stats(save),SaveArray_Get(save,SAVE_PCSTORAGE)));
    CHECK(Bag_GetQuantity(items,ITEM_POTION,HEAP_ID_FIELD2)==2&&!Bag_GetQuantity(items,ITEM_ETHER,HEAP_ID_FIELD2)&&!Bag_GetQuantity(items,ITEM_REVIVE,HEAP_ID_FIELD2));CHECK(Party_GetCount(party)==6);
    char path[]="/tmp/hg-battle-menus-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));SaveData *loaded=malloc(sizeof(*loaded));CHECK(loaded&&ds_save_flash_open(path)&&hg_save_manager_write(save,error,sizeof(error)));ds_save_flash_close();CHECK(ds_save_flash_open_existing(path)&&hg_save_manager_load(loaded,error,sizeof(error)));ds_save_flash_close();CHECK(!memcmp(party,hg_story_save_party(loaded),sizeof(*party))&&!memcmp(items,hg_story_save_bag(loaded),sizeof(*items)));CHECK(!unlink(path));free(loaded);
    hg_battle_free(b);hg_save_manager_bind(NULL);free(save);free(before);puts("Six-mon battle roster: original medicine effects/turn costs, no-effect/cancel, bench revival, PP restoration, voluntary/forced switching, original Falkner AI/replacement/rewards and durable publication pass");
}
