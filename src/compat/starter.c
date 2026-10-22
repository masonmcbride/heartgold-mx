#include "compat/motion.h"
#include "compat/pokemon.h"
#include "compat/starter.h"
#include "compat/pokemon_services.h"
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
    HGMotionFade local_fade,*previous_fade;int preview,pending,done,overlay_ready;
};
static HGStarter *owner(FieldSystem *field) { GF_ASSERT(field&&field->native_starter);return field->native_starter; }
static HGStarter *running;
static void scoped_fade(int screens,int type,int both,int color,int duration,int sync,enum HeapID heap) {
    GF_ASSERT(running&&screens==FADE_BOTH_SCREENS&&type==both&&(type==0||type==1)&&color==0&&duration>0&&duration<=60&&sync==1&&heap==HEAP_ID_FIELD1);
    GF_ASSERT(hg_motion_fade_begin(running->field->native_fade,(unsigned)type,(unsigned)color,(unsigned)duration,(unsigned)sync));
}
static BOOL fade_finished(void) { GF_ASSERT(running);return !running->field->native_fade->active; }
/* The original task waits on the same real OverlayManager lifecycle used by
 * every admitted field application. The host callbacks present the choice;
 * original CreateStarter retains ownership of its arguments and acquisition. */
static int choice_init(OverlayManager *manager,int *state) { (void)state;HGStarter *s=running;GF_ASSERT(s&&OverlayManager_GetArgs(manager)==s->args);return TRUE; }
static int choice_main(OverlayManager *manager,int *state) { (void)state;HGStarter *s=running;GF_ASSERT(s&&OverlayManager_GetArgs(manager)==s->args);return !s->pending; }
static int choice_exit(OverlayManager *manager,int *state) { (void)manager;(void)state;return TRUE; }
static void launch_native_choice(FieldSystem *field,const OverlayManagerTemplate *template,void *args) {
    HGStarter *s=owner(field);GF_ASSERT(!s->pending&&!field->application);s->args=args;s->pending=1;
    FieldSystem_LaunchApplication(field,template,args);
}
static void scene_available(FieldSystem *field) { owner(field)->overlay_ready=1; }
static BOOL overlay_ready(FieldSystem *field) { return owner(field)->overlay_ready; }
static void child_scene_started(void) { GF_ASSERT(running&&running->pending); }
typedef struct POKEWALKER POKEWALKER;
static POKEWALKER *unsupported_walker(SaveData *save) { (void)save;abort(); }
static void unsupported_course(POKEWALKER *walker,int course) { (void)walker;(void)course;abort(); }
#define BeginNormalPaletteFade scoped_fade
#define IsPaletteFadeFinished fade_finished
/* The original launch descriptor is statically linked to native presentation
 * callbacks; DS overlay relocation and original NNS UI do not run. */
#define ChooseStarter_Init choice_init
#define ChooseStarter_Main choice_main
#define ChooseStarter_Exit choice_exit
#define FS_OVERLAY_ID(name) FS_OVERLAY_ID_NONE
#define FieldSystem_LaunchApplication launch_native_choice
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
    if(!field||field->taskman||field->application||field->native_starter||!field->saveData||!field->location||hg_story_save_party(field->saveData)->core.curCount)return NULL;
    HGStarter *s=calloc(1,sizeof(*s));if(!s)return NULL;s->field=field;s->preview=preview;
    time_t now=time(NULL);struct tm date;if(!localtime_r(&now,&date)){free(s);return NULL;}
    s->date=(RTCDate){(u32)(date.tm_year-100),(u32)(date.tm_mon+1),(u32)date.tm_mday,(u32)date.tm_wday};
    s->previous_fade=field->native_fade;if(!field->native_fade)field->native_fade=&s->local_fade;
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
    HGPokemonServices previous=hg_pokemon_services_exchange(hg_mon_services(&s->date));
    GF_ASSERT(hg_motion_fade_tick(s->field->native_fade));
    if(s->preview&&s->pending)hg_starter_choose(s,0);
    hg_field_tasks_tick(s->field);
    hg_pokemon_services_exchange(previous);running=NULL;return 1;
}
void hg_starter_free(HGStarter *s) {
    if(!s)return;u32 seed=GetLCRNGSeed();
    /* All children are this adapter's original starter task. Finish it against
     * the private candidate before discarding, retaining original cleanup. */
    s->preview=1;for(unsigned i=0;i<128&&!s->done;i++)hg_starter_tick(s);
    GF_ASSERT(s->done&&hg_field_tasks_idle(s->field));SetLCRNGSeed(seed);s->field->native_starter=NULL;s->field->native_fade=s->previous_fade;free(s);
}
