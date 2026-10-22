/* Original tutorial party factory, using owned native allocation/save/resource
 * boundaries. Unused DS storage/chatot/application services stay absent. */
#include "field_data.h"
#include "map_header.h"
#include "metatile_behavior.h"
#include "save_local_field_data.h"
#include "game_name.h"
static BattleSetup *tutorial_allocate(enum HeapID heap,u32 type){
    GF_ASSERT(type==BATTLE_TYPE_TUTORIAL||type==BATTLE_TYPE_BUG_CONTEST);BattleSetup *s=Heap_Alloc(heap,sizeof(*s));s->battleType=type;
    if(type==BATTLE_TYPE_BUG_CONTEST){s->bugContestMon=&running_battle->contest_catch;return s;}
    for(unsigned i=0;i<4;i++){s->party[i]=running_battle->parties+i;s->profile[i]=running_battle->profiles+i;Party_InitWithMaxSize(s->party[i],6);PlayerProfile_Init(s->profile[i]);}
    s->bag=&running_battle->bag;s->pokedex=&running_battle->dex;s->options=&running_battle->options;return s;
}
static unsigned tutorial_behavior(FieldSystem *f,int x,int z){(void)x;(void)z;LocalMapObject *player=PlayerAvatar_GetMapObject(f->playerAvatar);uint16_t value;GF_ASSERT(player&&f->location&&hg_field_attribute(f->location->mapId,MapObject_GetXCoord(player),MapObject_GetZCoord(player),&value));return value&255;}
static struct LocalFieldData *tutorial_local(SaveData *s){(void)s;GF_ASSERT(running_battle->tutorial_field&&running_battle->tutorial_field->playerAvatar);if(running_battle->tutorial_field->native_local_field)return running_battle->tutorial_field->native_local_field;LocalFieldData *local=Heap_Alloc(HEAP_ID_BATTLE,Save_LocalFieldData_sizeof());Save_LocalFieldData_Init(local);*LocalFieldData_GetPlayer(local)=*PlayerAvatar_GetPlayerSaveData(running_battle->tutorial_field->playerAvatar);return local;}
static PCStorage *tutorial_storage(SaveData *s){(void)s;return NULL;} /* The demonstration never stores a caught mon. */
static unsigned tutorial_time(FieldSystem *f){(void)f;return RTC_TIMEOFDAY_DAY;}
static MsgData *tutorial_messages(int load,NarcId archive,int bank,enum HeapID heap){(void)load;(void)heap;GF_ASSERT(archive==NARC_msgdata_msg&&bank==445);return (MsgData *)(uintptr_t)445;}
static void tutorial_read_name(MsgData *data,int index,String *out){const uint8_t *text;size_t length;u16 name[8];GF_ASSERT(data==(MsgData *)(uintptr_t)445&&index>=0&&index<2&&native_resource_member(NARC_msgdata_msg,445,index,&text,&length)&&hg_name_encode((const char *)text,name,7));CopyU16ArrayToString(out,name);}
static const u16 *tutorial_cstr(String *text){return text->data;}
static void tutorial_destroy_messages(MsgData *data){GF_ASSERT(data==(MsgData *)(uintptr_t)445);}
#define BattleSetup_New tutorial_allocate
#define Save_PlayerData_GetProfile hg_story_save_profile
#define Save_PlayerData_GetOptionsAddr hg_story_save_options
#define Save_GameStats_Get hg_story_save_stats
#define Save_LocalFieldData_Get tutorial_local
#define SaveArray_PCStorage_Get tutorial_storage
#define Field_GetTimeOfDay tutorial_time
#define GetMetatileBehavior tutorial_behavior
#define NewMsgDataFromNarc tutorial_messages
#define ReadMsgDataIntoString tutorial_read_name
#define DestroyMsgData tutorial_destroy_messages
#define String_cstr tutorial_cstr
#define msg_0445 445
#include "tutorial_setup_decls.inc"
#include "tutorial_setup_kernel.inc"
#include "contest_setup_kernel.inc"
#undef BattleSetup_New
#undef Save_PlayerData_GetProfile
#undef Save_PlayerData_GetOptionsAddr
#undef Save_GameStats_Get
#undef Save_LocalFieldData_Get
#undef SaveArray_PCStorage_Get
#undef Field_GetTimeOfDay
#undef GetMetatileBehavior
#undef NewMsgDataFromNarc
#undef ReadMsgDataIntoString
#undef DestroyMsgData
#undef String_cstr

/* Native pointer animation clock; original callbacks own action selection and
 * their delays. This is a host view of only the fields those callbacks consume. */
typedef struct BattleFinger {unsigned frames;} BattleFinger;
#include "tutorial_input_record.inc"
struct BattleInput {BattleSystem *battleSystem;BattleInputTutorial tutorial;};
static void ov12_0226BCFC(BattleFinger *f,int x,int y,int z){(void)x;(void)y;(void)z;GF_ASSERT(f);}
static void ov12_0226BD4C(BattleFinger *f,int frames){GF_ASSERT(f&&frames>0);f->frames=(unsigned)frames;}
static int ov12_0226BD50(BattleFinger *f){GF_ASSERT(f);return !f->frames||!--f->frames;}
#define TOUCH_MENU_NO_INPUT -1
#include "tutorial_input_kernel.inc"
static int tutorial_select(HGBattle *b){return BattleInput_CatchingTutorialMain(b->system.battleInput);}
