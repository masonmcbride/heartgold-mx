#include "compat/bug_contest.h"
#include "compat/save_manager.h"
#include "constants/species.h"
#include "constants/items.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);} }while(0)
static char error[256];
static SaveData *new_save(unsigned count) {
    SaveData *save=calloc(1,sizeof(*save));CHECK(save);hg_save_manager_init_ram(save);hg_save_manager_bind(save);
    Party *party=SaveArray_Get(save,SAVE_PARTY);
    for(unsigned i=0;i<count;i++){Pokemon mon;CreateMon(&mon,SPECIES_CHIKORITA+i,15,32,FALSE,0,OT_ID_PLAYER_ID,0);CHECK(Party_AddMon(party,&mon));}
    return save;
}
static void close_save(SaveData *save){hg_save_manager_bind(NULL);free(save);}
int main(void) {
    SaveData *save=new_save(0);CHECK(!hg_bug_contest_new(save,2,error,sizeof(error)));close_save(save);
    for(unsigned count=1;count<=6;count++) {
        save=new_save(count);Party *party=SaveArray_Get(save,SAVE_PARTY);Party before=*party;
        /* Fainted leading members are kept in backup; the first valid fighter
         * carries damage/PP/modifiers back to its original slot on exit. */
        u32 zero=0;
        if(count>1)SetMonData(&party->core.mons[0],MON_DATA_HP,&zero);
        before=*party;
        BugContest *contest=hg_bug_contest_new(save,2,error,sizeof(error));CHECK(contest);
        CHECK(contest->sport_balls==20&&contest->elapsed_time==0&&contest->party_cur_num==count);
        CHECK(Party_GetCount(party)==1);CHECK(contest->lead_mon_idx==(count>1?1:0));
        unsigned seen=0;
        for(unsigned i=0;i<5;i++){CHECK(contest->contestants[i].id<10);CHECK(!(seen&(1u<<contest->contestants[i].id)));seen|=1u<<contest->contestants[i].id;CHECK(hg_bug_contest_registered(contest,contest->contestants[i].id));}
        for(unsigned i=0;i<100;i++) {
            unsigned species,level;CHECK(hg_bug_contest_encounter(contest,&species,&level));int found=0;
            for(unsigned j=0;j<10;j++)if(contest->encounters[j].species==species){CHECK(level>=contest->encounters[j].lvlmin&&level<=contest->encounters[j].lvlmax);found=1;}
            CHECK(found);
        }
        /* No catch means no contestant score; guaranteed consolation. */
        hg_bug_contest_judge(contest);CHECK(contest->contestants[5].score==0&&contest->placement==3&&contest->prize==ITEM_SHED_SHELL);
        Pokemon catch1,catch2;CreateMon(&catch1,SPECIES_CATERPIE,10,32,FALSE,0,OT_ID_PLAYER_ID,0);CreateMon(&catch2,SPECIES_SCYTHER,14,32,FALSE,0,OT_ID_PLAYER_ID,0);
        CHECK(hg_bug_contest_keep(contest,&catch1,0));CHECK(hg_bug_contest_keep(contest,&catch2,0));CHECK(GetMonData(contest->mon,MON_DATA_SPECIES,NULL)==SPECIES_CATERPIE);
        CHECK(hg_bug_contest_keep(contest,&catch2,1));CHECK(GetMonData(contest->mon,MON_DATA_SPECIES,NULL)==SPECIES_SCYTHER);
        /* Independently calculate the original score terms. */
        unsigned sum=0;for(unsigned i=0;i<6;i++)sum+=GetMonData(contest->mon,MON_DATA_HP_IV+i,NULL);
        unsigned expected=100+100+sum*100/(31*6)+GetMonData(contest->mon,MON_DATA_HP,NULL)*100/GetMonData(contest->mon,MON_DATA_MAX_HP,NULL);
        for(unsigned i=0;i<5;i++)contest->contestants[i].score=0;
        hg_bug_contest_judge(contest);CHECK(contest->contestants[5].score==expected&&contest->placement==0&&contest->prize==ITEM_SUN_STONE);
        unsigned hp=1;SetMonData(Party_GetMonByIndex(party,0),MON_DATA_HP,&hp);
        memset(&party->extra.aprijuiceModifiers[0],0x35,sizeof(PartyExtraSub));
        /* Owner rebinding is required after the script story transaction. */
        SaveData *moved=malloc(sizeof(*moved));CHECK(moved);*moved=*save;hg_bug_contest_rebind(contest,moved);hg_save_manager_bind(NULL);free(save);save=moved;party=SaveArray_Get(save,SAVE_PARTY);
        hg_bug_contest_delete(contest);
        CHECK(Party_GetCount(party)==(count<6?count+1:6));CHECK(GetMonData(Party_GetMonByIndex(party,count>1?1:0),MON_DATA_HP,NULL)==1);
        for(unsigned i=0;i<count;i++)if(i!=(count>1?1:0))CHECK(!memcmp(&before.core.mons[i],&party->core.mons[i],sizeof(Pokemon)));
        PartyExtraSub modifier;memset(&modifier,0x35,sizeof(modifier));CHECK(!memcmp(&modifier,&party->extra.aprijuiceModifiers[count>1?1:0],sizeof(modifier)));
        if(count<6)CHECK(GetMonData(Party_GetMonByIndex(party,count),MON_DATA_SPECIES,NULL)==SPECIES_SCYTHER);
        else{PCStorage *pc=SaveArray_Get(save,SAVE_PCSTORAGE);CHECK(GetBoxMonData(&pc->boxes[0].mons[0],MON_DATA_SPECIES,NULL)==SPECIES_SCYTHER);CHECK(pc->boxModifiedFlag&1);}
        close_save(save);
    }
    puts("Original contest party backup/restore, encounters, keep/replace, judging and party/PC retrieval pass");return 0;
}
