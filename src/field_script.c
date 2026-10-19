#include "compat/story.h"
#include "compat/pokemon.h"
#include "bag.h"
#include "save_pokegear.h"
#include "compat/apricorn.h"
#include "compat/story_records.h"
#include "compat/resources.h"
#include "map_header.h"
#include "constants/maps.h"
#include "constants/spawns.h"
#include "constants/sndseq.h"
#include "constants/std_script.h"
#include "constants/init_script_types.h"
#include "field_script_banks.inc"
#include "script_standard_banks.inc"
#include "script_sound_ids.inc"
#include "charmap.inc"
#include "game_stats.h"
#undef POKEHEARTGOLD_FIELD_TYPES_DEF_H
#include "field_types_def.h"
void Save_Pokedex_Init(Pokedex *);

typedef struct { SaveVarsFlags vars;PlayerProfile profile;Options options;Party party;GameStats stats;Pokedex dex;Bag bag;SavePokegear pokegear;SysInfo sys_info;u16 blackout_spawn;APRICORN_TREE trees[MAX_APRICORN_TREE];u8 temporary[NUM_TEMP_FLAGS/8];u32 rng; } StoryData;
_Static_assert(sizeof(SaveVarsFlags)==1100&&sizeof(PlayerProfile)==32,"Original story record layouts changed");
struct HGStoryState { StoryData data;unsigned long long revision;HGStoryMusic music; };
typedef struct { char names[8][128]; } MessageFormat; /* Native UTF-8 boundary. */
struct HGFieldScript {
    FieldSystem field;ScriptContext context;HGStoryState *owner;HGStoryState candidate;
    const uint8_t *bytes;uint8_t *code;size_t size,floor,start;unsigned message_bank;
    unsigned long long revision;u16 special[NUM_SPECIAL_VARS];
    HGScriptMessage message;unsigned face,sound,total;int waiting,acknowledged,yielded,done,failed,preview,committed;
    HGScriptSceneServices services;void *scene;MessageFormat format,*format_ptr;u8 movement_count;
    int menu_pending,menu_checked;unsigned menu_choices;u32 *menu_result;
    HGFieldScript *parent,*child;u8 waiters,context_count;ScriptContext *contexts[3];
    char error[160];
};
static HGFieldScript *root_script(HGFieldScript *s) { while(s->parent)s=s->parent;return s; }
static HGFieldScript *active_script;
static unsigned native_bgm_current(void) { GF_ASSERT(active_script);return active_script->candidate.music.sequence; }
static void native_bgm_play(unsigned sequence) { GF_ASSERT(active_script);active_script->candidate.music.sequence=sequence;active_script->candidate.music.requests++; }
static void native_bgm_stop(unsigned sequence,unsigned unused) { (void)sequence;(void)unused;native_bgm_play(0); }
static void native_bgm_fade(unsigned target,unsigned ticks) { GF_ASSERT(active_script);(void)target;active_script->candidate.music.fade_ticks=ticks;active_script->candidate.music.requests++; }
static void native_bgm_fade_in(unsigned volume,unsigned ticks,unsigned unused) { (void)volume;(void)unused;native_bgm_fade(0,ticks); }
static unsigned native_bgm_timer(void) { GF_ASSERT(active_script);return active_script->candidate.music.fade_ticks; }
static void native_se_play(unsigned sequence) { GF_ASSERT(active_script);active_script->sound=sequence;active_script->candidate.music.requests++; }
static void native_se_stop(unsigned sequence,unsigned unused) { (void)sequence;(void)unused;GF_ASSERT(active_script);active_script->candidate.music.requests++; }
static unsigned native_se_playing(unsigned sequence) { (void)sequence;return 0; /* Muted requests have no playback handle. */ }
static void native_fanfare_play(unsigned sequence) { (void)sequence;GF_ASSERT(active_script);active_script->candidate.music.requests++; }
static unsigned native_fanfare_playing(void) { return 0; }
#define PlaySE native_se_play
#define StopSE native_se_stop
#define IsSEPlaying native_se_playing
#define PlayFanfare native_fanfare_play
#define IsFanfarePlaying native_fanfare_playing
static unsigned native_map_bgm(FieldSystem *field,unsigned map) { (void)field;return MapHeader_GetDayMusicId(map); }
#define PlayBGM native_bgm_play
#define StopBGM native_bgm_stop
#define GF_GetCurrentPlayingBGM native_bgm_current
#define sub_02005E44 native_bgm_play
#define GF_SndStartFadeOutBGM native_bgm_fade
#define GF_SndStartFadeInBGM native_bgm_fade_in
#define GF_SndGetFadeTimer native_bgm_timer
#define FieldBGM_GetForMapHeader native_map_bgm
static void *native_attribute(FieldSystem *field,enum ScriptEnvField attr) {
    HGFieldScript *script=root_script((HGFieldScript *)(void *)field);
    if(attr==SCRIPTENV_FIELD_07)return &script->waiters;
    if(attr==SCRIPTENV_ACTIVE_SCRIPT_CONTEXT_COUNT)return &script->context_count;
    if(attr>=SCRIPTENV_SCRIPT_CONTEXT_0&&attr<SCRIPTENV_SCRIPT_CONTEXT_0+3)return &script->contexts[attr-SCRIPTENV_SCRIPT_CONTEXT_0];
    if(attr==SCRIPTENV_MESSAGE_FORMAT)return &script->format_ptr;
    if(attr==SCRIPTENV_ACTIVE_MOVEMENT_COUNTER)return &script->movement_count;
    GF_ASSERT(attr>=SCRIPTENV_SPECIAL_VAR_8000&&attr<SCRIPTENV_SPECIAL_VAR_8000+NUM_SPECIAL_VARS);
    return &script->special[attr-SCRIPTENV_SPECIAL_VAR_8000];
}
static SaveVarsFlags *native_vars(SaveData *save) { return &((StoryData *)(void *)save)->vars; }
SaveVarsFlags *hg_story_save_vars(SaveData *save) { return native_vars(save); }
PlayerProfile *hg_story_save_profile(SaveData *save) { return &((StoryData *)(void *)save)->profile; }
Options *hg_story_save_options(SaveData *save) { return &((StoryData *)(void *)save)->options; }
GameStats *hg_story_save_stats(SaveData *save) { return &((StoryData *)(void *)save)->stats; }
Pokedex *hg_story_save_dex(SaveData *save) { return &((StoryData *)(void *)save)->dex; }
Bag *hg_story_save_bag(SaveData *save) { return &((StoryData *)(void *)save)->bag; }
Party *hg_story_save_party(SaveData *save) { return &((StoryData *)(void *)save)->party; }
SavePokegear *hg_story_save_pokegear(SaveData *save) { return &((StoryData *)(void *)save)->pokegear; }
APRICORN_TREE *hg_story_save_apricorn_trees(SaveData *save) { return ((StoryData *)(void *)save)->trees; }
SysInfo *hg_story_save_sys_info(SaveData *save) { return &((StoryData *)(void *)save)->sys_info; }
u16 *hg_story_save_blackout_spawn(SaveData *save) { return &((StoryData *)(void *)save)->blackout_spawn; }
static SaveData *native_save(FieldSystem *field) { return field->saveData; }
static void native_buffer_player(MessageFormat *,unsigned,PlayerProfile *);
#define FieldSysGetAttrAddr native_attribute
#define Save_VarsFlags_Get native_vars
#define Save_PlayerData_GetProfile hg_story_save_profile
#define FieldSystem_GetSaveData native_save
#define SaveArray_Party_Get hg_story_save_party
#define Save_Pokedex_Get hg_story_save_dex
#define Save_SysInfo_Get hg_story_save_sys_info
/* Original HealParty's redundant TaskManager lookup selects this same field. */
#define TaskManager_GetFieldSystem(task) (ctx->fieldSystem)
#define Save_Bag_Get hg_story_save_bag
static ScriptContext *native_create_context(FieldSystem *,u16);
#define CreateScriptContext native_create_context
static void native_buffer_species(MessageFormat *,unsigned,BoxPokemon *);
static void native_buffer_species_article(MessageFormat *,unsigned,BoxPokemon *);
static unsigned color_control(const uint8_t *text,size_t size) {
    unsigned color;int consumed=0;
    if(size<10||sscanf((const char *)text,"{COLOR %u}%n",&color,&consumed)!=1||!consumed||color>255||(size_t)consumed>=size)return 0;
    return (unsigned)consumed;
}
static int resource_name(unsigned group,unsigned member,char *out,size_t capacity) {
    const uint8_t *bytes;size_t size;if(!native_resource_member(NARC_msgdata_msg,group,member,&bytes,&size)||size<2||bytes[size-1]||memchr(bytes,0,size-1))return 0;
    size_t written=0;
    for(size_t at=0;at+1<size;) {
        unsigned control=color_control(bytes+at,size-at);if(control){at+=control;continue;}
        if(bytes[at]=='{'||written+1>=capacity)return 0;out[written++]=(char)bytes[at++];
    }
    out[written]=0;return written!=0;
}
static void buffer_resource(MessageFormat *format,unsigned slot,unsigned group,unsigned member) {
    GF_ASSERT(slot<8&&resource_name(group,member,format->names[slot],sizeof(format->names[slot])));
}
static void buffer_item(MessageFormat *f,unsigned s,unsigned item) { buffer_resource(f,s,222,item); }
static void buffer_items(MessageFormat *f,unsigned s,unsigned item) { buffer_resource(f,s,224,item); }
static void buffer_pocket(MessageFormat *f,unsigned s,unsigned pocket) { buffer_resource(f,s,226,pocket); }
#define BufferItemName buffer_item
#define BufferItemNamePlural buffer_items
#define BufferPocketName buffer_pocket
static void native_menu_begin(FieldSystem *field,int x,int y,u32 *result) {
    HGFieldScript *s=(HGFieldScript *)(void *)field;GF_ASSERT(x==3&&y==3&&!s->menu_pending);s->menu_result=result;s->menu_pending=!s->preview;s->menu_checked=0;s->menu_choices=0;if(s->preview)*result=1;
}
static int native_menu_state(FieldSystem *field) { return ((HGFieldScript *)(void *)field)->menu_pending?0:3; }
static int native_menu_finished(FieldSystem *field) { return ((HGFieldScript *)(void *)field)->menu_pending?0:6; }
#define ov01_021F6ABC native_menu_begin
#define ov01_021F6B00 native_menu_state
#define ov01_021F6AEC native_menu_finished
#define BufferBoxMonSpeciesName native_buffer_species
#define BufferBoxMonSpeciesNameWithArticle native_buffer_species_article
#define BufferPlayersName native_buffer_player
#include "script_kernel_decls.inc"
#include "script_kernel.inc"

static const ScrCmdFunc original_commands[1024]={
#include "script_core_table.inc"
};
static unsigned half(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t word(const uint8_t *p) { return half(p)|(uint32_t)half(p+2)<<16; }
typedef struct { const uint8_t *bytes;size_t size,floor,start;unsigned messages; } ScriptResource;
static int bank_resource(unsigned bank,unsigned index,unsigned messages,ScriptResource *out) {
    unsigned entries=0;
    for(unsigned i=0;i<NELEMS(field_script_banks);i++)if(field_script_banks[i].bank==bank)entries=field_script_banks[i].entries;
    const uint8_t *bytes;size_t size,floor=4*(size_t)entries+2;
    if(index>=entries||!native_resource_member(NARC_fielddata_script_scr_seq,0,(int)bank,&bytes,&size)||size<floor||size>1024*1024)return 0;
    size_t table=4*(size_t)index;uint64_t start=table+4+(uint64_t)word(bytes+table);
    if(start<floor||start>=size)return 0;
    *out=(ScriptResource){bytes,size,floor,(size_t)start,messages};return 1;
}
static int standard_resource(unsigned id,ScriptResource *out) {
    for(unsigned i=0;i<NELEMS(standard_script_banks);i++)if(id>=standard_script_banks[i].first)
        return bank_resource(standard_script_banks[i].bank,id-standard_script_banks[i].first,standard_script_banks[i].messages,out);
    return 0;
}
static int flag_valid(unsigned flag) { return flag<NUM_FLAGS||(flag>=TEMP_FLAG_BASE&&flag<TEMP_FLAG_BASE+NUM_TEMP_FLAGS); }
static int variable(unsigned var) { return (var>=VAR_BASE&&var<VAR_BASE+NUM_VARS)||(var>=SPECIAL_VAR_BASE&&var<SPECIAL_VAR_BASE+NUM_SPECIAL_VARS); }
static int value_valid(unsigned value) { return value<VAR_BASE||variable(value); }
static int sound_valid(unsigned id) { for(unsigned i=0;i<NELEMS(script_sound_ids);i++)if(script_sound_ids[i]==id)return 1;return 0; }
static void fail(HGFieldScript *s,const char *reason,unsigned op,size_t offset) {
    snprintf(s->error,sizeof(s->error),"%s (opcode %u, byte %zu)",reason,op,offset);s->failed=1;StopScript(&s->context);
}
static int operands(unsigned op) {
    switch(op) {
#include "script_original_operands.inc"
    default:break;
    }
    switch(op) {
    case 0:case 2:case 27:case 50:case 51:case 53:case 58:case 95:case 96:case 97:case 104:case 167:case 609:case 621:case 505:case 21:case 608:case 603:case 746:case 747:case 79:return 0;
    case 11:case 12:case 20:case 30:case 31:case 32:case 60:case 73:case 132:case 144:case 332:case 386:case 131:case 602:case 604:case 605:case 748:case 78:return 2;
    case 105:case 3:case 17:case 18:case 22:case 26:case 39:case 40:case 41:case 42:case 43:case 294:case 354:case 130:return 4;
    case 339:return 10;
    case 28:case 29:return 5;
    case 45:case 57:case 190:return 1;case 56:case 59:return 3;case 55:case 94:case 106:return 6;case 193:return 3;case 127:case 125:return 6;default:return -1;
    }
}
static int scene_command_admitted(unsigned op) {
    switch(op){
#include "script_scene_admitted.inc"
    default:return 0;}
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
    if(op==20&&root_script(s)->context_count>=3)return 0;
    if(op==21&&!s->parent)return 0; /* Original restart shifts by context.id - 1. */
    if(op==82&&!s->field.location)return 0;
    if(op==84&&half(p+2)>3600)return 0;
    if(op==85&&half(p)>3600)return 0;
    if(op==281&&!variable(half(p)))return 0;
    if(op==290&&!variable(half(p)))return 0;
    if(op==280&&(!value_valid(half(p))||FieldSystem_VarGet(&s->field,half(p))<SPAWN_NEW_BARK||FieldSystem_VarGet(&s->field,half(p))>SPAWN_ROCK_TUNNEL))return 0;
    if((op==98||op==99||op==100||op==101)&&!value_valid(half(p)))return 0;
    if((op==98||op==99||op==100||op==101)&&!s->scene)return 0;
    if(op==20&&half(p)!=std_signpost){ScriptResource resource;if(!standard_resource(half(p),&resource))return 0;}
    if(op==73||op==74||op==75||op==78)if(!value_valid(half(p))||!sound_valid(FieldSystem_VarGet(&s->field,half(p))))return 0;
    if((op==80||op==87)&&!sound_valid(half(p)))return 0;
    if(op==294) {
        if(!value_valid(half(p))||!variable(half(p+2)))return 0;
        if(FieldSystem_VarGet(&s->field,(u16)half(p))>=16)return 0;
    }
    if((op==127||op==125||op==128)&&(!value_valid(half(p))||!value_valid(half(p+2))||!variable(half(p+4))||FieldSystem_VarGet(&s->field,half(p))>=ITEMS_COUNT))return 0;
    if(op==130&&(!value_valid(half(p))||!variable(half(p+2))||FieldSystem_VarGet(&s->field,half(p))>=ITEMS_COUNT))return 0;
    if(op==194||op==195||op==844) {
        if(p[0]>=8||!value_valid(half(p+1)))return 0;
        unsigned member=FieldSystem_VarGet(&s->field,half(p+1)),group=op==194?222:op==195?226:224;
        char checked[sizeof(s->format.names[0])];if(!resource_name(group,member,checked,sizeof(checked)))return 0;
    }
    if(op==131&&!value_valid(half(p)))return 0;
    if((op==146||op==147)&&!value_valid(half(p)))return 0;
    if(op==147&&!variable(half(p+2)))return 0;
    if(op==748&&!variable(half(p)))return 0;
    if(op==354&&(!value_valid(half(p))||!variable(half(p+2))||FieldSystem_VarGet(&s->field,half(p))>=Party_GetCount(&s->candidate.data.party)))return 0;
    if((op==193||op==845)&&(p[0]>=8||!value_valid(half(p+1))||FieldSystem_VarGet(&s->field,half(p+1))>=Party_GetCount(&s->candidate.data.party)))return 0;
    if(op==602&&half(p)>1)return 0;
    if(op==604&&half(p)!=48&&half(p)!=55)return 0;
    if(op==605&&(p[0]>3||p[1]>3))return 0;
    if(op==339)for(unsigned i=0;i<5;i++)if(!value_valid(half(p+2*i)))return 0;
    if(op==105&&(!variable(half(p))||!variable(half(p+2))))return 0;
    if(op==106&&(!value_valid(half(p))||!variable(half(p+2))||!variable(half(p+4))))return 0;
    if((op==332||op==386)&&!variable(half(p)))return 0;
    if(op==190&&p[0]>=NELEMS(s->format.names))return 0;
    if((op==94||op==95||op==105||op==106||op==339||op==167||op==609||op==621||op==386||op==605||op==608||op==602||op==603||op==604)&&!s->scene)return 0;
    if(scene_command_admitted(op)&&op!=97&&!s->scene)return 0;
    if(op>=307&&op<=311&&!s->scene)return 0;
    if(op==307&&(!value_valid(half(p+4))||!value_valid(half(p+6))))return 0;
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
static void native_buffer_species(MessageFormat *format,unsigned slot,BoxPokemon *mon) {
    GF_ASSERT(slot<NELEMS(format->names));const char *name=hg_pokemon_species_name(GetBoxMonData(mon,MON_DATA_SPECIES,NULL));
    GF_ASSERT(name&&strlen(name)<sizeof(format->names[slot]));strcpy(format->names[slot],name);
}
static void native_buffer_species_article(MessageFormat *format,unsigned slot,BoxPokemon *mon) {
    buffer_resource(format,slot,238,GetBoxMonData(mon,MON_DATA_SPECIES,NULL));
}
static void native_buffer_player(MessageFormat *format,unsigned slot,PlayerProfile *profile) {
    GF_ASSERT(slot<NELEMS(format->names));GF_ASSERT(profile_text(profile,format->names[slot]));
}
/* Message JSON uses a private UTF-8 representation. Expand only the admitted
 * original name token; unknown controls and unbound slots fail preflight. */
static int expand_message(HGFieldScript *s,const uint8_t *text,size_t length,char *out,size_t capacity) {
    MessageFormat *format=&root_script(s)->format;
    size_t at=0,written=0;
    while(at+1<length) {
        if(text[at]=='{') {
            unsigned control=color_control(text+at,length-at);if(control){at+=control;continue;}
            if(length-at>=9&&!memcmp(text+at,"{YESNO 0}",9)){at+=9;continue;}
            unsigned slot=8,type=99;int consumed=0;
            if(sscanf((const char *)text+at,"{STRVAR_1 %u, %u, 0}%n",&type,&slot,&consumed)!=2||(type!=0&&type!=3&&type!=8&&type!=27&&type!=31)||!consumed||slot>=8||!format->names[slot][0])return 0;
            size_t n=strlen(format->names[slot]);if(n>=capacity-written)return 0;
            memcpy(out+written,format->names[slot],n);written+=n;at+=(size_t)consumed;
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
        if(s->scene) {
            int effect=s->services.face_player(s->scene);
            if(!effect)fail(s,"Cannot face interaction actor",op,(size_t)(ctx->script_ptr-s->code));
            s->face=effect==1;
        }
    }
    else if(op==73)s->sound=ScriptReadHalfword(ctx);
    else if(scene_command_admitted(op)&&(op!=97||s->scene)) {
        BOOL yielded=FALSE;
        if(!s->services.command(s->scene,ctx,op,&yielded))fail(s,"Unsupported scene command",op,(size_t)(ctx->script_ptr-s->code));
        return yielded;
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
    } else if(op==50||op==51) {
        if(!s->acknowledged){fail(s,"Button wait has no presented message",op,(size_t)(ctx->script_ptr-s->code));return TRUE;}
        s->acknowledged=0;
    } else {
        int remaining=operands(op);while(remaining-->0)(void)ScriptReadByte(ctx);
    }
    return FALSE;
}
static BOOL dispatch(ScriptContext *ctx) {
    HGFieldScript *s=(HGFieldScript *)(void *)ctx->fieldSystem;unsigned op=half(ctx->script_ptr-2);
    s->yielded=op==20&&half(ctx->script_ptr)==std_signpost?presentation(ctx,op):original_commands[op]?original_commands[op](ctx):presentation(ctx,op);
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
    if(!s->parent){s->waiters=0;s->context_count=1;memset(s->contexts,0,sizeof(s->contexts));s->contexts[0]=&s->context;}
    memset(s->special,0,sizeof(s->special));memset(&s->message,0,sizeof(s->message));
    memset(&s->format,0,sizeof(s->format));s->format_ptr=&s->format;s->movement_count=0;
    if(s->services.clone) {
        if(s->scene)s->services.destroy(s->scene);s->scene=s->services.clone(s->services.source);
        if(!s->scene){fail(s,"Cannot prepare script scene",0,0);return;}
        s->services.bind(s->scene,&s->field,s->preview);
    }
    s->menu_pending=0;s->menu_result=NULL;s->face=s->sound=s->total=0;s->waiting=s->acknowledged=s->yielded=s->done=s->failed=0;s->error[0]=0;
}
typedef struct { u8 temporary[8];u32 rng;HGFieldScript *active; } ScriptGlobals;
static void enter_globals(HGFieldScript *s,ScriptGlobals *saved) {
    saved->active=active_script;active_script=s;
    memcpy(saved->temporary,s->candidate.data.temporary,8);hg_story_temp_exchange(saved->temporary);
    saved->rng=GetLCRNGSeed();SetLCRNGSeed(s->candidate.data.rng);
}
static void leave_globals(HGFieldScript *s,ScriptGlobals *saved) {
    active_script=saved->active;
    s->candidate.data.rng=GetLCRNGSeed();SetLCRNGSeed(saved->rng);
    hg_story_temp_exchange(saved->temporary);memcpy(s->candidate.data.temporary,saved->temporary,8);
}
int hg_field_script_tick(HGFieldScript *s) {
    if(!s||s->failed)return HG_SCRIPT_ERROR;if(s->done)return HG_SCRIPT_DONE;
    if(s->owner->revision!=s->revision){fail(s,"Story state changed during script",0,0);return HG_SCRIPT_ERROR;}
    if(s->child) {
        int status=hg_field_script_tick(s->child);
        if(status==HG_SCRIPT_ERROR){fail(s,hg_field_script_error(s->child),20,0);return HG_SCRIPT_ERROR;}
        if(status==HG_SCRIPT_DONE) {
            if(root_script(s)->waiters&(1u<<s->context.id)){fail(s,"Standard script ended without returning",20,0);return HG_SCRIPT_ERROR;}
            if(!hg_story_commit(&s->candidate,&s->child->candidate,s->child->revision)){fail(s,"Cannot publish child script",20,0);return HG_SCRIPT_ERROR;}
            HGFieldScript *root=root_script(s);root->contexts[s->child->context.id]=NULL;root->context_count--;
            hg_field_script_free(s->child);s->child=NULL;return HG_SCRIPT_RUNNING;
        }
        return status;
    }
    if(s->waiting)return HG_SCRIPT_MESSAGE;
    if(s->menu_pending)return HG_SCRIPT_RUNNING;
    if(s->candidate.music.fade_ticks)s->candidate.music.fade_ticks--;
    if(s->scene) {
        ScriptGlobals saved;enter_globals(s,&saved);int ok=s->services.tick(s->scene);leave_globals(s,&saved);
        if(!ok){fail(s,"Scene task/movement failed",94,0);return HG_SCRIPT_ERROR;}
        int busy=s->services.busy?s->services.busy(s->scene):0;
        if(busy) {
            if(busy==1&&++s->total>16384){fail(s,"Scene task budget exceeded",0,0);return HG_SCRIPT_ERROR;}
            return HG_SCRIPT_RUNNING;
        }
    }
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
        ScriptGlobals saved;enter_globals(s,&saved);
        s->yielded=0;int native=s->context.mode==SCRIPT_MODE_NATIVE;RunScriptCommand(&s->context);
        if(patched)memcpy(s->code+relocated,saved_operand,4);
        if(s->context.script_ptr) {
            uintptr_t position=(uintptr_t)s->context.script_ptr,base=(uintptr_t)s->code;size_t mirror=s->size+16;
            if(position>=base+mirror&&position<base+mirror+s->size)s->context.script_ptr-=mirror;
            else if(position<base||position>base+s->size)fail(s,"Script target outside arena",0,0);
        }
        if(s->context.mode==SCRIPT_MODE_BYTECODE&&!s->context.script_ptr)RunScriptCommand(&s->context);
        leave_globals(s,&saved);
        if(s->failed)return HG_SCRIPT_ERROR;
        if(s->context.mode==SCRIPT_MODE_STOPPED) {
            if(s->scene&&(s->services.active(s->scene)||(s->services.busy&&s->services.busy(s->scene)))){fail(s,"Unjoined scene task/movement",94,0);return HG_SCRIPT_ERROR;}
            s->done=1;return HG_SCRIPT_DONE;
        }
        if(s->waiting)return HG_SCRIPT_MESSAGE;
        if(s->menu_pending)return HG_SCRIPT_RUNNING;
        if(native||s->yielded)break;
    }
    return HG_SCRIPT_RUNNING;
}
int hg_field_script_ack(HGFieldScript *s) {
    if(s&&s->child)return hg_field_script_ack(s->child);
    if(!s||s->failed||s->done||!s->waiting)return 0;s->waiting=0;s->acknowledged=1;s->message.text[0]=0;return 1;
}
const HGStoryState *hg_field_script_candidate(const HGFieldScript *s) { return s&&s->done&&!s->failed?&s->candidate:NULL; }
int hg_field_script_commit(HGFieldScript *s) {
    if(!s||!s->done||s->failed||s->preview||s->committed||!hg_story_commit(s->owner,&s->candidate,s->revision))return 0;
    s->committed=1;return 1;
}
const HGScriptMessage *hg_field_script_message(const HGFieldScript *s) { return s&&s->child?hg_field_script_message(s->child):s&&s->waiting&&!s->failed?&s->message:NULL; }
const char *hg_field_script_error(const HGFieldScript *s) { return s?s->error:"Missing script"; }
void hg_field_script_free(HGFieldScript *s) { if(s){hg_field_script_free(s->child);if(s->scene)s->services.destroy(s->scene);free(s->code);free(s);} }
const void *hg_field_script_scene(const HGFieldScript *s) { return s?s->scene:NULL; }
static HGFieldScript *script_bytes(HGStoryState *owner,const uint8_t *bytes,size_t size,size_t floor,size_t start,unsigned bank,const HGScriptSceneServices *services,char *error,size_t capacity) {
    if(error&&capacity)error[0]=0;
    if(!owner||!bytes||size>1024*1024||start>=size)return NULL;
    if(services&&(!services->source||!services->clone||!services->destroy||!services->bind||!services->apply||
       !services->tick||!services->active||!services->command||!services->face_player))return NULL;
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
/* Native scenario input shares the same preflight/replay boundary as NARC banks. */
HGFieldScript *hg_field_script_scene_bytes(HGStoryState *owner,const uint8_t *bytes,size_t size,unsigned bank,const HGScriptSceneServices *services,char *error,size_t capacity) {
    return script_bytes(owner,bytes,size,0,0,bank,services,error,capacity);
}
HGFieldScript *hg_field_script_bytes(HGStoryState *owner,const uint8_t *bytes,size_t size,size_t start,unsigned bank,char *error,size_t capacity) {
    return script_bytes(owner,bytes,size,0,start,bank,NULL,error,capacity);
}
static HGFieldScript *script_bank_scene(HGStoryState *owner,unsigned bank,unsigned id,unsigned messages,const HGScriptSceneServices *services,char *error,size_t capacity) {
    ScriptResource resource;if(!id||!bank_resource(bank,id-1,messages,&resource))return NULL;
    return script_bytes(owner,resource.bytes,resource.size,resource.floor,resource.start,resource.messages,services,error,capacity);
}
static HGFieldScript *script_bank(HGStoryState *owner,unsigned bank,unsigned id,unsigned messages,char *error,size_t capacity) {
    return script_bank_scene(owner,bank,id,messages,NULL,error,capacity);
}
HGFieldScript *hg_field_script_scene_new(HGStoryState *owner,unsigned map,unsigned id,const HGScriptSceneServices *services,char *error,size_t capacity) {
    if(map>=MAP_ID_MAX)return NULL;
    if(id>=_std_misc){ScriptResource r;if(!standard_resource(id,&r))return NULL;return script_bytes(owner,r.bytes,r.size,r.floor,r.start,r.messages,services,error,capacity);}
    return script_bank_scene(owner,MapHeader_GetScriptsBank(map),id,MapHeader_GetMsgBank(map),services,error,capacity);
}
HGFieldScript *hg_field_script_new(HGStoryState *owner,unsigned map,unsigned id,char *error,size_t capacity) {
    if(map>=MAP_ID_MAX)return NULL;
    if(id>=_std_misc){ScriptResource r;if(!standard_resource(id,&r))return NULL;return script_bytes(owner,r.bytes,r.size,r.floor,r.start,r.messages,NULL,error,capacity);}
    return script_bank(owner,MapHeader_GetScriptsBank(map),id,MapHeader_GetMsgBank(map),error,capacity);
}
HGStoryState *hg_story_new(void) {
    HGStoryState *state=calloc(1,sizeof(*state));if(!state)return NULL;
    state->data.blackout_spawn=SPAWN_NEW_BARK;Save_VarsFlags_Init(&state->data.vars);PlayerProfile_Init(&state->data.profile);
    Party_InitWithMaxSize(&state->data.party,PARTY_SIZE);
    Save_Bag_Init(&state->data.bag);GameStats_Init(&state->data.stats);Save_Pokedex_Init(&state->data.dex);Options_Init(&state->data.options);SaveData_Pokegear_Init(&state->data.pokegear);
    ApricornTrees_Init(state->data.trees);sub_0202AE0C(state->data.trees); /* Original new-game availability. */
    /* Original NewGame_InitSaveData values, applied through original owners.
     * This is admitted record initialization, not the full save-array startup. */
    PlayerProfile_SetMoney(&state->data.profile,3000);
    Save_VarsFlags_SetFishingCompetitionLengthRecord(&state->data.vars,56150);
    SetFlag960(&state->data.vars);
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
    state->data=candidate->data;state->music=candidate->music;state->revision++;return 1;
}
HGStoryMusic hg_story_music(const HGStoryState *state) { return state?state->music:(HGStoryMusic){0}; }
int hg_story_phone_registered(const HGStoryState *state,unsigned contact) {
    return state && contact<NUM_PHONE_CONTACTS && SavePokegear_IsNumberRegistered((SavePokegear *)&state->data.pokegear,(u8)contact)!=0xff;
}
int hg_story_population_changed(const HGStoryState *a,const HGStoryState *b) {
    if(!a||!b)return 1;
    return memcmp(a->data.trees,b->data.trees,sizeof(a->data.trees))||memcmp(a->data.vars.flags,b->data.vars.flags,sizeof(a->data.vars.flags))||
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
static int story_enter(HGStoryState *state,unsigned map,int load,char *error,size_t size) {
    if(!state)return 0;HGStoryState before=*state;FieldSystem field={0};hg_story_bind_field(state,&field);ClearTempFieldEventData(&field);state->revision++;
    const unsigned phases[]={INIT_SCRIPT_ON_TRANSITION,INIT_SCRIPT_ON_LOAD};
    for(unsigned i=0;i<(load?NELEMS(phases):1);i++) {
        unsigned entry;if(!hg_story_map_script(state,map,phases[i],&entry)){*state=before;return 0;}
        if(entry==65535)continue;
        HGFieldScript *script=hg_field_script_new(state,map,entry,error,size);if(!script){*state=before;return 0;}
        int status;do{status=hg_field_script_tick(script);}while(status==HG_SCRIPT_RUNNING);
        if(status==HG_SCRIPT_DONE&&!hg_field_script_commit(script))status=HG_SCRIPT_ERROR;
        hg_field_script_free(script);if(status!=HG_SCRIPT_DONE){*state=before;return 0;}
    }
    return 1;
}
int hg_story_enter_map(HGStoryState *state,unsigned map,char *error,size_t size) { return story_enter(state,map,1,error,size); }
int hg_story_enter_zone(HGStoryState *state,unsigned map,char *error,size_t size) { return story_enter(state,map,0,error,size); }

static const HGFieldScript *menu_leaf(const HGFieldScript *s) { while(s&&s->child)s=s->child;return s; }
int hg_field_script_menu_pending(const HGFieldScript *s) { s=menu_leaf(s);return s&&s->menu_pending; }
static const u8 *fork_pointer(const HGFieldScript *source,HGFieldScript *copy,const u8 *pointer) {
    if(!pointer)return NULL;uintptr_t at=(uintptr_t)pointer,base=(uintptr_t)source->code;
    if(at<base||at>=base+2*source->size+16)return NULL;return copy->code+(at-base);
}
/* A continuation fork owns its code, scene, state and child contexts. It starts
 * at the actual choice point, preserving the acquired Pokemon and prior input,
 * instead of replaying a guessed default starter from the script's beginning. */
static HGFieldScript *fork_script(const HGFieldScript *source,HGFieldScript *parent) {
    HGFieldScript *copy=malloc(sizeof(*copy));if(!copy)return NULL;*copy=*source;
    copy->parent=parent;copy->child=NULL;copy->scene=NULL;copy->code=NULL;copy->committed=0;copy->preview=1;
    if(parent)copy->owner=&parent->candidate;
    copy->code=malloc(2*copy->size+16);if(!copy->code){hg_field_script_free(copy);return NULL;}
    memcpy(copy->code,source->code,2*copy->size+16);
    copy->context.script_ptr=fork_pointer(source,copy,source->context.script_ptr);
    for(unsigned i=0;i<20;i++)copy->context.stack[i]=fork_pointer(source,copy,source->context.stack[i]);
    copy->context.fieldSystem=&copy->field;copy->field.saveData=(SaveData *)(void *)&copy->candidate.data;copy->format_ptr=&copy->format;
    copy->field.native_starter=NULL;
    if(source->scene) {
        copy->scene=source->services.clone(source->scene);
        if(!copy->scene){hg_field_script_free(copy);return NULL;}
        copy->services.bind(copy->scene,&copy->field,1);
    } else if(parent)copy->field.location=parent->field.location;
    if(source->menu_result) {
        uintptr_t at=(uintptr_t)source->menu_result,base=(uintptr_t)source->context.data;
        if(at<base||at+sizeof(u32)>base+sizeof(source->context.data)||at%_Alignof(u32)){hg_field_script_free(copy);return NULL;}
        copy->menu_result=(u32 *)((uint8_t *)copy->context.data+(at-base));
    }
    if(source->child) {copy->child=fork_script(source->child,copy);if(!copy->child){hg_field_script_free(copy);return NULL;}}
    return copy;
}
int hg_field_script_menu_choice_supported(HGFieldScript *s,unsigned choice) {
    HGFieldScript *leaf=(HGFieldScript *)menu_leaf(s);if(!leaf||!leaf->menu_pending||choice>1)return 0;
    if(!leaf->menu_checked) {
        HGFieldScript *root=root_script(s);leaf->menu_checked=1;leaf->menu_choices=0;
        for(unsigned option=0;option<2;option++) {
            HGFieldScript *copy=fork_script(root,NULL);if(!copy)continue;
            memset(copy->contexts,0,sizeof(copy->contexts));
            for(HGFieldScript *node=copy;node;node=node->child)copy->contexts[node->context.id]=&node->context;
            HGFieldScript *target=(HGFieldScript *)menu_leaf(copy);target->menu_pending=0;*target->menu_result=option;
            int status=HG_SCRIPT_RUNNING;
            for(unsigned ticks=0;ticks<32768&&status!=HG_SCRIPT_DONE&&status!=HG_SCRIPT_ERROR;ticks++) {
                status=hg_field_script_tick(copy);if(status==HG_SCRIPT_MESSAGE)hg_field_script_ack(copy);
            }
            if(status==HG_SCRIPT_DONE)leaf->menu_choices|=1u<<option;
            hg_field_script_free(copy);
        }
    }
    return !!(leaf->menu_choices&(1u<<choice));
}
int hg_field_script_menu_choose(HGFieldScript *s,unsigned choice) {
 if(!hg_field_script_menu_choice_supported(s,choice))return 0;s=(HGFieldScript *)menu_leaf(s);*s->menu_result=choice;s->menu_pending=0;return 1;
}

static ScriptContext *native_create_context(FieldSystem *field,u16 id) {
 HGFieldScript *parent=(HGFieldScript *)(void *)field;HGFieldScript *root=root_script(parent);
 ScriptResource resource;GF_ASSERT(!parent->child&&root->context_count<3&&standard_resource(id,&resource));
 const uint8_t *bytes=resource.bytes;size_t size=resource.size,start=resource.start;
 HGFieldScript *child=calloc(1,sizeof(*child));GF_ASSERT(child);child->code=calloc(1,2*size+16);GF_ASSERT(child->code);memcpy(child->code,bytes,size);
 child->owner=&parent->candidate;child->revision=parent->candidate.revision;child->parent=parent;child->bytes=bytes;child->size=size;child->floor=resource.floor;child->start=start;child->message_bank=resource.messages;child->preview=parent->preview;
 initialize(child);child->field.location=parent->field.location;parent->child=child;return &child->context;
}

int hg_story_apricorn_tree(const HGStoryState *state,unsigned index,unsigned *kind,int *available) {
    if(!state||index>=NUM_APRICORN_TREE||!kind||!available)return 0;
    *kind=ApricornTrees_TryGetApricorn(state->data.trees,(int)index);*available=sub_0202AE38(state->data.trees,(int)index)!=0;return 1;
}
