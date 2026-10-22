#include "compat/battle.h"
#include "compat/misc.h"
#include "compat/naming.h"
#include "menu_input_state.h"
#include "game_name.h"
#include "naming_records.inc"
struct HGNaming {FieldSystem field;MenuInputStateMgr input;NamingScreenArgs *args;unsigned kind,species,max,party;u16 initial[20],*result;int has_initial,preview,pending,done,cancelled,phase;char text[80];};
static HGNaming *running_name;
static int name_init(OverlayManager *manager,int *state){(void)state;HGNaming *n=running_name;GF_ASSERT(n&&!n->args);n->args=OverlayManager_GetArgs(manager);n->pending=!n->cancelled;if(n->cancelled)n->args->noInput=1;else if(n->has_initial)GF_ASSERT(hg_name_decode(n->initial,20,n->text,sizeof(n->text)));return TRUE;}
static int name_main(OverlayManager *manager,int *state){(void)manager;(void)state;GF_ASSERT(running_name);return !running_name->pending;}
static int name_exit(OverlayManager *manager,int *state){(void)manager;(void)state;return TRUE;}
const OverlayManagerTemplate gOverlayTemplate_NamingScreen={name_init,name_main,name_exit,FS_OVERLAY_ID_NONE};
static void leave_field(TaskManager *task){(void)task;GF_ASSERT(running_name&&running_name->phase==0);running_name->phase=1;}
static void restore_field(TaskManager *task){(void)task;GF_ASSERT(running_name&&running_name->phase==1);running_name->phase=2;}
typedef struct SAV_FRIEND_GRP SAV_FRIEND_GRP;
static SAV_FRIEND_GRP *unsupported_group(SaveData *s){(void)s;GF_ASSERT(FALSE);return NULL;}
static u16 *unsupported_group_string(String *s){(void)s;GF_ASSERT(FALSE);return NULL;}
static BOOL unsupported_group_compare(SAV_FRIEND_GRP *g,const u16 *s){(void)g;(void)s;GF_ASSERT(FALSE);return FALSE;}
static void unsupported_group_set(SAV_FRIEND_GRP *g,int a,int b,String *s){(void)g;(void)a;(void)b;(void)s;GF_ASSERT(FALSE);}
#define Save_FriendGroup_Get unsupported_group
#define String_cstr unsupported_group_string
#define sub_0202C88C unsupported_group_compare
#define sub_0202C7F8 unsupported_group_set
#define Save_PlayerData_GetProfile hg_story_save_profile
#define Save_PlayerData_GetOptionsAddr hg_story_save_options
#define SaveArray_Party_Get hg_story_save_party
#define Save_Misc_Get hg_story_save_misc
#define menuInputState native_menu_input[0]
#define CallTask_LeaveOverworld leave_field
#define CallTask_RestoreOverworld restore_field
#include "naming_decls.inc"
#include "naming_kernel.inc"
static BOOL naming_root(TaskManager *task){
    HGNaming *n=TaskManager_GetEnvironment(task);u32 *state=TaskManager_GetStatePtr(task);
    if(!*state){*state=1;CallTask_NamingScreen(task,(NameScreenType)n->kind,(int)n->species,(int)n->max,(int)n->party,n->has_initial?n->initial:NULL,n->result);return FALSE;}
    n->done=1;n->args=NULL;return TRUE;
}
HGNaming *hg_naming_new(FieldSystem *field,unsigned kind,unsigned species,unsigned max,unsigned party,const u16 *initial,u16 *result,int preview){
    if(!field||!field->saveData||!result||(kind!=NAME_SCREEN_PLAYER&&kind!=NAME_SCREEN_RIVAL&&kind!=NAME_SCREEN_POKEMON)||!max||max>POKEMON_NAME_LENGTH||(kind==NAME_SCREEN_POKEMON&&party>=(unsigned)Party_GetCount(hg_story_save_party(field->saveData))))return NULL;
    HGNaming *n=calloc(1,sizeof(*n));if(!n)return NULL;n->field=*field;n->field.taskman=NULL;n->field.application=NULL;n->field.native_menu_input=&n->input;n->kind=kind;n->species=species;n->max=max;n->party=party;n->result=result;n->preview=preview;
    if(initial){unsigned i=0;for(;i<max&&initial[i]!=0xffff;i++)n->initial[i]=initial[i];n->initial[i]=0xffff;n->has_initial=1;}
    FieldSystem_CreateTask(&n->field,naming_root,n);return n;
}
int hg_naming_edit(HGNaming *n,const char *text){if(!n||!n->pending||!text)return 0;u16 encoded[20];if(text[0]&&!hg_name_encode(text,encoded,n->max))return 0;if(strlen(text)>=sizeof(n->text))return 0;snprintf(n->text,sizeof(n->text),"%s",text);return 1;}
int hg_naming_submit(HGNaming *n,const char *text,int cancel){
    if(!n||!n->pending||!n->args)return 0;u16 encoded[20]={0};if(!cancel&&!hg_name_encode(text,encoded,n->max))return 0;
    if(cancel)n->args->noInput=1;else{memcpy(n->args->nameInputFlat,encoded,sizeof(encoded));CopyU16ArrayToString(n->args->nameInputString,encoded);n->args->noInput=0;}
    n->pending=0;return 1;
}
int hg_naming_tick(HGNaming *n){if(!n)return -1;if(n->done)return 1;HGNaming *previous=running_name;running_name=n;hg_field_tasks_tick(&n->field);running_name=previous;
    if(n->pending&&n->preview)GF_ASSERT(hg_naming_submit(n,n->kind==NAME_SCREEN_RIVAL?"Silver":n->has_initial?n->text:"Ethan",0));return n->done;
}
void hg_naming_free(HGNaming *n){if(!n)return;n->cancelled=1;if(n->pending)hg_naming_submit(n,NULL,1);for(unsigned i=0;i<64&&!hg_field_tasks_idle(&n->field);i++)hg_naming_tick(n);GF_ASSERT(hg_field_tasks_idle(&n->field));free(n);}
int hg_naming_pending(const HGNaming *n){return n&&n->pending;}
unsigned hg_naming_kind(const HGNaming *n){return n?n->kind:~0u;}
const char *hg_naming_text(const HGNaming *n){return n?n->text:"";}
