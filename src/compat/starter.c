#include "compat/motion.h"
#include "compat/pokemon.h"
#include "compat/starter.h"
#include "motion_guest.h"
#include "map_header.h"
#include "choose_starter_app.h"
#include "constants/pokewalker_courses.h"
#include "game_stats.h"
typedef int PMLCDTarget;
#include "screen_fade.h"
#define RGB_BLACK 0
#include <time.h>
struct HGStarter {
    FieldSystem *field;ChooseStarterArgs *args;RTCDate date;
    unsigned fade,fade_total;int fade_type,preview,pending,done,overlay_ready;
};
static HGStarter *owner(FieldSystem *field) { GF_ASSERT(field&&field->native_starter);return field->native_starter; }
static HGStarter *running;
static void scoped_fade(int screens,int type,int both,int color,int duration,int sync,enum HeapID heap) {
    GF_ASSERT(running&&screens==FADE_BOTH_SCREENS&&type==both&&(type==0||type==1)&&color==0&&duration>0&&duration<=60&&sync==1&&heap==HEAP_ID_FIELD1);
    running->fade=(unsigned)duration;running->fade_total=(unsigned)duration;running->fade_type=type;
}
static BOOL fade_finished(void) { GF_ASSERT(running);return !running->fade; }
static void launch_choice(FieldSystem *field,ChooseStarterArgs *args) { HGStarter *s=owner(field);GF_ASSERT(!s->pending);s->args=args;s->pending=1; }
static BOOL app_running(FieldSystem *field) { return owner(field)->pending; }
static void scene_available(FieldSystem *field) { owner(field)->overlay_ready=1; }
static BOOL overlay_ready(FieldSystem *field) { return owner(field)->overlay_ready; }
static void child_scene_started(void) { GF_ASSERT(running&&running->pending); }
static BOOL compatible(void *opaque,unsigned location) { (void)opaque;int result;GF_ASSERT(hg_motion_location_compatible(location,&result));return result; }
static int map_location(void *opaque,unsigned type,unsigned index) { (void)opaque;unsigned result;GF_ASSERT(hg_motion_map_location(type,index,&result));return (int)result; }
static void copy_date(void *opaque,HGMonDate *out) { *out=((HGStarter *)opaque)->date; }
typedef struct POKEWALKER POKEWALKER;
static POKEWALKER *unsupported_walker(SaveData *save) { (void)save;abort(); }
static void unsupported_course(POKEWALKER *walker,int course) { (void)walker;(void)course;abort(); }
#define BeginNormalPaletteFade scoped_fade
#define IsPaletteFadeFinished fade_finished
#define ChooseStarter_LaunchApp launch_choice
#define FieldSystem_ApplicationIsRunning app_running
#define FieldSystem_LoadFieldOverlay scene_available
#define sub_020505C8 overlay_ready
#define sub_0203E30C child_scene_started
#define Save_PlayerData_GetOptionsAddr hg_story_save_options
#define Save_PlayerData_GetProfile hg_story_save_profile
#define SaveArray_Party_Get hg_story_save_party
#define Save_Pokedex_Get hg_story_save_dex
#define Save_GameStats_Get hg_story_save_stats
#define Save_Pokewalker_Get unsupported_walker
#define Pokewalker_UnlockCourse unsupported_course
#include "starter_decls.inc"
#include "starter_kernel.inc"
static BOOL root_task(TaskManager *task) {
    HGStarter *s=TaskManager_GetEnvironment(task);s->done=1;s->args=NULL;return TRUE;
}
HGStarter *hg_starter_begin(FieldSystem *field,int preview) {
    if(!field||field->taskman||field->native_starter||!field->saveData||!field->location||hg_story_save_party(field->saveData)->core.curCount)return NULL;
    HGStarter *s=calloc(1,sizeof(*s));if(!s)return NULL;s->field=field;s->preview=preview;
    time_t now=time(NULL);struct tm date;if(!localtime_r(&now,&date)){free(s);return NULL;}
    s->date=(RTCDate){(u32)(date.tm_year-100),(u32)(date.tm_mon+1),(u32)date.tm_mday,(u32)date.tm_wday};
    field->native_starter=s;FieldSystem_CreateTask(field,root_task,s);return s;
}
int hg_starter_pending(const HGStarter *s) { return s&&s->pending; }
int hg_starter_busy(const HGStarter *s) { return s&&!s->done; }
int hg_starter_option(HGStarter *s,unsigned index,HGStarterOption *out) {
    if(!s||!s->pending||!s->args||index>=3||!out)return 0;Pokemon *mon=s->args->starters+index;
    unsigned species=GetMonData(mon,MON_DATA_SPECIES,NULL);const char *name=hg_pokemon_species_name(species);if(!name)return 0;
    *out=(HGStarterOption){species,GetMonData(mon,MON_DATA_LEVEL,NULL),GetMonData(mon,MON_DATA_HP,NULL),name};return 1;
}
int hg_starter_choose(HGStarter *s,unsigned index) {
    if(!s||!s->pending||!s->args||index>=3)return 0;s->args->cursorPos=(int)index;s->pending=0;return 1;
}
int hg_starter_tick(HGStarter *s) {
    if(!s)return 0;if(s->done)return 1;GF_ASSERT(!running);running=s;
    HGPokemonServices previous=hg_pokemon_services_exchange((HGPokemonServices){s,compatible,map_location,copy_date});
    if(s->fade)s->fade--;
    if(s->preview&&s->pending)hg_starter_choose(s,0);
    FieldSystem_RunTaskFrame(s->field);
    hg_pokemon_services_exchange(previous);running=NULL;return 1;
}
void hg_starter_free(HGStarter *s) {
    if(!s)return;u32 seed=GetLCRNGSeed();
    /* All children are this adapter's original starter task. Finish it against
     * the private candidate before discarding, retaining original cleanup. */
    s->preview=1;for(unsigned i=0;i<128&&!s->done;i++)hg_starter_tick(s);
    GF_ASSERT(s->done&&!s->field->taskman);SetLCRNGSeed(seed);s->field->native_starter=NULL;free(s);
}

double hg_starter_fade(const HGStarter *s) { if(!s||!s->fade_total)return 0;double remaining=(double)s->fade/s->fade_total;return s->fade_type?remaining:1-remaining; }
