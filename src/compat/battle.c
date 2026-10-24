#include "compat/battle.h"
#include "compat/motion.h"
#include "compat/pokemon_services.h"
#include "battle_runtime.h"
#include "compat/resources.h"
#include "ds_audio.h"
#include "ds_thumb.h"
#include "trainer_ai_records.inc"
#include <setjmp.h>
#include <math.h>
#include <time.h>
#include "battle_aliases.inc"
#include "battle_boundary_decls.inc"
#include "battle_imports.inc"
#include "battle_decls.inc"
#undef NULL
#define NULL 0
struct BgConfig {unsigned priorities[4],frame_style;};
struct PaletteData {unsigned archive,member,buffer,size,position;};
struct MessageFormat {char names[6][256];};
struct HGBattle {
    BattleSystem system;BugContest *contest;BugContest initial_contest;Pokemon initial_kept,contest_catch;FieldSystem *tutorial_field;unsigned capture_phase,capture_ticks,capture_fade,capture_pic_x;Party parties[4];OpponentData opponents[4];PlayerProfile profiles[4];
    void *task_arena,*allocations[128];SysTaskQueue *tasks;
    u8 recorded_inputs[4][1024];
    unsigned encounter_time,blackout_ticks;int blackout_brightness;
    uint8_t *ai_ram;unsigned ai_calls;size_t ai_steps;
    u32 global_rng;int caller_keys;
    BgConfig background;PaletteData palette;Window windows[2];
    unsigned summary_open,summary_frame,hp_bars_hidden;char summary[6][2][128];
    MessageFormat format;unsigned direct_message,wait_fanfare,wait_effect;
    BagCursor bag_cursor;
    u16 bag_items[ITEMS_COUNT],chosen_item;unsigned bag_count,item_target,menu_battler;
    unsigned yesno_type;int direct_ball,party_forced,party_cannot_switch,party_excluded;char menu_hint[160];
    PCStorage storage,initial_storage;int has_storage;
    Party initial_party;PlayerProfile initial_profile;Bag initial_bag;Pokedex initial_dex;GameStats initial_stats;unsigned published;
    HGPokemonServices caller_mon_services;HGMonDate mon_date;
    Options options;Pokedex dex;GameStats stats;Bag bag;DSAudio *audio;
    struct {unsigned battler,size,presented;_Alignas(8) u8 bytes[256];} events[64];
    unsigned read,count,menu,waiting,done,frames,budget,input_pending;char text[1024],error[192];jmp_buf trap;
};
_Static_assert(offsetof(HGBattle,system)==0,"Original battle system must remain first in its native owner");
static HGBattle *running_battle;
static void battle_leave(HGBattle *battle,u32 host_rng){
    hg_pokemon_services_exchange(battle->caller_mon_services);
    battle->global_rng=GetLCRNGSeed();SetLCRNGSeed(host_rng);gSystem.newKeys=battle->caller_keys;running_battle=NULL;
}
static _Noreturn void battle_boundary_failed(const char *name) {
    GF_ASSERT(running_battle);snprintf(running_battle->error,sizeof(running_battle->error),"Unsupported battle service: %s",name);longjmp(running_battle->trap,1);
}
#undef GF_ASSERT
#define GF_ASSERT(condition) do{if(!(condition))battle_boundary_failed("assertion: " #condition);}while(0)
/* Keep battle's recoverable failure policy local; archive storage is shared. */
#define ReadWholeNarcMemberByIdPair battle_read_whole_member
#define GetNarcMemberSizeByIdPair battle_member_size
static void battle_read_whole_member(void *,NarcId,s32);
static u32 battle_member_size(NarcId,s32);
#include "battle_kernel.inc"
#include "battle_data.inc"
BtlCmdFunc sBattleScriptCommandTable[]={
#include "battle_table.inc"
};
#include "battle_unsupported.inc"
#include "battle_providers.inc"
#include "capture_host.inc"
#include "battle_menu_host.inc"
#include "tutorial_host.inc"
#include "trainer_ai_host.inc"
#include "trainer_names.inc"
BOOL CheckMetBill(SaveVarsFlags *);
static HGBattle *battle_new(const HGBattleSetup *setup,char *error,size_t size) {
    if(error&&size)error[0]=0;
    const Party *party=setup->party;unsigned species=setup->enemy_species,level=setup->enemy_level,trainer=setup->trainer_id;DSAudio *audio=setup->audio;
    if((setup->contest&&(!setup->contest->sport_balls||setup->contest->elapsed_time>=20||trainer||setup->tutorial_field))||(setup->tutorial_field&&(trainer||setup->opponent||species||level||setup->allow_loss||!setup->tutorial_field->location||!setup->tutorial_field->playerAvatar||!setup->tutorial_field->saveData))||setup->allow_loss>1||(setup->allow_loss&&!trainer)||!party||!Party_GetCount(party)||(!setup->tutorial_field&&((trainer!=0)+(setup->opponent!=NULL)+(species!=0||level!=0)!=1))||(!setup->tutorial_field&&!trainer&&!setup->opponent&&(!species||species>493||!level||level>100))||(trainer&&trainer>=NELEMS(trainer_names))||(setup->opponent&&Party_GetCount(setup->opponent)!=1)){
        if(error&&size)snprintf(error,size,"Invalid battle setup");return NULL;
    }
    HGBattle *battle=calloc(1,sizeof(*battle));if(!battle)return NULL;u32 host_rng=GetLCRNGSeed();time_t now=time(NULL);struct tm date;if(!localtime_r(&now,&date)){free(battle);return NULL;}battle->mon_date=(HGMonDate){(u32)(date.tm_year-100),(u32)(date.tm_mon+1),(u32)date.tm_mday,(u32)date.tm_wday};battle->global_rng=host_rng;battle->caller_keys=gSystem.newKeys;gSystem.newKeys=0;running_battle=battle;battle->caller_mon_services=hg_pokemon_services_exchange(hg_mon_services(&battle->mon_date));
    if(setjmp(battle->trap)){if(error&&size)snprintf(error,size,"%s",battle->error);battle_leave(battle,host_rng);hg_battle_free(battle);return NULL;}
    battle->task_arena=calloc(1,SysTaskQueue_GetArenaSize(32));GF_ASSERT(battle->task_arena);battle->tasks=SysTaskQueue_PlacementNew(32,battle->task_arena);
    if(setup->storage){battle->has_storage=1;battle->storage=battle->initial_storage=*setup->storage;battle->system.storage=&battle->storage;}
    BattleSystem *system=&battle->system;system->maxBattlers=2;system->battleType=(trainer?BATTLE_TYPE_TRAINER:0)|(setup->allow_loss?BATTLE_TYPE_11:0);system->rand=setup->seed;system->options=&battle->options;
    system->bagCursor=&battle->bag_cursor;system->bgConfig=&battle->background;system->palette=&battle->palette;system->window=battle->windows;system->msgFormat=&battle->format;system->pokedex=&battle->dex;system->gameStats=&battle->stats;system->bag=&battle->bag;battle->audio=audio;
    Options_Init(system->options);Save_Pokedex_Init(system->pokedex);
    for(unsigned i=0;i<4;i++){Party_InitWithMaxSize(battle->parties+i,6);system->trainerParty[i]=battle->parties+i;system->opponentData[i]=battle->opponents+i;system->playerProfile[i]=battle->profiles+i;PlayerProfile_Init(battle->profiles+i);battle->opponents[i].battlerType=i;battle->opponents[i].native_controller=!(i&1);}
    battle->parties[0]=*party;
    if(setup->profile)battle->profiles[0]=*setup->profile;
    if(setup->bag)battle->bag=*setup->bag;
    if(setup->options)battle->options=*setup->options;
    if(setup->pokedex)battle->dex=*setup->pokedex;
    if(setup->stats)battle->stats=*setup->stats;
    /* Original BattleSetup_InitFromFieldSystem player trainer record. */
    system->trainers[0].data.trainerClass=battle->profiles[0].gender;memcpy(system->trainers[0].name,battle->profiles[0].name,sizeof(system->trainers[0].name));system->trainers[2]=system->trainers[0];
    battle->initial_party=battle->parties[0];battle->initial_profile=battle->profiles[0];battle->initial_bag=battle->bag;battle->initial_dex=battle->dex;battle->initial_stats=battle->stats;
    if(setup->tutorial_field){
        battle->tutorial_field=setup->tutorial_field;Save_Bag_Init(&battle->bag);
        BattleSetup *demo=BattleSetup_New_Tutorial(HEAP_ID_BATTLE,setup->tutorial_field);system->battleType=demo->battleType;system->terrain=demo->terrain;system->location=MapHeader_GetMapSec(setup->tutorial_field->location->mapId);system->trainers[0]=demo->trainer[0];system->trainers[2]=demo->trainer[2];
        BattleInput *input=Heap_Alloc(HEAP_ID_BATTLE,sizeof(*input));input->battleSystem=system;input->tutorial.finger=Heap_Alloc(HEAP_ID_BATTLE,sizeof(BattleFinger));system->battleInput=input;Heap_Free(demo);
    }else if(trainer){
        BattleSetup opponents={0};for(unsigned i=0;i<4;i++)opponents.party[i]=battle->parties+i;
        opponents.trainerId[1]=trainer;TrainerData_ReadTrData(trainer,opponents.trainer+1);
        TrainerData *data=&opponents.trainer[1].data;const uint8_t *bytes;size_t length;
        GF_ASSERT(data->trainerClass<NELEMS(sTrainerGenders)&&data->npoke>=1&&data->npoke<=6&&data->trainerType<=TRTYPE_MON_ITEM_MOVES&&!data->doubleBattle);
        const size_t strides[]={sizeof(TRPOKE_NOITEM_DFLTMOVES),sizeof(TRPOKE_NOITEM_CUSTMOVES),sizeof(TRPOKE_ITEM_DFLTMOVES),sizeof(TRPOKE_ITEM_CUSTMOVES)};
        GF_ASSERT(native_resource_member(NARC_poketool_trainer_trpoke,0,trainer,&bytes,&length));
        size_t payload=strides[data->trainerType]*data->npoke;
        /* The original assembly template aligns each trainer member to four
         * bytes; odd parties of 10/18-byte records carry two zero pad bytes. */
        GF_ASSERT(length==payload||length==((payload+3)&~(size_t)3));
        for(size_t pad=payload;pad<length;pad++)GF_ASSERT(!bytes[pad]);
        /* The first rival uses an ordinary short name. Longer packed names
         * require the original compressed-string boundary before admission. */
        const unsigned char *name=(const unsigned char *)trainer_names[trainer];unsigned n=0;
        while(*name){GF_ASSERT(n<PLAYER_NAME_LENGTH&&*name<128);u16 glyph=0;for(unsigned j=0;j<NELEMS(glyph_map);j++)if(glyph_map[j].unicode==*name){glyph=glyph_map[j].glyph;break;}GF_ASSERT(glyph);opponents.trainer[1].name[n++]=glyph;name++;}
        opponents.trainer[1].name[n]=0xffff;
        CreateNPCTrainerParty(&opponents,1,HEAP_ID_BATTLE);system->trainers[1]=opponents.trainer[1];system->trainerId[1]=trainer;
    }else if(setup->opponent){
        battle->parties[1]=*setup->opponent;Pokemon *mon=Party_GetMonByIndex(battle->parties+1,0);GF_ASSERT(GetMonData(mon,MON_DATA_HP,NULL)>0&&!GetMonData(mon,MON_DATA_IS_EGG,NULL));
    }else{
        Pokemon *mon=AllocMonZeroed(HEAP_ID_BATTLE);CreateMon(mon,species,level,0,FALSE,0,0,0);Party_AddMon(battle->parties+1,mon);Heap_Free(mon);
    }
    if(setup->field){
        GF_ASSERT(setup->field->location&&setup->field->playerAvatar&&setup->field->saveData);
        unsigned map=setup->field->location->mapId;
        system->location=MapHeader_GetMapSec(map);
        system->terrain=FieldSystem_GetTerrainFromStandingTile(setup->field,MapHeader_GetBattleBg(map));
        system->metBill=CheckMetBill(SaveArray_Get(setup->field->saveData,SAVE_FLAGS));
        if(!trainer)GameStats_Inc(&battle->stats,GAME_STAT_WILD_ENCOUNTERS);
    }
    if(setup->contest){
        battle->contest=setup->contest;battle->initial_contest=*setup->contest;battle->initial_kept=*setup->contest->mon;BattleSetup *contest=BattleSetup_New_BugContest(HEAP_ID_BATTLE,setup->contest->sport_balls,setup->contest->mon);
        system->battleType=contest->battleType;system->safariBallCnt=contest->safariBalls;system->bugContestCaughtMon=&battle->contest_catch;Heap_Free(contest);
    }
    system->ctx=BattleContext_New(system);battle_leave(battle,host_rng);return battle;
}
void hg_battle_free(HGBattle *battle){
    if(!battle)return;HGBattle *previous=running_battle;running_battle=battle;
    if(battle->system.ctx)BattleContext_Delete(battle->system.ctx);
    for(unsigned i=0;i<NELEMS(battle->allocations);i++)free(battle->allocations[i]);
    free(battle->task_arena);free(battle->ai_ram);running_battle=previous;free(battle);
}
int hg_battle_tick(HGBattle *battle) {
    if(!battle||battle->error[0])return HG_BATTLE_FAILED;if(battle->done)return HG_BATTLE_FINISHED;
    u32 host_rng=GetLCRNGSeed();SetLCRNGSeed(battle->global_rng);battle->caller_keys=gSystem.newKeys;gSystem.newKeys=0;running_battle=battle;battle->caller_mon_services=hg_pokemon_services_exchange(hg_mon_services(&battle->mon_date));if(setjmp(battle->trap)){battle_leave(battle,host_rng);return HG_BATTLE_FAILED;}
    if(++battle->frames>20000)battle_boundary_failed("frame budget");
    if(battle->waiting){battle_leave(battle,host_rng);return HG_BATTLE_MESSAGE;}
    if(battle->menu){int status=native_menu_state(battle);battle_leave(battle,host_rng);return status;}
    SysTaskQueue_RunTasks(battle->tasks);
    if(battle->waiting){battle_leave(battle,host_rng);return HG_BATTLE_MESSAGE;}
    if(battle->count){process_event(battle);battle_leave(battle,host_rng);return battle->waiting?HG_BATTLE_MESSAGE:battle->menu?native_menu_state(battle):HG_BATTLE_RUNNING;}
    battle->budget=10000;battle->done=BattleContext_Main(&battle->system,battle->system.ctx);battle_leave(battle,host_rng);return battle->done?HG_BATTLE_FINISHED:HG_BATTLE_RUNNING;
}
int hg_battle_input(HGBattle *battle,unsigned choice) {
    if(!battle||battle->error[0]||(!battle->waiting&&!battle->menu))return 0;
    u32 host_rng=GetLCRNGSeed();SetLCRNGSeed(battle->global_rng);battle->caller_keys=gSystem.newKeys;gSystem.newKeys=0;running_battle=battle;battle->caller_mon_services=hg_pokemon_services_exchange(hg_mon_services(&battle->mon_date));if(setjmp(battle->trap)){battle_leave(battle,host_rng);return 0;}
    if((battle->wait_fanfare&&IsFanfarePlaying())||(battle->wait_effect&&battle->audio&&ds_audio_playing(battle->audio,DS_AUDIO_EFFECT,0))){battle_leave(battle,host_rng);return 0;}
    battle->wait_fanfare=battle->wait_effect=0;
    if(battle->menu==1&&battle->contest&&choice!=1&&choice!=2&&choice!=4){battle_leave(battle,host_rng);return 0;}
    if(battle->menu>=4){int accepted=native_menu_input(battle,choice);battle_leave(battle,host_rng);return accepted;}
    if(battle->menu==1&&choice==5){battle->direct_ball=0;choice=2;}
    else if(battle->menu==1&&choice==2)battle->direct_ball=1;
    if(battle->menu){if((battle->menu==1&&choice!=1&&choice!=3&&choice!=4&&!(choice==2&&(!battle->direct_ball||hg_battle_can_throw(battle))))||(battle->menu==3&&choice!=255)||(battle->menu==2&&(choice<1||choice>4))){battle_leave(battle,host_rng);return 0;}if(battle->menu==2&&(StruggleCheck(&battle->system,battle->system.ctx,0,0,-1)&(1u<<(choice-1)))){battle_leave(battle,host_rng);return 0;}battle->system.ctx->battleBuffer[0][0]=choice;battle->menu=0;}
    battle->input_pending=1;battle->waiting=0;battle->text[0]=0;if(battle->direct_message)battle->direct_message=0;else finish_event(battle);battle_leave(battle,host_rng);return 1;
}
const char *hg_battle_message(const HGBattle *b){return b?b->text:"";}
const char *hg_battle_error(const HGBattle *b){return b?b->error:"Cannot start battle";}
unsigned hg_battle_result(const HGBattle *b){return b?b->system.battleOutcomeFlag:0;}
const Party *hg_battle_party(const HGBattle *b){return b?b->parties:NULL;}
int hg_battle_mon(const HGBattle *b,unsigned side,HGBattleMonView *out){if(!b||!out||side>1||!b->system.ctx)return 0;const BattleMon *m=b->system.ctx->battleMons+side;*out=(HGBattleMonView){.species=m->species,.level=m->level,.hp=m->hp>0?(unsigned)m->hp:0,.max_hp=m->maxHp,.status=m->status,.gender=m->gender,.form=m->form,.shiny=m->shiny};for(unsigned i=0;i<4;i++){out->moves[i]=m->moves[i];out->pp[i]=m->movePPCur[i];}return 1;}

HGBattle *hg_battle_new_setup(const HGBattleSetup *setup,char *error,size_t size){
    if(!setup||!setup->party||!setup->profile||!setup->bag||!setup->options||!setup->pokedex||!setup->stats){if(error&&size)snprintf(error,size,"Missing session battle state");return NULL;}
    return battle_new(setup,error,size);
}
HGBattle *hg_battle_new(const Party *party,unsigned species,unsigned level,int trainer,DSAudio *audio,char *error,size_t size){
    if(trainer){if(error&&size)snprintf(error,size,"Trainer battles require an original trainer ID and session setup");return NULL;}
    HGBattleSetup setup={.party=party,.enemy_species=species,.enemy_level=level,.seed=1,.audio=audio};return battle_new(&setup,error,size);
}
const Party *hg_battle_opponent(const HGBattle *battle){return battle?battle->parties+1:NULL;}

int hg_battle_commit_storage(HGBattle *battle,Party *party,PlayerProfile *profile,Bag *bag,Pokedex *dex,GameStats *stats,PCStorage *storage){
    if(!battle||!battle->done||battle->error[0]||battle->published||battle->count||battle->waiting||battle->menu||!battle->tasks||battle->tasks->activeCount||!party||!profile||!bag||!dex||!stats)return 0;
    if(battle->has_storage&&(!storage||memcmp(storage,&battle->initial_storage,sizeof(*storage))))return 0;
    if(memcmp(party,&battle->initial_party,sizeof(*party))||memcmp(profile,&battle->initial_profile,sizeof(*profile))||memcmp(bag,&battle->initial_bag,sizeof(*bag))||memcmp(dex,&battle->initial_dex,sizeof(*dex))||memcmp(stats,&battle->initial_stats,sizeof(*stats)))return 0;
    if(battle->system.battleType&BATTLE_TYPE_TUTORIAL){battle->published=1;return 1;}
    if(battle->contest&&(memcmp(battle->contest,&battle->initial_contest,sizeof(*battle->contest))||memcmp(battle->contest->mon,&battle->initial_kept,sizeof(Pokemon))))return 0;
    if(battle->contest)battle->contest->sport_balls=(u16)battle->system.safariBallCnt;
    if(battle->has_storage)*storage=battle->storage;
    *party=battle->parties[0];*profile=battle->profiles[0];*bag=battle->bag;*dex=battle->dex;*stats=battle->stats;battle->published=1;return 1;
}

#include "map_header.h"
#include "metatile_behavior.h"
int hg_encounter_step(HGEncounterState *state,FieldSystem *field,unsigned map,unsigned behavior,unsigned time_of_day,Party *opponent,char *error,size_t size){
    if(error&&size)error[0]=0;
    if(!state||!field||!field->playerAvatar||!field->saveData||!opponent||time_of_day>=RTC_TIMEOFDAY_COUNT)return -1;
    if(state->inhibit_steps<65535)state->inhibit_steps++;
    if(state->inhibit_steps<=3||!MetatileBehavior_CanGenerateWalkingEncounters(behavior)||MetatileBehavior_IsSurfableWater(behavior))return 0;
    unsigned bank=MapHeader_GetWildEncounterBank(map);if(bank==255)return 0;
    const uint8_t *bytes;size_t length;
    if(!native_resource_member(NARC_fielddata_encountdata_g_enc_data,0,bank,&bytes,&length)||length!=sizeof(EncounterData)){if(error&&size)snprintf(error,size,"Missing original encounter bank %u",bank);return -1;}
    EncounterData data;memcpy(&data,bytes,sizeof(data));if(!data.encounterRate_walking)return 0;
    Party *party=hg_story_save_party(field->saveData);if(!Party_GetCount(party))return 0;
    Pokemon *lead=Party_GetMonByIndex(party,0);if(GetMonData(lead,MON_DATA_IS_EGG,NULL))return 0;
    unsigned alive=0;for(unsigned i=0;i<Party_GetCount(party);i++)alive|=GetMonData(Party_GetMonByIndex(party,i),MON_DATA_HP,NULL)>0;
    if(!alive)return 0;
    HGBattle *scratch=calloc(1,sizeof(*scratch));if(!scratch)return -1;scratch->encounter_time=time_of_day;
    HGBattle *previous=running_battle;u32 caller_rng=GetLCRNGSeed();running_battle=scratch;SetLCRNGSeed(state->rng);int result=0;
    if(setjmp(scratch->trap)){if(error&&size)snprintf(error,size,"%s",scratch->error);result=-1;}
    else {
        field->reverseTurnFrameSteps=state->reverse_steps;field->lastFacingDirection=state->last_facing;
        FieldSystem_UpdateTurnFrameCounter(field);state->reverse_steps=field->reverseTurnFrameSteps;state->last_facing=field->lastFacingDirection;
        EncounterGenState gen={.playerId=hg_story_save_profile(field->saveData)->id,.ability=GetMonData(lead,MON_DATA_ABILITY,NULL),.pokedex=hg_story_save_dex(field->saveData)};
        u8 rate=ApplyAbilityEffectToEncounterRate(FALSE,data.encounterRate_walking,&gen,MapHeader_GetWeatherType(map),lead);
        ApplyLeadMonHeldItemEffectToEncounterRate(lead,&rate);
        if(FieldSystem_EncounterRateRoll(field,rate,behavior)){
            EncounterSlot slots[NUM_ENCOUNTERS_LAND];EncSlotArray_Init_Land(&data,slots);
            BattleSetup setup={0};Party_InitWithMaxSize(opponent,6);setup.party[1]=opponent;
            if(field->bugContest){
                BugContest *contest=field->bugContest;
                if(contest->sport_balls&&contest->elapsed_time<20)result=FieldSystem_GenerateBugContestEncounter_Internal(field,lead,ROD_TYPE_NONE,&gen,ENCOUNTER_TYPE_LAND,1,&setup);
            }else result=FieldSystem_GenerateRegularEncounter(lead,ROD_TYPE_NONE,&gen,slots,ENCOUNTER_TYPE_LAND,1,&setup);
        }
    }
    state->rng=GetLCRNGSeed();SetLCRNGSeed(caller_rng);running_battle=previous;
    for(unsigned i=0;i<NELEMS(scratch->allocations);i++)free(scratch->allocations[i]);free(scratch);return result;
}

int hg_battle_move_allowed(HGBattle *battle,unsigned slot){
    if(!battle||slot>=4||battle->error[0]||battle->menu!=2)return 0;
    u32 host_rng=GetLCRNGSeed();SetLCRNGSeed(battle->global_rng);battle->caller_keys=gSystem.newKeys;gSystem.newKeys=0;running_battle=battle;battle->caller_mon_services=hg_pokemon_services_exchange(hg_mon_services(&battle->mon_date));
    if(setjmp(battle->trap)){battle_leave(battle,host_rng);return 0;}
    int allowed=!(StruggleCheck(&battle->system,battle->system.ctx,0,0,-1)&(1u<<slot));battle_leave(battle,host_rng);return allowed;
}

size_t hg_battle_ai_steps(const HGBattle *b){return b?b->ai_steps:0;}
unsigned hg_battle_ai_calls(const HGBattle *b){return b?b->ai_calls:0;}

int hg_battle_tutorial(const HGBattle *b){return b&&(b->system.battleType&BATTLE_TYPE_TUTORIAL);}
unsigned hg_battle_capture_phase(const HGBattle *b){return b?b->capture_phase:0;}

unsigned hg_battle_tutorial_action(const HGBattle *b){return b&&b->system.battleInput?b->system.battleInput->tutorial.state:0;}

int hg_battle_contest_catch(const HGBattle *b,void *out,size_t size){
    if(!b||!out||size!=sizeof(Pokemon)||!b->contest||!b->done||b->error[0]||!(b->system.battleOutcomeFlag&BATTLE_OUTCOME_MON_CAUGHT))return 0;
    memcpy(out,&b->contest_catch,sizeof(Pokemon));return 1;
}
unsigned hg_battle_sport_balls(const HGBattle *b){return b&&b->contest?(unsigned)b->system.safariBallCnt:0;}

int hg_battle_is_contest(const HGBattle *battle) { return battle&&battle->contest; }

int hg_battle_commit(HGBattle *b,Party *p,PlayerProfile *profile,Bag *bag,Pokedex *dex,GameStats *stats){return hg_battle_commit_storage(b,p,profile,bag,dex,stats,NULL);}

unsigned hg_battle_command_count(const HGBattle *b){return b?(b->contest?3u:b->system.battleType&BATTLE_TYPE_TRAINER?4u:5u):0;}
const char *hg_battle_ball_unavailable(const HGBattle *b){
    if(!b)return "No battle is active.";
    if(b->system.battleType&BATTLE_TYPE_TRAINER)return "You cannot catch a Trainer's Pokemon.";
    if(b->contest)return b->system.safariBallCnt>0?"":"No Sport Balls left.";
    if(!Bag_GetQuantity((Bag *)&b->bag,ITEM_POKE_BALL,HEAP_ID_BATTLE))return "No Poke Balls in your Bag.";
    if(!b->has_storage)return "Pokemon storage is unavailable.";
    if(Party_GetCount(b->parties)>=6&&!PCStorage_CountEmptySpotsInAllBoxes((PCStorage *)&b->storage))return "Your party and PC boxes are full.";
    return "";
}
int hg_battle_can_throw(const HGBattle *b){return !hg_battle_ball_unavailable(b)[0];}

int hg_battle_brightness(const HGBattle *b){return b?b->blackout_brightness:0;}
