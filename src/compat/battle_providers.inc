static int native_battle_medicine(unsigned);
static unsigned native_trainer_party(HGBattle *,unsigned);
static int tutorial_select(HGBattle *);
static unsigned native_trainer_command(HGBattle *,unsigned);
static unsigned native_trainer_move(HGBattle *,unsigned);
static void format_battle_message(HGBattle *,const BattleMessage *);
/* Native owner services. Original battle emitters produce all event packets. */
static void BattleController_SendLocalMessage(BattleSystem *sys,int recipient,int battler,void *bytes,u8 size) {
    HGBattle *b=(HGBattle *)sys;GF_ASSERT(battler>=0&&battler<4&&size);
    if(recipient==0){memcpy(sys->ctx->battleBuffer[battler],bytes,size);return;}
    GF_ASSERT(recipient==1&&b->count<NELEMS(b->events));unsigned at=(b->read+b->count++)%NELEMS(b->events);
    b->events[at].presented=0;b->events[at].battler=battler;b->events[at].size=size;memcpy(b->events[at].bytes,bytes,size);
}
void *Heap_Alloc(enum HeapID heap,u32 size) {
    (void)heap;void *p=calloc(1,size);GF_ASSERT(p);
    unsigned slot=0;while(slot<NELEMS(running_battle->allocations)&&running_battle->allocations[slot])slot++;
    if(slot==NELEMS(running_battle->allocations)){free(p);battle_boundary_failed("allocation budget");}
    running_battle->allocations[slot]=p;
    if(size==sizeof(BattleContext)&&running_battle&&!running_battle->system.ctx)running_battle->system.ctx=p;return p;
}
void ReadWholeNarcMemberByIdPair(void *out,NarcId id,s32 member) {const uint8_t *bytes;size_t size;if(!native_resource_member(id,0,member,&bytes,&size))battle_boundary_failed("battle archive member");memcpy(out,bytes,size);}
u32 GetNarcMemberSizeByIdPair(NarcId id,s32 member) {const uint8_t *bytes;size_t size;if(!native_resource_member(id,0,member,&bytes,&size))battle_boundary_failed("battle archive size");return (u32)size;}
void MI_CpuClear32(void *out,u32 size){memset(out,0,size);}
u32 MaskOfFlagNo(int bit){GF_ASSERT(bit>=0&&bit<32);return 1u<<bit;}
int LowestFlagNo(u32 value){return value?__builtin_ctz(value):32;}
u8 ov12_02261258(OpponentData *data){return data->battlerType;}
u32 ov12_02261264(OpponentData *data){return data->native_controller;}
void ov12_02261294(OpponentData *data,s32 unused){(void)data;(void)unused;}
void PlaySE(u16 id){if(running_battle->audio&&!ds_audio_play(running_battle->audio,DS_AUDIO_EFFECT,id))battle_boundary_failed("sound effect");}
void PlayBGM(u16 id){if(running_battle->audio&&!ds_audio_play(running_battle->audio,DS_AUDIO_BGM,id))battle_boundary_failed("battle music");}
void PlayFanfare(u16 id){if(running_battle->audio&&!ds_audio_play(running_battle->audio,DS_AUDIO_FANFARE,id))battle_boundary_failed("fanfare");}
BOOL IsFanfarePlaying(void){return ds_audio_playing(running_battle->audio,DS_AUDIO_FANFARE,0);}
u8 GetMonPicHeightBySpeciesGenderForm(u16 species,u8 gender,u8 facing,u8 form,u32 shiny){(void)species;(void)gender;(void)facing;(void)form;(void)shiny;return 0; /* Native framing owns sprite placement. */}
static const char *mon_name(HGBattle *b,unsigned battler){return hg_pokemon_species_name(b->system.ctx->battleMons[battler].species);}
static void finish_event(HGBattle *b){GF_ASSERT(b->count);unsigned command=b->events[b->read].bytes[0];ov12_0224ED00(b->system.ctx,0,b->events[b->read].battler,command);b->read=(b->read+1)%NELEMS(b->events);b->count--;}
static void process_event(HGBattle *b) {
    unsigned command=b->events[b->read].bytes[0],side=b->events[b->read].battler;BattleContext *ctx=b->system.ctx;
    switch(command){
    case BATTLE_COMMAND_SET_COMMAND_SELECTION:
        if(side){ctx->battleBuffer[side][0]=(b->system.battleType&BATTLE_TYPE_TRAINER)?native_trainer_command(b,side):BATTLE_INPUT_FIGHT;break;}
        if(b->system.battleType&BATTLE_TYPE_TUTORIAL){
            if(!b->events[b->read].presented){BattleMessage message={0};message.id=b->system.unk2421?1226+b->profiles[0].gender:921;message.tag=b->system.unk2421?TAG_NONE:TAG_NICKNAME;message.param[0]=0;format_battle_message(b,&message);b->events[b->read].presented=1;b->waiting=b->direct_message=1;return;}
            int action=tutorial_select(b);if(action<0)return;ctx->battleBuffer[0][0]=action==CURSOR_INPUT_FIGHT?BATTLE_INPUT_FIGHT:BATTLE_INPUT_BAG;break;}
        snprintf(b->text,sizeof(b->text),"What will %s do?",mon_name(b,0));b->menu=1;return;
    case BATTLE_COMMAND_SHOW_MOVE_SELECT_MENU:
        if(side){if(b->system.battleType&BATTLE_TYPE_TRAINER){ctx->battleBuffer[side][0]=native_trainer_move(b,side)+1;break;}unsigned legal=StruggleCheck(&b->system,ctx,side,0,-1);unsigned moves[4],count=0;for(unsigned i=0;i<4;i++)if(!(legal&(1u<<i)))moves[count++]=i+1;GF_ASSERT(count);ctx->battleBuffer[side][0]=moves[BattleSystem_Random(&b->system)%count];break;}
        if(b->system.battleType&BATTLE_TYPE_TUTORIAL){int action=tutorial_select(b);if(action<0)return;GF_ASSERT(action==CURSOR_INPUT_MOVE_1);ctx->battleBuffer[0][0]=1;ov12_0223BB10(&b->system,1);break;}
        b->menu=2;return;
    case BATTLE_COMMAND_SHOW_BAG_MENU:{
        GF_ASSERT(side==0);
        if(!(b->system.battleType&BATTLE_TYPE_TUTORIAL)&&!b->direct_ball){
            b->bag_count=0;b->menu_hint[0]=0;
            for(unsigned item=1;item<ITEMS_COUNT;item++)if(native_battle_medicine(item)&&Bag_GetQuantity(&b->bag,item,HEAP_ID_BATTLE))b->bag_items[b->bag_count++]=(u16)item;
            b->menu=4;return;
        }
        GF_ASSERT((b->system.battleType&BATTLE_TYPE_TUTORIAL)||hg_battle_can_throw(b));BattleItemUse use={.item=ITEM_POKE_BALL,.category=BTLPOCKETLIST_BALLS,.target=0};
        GF_ASSERT(Bag_GetQuantity(&b->bag,ITEM_POKE_BALL,HEAP_ID_BATTLE));BattleController_EmitSelectedBagItem(&b->system,side,use);break;}
    case BATTLE_COMMAND_SHOW_PARTY_MENU:{
        PartyMenuMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));
        GF_ASSERT(m.battler==side&&m.listMode<=1);
        if(side){GF_ASSERT(!m.canSwitch);BattleController_EmitPartyMenuResult(&b->system,side,native_trainer_party(b,side)+1);break;}
        GF_ASSERT(side==0);
        b->menu_battler=side;b->party_forced=m.listMode==1;b->party_cannot_switch=m.canSwitch;b->party_excluded=m.doublesSelection;b->menu_hint[0]=0;b->menu=7;return;}
    case BATTLE_COMMAND_SHOW_YES_NO_MENU:{
        YesNoMenuMessage prompt;GF_ASSERT(b->events[b->read].size==sizeof(prompt));memcpy(&prompt,b->events[b->read].bytes,sizeof(prompt));
        GF_ASSERT(side==0);
        if(prompt.yesNoType==3||prompt.yesNoType==4){GF_ASSERT(!prompt.promptMsg);b->yesno_type=prompt.yesNoType;b->menu=8;return;}
        GF_ASSERT(prompt.yesNoType==5&&prompt.promptMsg==0x364);
        snprintf(b->text,sizeof(b->text),"Keep %s's original name?",mon_name(b,1));b->menu=3;return;}
    case BATTLE_COMMAND_OPEN_CAPTURE_BALL:break;
    case BATTLE_COMMAND_SHOW_POKEMON:
        /* Original breakout reuses send-out. End the native capture visual
         * when that command is presented, before its acknowledgement. */
        if(side==BATTLER_ENEMY)b->capture_phase=0;
        break;
    case BATTLE_COMMAND_PRINT_ATTACK_MESSAGE:{
        AttackMsgMessage message;GF_ASSERT(b->events[b->read].size==sizeof(message));memcpy(&message,b->events[b->read].bytes,sizeof(message));
        const uint8_t *name;size_t size;GF_ASSERT(native_resource_member(NARC_msgdata_msg,750,message.move,&name,&size));snprintf(b->text,sizeof(b->text),"%s used\n%s!",mon_name(b,side),(const char *)name);b->waiting=1;return;}
    /* Single-battle message bindings from original ov12_02261544/615F0.
     * Keep their original message bank and shared side/tag formatting. */
    case BATTLE_COMMAND_PRINT_ENCOUNTER_MESSAGE:{
        BattleMessage msg={0};GF_ASSERT(b->system.maxBattlers==2);
        if(b->system.battleType&BATTLE_TYPE_TRAINER){msg.id=969;msg.tag=TAG_TRCLASS_TRNAME;msg.param[0]=msg.param[1]=1;}
        else {msg.id=965;msg.tag=TAG_NICKNAME|0x80;msg.param[0]=1|(ctx->selectedMonIndex[1]<<8);}
        format_battle_message(b,&msg);b->waiting=1;return;}
    case BATTLE_COMMAND_PRINT_LEAD_MON_MESSAGE:{
        BattleMessage msg={0};GF_ASSERT(b->system.maxBattlers==2);
        if(side==1){GF_ASSERT(b->system.battleType&BATTLE_TYPE_TRAINER);msg.id=972;msg.tag=TAG_TRCLASS_TRNAME_NICKNAME;msg.param[0]=msg.param[1]=side;msg.param[2]=side|(ctx->selectedMonIndex[side]<<8);}
        else {msg.id=979;msg.tag=TAG_NICKNAME|0x80;msg.param[0]=side|(ctx->selectedMonIndex[side]<<8);}
        format_battle_message(b,&msg);b->waiting=1;return;}
    /* Native message binding of original single-battle formatters
     * ov12_02261390 / ov12_02261464; preserve original HP thresholds/tags. */
    case BATTLE_COMMAND_SET_STATUS_ICON:{
        /* Native HUD reads the original BattleMon status updated by the script. */
        SetStatusIconMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));break;}
    case BATTLE_COMMAND_PRINT_RETURN_MESSAGE:{
        ReturnMsgMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));BattleMessage msg={0};
        if(side){msg.id=989;msg.tag=TAG_TRCLASS_TRNAME_NICKNAME;msg.param[0]=msg.param[1]=side;msg.param[2]=side|(m.partySlot<<8);}
        else {msg.id=!m.hpPercent?984:m.hpPercent<25?985:m.hpPercent<50?988:m.hpPercent<75?986:987;msg.tag=TAG_NICKNAME;msg.param[0]=side|(m.partySlot<<8);}
        format_battle_message(b,&msg);b->waiting=1;return;}
    case BATTLE_COMMAND_PRINT_SEND_OUT_MESSAGE:{
        SendOutMsgMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));BattleMessage msg={0};
        if(side){msg.id=972;msg.tag=TAG_TRCLASS_TRNAME_NICKNAME;msg.param[0]=msg.param[1]=side;msg.param[2]=side|(m.partySlot<<8);}
        else {msg.id=m.hpPercent<100?982:m.hpPercent<325?983:m.hpPercent<550?981:m.hpPercent<775?980:979;msg.tag=TAG_NICKNAME;msg.param[0]=side|(m.partySlot<<8);}
        format_battle_message(b,&msg);b->waiting=1;return;}
    case BATTLE_COMMAND_PRINT_TRAINER_MESSAGE:{
        TrainerMsgMessage msg;GF_ASSERT(b->events[b->read].size==sizeof(msg));memcpy(&msg,b->events[b->read].bytes,sizeof(msg));
        const uint8_t *table,*text;size_t length,text_length;GF_ASSERT(native_resource_member(NARC_poketool_trmsg_trtbl,0,0,&table,&length));
        unsigned trainer=b->system.trainerId[side];int found=0;
        for(size_t at=0;at+4<=length;at+=4){u16 id,kind;memcpy(&id,table+at,2);memcpy(&kind,table+at+2,2);if(id==trainer&&kind==msg.msg){
            GF_ASSERT(native_resource_member(NARC_msgdata_msg,728,(unsigned)(at/4),&text,&text_length));GF_ASSERT(!memchr(text,'{',text_length));snprintf(b->text,sizeof(b->text),"%s",text);found=1;break;}}
        GF_ASSERT(found);b->waiting=1;return;}
    case BATTLE_COMMAND_PRINT_MESSAGE:{BattleMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));format_battle_message(b,&m);b->waiting=1;return;}
    case BATTLE_COMMAND_67:{
        Message_022645C8 fade;GF_ASSERT(b->events[b->read].size==sizeof(fade));memcpy(&fade,b->events[b->read].bytes,sizeof(fade));GF_ASSERT(fade.unk1<=1);
        if(!b->events[b->read].presented){b->events[b->read].presented=1;b->blackout_ticks=0;if(fade.unk1)ds_audio_fade(b->audio,0,16);}
        unsigned frame=b->blackout_ticks++;if(frame>16)break;
        if((int)frame>b->blackout_brightness)b->blackout_brightness=frame;return;}

    case BATTLE_COMMAND_PLAY_SOUND:{PlaySoundMessage msg;GF_ASSERT(b->events[b->read].size==sizeof(msg));memcpy(&msg,b->events[b->read].bytes,sizeof(msg));PlaySE(msg.sdatID);break;}
    case BATTLE_COMMAND_PRINT_ESCAPE_MESSAGE:{
        EscapeMsgMessage msg;GF_ASSERT(b->events[b->read].size==sizeof(msg));memcpy(&msg,b->events[b->read].bytes,sizeof(msg));
        GF_ASSERT(!(b->system.battleType&~BATTLE_TYPE_BUG_CONTEST)&&msg.escaperBitmask==1);
        /* Original single-player branch of ov12_022619E4. */
        BattleMessage text={.id=781,.tag=TAG_NONE};BattleSystem_SetBattleOutcomeFlags(&b->system,0xc2);format_battle_message(b,&text);b->waiting=1;return;}
    case BATTLE_COMMAND_INCREMENT_RECORD:{RecordIncrementMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));
        if((m.battlerType!=0)==(b->opponents[side].native_controller!=0))BattleSystem_GameStatIncrement(&b->system,m.record);break;}
    case BATTLE_COMMAND_FLY_MOVE_HIT_SOUND_EFFECT:{
        MoveHitSoundMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));GF_ASSERT(m.effectiveness<=2);
        static const u16 effects[]={0x6fe,0x6fd,0x6fc};PlaySE(effects[m.effectiveness]);break;}
    case BATTLE_COMMAND_PLAY_MUSIC:{MusicPlayMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));PlayBGM(m.bgmID);break;}
    case BATTLE_COMMAND_UPDATE_PARTY_MON:{UpdatePartyMonMessage m;GF_ASSERT(b->events[b->read].size==sizeof(m));memcpy(&m,b->events[b->read].bytes,sizeof(m));Pokemon *mon=BattleSystem_GetPartyMon(&b->system,side,m.partySlot);SetMonData(mon,MON_DATA_HP,&m.curHP);SetMonData(mon,MON_DATA_STATUS,&m.status);for(unsigned i=0;i<4;i++)SetMonData(mon,MON_DATA_MOVE1_PP+i,m.ppCur+i);break;}
    case BATTLE_COMMAND_SETUP_UI:case BATTLE_COMMAND_SET_ENCOUNTER:case BATTLE_COMMAND_SHOW_ENCOUNTER:
    case BATTLE_COMMAND_SET_TRAINER_ENCOUNTER:case BATTLE_COMMAND_RETURN_POKEMON:
    case BATTLE_COMMAND_DELETE_POKEMON:case BATTLE_COMMAND_THROW_TRAINER_BALL:case BATTLE_COMMAND_SLIDE_TRAINER_OUT:
    case BATTLE_COMMAND_SLIDE_TRAINER_IN:case BATTLE_COMMAND_SLIDE_HEALTHBOX_IN:case BATTLE_COMMAND_SLIDE_HEALTHBOX_OUT:
    case BATTLE_COMMAND_SET_MOVE_ANIMATION:case BATTLE_COMMAND_FLICKER_BATTLER:case BATTLE_COMMAND_UPDATE_HP_GAUGE:
    case BATTLE_COMMAND_FADE_OUT:case BATTLE_COMMAND_SLIDE_IN_BACKGROUND:case BATTLE_COMMAND_SHOW_BATTLE_START_PARTY_GAUGE:
    case BATTLE_COMMAND_HIDE_BATTLE_START_PARTY_GAUGE:case BATTLE_COMMAND_SHOW_PARTY_GAUGE:case BATTLE_COMMAND_HIDE_PARTY_GAUGE:
    case BATTLE_COMMAND_LOAD_PARTY_GAUGE_GRAPHICS:case BATTLE_COMMAND_FREE_PARTY_GAUGE_GRAPHICS:
    /* Native view reads current battle state; original transition animations
     * are not reproduced. These packets require no additional state mutation. */
    case BATTLE_COMMAND_SPRITE_TO_OAM:case BATTLE_COMMAND_OAM_TO_SPRITE:case BATTLE_COMMAND_UPDATE_BG:case BATTLE_COMMAND_STOP_GAUGE_ANIMATION:case BATTLE_COMMAND_PLAY_FAINTING_SEQUENCE:case BATTLE_COMMAND_UPDATE_EXP_GAUGE:case BATTLE_COMMAND_PLAY_LEVEL_UP_ANIMATION:case BATTLE_COMMAND_REFRESH_HP_GAUGE:
    case BATTLE_COMMAND_CLEAR_MESSAGE_BOX:case BATTLE_COMMAND_CLEAR_TOUCH_SCREEN:break;
    default:{char missing[48];snprintf(missing,sizeof(missing),"presentation command %u",command);battle_boundary_failed(missing);}
    }
    finish_event(b);
}
static void BattleSystem_LoadLevelUpNameplate(BattleSystem *s,GetterWork *w,Pokemon *m){(void)s;(void)w;(void)m;battle_boundary_failed("level-up nameplate");}
static void BattleSystem_UnloadLevelUpNameplate(BattleSystem *s,GetterWork *w){(void)s;(void)w;battle_boundary_failed("level-up nameplate");}


static u32 native_sqrt_input;
void CP_SetSqrt32(u32 value){native_sqrt_input=value;}
void CP_WaitSqrt(void){} /* Synchronous host integer square root. */
u32 CP_GetSqrtResult32(void){return (u32)sqrt((double)native_sqrt_input);}
void G2_SetBG0Priority(int value){GF_ASSERT(value>=0&&value<4);running_battle->background.priorities[0]=value;}
void sub_02074F9C(BattleSystem *s,int recipient,int battler,void *message,u8 size){(void)s;(void)recipient;(void)battler;(void)message;(void)size;battle_boundary_failed("link battle transport");}
HGMonString *hg_battle_NewString_ReadMsgData(MsgData *data,s32 id){(void)data;(void)id;battle_boundary_failed("encoded battle message");}

BOOL System_GetTouchNew(void){BOOL pressed=running_battle->input_pending;running_battle->input_pending=0;return pressed;}

/* Original overlay switch unloads 7/loads 10 on entry to selection, and
 * reverses that after selection. Native code is resident; preserve the observed
 * phase and reject unsupported trainer AI rather than inventing its result. */
void ov12_02237ED0(BattleSystem *system,int selection) {
    GF_ASSERT(selection==0 || selection==1);
    GF_ASSERT(system->unk2445!=selection);
    system->unk2445=selection;
}

/* Original recording buffer uses four 1024-byte battler lanes. Keep the
 * storage with its battle rather than the DS singleton _021D2AF8. */
void sub_02030260(int battler,u32 offset,u8 value){GF_ASSERT(battler>=0&&battler<4&&offset<1024);running_battle->recorded_inputs[battler][offset]=value;}
u8 sub_0203027C(int battler,u32 offset){GF_ASSERT(battler>=0&&battler<4&&offset<1024);return running_battle->recorded_inputs[battler][offset];}

void Heap_Free(void *p){
    if(running_battle)for(unsigned i=0;i<NELEMS(running_battle->allocations);i++)if(running_battle->allocations[i]==p){running_battle->allocations[i]=NULL;break;}
    free(p);
}
SysTask *SysTask_CreateOnMainQueue(SysTaskFunc function,void *data,u32 priority){
    GF_ASSERT(running_battle->tasks);SysTask *task=SysTaskQueue_InsertTask(running_battle->tasks,function,data,priority);GF_ASSERT(task);return task;
}

#include "charmap.inc"
static void buffer_message_resource(MessageFormat *format,u32 slot,unsigned bank,unsigned member){
    const uint8_t *bytes;size_t size;GF_ASSERT(slot<NELEMS(format->names));
    GF_ASSERT(native_resource_member(NARC_msgdata_msg,bank,member,&bytes,&size)&&size&&size<=sizeof(format->names[slot])&&bytes[size-1]==0);
    memcpy(format->names[slot],bytes,size);
}
void BufferMoveName(MessageFormat *f,u32 slot,u32 move){buffer_message_resource(f,slot,750,move);}
void BufferItemName(MessageFormat *f,u32 slot,u32 item){buffer_message_resource(f,slot,222,item);}
void BufferAbilityName(MessageFormat *f,u32 slot,u32 ability){buffer_message_resource(f,slot,720,ability);}
void BufferTypeName(MessageFormat *f,u32 slot,u32 type){buffer_message_resource(f,slot,735,type);}
void BufferStatName(MessageFormat *f,u32 slot,u32 stat){buffer_message_resource(f,slot,443,stat);}
void BufferStatusName(MessageFormat *f,u32 slot,u32 status){buffer_message_resource(f,slot,44,status);}
void BufferBoxMonSpeciesName(MessageFormat *f,u32 slot,BoxPokemon *mon){buffer_message_resource(f,slot,237,GetBoxMonData(mon,MON_DATA_SPECIES,NULL));}
static void buffer_encoded_name(MessageFormat *f,u32 slot,const u16 *encoded,unsigned count){
    GF_ASSERT(slot<NELEMS(f->names));size_t written=0;
    for(unsigned i=0;i<count;i++){
        if(encoded[i]==0xffff){f->names[slot][written]=0;return;}
        u32 cp=0;for(unsigned j=0;j<NELEMS(glyph_map);j++)if(glyph_map[j].glyph==encoded[i]){cp=glyph_map[j].unicode;break;}
        if(!cp||cp>=65536||written+3>=sizeof(f->names[slot])){char error[96];snprintf(error,sizeof(error),"encoded name glyph %04x at %u",encoded[i],i);battle_boundary_failed(error);}
        if(cp<128)f->names[slot][written++]=cp;
        else if(cp<2048){f->names[slot][written++]=0xc0|(cp>>6);f->names[slot][written++]=0x80|(cp&63);}
        else {f->names[slot][written++]=0xe0|(cp>>12);f->names[slot][written++]=0x80|((cp>>6)&63);f->names[slot][written++]=0x80|(cp&63);}
    }
    battle_boundary_failed("unterminated nickname");
}
void BufferBoxMonNickname(MessageFormat *f,u32 slot,BoxPokemon *mon){u16 name[11];GetBoxMonData(mon,MON_DATA_NICKNAME,name);buffer_encoded_name(f,slot,name,NELEMS(name));}
void BufferTrainerNameFromDataStruct(MessageFormat *f,u32 slot,Trainer *trainer){buffer_encoded_name(f,slot,trainer->name,NELEMS(trainer->name));}
void BufferTrainerClassNameFromDataStruct(MessageFormat *f,u32 slot,Trainer *trainer){buffer_message_resource(f,slot,730,trainer->data.trainerClass);}
void ReadFromNarcMemberByIdPair(void *out,NarcId archive,s32 member,u32 offset,u32 length){const uint8_t *bytes;GF_ASSERT(native_resource_range(archive,0,member,offset,length,&bytes));memcpy(out,bytes,length);}
void BufferIntegerAsString(MessageFormat *f,u32 slot,s32 number,u32 digits,PrintingMode mode,BOOL charset){
    (void)charset;GF_ASSERT(slot<NELEMS(f->names)&&digits<=10);
    if(mode==PRINTING_MODE_LEFT_ALIGN)snprintf(f->names[slot],sizeof(f->names[slot]),"%d",number);
    else if(mode==PRINTING_MODE_RIGHT_ALIGN)snprintf(f->names[slot],sizeof(f->names[slot]),"%*d",(int)digits,number);
    else if(mode==PRINTING_MODE_LEADING_ZEROS)snprintf(f->names[slot],sizeof(f->names[slot]),"%0*d",(int)digits,number);
    else battle_boundary_failed("integer printing mode");
}
static void format_battle_message(HGBattle *battle,const BattleMessage *source){
    /* Original MessageFormat slots persist across messages (TAG_NONE can reuse
     * a prior trainer/name binding, including the blackout announcement). */
    BattleMessage msg=*source;
    BattleSystem_AdjustMessageForSide(&battle->system,&msg);BattleSystem_BufferMessage(&battle->system,&msg);
    const uint8_t *bytes;size_t size;GF_ASSERT(native_resource_member(NARC_msgdata_msg,197,msg.id,&bytes,&size)&&size&&bytes[size-1]==0);
    size_t at=0,written=0;
    while(at+1<size){
        if(bytes[at]=='{'){
            if(size-at>=sizeof("{YESNO 0}")-1&&!memcmp(bytes+at,"{YESNO 0}",sizeof("{YESNO 0}")-1)){at+=sizeof("{YESNO 0}")-1;continue;}
            unsigned control;int control_length=0;
            if(sscanf((const char *)bytes+at,"{WAIT %u}%n",&control,&control_length)==1&&control_length){
                switch(control){case 1:battle->wait_effect=1;break;case 2:battle->wait_fanfare=1;break;
                case 3:PlayFanfare(SEQ_ME_POKEGET);break;case 4:PlaySE(SEQ_SE_DP_KON);break;case 5:PlayFanfare(SEQ_ME_LVUP);break;
                default:battle_boundary_failed("battle text callback");}
                at+=control_length;continue;
            }
            unsigned type,slot,last;int consumed=0;
            if(sscanf((const char *)bytes+at,"{STRVAR_1 %u, %u, %u}%n",&type,&slot,&last,&consumed)!=3||!consumed||last!=0||slot>=NELEMS(battle->format.names)||!battle->format.names[slot][0]){char reason[96];snprintf(reason,sizeof(reason),"battle message %u control %.48s",msg.id,bytes+at);battle_boundary_failed(reason);}
            const char *value=battle->format.names[slot];size_t n=strlen(value);GF_ASSERT(n<sizeof(battle->text)-written);memcpy(battle->text+written,value,n);written+=n;at+=consumed;
        }else{GF_ASSERT(written+1<sizeof(battle->text));battle->text[written++]=bytes[at]=='\f'?'\n':bytes[at];at++;}
    }
    battle->text[written]=0;
}
u32 BattleSystem_PrintBattleMessage(BattleSystem *system,MsgData *data,BattleMessage *msg,u8 delay){
    (void)data;(void)delay;HGBattle *battle=(HGBattle *)system;GF_ASSERT(!battle->waiting&&!battle->direct_message);
    format_battle_message(battle,msg);battle->waiting=1;battle->direct_message=1;return 1;
}
u8 TextPrinterCheckActive(u8 printer){GF_ASSERT(printer==1);return running_battle->direct_message;}

/* Native summary surfaces retain the original task's window/row lifecycle.
 * DS tile/frame/palette requests describe this surface; the headless backend
 * retains those requests and emits text pages instead of synthesizing DS VRAM. */
void SetBgPriority(u8 layer,u16 priority){GF_ASSERT(layer<4&&priority<4);running_battle->background.priorities[layer]=priority;}
void ov12_0223C224(BattleSystem *system,int hidden){GF_ASSERT(hidden==0||hidden==1);((HGBattle *)system)->hp_bars_hidden=hidden;}
void sub_0200E398(BgConfig *background,u32 layer,u32 tile,u32 style,enum HeapID heap){
    (void)heap;GF_ASSERT(background==&running_battle->background&&layer==2&&tile==1&&style==0);background->frame_style=style;
}
u32 sub_0200E3D8(void){return 25;} /* Original frame-palette member selector. */
void PaletteData_LoadNarc(PaletteData *palette,NarcId archive,s32 member,enum HeapID heap,PaletteBufferId buffer,u32 size,u16 position){
    (void)heap;GF_ASSERT(palette==&running_battle->palette&&archive==NARC_a_0_3_8&&member==25&&buffer==PLTTBUF_MAIN_BG&&size==32&&position==128);
    palette->archive=archive;palette->member=member;palette->buffer=buffer;palette->size=size;palette->position=position;
}
void AddWindowParameterized(BgConfig *background,Window *window,u8 layer,u8 x,u8 y,u8 width,u8 height,u8 palette,u16 tile){
    HGBattle *b=running_battle;GF_ASSERT(background==&b->background&&window==b->windows+1&&!b->summary_open);
    GF_ASSERT(layer==2&&x==17&&y==7&&width==14&&height==12&&palette==11&&tile==10);
    window->width=width*8;window->height=height*8;window->revision++;b->summary_open=1;memset(b->summary,0,sizeof(b->summary));
}
void FillWindowPixelBuffer(Window *window,u8 color){
    HGBattle *b=running_battle;GF_ASSERT(window==b->windows+1&&b->summary_open&&color==255);memset(b->summary,0,sizeof(b->summary));window->revision++;
}
void FillWindowPixelRect(Window *window,u8 color,u16 x,u16 y,u16 width,u16 height){
    HGBattle *b=running_battle;GF_ASSERT(window==b->windows+1&&b->summary_open&&color==15&&x==80&&y==0&&width==36&&height==96);
    for(unsigned i=0;i<6;i++)b->summary[i][1][0]=0;window->revision++;
}
void DrawFrameAndWindow1(Window *window,BOOL no_copy,u16 tile,u8 palette){
    HGBattle *b=running_battle;GF_ASSERT(window==b->windows+1&&b->summary_open&&!no_copy&&tile==1&&palette==8);b->summary_frame=1;
}
void sub_0200E5D4(Window *window,BOOL no_copy){GF_ASSERT(window==running_battle->windows+1&&!no_copy&&running_battle->summary_frame);running_battle->summary_frame=0;}
void RemoveWindow(Window *window){GF_ASSERT(window==running_battle->windows+1&&running_battle->summary_open&&!running_battle->summary_frame);running_battle->summary_open=0;window->width=window->height=0;window->revision++;}
u32 ov12_0223C4E8(BattleSystem *system,Window *window,MsgData *data,BattleMessage *msg,int x,int y,int flags,int width,int delay){
    (void)data;HGBattle *b=(HGBattle *)system;GF_ASSERT(window==b->windows+1&&b->summary_open&&b->summary_frame);
    GF_ASSERT(y>=0&&y<96&&!(y%16)&&delay==0&&(x==0||x==72||x==80));
    GF_ASSERT((x==72&&flags==2&&width==36)||(x!=72&&flags==0&&width==0));
    unsigned row=y/16,column=x!=0;format_battle_message(b,msg);GF_ASSERT(strlen(b->text)<sizeof(b->summary[row][column]));strcpy(b->summary[row][column],b->text);
    if(row==5&&column){
        size_t written=0;for(unsigned i=0;i<6;i++){
            int n=snprintf(b->text+written,sizeof(b->text)-written,"%s  %s\n",b->summary[i][0],b->summary[i][1]);
            GF_ASSERT(n>=0&&(size_t)n<sizeof(b->text)-written);written+=n;
        }
        GF_ASSERT(!b->waiting&&!b->direct_message);b->waiting=1;b->direct_message=1;
    }
    return 1;
}

static u8 EncounterGen_ChooseUnownForm(EncounterGenState *state){(void)state;battle_boundary_failed("Unown encounter forms");}
enum RadioMusicPlayingSeq GetRadioMusicPlayingSeq(void){return RADIO_MUSIC_SEQ_NONE;}
TIMEOFDAY GF_RTC_GetTimeOfDay(void){return (TIMEOFDAY)running_battle->encounter_time;}
BOOL sub_0205DE98(PlayerAvatar *avatar){u32 command=PlayerAvatar_GetUnk8(avatar);return command>=88&&command<=91;}
