#include "compat/story.h"
#include "compat/story_records.h"
#include "compat/resources.h"
#include "map_header.h"
#include "constants/maps.h"
#include "constants/sndseq.h"
#include "constants/std_script.h"
#include "constants/init_script_types.h"
#include "field_script_banks.inc"
#include "charmap.inc"

typedef struct { SaveVarsFlags vars;PlayerProfile profile;Party party;u8 temporary[NUM_TEMP_FLAGS/8];u32 rng; } StoryData;
_Static_assert(sizeof(SaveVarsFlags)==1100&&sizeof(PlayerProfile)==32,"Original story record layouts changed");
struct HGStoryState { StoryData data;unsigned long long revision; };
typedef struct { char names[8][32]; } MessageFormat; /* Native UTF-8 boundary. */
struct HGFieldScript {
    FieldSystem field;ScriptContext context;HGStoryState *owner;HGStoryState candidate;
    const uint8_t *bytes;uint8_t *code;size_t size,floor,start;unsigned message_bank;
    unsigned long long revision;u16 special[NUM_SPECIAL_VARS];
    HGScriptMessage message;unsigned face,sound,total;int waiting,acknowledged,yielded,done,failed,preview,committed;
    HGScriptSceneServices services;void *scene;MessageFormat format,*format_ptr;u8 movement_count;
    char error[160];
};
static void *native_attribute(FieldSystem *field,enum ScriptEnvField attr) {
    HGFieldScript *script=(HGFieldScript *)(void *)field;
    if(attr==SCRIPTENV_MESSAGE_FORMAT)return &script->format_ptr;
    if(attr==SCRIPTENV_ACTIVE_MOVEMENT_COUNTER)return &script->movement_count;
    GF_ASSERT(attr>=SCRIPTENV_SPECIAL_VAR_8000&&attr<SCRIPTENV_SPECIAL_VAR_8000+NUM_SPECIAL_VARS);
    return &script->special[attr-SCRIPTENV_SPECIAL_VAR_8000];
}
static SaveVarsFlags *native_vars(SaveData *save) { return &((StoryData *)(void *)save)->vars; }
static PlayerProfile *native_profile(SaveData *save) { return &((StoryData *)(void *)save)->profile; }
static Party *native_party(SaveData *save) { return &((StoryData *)(void *)save)->party; }
static SaveData *native_save(FieldSystem *field) { return field->saveData; }
static void native_buffer_player(MessageFormat *,unsigned,PlayerProfile *);
#define FieldSysGetAttrAddr native_attribute
#define Save_VarsFlags_Get native_vars
#define Save_PlayerData_GetProfile native_profile
#define FieldSystem_GetSaveData native_save
#define SaveArray_Party_Get native_party
#define BufferPlayersName native_buffer_player
#include "script_kernel_decls.inc"
#include "script_kernel.inc"

static const ScrCmdFunc original_commands[1024]={
#include "script_core_table.inc"
};
static unsigned half(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t word(const uint8_t *p) { return half(p)|(uint32_t)half(p+2)<<16; }
static int flag_valid(unsigned flag) { return flag<NUM_FLAGS||(flag>=TEMP_FLAG_BASE&&flag<TEMP_FLAG_BASE+NUM_TEMP_FLAGS); }
static int variable(unsigned var) { return (var>=VAR_BASE&&var<VAR_BASE+NUM_VARS)||(var>=SPECIAL_VAR_BASE&&var<SPECIAL_VAR_BASE+NUM_SPECIAL_VARS); }
static int value_valid(unsigned value) { return value<VAR_BASE||variable(value); }
static void fail(HGFieldScript *s,const char *reason,unsigned op,size_t offset) {
    snprintf(s->error,sizeof(s->error),"%s (opcode %u, byte %zu)",reason,op,offset);s->failed=1;StopScript(&s->context);
}
static int operands(unsigned op) {
    switch(op) {
    case 0:case 2:case 27:case 50:case 53:case 58:case 95:case 96:case 97:case 104:case 505:return 0;
    case 11:case 12:case 20:case 30:case 31:case 32:case 60:case 73:case 132:case 144:case 332:case 386:return 2;
    case 3:case 17:case 18:case 22:case 26:case 39:case 40:case 41:case 42:case 43:case 294:return 4;
    case 28:case 29:return 5;
    case 45:case 57:case 190:return 1;case 56:case 59:return 3;case 55:case 94:return 6;default:return -1;
    }
}
static int valid_operands(HGFieldScript *s,unsigned op,const uint8_t *p) {
    if((op==30||op==31||op==32)&&!flag_valid(half(p)))return 0;
    if((op==17||op==18||op==39||op==40||op==41||op==42||op==43||op==144)&&!variable(half(p)))return 0;
    if((op==18||op==42)&&!variable(half(p+2)))return 0;
    if((op==39||op==40||op==43)&&!value_valid(half(p+2)))return 0;
    if(op==3&&(!half(p)||!variable(half(p+2))))return 0;
    if(op==11&&(p[0]>=4||p[1]>=4))return 0;
    if(op==12&&p[0]>=4)return 0;
    if((op==28||op==29)&&(p[0]>=6||s->context.comparisonResult>2))return 0;
    if(op==26||op==29)if(s->context.stackDepth+1>=20)return 0;
    if(op==22||op==26||op==28||op==29) {
        const uint8_t *relative=p+(op==28||op==29);int32_t delta=(int32_t)word(relative);
        int64_t target=(int64_t)(relative-s->bytes)+4+delta;
        if(target<(int64_t)s->floor||target>=(int64_t)s->size)return 0;
    }
    if(op==20&&half(p)!=std_signpost)return 0;
    if(op==73&&half(p)!=SEQ_SE_DP_SELECT)return 0;
    if(op==294) {
        if(!value_valid(half(p))||!variable(half(p+2)))return 0;
        if(FieldSystem_VarGet(&s->field,(u16)half(p))>=16)return 0;
    }
    if((op==332||op==386)&&!variable(half(p)))return 0;
    if(op==190&&p[0]>=NELEMS(s->format.names))return 0;
    if((op==94||op==95||op==386)&&!s->scene)return 0;
    if(op==94) {
        int64_t target=(int64_t)(p-s->bytes)+6+(int32_t)word(p+2);
        if(!value_valid(half(p))||target<(int64_t)s->floor||target>=(int64_t)s->size)return 0;
    }
    return 1;
}
static int profile_text(const PlayerProfile *profile,char out[32]) {
    const u16 *name=PlayerProfile_GetNamePtr(profile);size_t length=0;
    for(unsigned n=0;n<=PLAYER_NAME_LENGTH;n++) {
        if(name[n]==0xffff||!name[n]){out[length]=0;return length!=0;}
        if(n==PLAYER_NAME_LENGTH)return 0;
        uint32_t cp=0;for(unsigned i=0;i<NELEMS(glyph_map);i++)if(glyph_map[i].glyph==name[n]){cp=glyph_map[i].unicode;break;}
        if(!cp)return 0;
        if(cp<128)out[length++]=(char)cp;
        else if(cp<2048){out[length++]=(char)(0xc0|(cp>>6));out[length++]=(char)(0x80|(cp&63));}
        else {out[length++]=(char)(0xe0|(cp>>12));out[length++]=(char)(0x80|((cp>>6)&63));out[length++]=(char)(0x80|(cp&63));}
    }
    return 0;
}
static void native_buffer_player(MessageFormat *format,unsigned slot,PlayerProfile *profile) {
    GF_ASSERT(slot<NELEMS(format->names));GF_ASSERT(profile_text(profile,format->names[slot]));
}
/* Message JSON uses a private UTF-8 representation. Expand only the admitted
 * original name token; unknown controls and unbound slots fail preflight. */
static int expand_message(HGFieldScript *s,const uint8_t *text,size_t length,char *out,size_t capacity) {
    size_t at=0,written=0;
    while(at+1<length) {
        if(text[at]=='{') {
            unsigned slot=8;int consumed=0;
            if(sscanf((const char *)text+at,"{STRVAR_1 3, %u, 0}%n",&slot,&consumed)!=1||!consumed||slot>=8||!s->format.names[slot][0])return 0;
            size_t n=strlen(s->format.names[slot]);if(n>=capacity-written)return 0;
            memcpy(out+written,s->format.names[slot],n);written+=n;at+=(size_t)consumed;
        } else {
            if(written+1>=capacity)return 0;
            out[written++]=text[at]=='\f'?'\n':(char)text[at];at++;
        }
    }
    out[written]=0;return written!=0;
}
static BOOL presentation(ScriptContext *ctx,unsigned op) {
    HGFieldScript *s=(HGFieldScript *)(void *)ctx->fieldSystem;
    if(op==104) {
        s->face=1;
        if(s->scene&&!s->services.face_player(s->scene))fail(s,"Missing interaction actor",op,(size_t)(ctx->script_ptr-s->code));
    }
    else if(op==73)s->sound=ScriptReadHalfword(ctx);
    else if(op==386) {
        /* The scene owns a native avatar snapshot, including original facing. */
        u16 *direction=ScriptGetVarPointer(ctx);*direction=(u16)s->services.facing(s->scene);
    } else if(op==94) {
        unsigned person=ScriptGetVar(ctx);int32_t delta=(int32_t)ScriptReadWord(ctx);
        size_t target=(size_t)((ctx->script_ptr-s->code)+(int64_t)delta);
        if(!s->services.apply(s->scene,person,s->bytes+target,s->size-target))fail(s,"Unsupported scripted movement",op,target);
        s->movement_count=(u8)s->services.active(s->scene);
    } else if(op==45||op==55||op==59||op==132) {
        if(s->movement_count){fail(s,"Concurrent dialogue/movement is not admitted",op,(size_t)(ctx->script_ptr-s->code));return TRUE;}
        unsigned message=ScriptReadByte(ctx);const uint8_t *text;size_t length;
        if(op==132){unsigned female=ScriptReadByte(ctx);if(PlayerProfile_GetTrainerGender(&s->candidate.data.profile))message=female;}
        if(!native_resource_member(NARC_msgdata_msg,(int)s->message_bank,(int)message,&text,&length)||length<2||
           text[length-1]||memchr(text,0,length-1)||!expand_message(s,text,length,s->message.text,sizeof(s->message.text))) {
            fail(s,"Unsupported message/format",op,(size_t)(ctx->script_ptr-s->code));return TRUE;
        }
        s->message.face_player=s->face;s->message.select_sound=s->sound;
        s->face=s->sound=0;s->waiting=1;s->acknowledged=0;
        int remaining=operands(op)-(op==132?2:1);while(remaining-->0)(void)ScriptReadByte(ctx);return TRUE;
    } else if(op==50) {
        if(!s->acknowledged){fail(s,"Button wait has no presented message",op,(size_t)(ctx->script_ptr-s->code));return TRUE;}
        s->acknowledged=0;
    } else {
        int remaining=operands(op);while(remaining-->0)(void)ScriptReadByte(ctx);
    }
    return FALSE;
}
static BOOL dispatch(ScriptContext *ctx) {
    HGFieldScript *s=(HGFieldScript *)(void *)ctx->fieldSystem;unsigned op=half(ctx->script_ptr-2);
    s->yielded=original_commands[op]?original_commands[op](ctx):presentation(ctx,op);
    return TRUE; /* One bounded dispatch; native owner batches false results. */
}
static ScrCmdFunc dispatch_table[1024];
static void initialize(HGFieldScript *s) {
    /* Original InitScriptContext does not reset comparisonResult. Replay must
     * start with fresh native allocation state, not the preview's last result. */
    memset(&s->context,0,sizeof(s->context));
    InitScriptContext(&s->context,dispatch_table,NELEMS(dispatch_table));s->context.fieldSystem=&s->field;
    SetupBytecodeScript(&s->context,s->code+s->start);s->candidate=*s->owner;
    s->field.saveData=(SaveData *)(void *)&s->candidate.data;
    memset(s->special,0,sizeof(s->special));memset(&s->message,0,sizeof(s->message));
    memset(&s->format,0,sizeof(s->format));s->format_ptr=&s->format;s->movement_count=0;
    if(s->services.clone) {
        if(s->scene)s->services.destroy(s->scene);s->scene=s->services.clone(s->services.source);
        if(!s->scene){fail(s,"Cannot prepare script scene",0,0);return;}
        s->services.bind(s->scene,&s->field);
    }
    s->face=s->sound=s->total=0;s->waiting=s->acknowledged=s->yielded=s->done=s->failed=0;s->error[0]=0;
}
int hg_field_script_tick(HGFieldScript *s) {
    if(!s||s->failed)return HG_SCRIPT_ERROR;if(s->done)return HG_SCRIPT_DONE;
    if(s->owner->revision!=s->revision){fail(s,"Story state changed during script",0,0);return HG_SCRIPT_ERROR;}
    if(s->waiting)return HG_SCRIPT_MESSAGE;
    if(s->scene&&!s->services.tick(s->scene)){fail(s,"Scripted movement failed",94,0);return HG_SCRIPT_ERROR;}
    if(s->scene)s->movement_count=(u8)s->services.active(s->scene);
    for(unsigned batch=0;batch<32;batch++) {
        size_t relocated=0;uint8_t saved_operand[4];int patched=0;
        if(++s->total>16384){fail(s,"Script instruction/frame budget exceeded",0,0);return HG_SCRIPT_ERROR;}
        if(s->context.mode==SCRIPT_MODE_BYTECODE) {
            size_t at=(size_t)(s->context.script_ptr-s->code);
            if(at<s->floor||at>s->size||s->size-at<2){fail(s,"Script pointer outside bank",0,at);return HG_SCRIPT_ERROR;}
            unsigned op=half(s->bytes+at);int length=operands(op);
            if(op>=NELEMS(dispatch_table)||length<0||(size_t)length>s->size-at-2||!valid_operands(s,op,s->bytes+at+2)) {
                fail(s,"Unsupported opcode/operand",op,at);return HG_SCRIPT_ERROR;
            }
            if(op==22||op==26||op==28||op==29) {
                relocated=at+2+(op==28||op==29);int32_t delta=(int32_t)word(s->bytes+relocated);
                if(delta<0) {
                    /* Original unsigned branch addition remains unchanged.
                     * Re-encode a negative DS offset as a positive jump into
                     * an owned mirror region, then canonicalize after exactly
                     * one original command. The 16-byte gap distinguishes a
                     * fallthrough at EOF from a taken jump to logical byte 0. */
                    uint32_t forward=(uint32_t)((int64_t)s->size+16+delta);
                    memcpy(saved_operand,s->code+relocated,4);
                    for(unsigned i=0;i<4;i++)s->code[relocated+i]=(uint8_t)(forward>>(8*i));patched=1;
                }
            }
        }
        u8 temporary[8];memcpy(temporary,s->candidate.data.temporary,8);hg_story_temp_exchange(temporary);
        u32 previous_rng=GetLCRNGSeed();SetLCRNGSeed(s->candidate.data.rng);
        s->yielded=0;int native=s->context.mode==SCRIPT_MODE_NATIVE;RunScriptCommand(&s->context);
        if(patched)memcpy(s->code+relocated,saved_operand,4);
        if(s->context.script_ptr) {
            uintptr_t position=(uintptr_t)s->context.script_ptr,base=(uintptr_t)s->code;size_t mirror=s->size+16;
            if(position>=base+mirror&&position<base+mirror+s->size)s->context.script_ptr-=mirror;
            else if(position<base||position>base+s->size)fail(s,"Script target outside arena",0,0);
        }
        if(s->context.mode==SCRIPT_MODE_BYTECODE&&!s->context.script_ptr)RunScriptCommand(&s->context);
        s->candidate.data.rng=GetLCRNGSeed();SetLCRNGSeed(previous_rng);
        hg_story_temp_exchange(temporary);memcpy(s->candidate.data.temporary,temporary,8);
        if(s->failed)return HG_SCRIPT_ERROR;
        if(s->context.mode==SCRIPT_MODE_STOPPED) {
            if(s->scene&&s->services.active(s->scene)){fail(s,"Unjoined scripted movement",94,0);return HG_SCRIPT_ERROR;}
            s->done=1;return HG_SCRIPT_DONE;
        }
        if(s->waiting)return HG_SCRIPT_MESSAGE;
        if(native||s->yielded)break;
    }
    return HG_SCRIPT_RUNNING;
}
int hg_field_script_ack(HGFieldScript *s) {
    if(!s||s->failed||s->done||!s->waiting)return 0;s->waiting=0;s->acknowledged=1;s->message.text[0]=0;return 1;
}
const HGStoryState *hg_field_script_candidate(const HGFieldScript *s) { return s&&s->done&&!s->failed?&s->candidate:NULL; }
int hg_field_script_commit(HGFieldScript *s) {
    if(!s||!s->done||s->failed||s->preview||s->committed||!hg_story_commit(s->owner,&s->candidate,s->revision))return 0;
    s->committed=1;return 1;
}
const HGScriptMessage *hg_field_script_message(const HGFieldScript *s) { return s&&s->waiting&&!s->failed?&s->message:NULL; }
const char *hg_field_script_error(const HGFieldScript *s) { return s?s->error:"Missing script"; }
void hg_field_script_free(HGFieldScript *s) { if(s){if(s->scene)s->services.destroy(s->scene);free(s->code);free(s);} }
const void *hg_field_script_scene(const HGFieldScript *s) { return s?s->scene:NULL; }
static HGFieldScript *script_bytes(HGStoryState *owner,const uint8_t *bytes,size_t size,size_t floor,size_t start,unsigned bank,const HGScriptSceneServices *services,char *error,size_t capacity) {
    if(error&&capacity)error[0]=0;
    if(!owner||!bytes||size>1024*1024||start>=size)return NULL;
    if(services&&(!services->source||!services->clone||!services->destroy||!services->bind||!services->apply||
       !services->tick||!services->active||!services->facing||!services->face_player))return NULL;
    HGFieldScript *s=calloc(1,sizeof(*s));if(!s)return NULL;
    s->code=calloc(1,2*size+16);if(!s->code){free(s);return NULL;}memcpy(s->code,bytes,size);
    for(unsigned i=0;i<NELEMS(dispatch_table);i++)dispatch_table[i]=dispatch;
    if(services)s->services=*services;
    s->owner=owner;s->revision=owner->revision;s->bytes=bytes;s->size=size;s->floor=floor;s->start=start;s->message_bank=bank;s->preview=1;initialize(s);
    int status=HG_SCRIPT_RUNNING;
    while(status!=HG_SCRIPT_DONE&&status!=HG_SCRIPT_ERROR) {
        status=hg_field_script_tick(s);if(status==HG_SCRIPT_MESSAGE)hg_field_script_ack(s);
    }
    if(status==HG_SCRIPT_ERROR){if(error&&capacity)snprintf(error,capacity,"%s",s->error);hg_field_script_free(s);return NULL;}
    s->preview=0;initialize(s);if(s->failed){hg_field_script_free(s);return NULL;}return s;
}
HGFieldScript *hg_field_script_bytes(HGStoryState *owner,const uint8_t *bytes,size_t size,size_t start,unsigned bank,char *error,size_t capacity) {
    return script_bytes(owner,bytes,size,0,start,bank,NULL,error,capacity);
}
static HGFieldScript *script_bank_scene(HGStoryState *owner,unsigned bank,unsigned id,unsigned messages,const HGScriptSceneServices *services,char *error,size_t capacity) {
    unsigned entries=0;for(unsigned i=0;i<NELEMS(field_script_banks);i++)if(field_script_banks[i].bank==bank)entries=field_script_banks[i].entries;
    if(!id||id>entries)return NULL;const uint8_t *bytes;size_t size;
    if(!native_resource_member(NARC_fielddata_script_scr_seq,0,(int)bank,&bytes,&size)||size<4*entries+2)return NULL;
    size_t table=4*(id-1);uint64_t start=(uint64_t)table+4+word(bytes+table);
    if(start<4*entries+2||start>=size)return NULL;
    return script_bytes(owner,bytes,size,4*entries+2,(size_t)start,messages,services,error,capacity);
}
static HGFieldScript *script_bank(HGStoryState *owner,unsigned bank,unsigned id,unsigned messages,char *error,size_t capacity) {
    return script_bank_scene(owner,bank,id,messages,NULL,error,capacity);
}
HGFieldScript *hg_field_script_scene_new(HGStoryState *owner,unsigned map,unsigned id,const HGScriptSceneServices *services,char *error,size_t capacity) {
    if(map>=MAP_ID_MAX)return NULL;
    return script_bank_scene(owner,MapHeader_GetScriptsBank(map),id,MapHeader_GetMsgBank(map),services,error,capacity);
}
HGFieldScript *hg_field_script_new(HGStoryState *owner,unsigned map,unsigned id,char *error,size_t capacity) {
    if(map>=MAP_ID_MAX)return NULL;
    return script_bank(owner,MapHeader_GetScriptsBank(map),id,MapHeader_GetMsgBank(map),error,capacity);
}
HGStoryState *hg_story_new(void) {
    HGStoryState *state=calloc(1,sizeof(*state));if(!state)return NULL;
    Save_VarsFlags_Init(&state->data.vars);PlayerProfile_Init(&state->data.profile);
    state->data.party.core.maxCount=PARTY_SIZE; /* Native empty inspection party. */
    if(!hg_story_profile_set(state,"Ethan",0)){free(state);return NULL;}
    state->revision=0;
    HGFieldScript *init=script_bank(state,149,1,40,NULL,0);if(!init){free(state);return NULL;}
    int status;do{status=hg_field_script_tick(init);}while(status==HG_SCRIPT_RUNNING);
    if(status==HG_SCRIPT_DONE&&!hg_field_script_commit(init))status=HG_SCRIPT_ERROR;
    hg_field_script_free(init);if(status!=HG_SCRIPT_DONE){free(state);return NULL;}return state;
}
void hg_story_free(HGStoryState *state) { free(state); }
int hg_story_profile_set(HGStoryState *state,const char *name,unsigned gender) {
    if(!state||!name||gender>1)return 0;u16 encoded[PLAYER_NAME_LENGTH+1]={0};unsigned count=0;
    const unsigned char *p=(const unsigned char *)name;
    while(*p) {
        if(count==PLAYER_NAME_LENGTH)return 0;uint32_t cp=*p++;
        if(cp>=128) {
            unsigned tail=cp>=0xe0&&cp<0xf0?2:cp>=0xc2&&cp<0xe0?1:0;if(!tail)return 0;
            cp&=tail==2?15:31;for(unsigned i=0;i<tail;i++){if((*p&0xc0)!=0x80)return 0;cp=(cp<<6)|(*p++&63);}
            if(cp<(tail==2?0x800u:0x80u)||(cp>=0xd800&&cp<=0xdfff))return 0;
        }
        unsigned glyph=0;for(unsigned i=0;i<NELEMS(glyph_map);i++)if(glyph_map[i].unicode==cp){glyph=glyph_map[i].glyph;break;}
        if(!glyph||cp<32||cp=='{'||cp=='}')return 0;encoded[count++]=(u16)glyph;
    }
    if(!count)return 0;encoded[count]=0xffff;
    memcpy(state->data.profile.name,encoded,sizeof(encoded));state->data.profile.gender=(u8)gender;state->revision++;return 1;
}
HGStoryState *hg_story_clone(const HGStoryState *state) {
    if(!state)return NULL;HGStoryState *copy=malloc(sizeof(*copy));if(copy)*copy=*state;return copy;
}
int hg_story_commit(HGStoryState *state,const HGStoryState *candidate,unsigned long long expected) {
    if(!state||!candidate||state->revision!=expected)return 0;
    state->data=candidate->data;state->revision++;return 1;
}
int hg_story_population_changed(const HGStoryState *a,const HGStoryState *b) {
    if(!a||!b)return 1;
    return memcmp(a->data.vars.flags,b->data.vars.flags,sizeof(a->data.vars.flags))||
        memcmp(a->data.vars.vars+VAR_OBJ_GFX_BASE-VAR_BASE,b->data.vars.vars+VAR_OBJ_GFX_BASE-VAR_BASE,NUM_OBJ_GFX_VARS*sizeof(u16));
}
unsigned long long hg_story_revision(const HGStoryState *state) { return state?state->revision:0; }
void hg_story_bind_field(HGStoryState *state,FieldSystem *field) { if(field)field->saveData=state?(SaveData *)(void *)&state->data:NULL; }
int hg_story_var_get(const HGStoryState *state,unsigned var,uint16_t *out) {
    if(!state||!out||var<VAR_BASE||var>=VAR_BASE+NUM_VARS)return 0;
    FieldSystem field={0};hg_story_bind_field((HGStoryState *)state,&field);*out=FieldSystem_VarGet(&field,(u16)var);return 1;
}
int hg_story_var_set(HGStoryState *state,unsigned var,uint16_t value) {
    if(!state||var<VAR_BASE||var>=VAR_BASE+NUM_VARS)return 0;
    FieldSystem field={0};hg_story_bind_field(state,&field);
    if(!FieldSystem_VarSet(&field,(u16)var,value))return 0;state->revision++;return 1;
}
int hg_story_flag_get(const HGStoryState *state,unsigned flag,int *out) {
    if(!state||!out||!flag_valid(flag))return 0;
    u8 temporary[8];memcpy(temporary,state->data.temporary,8);hg_story_temp_exchange(temporary);
    *out=Save_VarsFlags_CheckFlagInArray((SaveVarsFlags *)&state->data.vars,(u16)flag);hg_story_temp_exchange(temporary);return 1;
}
int hg_story_flag_set(HGStoryState *state,unsigned flag,int value) {
    if(!state||!flag_valid(flag)||(value!=0&&value!=1))return 0;
    u8 temporary[8];memcpy(temporary,state->data.temporary,8);hg_story_temp_exchange(temporary);
    if(value)Save_VarsFlags_SetFlagInArray(&state->data.vars,(u16)flag);else Save_VarsFlags_ClearFlagInArray(&state->data.vars,(u16)flag);
    hg_story_temp_exchange(temporary);memcpy(state->data.temporary,temporary,8);state->revision++;return 1;
}
int hg_story_header_script(HGStoryState *state,const uint8_t *bytes,size_t size,unsigned type,unsigned *out) {
    if(!state||!bytes||!size||size>65536||!out||type<1||type>4)return 0;
    size_t at=0;
    while(at<size&&bytes[at]) {
        if(size-at<5||bytes[at]>4)return 0;
        if(bytes[at]==INIT_SCRIPT_ON_FRAME_TABLE) {
            uint32_t delta=word(bytes+at+1);uint64_t next=(uint64_t)at+5+delta;
            if(delta) {
                if(next>=size)return 0;size_t row=(size_t)next;
                while(1) {
                    if(size-row<2)return 0;unsigned left=half(bytes+row);if(!left)break;
                    if(size-row<6)return 0;unsigned right=half(bytes+row+2);
                    /* Frame hooks have no active script's special variables. */
                    if(!value_valid(left)||!value_valid(right)||left>=SPECIAL_VAR_BASE||right>=SPECIAL_VAR_BASE)return 0;
                    row+=6;
                }
            }
        }
        at+=5;
    }
    if(at==size)return 0;FieldSystem field={0};hg_story_bind_field(state,&field);
    *out=type==INIT_SCRIPT_ON_FRAME_TABLE?GetMapSceneScriptId(&field,(u8 *)bytes,(u8)type):GetMapLoadScriptId((u8 *)bytes,(u8)type);
    return 1;
}
int hg_story_map_script(HGStoryState *state,unsigned map,unsigned type,unsigned *out) {
    if(map>=MAP_ID_MAX)return 0;const uint8_t *bytes;size_t size;
    if(!native_resource_member(NARC_fielddata_script_scr_seq,0,MapHeader_GetScriptHeaderBank(map),&bytes,&size))return 0;
    return hg_story_header_script(state,bytes,size,type,out);
}
int hg_story_enter_map(HGStoryState *state,unsigned map,char *error,size_t size) {
    if(!state)return 0;HGStoryState before=*state;FieldSystem field={0};hg_story_bind_field(state,&field);ClearTempFieldEventData(&field);state->revision++;
    unsigned entry;if(!hg_story_map_script(state,map,INIT_SCRIPT_ON_TRANSITION,&entry)){*state=before;return 0;}
    if(entry==65535)return 1;
    HGFieldScript *script=hg_field_script_new(state,map,entry,error,size);if(!script){*state=before;return 0;}
    int status;do{status=hg_field_script_tick(script);}while(status==HG_SCRIPT_RUNNING);
    if(status==HG_SCRIPT_DONE&&!hg_field_script_commit(script))status=HG_SCRIPT_ERROR;
    hg_field_script_free(script);if(status!=HG_SCRIPT_DONE){*state=before;return 0;}return 1;
}
