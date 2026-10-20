#include "ds_audio.h"
#include "compat/pokemon.h"
#include "compat/phone.h"
#include "compat/resources.h"
#include "compat/story_records.h"
#include "gear_phone.h"
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
struct HGPhone { PokegearPhoneCallContext context;FieldSystem *field;HGPhoneServices services;MenuInputState input;int configured,cancelled,failed,launched; };
static HGPhone *owner(PokegearPhoneCallContext *ctx) { return (HGPhone *)(void *)ctx; }
static void animate(PokegearPhoneCallContext *ctx) { (void)ctx; /* Native button presentation has no DS sprite controller. */ }
static void init_message(PokegearPhoneCallContext *ctx) { ctx->msgData_PhoneContact=(MsgData *)(uintptr_t)(sPhoneMessageGmm[ctx->state.callerID]+1); }
static void print(PokegearPhoneCallContext *ctx,MsgData *bank,const u8 *ids) {
    HGPhone *p=owner(ctx);if(!p->services.message(p->services.owner,(unsigned)((uintptr_t)bank-1),ids[ctx->playerGender]))p->failed=1;
}
static BOOL printing_done(PokegearPhoneCallContext *ctx) { HGPhone *p=owner(ctx);return !p->services.waiting(p->services.owner); }
static BOOL wait_button(PokegearPhoneCallContext *ctx) { HGPhone *p=owner(ctx);return p->services.acknowledged(p->services.owner); }
static void side_effect(PokegearPhoneCallContext *ctx,const PhoneCallScriptDef *def) { (void)ctx;GF_ASSERT(def->scriptType==PHONESCRIPTTYPE_NONE); }
static BOOL greeting(PokegearPhoneCallContext *ctx) { (void)ctx;GF_ASSERT(FALSE);return FALSE; /* Admission requires the original no-greeting contact. */ }
static void signal_message(PokegearPhoneCallContext *ctx,u8 kind,u8 stage) { (void)ctx;(void)kind;(void)stage; /* Native call view retains contact/status instead of DS window tone glyphs. */ }
static void destroy_message(MsgData *bank) { GF_ASSERT((uintptr_t)bank>1); /* Borrowed cached native message-bank handle. */ }
static void gendered(PokegearPhoneCallContext *ctx,MsgData *bank,u8 a,u8 b) { const u8 ids[]={a,b};print(ctx,bank,ids); }
static void ungendered(PokegearPhoneCallContext *ctx,MsgData *bank,u8 id) { gendered(ctx,bank,id,id); }
static HGPhone *running_phone;
static void native_se(unsigned id) { GF_ASSERT(running_phone);if(!ds_audio_play(running_phone->field->native_audio,DS_AUDIO_EFFECT,id))running_phone->failed=1; }
static BOOL native_playing(unsigned id) { GF_ASSERT(running_phone);return ds_audio_playing(running_phone->field->native_audio,DS_AUDIO_EFFECT,id); }
#define PhoneCall_AnimateFastForwardButtonOnTouch animate
#define PhoneCall_InitMsgDataAndBufferNames init_message
#define PhoneCallMessagePrint print
#define PhoneCallMessagePrint_Ungendered ungendered
#define PhoneCall_IsMessageDonePrinting printing_done
#define PhoneCall_WaitButtonBeforeHangup wait_button
#define PhoneCall_ApplyGenericNPCcallSideEffect side_effect
#define PhoneCall_PrintGreeting greeting
#define PhoneCall_NoSignalMessage signal_message
#define DestroyMsgData destroy_message
#define PlaySE native_se
#define IsSEPlaying native_playing
#define msg_0271_00037 37
#include "phone_kernel.inc"
_Static_assert(sizeof(PhoneBookEntry)==20&&sizeof(PhoneCallScriptDef)==6,"Original phone record widths changed");
int hg_phone_admit(unsigned caller,unsigned kind,unsigned script) {
    if(caller!=PHONE_CONTACT_PROF__ELM||kind!=2||script>=NELEMS(sPhoneCallData_ProfElm_MapScripts)||caller>=NELEMS(phone_entries))return 0;
    unsigned id=sPhoneCallData_ProfElm_MapScripts[script][1];
    return !sPhoneCallData_ProfElm_MapScripts[script][0]&&phone_entries[caller].id==caller&&phone_entries[caller].unkC==255&&id<NELEMS(gPhoneCallScriptDef)&&!gPhoneCallScriptDef[id].scriptType;
}
HGPhone *hg_phone_new(FieldSystem *field,HGPhoneServices services) {
    if(!field||!field->saveData||!services.message||!services.waiting||!services.acknowledged)return NULL;
    HGPhone *p=calloc(1,sizeof(*p));if(p){p->field=field;p->services=services;}return p;
}
void hg_phone_set(HGPhone *p,unsigned caller,unsigned kind,unsigned script) {
    GF_ASSERT(p&&hg_phone_admit(caller,kind,script)&&!p->configured);p->configured=1;
    PokegearPhoneCallContext *ctx=&p->context;ctx->saveData=p->field->saveData;ctx->playerProfile=hg_story_save_profile(ctx->saveData);ctx->saveVarsFlags=hg_story_save_vars(ctx->saveData);
    ctx->playerGender=PlayerProfile_GetTrainerGender(ctx->playerProfile);ctx->menuInputStatePtr=&p->input;
    ctx->state.callerID=caller;ctx->state.isIncomingCall=1;ctx->state.isScriptedCall=kind;ctx->state.predefinedScriptID=script;ctx->state.phoneBookEntry=(PhoneBookEntry *)(void *)(phone_entries+caller);
    ctx->state.mainState=1;ctx->state.scriptID=PhoneCall_GetScriptId_ProfElm(ctx,&ctx->state);ctx->state.scriptDef=gPhoneCallScriptDef+ctx->state.scriptID;
    GF_ASSERT(!ctx->state.scriptType);
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

int hg_phone_ready(const HGPhone *p) { return p&&p->configured&&!p->launched&&!p->field->application; }
