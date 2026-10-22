#include "ds_audio.h"
#include "compat/pokemon.h"
#include "compat/phone.h"
#include "compat/save_c_records.h"
#include "sys_vars.h"
#include "constants/phone_scripts.h"
#include "compat/resources.h"
#include "compat/story_records.h"
#include "gear_phone.h"
#include "map_header.h"
#include "sys_flags.h"
#include "save_pokegear.h"
SaveVarsFlags *hg_story_save_vars(SaveData *);
#include "constants/phone_contacts.h"
#include "constants/phone_scripts.h"
#include "constants/trainer_class.h"
#include "constants/trainers.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/sndseq.h"
typedef int MenuInputState;
typedef struct PokegearPhoneAppData PokegearPhoneAppData;
typedef struct BgConfig BgConfig;
typedef struct TouchscreenListMenu TouchscreenListMenu;
typedef struct PaletteData PaletteData;
typedef struct Sprite Sprite;
typedef struct Daycare Daycare;
typedef struct MessageFormat MessageFormat;
#include "phone_types.inc"
#include "phone_catalog.inc"
#include "phone_decls.inc"
BOOL CheckGameClearFlag(SaveVarsFlags *);
BOOL Save_VarsFlags_IsInRocketTakeover(SaveVarsFlags *);
struct HGPhone { PokegearPhoneCallContext context;unsigned pending_bank,pending_message;FieldSystem *field;HGPhoneServices services;MenuInputState input;int configured,cancelled,failed,launched,menu_open;u32 menu_result; };
_Static_assert(offsetof(HGPhone,context)==0,"Original phone context must remain the first owner member");
static HGPhone *owner(PokegearPhoneCallContext *ctx) { return (HGPhone *)(void *)ctx; }
static void animate(PokegearPhoneCallContext *ctx) { (void)ctx; /* Native button presentation has no DS sprite controller. */ }
static void init_message(PokegearPhoneCallContext *ctx) { ctx->msgData_PhoneContact=(MsgData *)(uintptr_t)(sPhoneMessageGmm[ctx->state.callerID]+1);
    HGPhone *p=owner(ctx);if(p->services.buffer_message&&!p->services.buffer_message(p->services.owner,1,sPhoneMessageGmm[ctx->state.callerID],0))p->failed=1;
}
static void print(PokegearPhoneCallContext *ctx,MsgData *bank,const u8 *ids) {
    HGPhone *p=owner(ctx);if(!p->services.message(p->services.owner,(unsigned)((uintptr_t)bank-1),ids[ctx->playerGender]))p->failed=1;
}
static BOOL printing_done(PokegearPhoneCallContext *ctx) { HGPhone *p=owner(ctx);return !p->services.waiting(p->services.owner); }
static BOOL wait_button(PokegearPhoneCallContext *ctx) { HGPhone *p=owner(ctx);return p->services.acknowledged(p->services.owner); }
static void signal_message(PokegearPhoneCallContext *ctx,u8 kind,u8 stage) { (void)ctx;(void)kind;(void)stage; /* Native call view retains contact/status instead of DS window tone glyphs. */ }
static void destroy_message(MsgData *bank) { GF_ASSERT((uintptr_t)bank>1); /* Borrowed cached native message-bank handle. */ }
static void gendered(PokegearPhoneCallContext *ctx,MsgData *bank,u8 a,u8 b) { const u8 ids[]={a,b};print(ctx,bank,ids); }
static void ungendered(PokegearPhoneCallContext *ctx,MsgData *bank,u8 id) { gendered(ctx,bank,id,id); }
static HGPhone *running_phone;
static void native_se(unsigned id) { GF_ASSERT(running_phone);if(!ds_audio_play(running_phone->field->native_audio,DS_AUDIO_EFFECT,id))running_phone->failed=1; }
static BOOL native_playing(unsigned id) { GF_ASSERT(running_phone);return ds_audio_playing(running_phone->field->native_audio,DS_AUDIO_EFFECT,id); }
static void buffer_area(MessageFormat *format,u32 slot,u32 area) {
    HGPhone *p=(HGPhone *)(void *)format;
    if(!p->services.buffer_safari_area||!p->services.buffer_safari_area(p->services.owner,slot,area))p->failed=1;
}
static void read_word(MsgData *bank,u32 id,void *dest) {
    HGPhone *p=(HGPhone *)dest;p->pending_bank=(unsigned)((uintptr_t)bank-1);p->pending_message=id;
}
static void buffer_word(MessageFormat *format,u32 slot,void *value,int a,int b,int c) {
    (void)a;(void)b;(void)c;HGPhone *p=(HGPhone *)(void *)format;GF_ASSERT(p==(HGPhone *)value);
    if(!p->services.buffer_message||!p->services.buffer_message(p->services.owner,slot,p->pending_bank,p->pending_message))p->failed=1;
}
/* Full original selector retains outgoing branches; admission never permits
 * these unported daycare menu/breeding services to execute. */
static DaycareMon *unsupported_daycare_mon(Daycare *d,int i) { (void)d;(void)i;GF_ASSERT(FALSE);return NULL; }
static BoxPokemon *unsupported_daycare_box(DaycareMon *d) { (void)d;GF_ASSERT(FALSE);return NULL; }
#define Save_Daycare_GetMonX unsupported_daycare_mon
#define DaycareMon_GetBoxMon unsupported_daycare_box
static void BufferBoxMonNickname(MessageFormat *f,unsigned s,BoxPokemon *b) { (void)f;(void)s;(void)b;GF_ASSERT(FALSE); }
static int Save_Daycare_CalcCompatibility(Daycare *d) { (void)d;GF_ASSERT(FALSE);return 0; }
#define BufferSafariZoneAreaName buffer_area
#define ReadMsgDataIntoString read_word
#define BufferString buffer_word
#define PhoneCallMessagePrint_Gendered gendered
#define Save_Pokedex_Get hg_story_save_dex
#define ASSERT_POKEDEX(x) GF_ASSERT((x)&&((x)->magic==0xBEEFCAFE))
#define PhoneCall_AnimateFastForwardButtonOnTouch animate
#define PhoneCall_InitMsgDataAndBufferNames init_message
#define PhoneCallMessagePrint print
#define PhoneCallMessagePrint_Ungendered ungendered
#define PhoneCall_IsMessageDonePrinting printing_done
#define PhoneCall_WaitButtonBeforeHangup wait_button
#define PhoneCall_NoSignalMessage signal_message
#define DestroyMsgData destroy_message
#define PlaySE native_se
#define IsSEPlaying native_playing
static void phone_menu_create(PokegearPhoneCallContext *ctx,unsigned kind) {
    HGPhone *p=owner(ctx);GF_ASSERT(kind==2&&p->services.menu_begin);p->menu_open=1;p->menu_result=1;p->services.menu_begin(p->services.owner,&p->menu_result);
}
static int phone_menu_input(PokegearPhoneCallContext *ctx) {
    HGPhone *p=owner(ctx);return p->services.menu_waiting(p->services.owner)?-1:(int)p->menu_result;
}
static void phone_menu_destroy(PokegearPhoneCallContext *ctx) { owner(ctx)->menu_open=0; }
static void phone_integer(MessageFormat *format,unsigned slot,unsigned value,unsigned digits,unsigned mode,BOOL spaces) {
    (void)digits;(void)mode;(void)spaces;HGPhone *p=(HGPhone *)(void *)format;
    if(!p->services.buffer_integer||!p->services.buffer_integer(p->services.owner,slot,value))p->failed=1;
}
#define PRINTING_MODE_LEFT_ALIGN 0
#define BufferIntegerAsString phone_integer
#define PhoneCall_TouchscreenListMenu_Create phone_menu_create
#define PhoneCall_TouchscreenListMenu_HandleInput phone_menu_input
#define PhoneCall_TouchscreenListMenu_Destroy phone_menu_destroy
#include "phone_kernel.inc"
_Static_assert(sizeof(PhoneBookEntry)==20&&sizeof(PhoneCallScriptDef)==6,"Original phone record widths changed");
const PhoneBookEntry *hg_phone_entries(unsigned *count) { if(count)*count=NELEMS(phone_entries);return phone_entries; }
int hg_phone_admit(unsigned caller,unsigned kind,unsigned script) {
    if(caller>=NELEMS(phone_entries)||phone_entries[caller].id!=caller)return 0;
    if(kind==2)return (caller==PHONE_CONTACT_PROF__ELM&&script<NELEMS(sPhoneCallData_ProfElm_MapScripts))||(caller==PHONE_CONTACT_MOTHER&&script==0);
    if(kind!=3||script>=NELEMS(gPhoneCallScriptDef))return 0;
    switch(caller) {
    case PHONE_CONTACT_PROF__ELM:case PHONE_CONTACT_BIKE_SHOP:case PHONE_CONTACT_BILL:
    case PHONE_CONTACT_PROF__OAK:case PHONE_CONTACT_DAY_C_MAN:case PHONE_CONTACT_MOTHER:
        return 1;
    case PHONE_CONTACT_BAOBA:return script==0||(script>=142&&script<=146);
    default:return 0;
    }
}
HGPhone *hg_phone_new(FieldSystem *field,HGPhoneServices services) {
    if(!field||!field->saveData||!services.message||!services.waiting||!services.acknowledged)return NULL;
    HGPhone *p=calloc(1,sizeof(*p));if(p){p->field=field;p->services=services;}return p;
}
void hg_phone_set(HGPhone *p,unsigned caller,unsigned kind,unsigned script) {
    GF_ASSERT(p&&hg_phone_admit(caller,kind,script)&&!p->configured);p->configured=1;
    if(caller==PHONE_CONTACT_MOTHER&&kind==2&&(!p->services.menu_begin||!p->services.menu_waiting||!p->services.buffer_integer)){p->failed=1;return;}
    PokegearPhoneCallContext *ctx=&p->context;ctx->saveData=p->field->saveData;ctx->playerProfile=hg_story_save_profile(ctx->saveData);ctx->saveVarsFlags=hg_story_save_vars(ctx->saveData);
    ctx->playerGender=PlayerProfile_GetTrainerGender(ctx->playerProfile);ctx->menuInputStatePtr=&p->input;
    ctx->callPersistentState=SaveData_GetPhoneCallPersistentState(ctx->saveData);ctx->heapID=HEAP_ID_FIELD2;
    ctx->msgData_0640=(MsgData *)(uintptr_t)641;ctx->msgFormat=(MessageFormat *)(void *)p;ctx->msgExpansionBuff=(void *)p;
    GF_RTC_CopyDateTime(&ctx->state.date,&ctx->state.time);
    unsigned hour=ctx->state.time.hour;ctx->state.timeOfDay=hour>=4&&hour<10?0:hour>=10&&hour<20?1:2;
    ctx->playerMapSec=0xFFFF;
    ctx->state.callerID=caller;ctx->state.isIncomingCall=1;ctx->state.isScriptedCall=kind;ctx->state.predefinedScriptID=script;ctx->state.phoneBookEntry=(PhoneBookEntry *)(void *)(phone_entries+caller);
    ctx->state.mainState=1;
    switch(caller) {
    case PHONE_CONTACT_PROF__ELM:ctx->state.scriptID=PhoneCall_GetScriptId_ProfElm(ctx,&ctx->state);break;
    case PHONE_CONTACT_PROF__OAK:ctx->state.scriptID=PhoneCall_GetScriptId_ProfOak(ctx,&ctx->state);break;
    case PHONE_CONTACT_BIKE_SHOP:ctx->state.scriptID=PhoneCall_GetScriptId_BikeShop(ctx,&ctx->state);break;
    case PHONE_CONTACT_BILL:ctx->state.scriptID=PhoneCall_GetScriptId_Bill(ctx,&ctx->state);break;
    case PHONE_CONTACT_DAY_C_MAN:ctx->state.scriptID=PhoneCall_GetScriptId_DayCareMan(ctx,&ctx->state);break;
    case PHONE_CONTACT_BAOBA:ctx->state.scriptID=PhoneCall_GetScriptId_Baoba(ctx,&ctx->state);break;
    case PHONE_CONTACT_MOTHER:ctx->state.scriptID=PhoneCall_GetScriptId_Mother(ctx,&ctx->state);break;
    default:p->failed=1;return;
    }
    if(ctx->state.scriptID>=NELEMS(gPhoneCallScriptDef)||ctx->state.scriptType>=NELEMS(sPhoneCallHandlers)||!sPhoneCallHandlers[ctx->state.scriptType]){p->failed=1;return;}
    ctx->state.scriptDef=gPhoneCallScriptDef+ctx->state.scriptID;
}
int hg_phone_set_incoming(HGPhone *p,unsigned caller,unsigned kind,unsigned script,unsigned trigger) {
    if(!p||p->configured||kind!=3||trigger>=13||(caller==PHONE_CONTACT_BAOBA&&!script&&!p->services.buffer_safari_area)||!hg_phone_admit(caller,kind,script))return 0;
    hg_phone_set(p,caller,kind,script);
    if(p->failed)return 0;
    PhoneCallPersistentState_ClearCallTriggerFlag(p->context.callPersistentState,trigger);return 1;
}
static int app_init(OverlayManager *manager,int *state) { (void)state;HGPhone *p=OverlayManager_GetArgs(manager);return p->configured; }
static int app_main(OverlayManager *manager,int *state) { (void)state;HGPhone *p=OverlayManager_GetArgs(manager);running_phone=p;int done=p->cancelled||p->failed||PhoneCall_Main(&p->context);running_phone=NULL;return done; }
static int app_exit(OverlayManager *manager,int *state) { (void)state;HGPhone *p=OverlayManager_GetArgs(manager);p->field->unkD2_7=FALSE;return TRUE; }
void *hg_phone_launch(HGPhone *p) {
    GF_ASSERT(hg_phone_ready(p));p->launched=1;void *args=Heap_Alloc(HEAP_ID_FIELD2,4);
    static const OverlayManagerTemplate template={app_init,app_main,app_exit,FS_OVERLAY_ID_NONE};FieldSystem_LaunchApplication(p->field,&template,p);return args;
}
void hg_phone_free(HGPhone *p) {
    if(!p)return;p->cancelled=1;
    if(p->field->application&&OverlayManager_GetArgs(p->field->application)==p)for(unsigned i=0;i<8&&p->field->application;i++)hg_field_tasks_tick(p->field);
    GF_ASSERT(!p->field->application);free(p);
}
unsigned hg_phone_bank(const HGPhone *p) { return p&&p->configured?sPhoneMessageGmm[p->context.state.callerID]:0; }

int hg_phone_failed(const HGPhone *p) { return p&&p->failed; }
const char *hg_phone_contact(const HGPhone *p) {
    const uint8_t *text;size_t size;
    if(!p||!native_resource_member(NARC_msgdata_msg,hg_phone_bank(p),0,&text,&size)||!size||text[size-1]||size>128)return "Phone";
    return (const char *)text;
}

int hg_phone_ready(const HGPhone *p) { return p&&p->configured&&!p->failed&&!p->launched&&!p->field->application; }

int hg_phone_menu_active(const HGPhone *p) { return p&&p->menu_open; }
