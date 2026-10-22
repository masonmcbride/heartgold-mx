#include "compat/battle.h"
#include "battle_runtime.h"
#include "game_stats.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static Party party_for(unsigned species,PlayerProfile *profile){
    Party party;Pokemon mon;Party_InitWithMaxSize(&party,6);CreateMon(&mon,species,5,31,FALSE,0,OT_ID_PRESET,profile->id);SetMonData(&mon,MON_DATA_OT_NAME,profile->name);SetMonData(&mon,MON_DATA_OT_GENDER,&profile->gender);CHECK(Party_AddMon(&party,&mon));return party;
}
int main(void){
    const unsigned trainer_ids[]={496,497,495},starters[]={152,155,158},opponents[]={155,158,152};
    PlayerProfile profile;PlayerProfile_Init(&profile);profile.id=12345;profile.name[0]=0x12b;profile.name[1]=0xffff;
    Bag bag;Save_Bag_Init(&bag);CHECK(Bag_AddItem(&bag,ITEM_POTION,5,HEAP_ID_BATTLE));
    Options options;Options_Init(&options);Pokedex dex;Save_Pokedex_Init(&dex);GameStats stats;GameStats_Init(&stats);
    for(unsigned i=0;i<3;i++){
        Party party=party_for(starters[i],&profile),initial=party;u32 seed=GetLCRNGSeed();char error[256]={0};
        HGBattleSetup setup={.party=&party,.profile=&profile,.bag=&bag,.options=&options,.pokedex=&dex,.stats=&stats,.trainer_id=trainer_ids[i],.seed=1};
        HGBattle *battle=hg_battle_new_setup(&setup,error,sizeof(error));if(!battle)fprintf(stderr,"%s\n",error);CHECK(battle);CHECK(GetLCRNGSeed()==seed);
        Party enemy=*hg_battle_opponent(battle);CHECK(Party_GetCount(&enemy)==1);Pokemon *mon=Party_GetMonByIndex(&enemy,0);
        CHECK(GetMonData(mon,MON_DATA_SPECIES,NULL)==opponents[i]);CHECK(GetMonData(mon,MON_DATA_LEVEL,NULL)==5);
        CHECK(GetMonData(mon,MON_DATA_HP_IV,NULL)==0);CHECK(GetMonData(mon,MON_DATA_FRIENDSHIP,NULL)==255);
        CHECK(!hg_battle_commit(battle,&party,&profile,&bag,&dex,&stats));
        HGBattle *twin=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(twin);
        int finished=0,introduction=0;for(unsigned frame=0;frame<4000;frame++){
            u32 host_rng=GetLCRNGSeed();int state=hg_battle_tick(battle);CHECK(GetLCRNGSeed()==host_rng);int twin_state=hg_battle_tick(twin);CHECK(twin_state==state&&GetLCRNGSeed()==host_rng);CHECK(!strcmp(hg_battle_message(battle),hg_battle_message(twin)));if(state==HG_BATTLE_FAILED)fprintf(stderr,"Trainer stopped: %s\n",hg_battle_error(battle));CHECK(state!=HG_BATTLE_FAILED);
            if(state==HG_BATTLE_FINISHED){finished=1;break;}
            if(state==HG_BATTLE_MESSAGE){CHECK(!strstr(hg_battle_message(battle),"wild"));if(strstr(hg_battle_message(battle),"Boy"))introduction=1;}
            if(state==HG_BATTLE_MESSAGE||state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES){CHECK(hg_battle_input(battle,1));CHECK(hg_battle_input(twin,1));}
        }
        CHECK(!memcmp(hg_battle_party(battle),hg_battle_party(twin),sizeof(party)));CHECK(hg_battle_ai_steps(battle)==hg_battle_ai_steps(twin));hg_battle_free(twin);
        CHECK(finished&&introduction);CHECK(hg_battle_result(battle)==BATTLE_OUTCOME_WIN);CHECK(hg_battle_ai_calls(battle)>0&&hg_battle_ai_steps(battle)>0);
        CHECK(!memcmp(&party,&initial,sizeof(party)));
        printf("Original rival %u: victory, %u original AI command/move calls, %zu Thumb instructions\n",trainer_ids[i],hg_battle_ai_calls(battle),hg_battle_ai_steps(battle));
        CHECK(hg_battle_commit(battle,&party,&profile,&bag,&dex,&stats));CHECK(GetMonData(Party_GetMonByIndex(&party,0),MON_DATA_EXPERIENCE,NULL)>GetMonData(Party_GetMonByIndex(&initial,0),MON_DATA_EXPERIENCE,NULL));
        hg_battle_free(battle);
        battle=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(battle);CHECK(!memcmp(&enemy,hg_battle_opponent(battle),sizeof(enemy)));hg_battle_free(battle);
        printf("Original rival %u: correct level-5 opposing starter, IVs and deterministic party\n",trainer_ids[i]);
    }
    Party enemy;Party_InitWithMaxSize(&enemy,6);Pokemon wild;CreateMon(&wild,19,2,17,TRUE,1234,OT_ID_PRESET,profile.id);CHECK(Party_AddMon(&enemy,&wild));Party enemy_snapshot=enemy;
    Party party=party_for(155,&profile),initial=party;HGBattleSetup setup={.party=&party,.profile=&profile,.bag=&bag,.options=&options,.pokedex=&dex,.stats=&stats,.opponent=&enemy,.seed=1};
    char error[256]={0};HGBattle *battle=hg_battle_new_setup(&setup,error,sizeof(error));CHECK(battle);CHECK(!memcmp(hg_battle_opponent(battle),&enemy,sizeof(enemy)));int finished=0;
    for(unsigned n=0;n<3000;n++){
        int state=hg_battle_tick(battle);if(state==HG_BATTLE_FAILED)fprintf(stderr,"%s\n",hg_battle_error(battle));CHECK(state!=HG_BATTLE_FAILED);
        if(state==HG_BATTLE_FINISHED){finished=1;break;}
        if(state==HG_BATTLE_MESSAGE){CHECK(!strstr(hg_battle_message(battle),"boosted"));CHECK(hg_battle_input(battle,1));}
        else if(state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES)CHECK(hg_battle_input(battle,1));
    }
    CHECK(finished);CHECK(!memcmp(&enemy,&enemy_snapshot,sizeof(enemy)));CHECK(!memcmp(&party,&initial,sizeof(party)));
    Bag saved=bag;CHECK(Bag_AddItem(&bag,ITEM_POTION,1,HEAP_ID_BATTLE));
    CHECK(!hg_battle_commit(battle,&party,&profile,&bag,&dex,&stats));CHECK(!memcmp(&party,&initial,sizeof(party)));
    bag=saved;CHECK(hg_battle_commit(battle,&party,&profile,&bag,&dex,&stats));CHECK(memcmp(&party,&initial,sizeof(party)));
    CHECK(!hg_battle_commit(battle,&party,&profile,&bag,&dex,&stats));CHECK(Bag_GetQuantity(&bag,ITEM_POTION,HEAP_ID_BATTLE)==5);
    hg_battle_free(battle);
    setup.trainer_id=9999;setup.opponent=NULL;setup.enemy_species=setup.enemy_level=0;CHECK(!hg_battle_new_setup(&setup,error,sizeof(error))&&error[0]);
    puts("Session profile affects original EXP; stale/unfinished/double result publication rejected");return 0;
}
