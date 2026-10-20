#include "compat/pokemon.h"
#include "battle_runtime.h"
#include "ds_audio.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static Party make_party(unsigned species){
    Party party;Party_InitWithMaxSize(&party,6);Pokemon mon;CreateMon(&mon,species,5,31,FALSE,0,0,0);u32 exp; /* Place the original level-5 mon one point below level 6. */
    u8 next=6;SetMonData(&mon,MON_DATA_LEVEL,&next);exp=GetMonBaseExperienceAtCurrentLevel(&mon)-1;next=5;SetMonData(&mon,MON_DATA_LEVEL,&next);SetMonData(&mon,MON_DATA_EXPERIENCE,&exp);
    CHECK(Party_AddMon(&party,&mon));return party;
}
static int summary_pages;
static int advance(HGBattle *battle){
    int state=hg_battle_tick(battle);
    if(state==HG_BATTLE_FAILED)fprintf(stderr,"%s\n",hg_battle_error(battle));
    CHECK(state!=HG_BATTLE_FAILED);
    if(state==HG_BATTLE_MESSAGE){if(strstr(hg_battle_message(battle),"HP  "))summary_pages++;CHECK(hg_battle_message(battle)[0]);CHECK(!strstr(hg_battle_message(battle),"Battle message"));CHECK(!strchr(hg_battle_message(battle),'{'));CHECK(hg_battle_input(battle,1));}
    else if(state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES){CHECK(!hg_battle_input(battle,0));CHECK(hg_battle_input(battle,1));}
    return state;
}
int main(void){
    gSystem.newKeys=1;const unsigned starters[]={152,155,158};
    for(unsigned n=0;n<NELEMS(starters);n++){
        SetLCRNGSeed(1234);Party party=make_party(starters[n]),snapshot=party;
        unsigned before_exp=GetMonData(Party_GetMonByIndex(&party,0),MON_DATA_EXPERIENCE,NULL);
        unsigned before_pp=GetMonData(Party_GetMonByIndex(&party,0),MON_DATA_MOVE1_PP,NULL);
        u32 host_seed=GetLCRNGSeed();char error[256]={0};HGBattle *a=hg_battle_new(&party,19,2,0,NULL,error,sizeof(error));CHECK(a);
        HGBattle *b=hg_battle_new(&party,19,2,0,NULL,error,sizeof(error));CHECK(b);
        summary_pages=0;unsigned done_a=0,done_b=0,ticks;
        for(ticks=0;ticks<3000&&(!done_a||!done_b);ticks++){
            if(!done_a)done_a=advance(a)==HG_BATTLE_FINISHED;
            if(!done_b)done_b=advance(b)==HG_BATTLE_FINISHED;
        }
        CHECK(gSystem.newKeys==1);CHECK(GetLCRNGSeed()==host_seed);CHECK(summary_pages==4);CHECK(done_a&&done_b);CHECK(hg_battle_result(a)==BATTLE_OUTCOME_WIN&&hg_battle_result(b)==BATTLE_OUTCOME_WIN);
        CHECK(!memcmp(&party,&snapshot,sizeof(party)));
        const Party *result=hg_battle_party(a);CHECK(result);
        Pokemon *mon=Party_GetMonByIndex((Party *)result,0);
        CHECK(GetMonData(mon,MON_DATA_EXPERIENCE,NULL)>before_exp);CHECK(GetMonData(mon,MON_DATA_LEVEL,NULL)==6);
        CHECK(GetMonData(mon,MON_DATA_MOVE1_PP,NULL)<before_pp);
        CHECK(GetMonData(mon,MON_DATA_MAX_HP,NULL)>GetMonData(Party_GetMonByIndex(&party,0),MON_DATA_MAX_HP,NULL));
        u16 learnset[MAX_LEARNED_MOVES];LoadLevelUpLearnset_HandleAlternateForm(starters[n],0,learnset);unsigned learned=0;
        for(unsigned i=0;i<MAX_LEARNED_MOVES&&learnset[i]!=LEVEL_UP_LEARNSET_END;i++)if((learnset[i]>>9)==6){
            unsigned found=0;for(unsigned slot=0;slot<4;slot++)found|=GetMonData(mon,MON_DATA_MOVE1+slot,NULL)==(learnset[i]&511);
            CHECK(found);learned++;
        }
        CHECK(learned>0);
        HGBattleMonView opponent;CHECK(hg_battle_mon(a,1,&opponent)&&opponent.hp==0);
        CHECK(hg_battle_result(a)==hg_battle_result(b));
        CHECK(!memcmp(hg_battle_party(a),hg_battle_party(b),sizeof(Party)));
        hg_battle_free(a);hg_battle_free(b);
        /* Cancel while the summary owns the original previous-level stat allocation. */
        a=hg_battle_new(&party,19,2,0,NULL,error,sizeof(error));CHECK(a);int cancelled=0;
        for(ticks=0;ticks<3000;ticks++){
            int state=hg_battle_tick(a);CHECK(state!=HG_BATTLE_FAILED&&state!=HG_BATTLE_FINISHED);
            if(state==HG_BATTLE_MESSAGE&&strstr(hg_battle_message(a),"HP  +")){cancelled=1;break;}
            if(state==HG_BATTLE_MESSAGE||state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES)CHECK(hg_battle_input(a,1));
        }
        CHECK(cancelled);hg_battle_free(a);CHECK(gSystem.newKeys==1);CHECK(GetLCRNGSeed()==host_seed);CHECK(!memcmp(&party,&snapshot,sizeof(party)));
        printf("Starter %u: interleaved original battles, victory/EXP/PP, level 6, both stat pages and summary cancellation passed\n",starters[n]);
    }
    {
        Party party=make_party(155);char error[256]={0};HGBattle *battle=hg_battle_new(&party,19,2,0,NULL,error,sizeof(error));CHECK(battle);unsigned done=0,escaped=0;
        for(unsigned tick=0;tick<3000;tick++){
            int state=hg_battle_tick(battle);if(state==HG_BATTLE_FAILED)fprintf(stderr,"Flee: %s\n",hg_battle_error(battle));CHECK(state!=HG_BATTLE_FAILED);
            if(state==HG_BATTLE_FINISHED){done=1;break;}
            if(state==HG_BATTLE_MESSAGE){escaped|=strstr(hg_battle_message(battle),"Got away safely")!=NULL;CHECK(hg_battle_input(battle,1));}
            else if(state==HG_BATTLE_COMMAND)CHECK(hg_battle_input(battle,4));
            else if(state==HG_BATTLE_MOVES)CHECK(hg_battle_input(battle,1));
        }
        CHECK(done&&escaped);hg_battle_free(battle);puts("Original Run command, escape message and completion passed");
    }
    /* The caller advances the borrowed audio renderer; no device is opened. */
    Party party=make_party(155);DSAudio *audio=ds_audio_new();CHECK(audio);char error[256]={0};
    HGBattle *battle=hg_battle_new(&party,19,2,0,audio,error,sizeof(error));CHECK(battle);
    unsigned blocked=0,finished=0;
    for(unsigned ticks=0;ticks<3000;ticks++){
        CHECK(ds_audio_tick(audio));int state=hg_battle_tick(battle);CHECK(state!=HG_BATTLE_FAILED);
        if(state==HG_BATTLE_FINISHED){finished=1;break;}
        if(state==HG_BATTLE_MESSAGE){
            int accepted=hg_battle_input(battle,1);
            if(!accepted){CHECK(ds_audio_playing(audio,DS_AUDIO_FANFARE,0));blocked++;}
        }else if(state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES)CHECK(hg_battle_input(battle,1));
    }
    CHECK(finished&&blocked>0);hg_battle_free(battle);ds_audio_free(audio);
    printf("Original level-up fanfare blocked acknowledgement for %u audio frames\n",blocked);
    return 0;
}
