#include "compat/bug_contest.h"
#include "compat/save_manager.h"
#include "constants/items.h"
#include "constants/trainer_class.h"
#include "sys_flags.h"
#include "bug_contest_types.inc"
#include "bug_contest_resources.inc"
#include "save_c_records_decls.inc"
void FieldSystem_StartBugContestTimer(FieldSystem *);
/* Filesystem boundary: exact original generated binary payloads, no runtime
 * source tree dependency and no substitution for missing resource reads. */
typedef struct { const u8 *bytes;u32 length; } FSFile;
static void FS_InitFile(FSFile *file) { memset(file,0,sizeof(*file)); }
static BOOL FS_OpenFile(FSFile *file,const char *path) {
    if(!strcmp(path,"data/mushi/mushi_encount.bin")){file->bytes=mushi_encount;file->length=sizeof(mushi_encount);}
    else if(!strcmp(path,"data/mushi/mushi_trainer.bin")){file->bytes=mushi_trainer;file->length=sizeof(mushi_trainer);}
    else return FALSE;return TRUE;
}
static u32 FS_GetLength(FSFile *file) { return file->length; }
static s32 FS_ReadFile(FSFile *file,void *out,u32 size) { if(size!=file->length)return -1;memcpy(out,file->bytes,size);return size; }
static void FS_CloseFile(FSFile *file) { FS_InitFile(file); }
/* The original ARM leaf returns the alive non-egg party index. Reuse its
 * original C counterpart; admission below guarantees its precondition. */
#define RTC_WEEK_MAX 7
#include "bug_contest_decls.inc"
static u16 Save_GetPartyLeadAlive(SaveData *save) { return GetIdxOfFirstAliveMonInParty_CrashIfNone(SaveArray_Get(save,SAVE_PARTY)); }
#define SaveArray_Party_Get(save) ((Party *)SaveArray_Get((save),SAVE_PARTY))
#define SaveArray_PCStorage_Get(save) ((PCStorage *)SaveArray_Get((save),SAVE_PCSTORAGE))
#define Save_Pokedex_Get(save) ((Pokedex *)SaveArray_Get((save),SAVE_POKEDEX))
#include "bug_contest_kernel.inc"

BugContest *hg_bug_contest_new(SaveData *save,unsigned weekday,char *error,size_t size) {
    if(error&&size)error[0]=0;
    if(!save||!hg_save_manager_validate_ram(save)||weekday>6){if(error&&size)snprintf(error,size,"Invalid contest save or weekday");return NULL;}
    Party *party=SaveArray_Get(save,SAVE_PARTY);int alive=0;
    for(int i=0;i<Party_GetCount(party);i++)if(MonNotFaintedOrEgg(Party_GetMonByIndex(party,i)))alive=1;
    if(!alive){if(error&&size)snprintf(error,size,"A conscious non-egg Pokemon is required for the contest");return NULL;}
    SaveData *previous_save=hg_save_manager_exchange(save);FieldSystem field={0};field.saveData=save;
    /* Entry runs after the shared field clock transaction. The original New
     * timer samples that same baseline, and regular field updates subsequently
     * advance this owner; no second phone/clock controller is constructed. */
    SysInfo_RTC *rtc=&((SysInfo *)SaveArray_Get(save,SAVE_SYSINFO))->rtc_info;
    HGSaveRTC baseline={rtc->date,rtc->time};
    const HGSaveRTC *previous=hg_save_rtc_exchange(&baseline);
    BugContest *contest=BugContest_New(&field,weekday);
    hg_save_rtc_exchange(previous);hg_save_manager_exchange(previous_save);return contest;
}
BugContest *hg_bug_contest_clone(const BugContest *source) {
    if(!source)return NULL;
    BugContest *copy=malloc(sizeof(*copy));if(!copy)return NULL;*copy=*source;
    copy->mon=malloc(sizeof(*copy->mon));copy->party_bak=malloc(sizeof(*copy->party_bak));
    if(!copy->mon||!copy->party_bak){free(copy->mon);free(copy->party_bak);free(copy);return NULL;}
    *copy->mon=*source->mon;*copy->party_bak=*source->party_bak;return copy;
}
void hg_bug_contest_discard(BugContest *contest) { if(contest){free(contest->party_bak);free(contest->mon);free(contest);} }
void hg_bug_contest_rebind(BugContest *contest,SaveData *save) { if(contest){GF_ASSERT(save);contest->saveData=save;contest->party_cur=SaveArray_Get(save,SAVE_PARTY);} }
void hg_bug_contest_delete(BugContest *contest) { if(contest){SaveData *previous=hg_save_manager_exchange(contest->saveData);BugContest_Delete(contest);hg_save_manager_exchange(previous);} }
void hg_bug_contest_judge(BugContest *contest) { if(contest)BugContest_Judge(contest); }
int hg_bug_contest_encounter(BugContest *contest,unsigned *species,unsigned *level) {
    if(!contest||!species||!level||!contest->sport_balls||contest->elapsed_time>=20)return 0;
    EncounterSlot *slot=BugContest_GetEncounterSlot(contest,HEAP_ID_3);*species=slot->species;*level=slot->maxLevel;Heap_Free(slot);return 1;
}
int hg_bug_contest_keep(BugContest *contest,const Pokemon *caught,int replace) {
    if(!contest||!caught)return 0;
    if(!contest->caught_poke||replace)CopyPokemonToPokemon(caught,contest->mon);
    contest->caught_poke=TRUE;return 1;
}
int hg_bug_contest_registered(BugContest *contest,unsigned id) { return contest&&id<10&&BugContest_ContestantIsRegistered(contest,id); }
