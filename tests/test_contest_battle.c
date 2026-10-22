#include "compat/bug_contest.h"
#include "compat/save_manager.h"
#include "battle_runtime.h"
#include "bag.h"
#include "game_name.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);} } while(0)
static char error[256];
int main(void) {
    SaveData *save=malloc(sizeof(*save));CHECK(save);hg_save_manager_init_ram(save);
    CHECK(hg_name_encode("Ethan",hg_story_save_profile(save)->name,7));
    Party *party=SaveArray_Get(save,SAVE_PARTY);Pokemon mon;CreateMon(&mon,152,15,31,TRUE,123,OT_ID_PRESET,42);CHECK(Party_AddMon(party,&mon));
    BugContest *contest=hg_bug_contest_new(save,2,error,sizeof(error));CHECK(contest);
    Party opponent;Party_InitWithMaxSize(&opponent,6);CreateMon(&mon,10,3,31,TRUE,124,OT_ID_PRESET,42);
    u32 hp=1,status=3;SetMonData(&mon,MON_DATA_HP,&hp);SetMonData(&mon,MON_DATA_STATUS,&status);CHECK(Party_AddMon(&opponent,&mon));
    HGBattleSetup setup={.party=party,.opponent=&opponent,.profile=hg_story_save_profile(save),.bag=hg_story_save_bag(save),.options=hg_story_save_options(save),.pokedex=hg_story_save_dex(save),.stats=hg_story_save_stats(save),.contest=contest,.seed=1};
    contest->elapsed_time=20;
    CHECK(!hg_battle_new_setup(&setup,error,sizeof(error)));
    CHECK(contest->sport_balls==20&&Party_GetCount(party)==1);
    contest->elapsed_time=0;
    HGBattle *battle=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(battle);
    unsigned ticks=0,balls=0;int state=HG_BATTLE_RUNNING;
    while(state!=HG_BATTLE_FINISHED&&ticks++<20000) {
        state=hg_battle_tick(battle);
        if(state==HG_BATTLE_FAILED)snprintf(error,sizeof(error),"%s",hg_battle_error(battle));CHECK(state!=HG_BATTLE_FAILED);
        if(state==HG_BATTLE_MESSAGE)hg_battle_input(battle,1);
        if(state==HG_BATTLE_COMMAND){CHECK(hg_battle_input(battle,2));balls++;}
        CHECK(contest->sport_balls==20&&Party_GetCount(party)==1);
    }
    CHECK(ticks<20000&&balls&&balls<20);
    Pokemon caught;CHECK(hg_battle_contest_catch(battle,&caught,sizeof(caught)));
    CHECK(GetMonData(&caught,MON_DATA_SPECIES,NULL)==10&&hg_battle_sport_balls(battle)==20-balls);
    contest->sport_balls--;
    CHECK(!hg_battle_commit(battle,party,hg_story_save_profile(save),hg_story_save_bag(save),hg_story_save_dex(save),hg_story_save_stats(save)));
    CHECK(contest->sport_balls==19&&Party_GetCount(party)==1);
    contest->sport_balls++;
    CHECK(hg_battle_commit(battle,party,hg_story_save_profile(save),hg_story_save_bag(save),hg_story_save_dex(save),hg_story_save_stats(save)));
    CHECK(Party_GetCount(party)==1&&contest->sport_balls==20-balls&&!contest->caught_poke);
    CHECK(hg_bug_contest_keep(contest,&caught,1)&&contest->caught_poke);
    hg_battle_free(battle);hg_bug_contest_delete(contest);CHECK(Party_GetCount(party)==2);
    hg_save_manager_bind(NULL);free(save);
    puts("Original contest battle: Sport Ball command/capture/Pokedex, private result, atomic ball count, retained catch and party restoration pass");
}
