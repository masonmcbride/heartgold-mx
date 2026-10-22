/* Original Thumb AI and original C dispatcher share explicit value records. */
#include "trainer_ai_guest.inc"
#define AI_RAM_SIZE 0x40000u
#define AI_CONTEXT 0x02020000u
#define AI_SYSTEM 0x02018000u
static void ai_copy(HGBattle *b,HGAIBattleContext *g,int to_guest){
    BattleContext *c=b->system.ctx;
#define AI_FIELD(type,field) do { \
    type *n=__builtin_choose_expr(__builtin_types_compatible_p(type,TrainerAIData),(void *)&c->trainerAIData,(void *)c); \
    HGAI##type *v=__builtin_choose_expr(__builtin_types_compatible_p(type,TrainerAIData),(void *)&g->trainerAIData,(void *)g); \
    _Static_assert(sizeof(n->field)==sizeof(v->field),"AI field layout"); \
    if(to_guest)memcpy(&v->field,&n->field,sizeof(n->field));else memcpy(&n->field,&v->field,sizeof(n->field)); \
}while(0)
#include "trainer_ai_copy.inc"
#undef AI_FIELD
    if(to_guest){g->battleContinueFlag=c->battleContinueFlag;g->unused=c->unused;g->getterWork=g->prevLevelStats=0;g->unk_2134=HG_AI_ov10_02220AAC;g->trainerAIData.unk14=g->trainerAIData.itemData=0;}else{c->battleContinueFlag=g->battleContinueFlag;c->unused=g->unused;}
}
#define AI_MON_TOKEN 0x02200000u
#define AI_PARTY_TOKEN 0x02300000u
static Party *ai_party(BattleSystem *s,u32 token){
    GF_ASSERT(token>=AI_PARTY_TOKEN&&token<AI_PARTY_TOKEN+0x4000&&!(token&0xfff));return s->trainerParty[(token-AI_PARTY_TOKEN)/0x1000];
}
static Pokemon *ai_mon(BattleSystem *s,u32 token){
    GF_ASSERT(token>=AI_MON_TOKEN&&token<AI_MON_TOKEN+0x4000&&!(token&0xff));unsigned side=(token-AI_MON_TOKEN)/0x1000,slot=((token-AI_MON_TOKEN)%0x1000)/0x100;
    GF_ASSERT(slot<(unsigned)Party_GetCount(s->trainerParty[side]));return Party_GetMonByIndex(s->trainerParty[side],slot);
}
static int ai_boundary(DSThumb *cpu,void *owner){
    HGBattle *b=owner;BattleSystem *s=&b->system;BattleContext *ctx=s->ctx;
    HGAIBattleContext *g=ds_translate(&cpu->memory,AI_CONTEXT,sizeof(*g));GF_ASSERT(g);
    const char *name=NULL;for(unsigned i=0;i<NELEMS(hg_ai_imports);i++)if(hg_ai_imports[i].address==cpu->r[15]){name=hg_ai_imports[i].name;break;}
    if(!name)return 0;
    ai_copy(b,g,0);u32 a=cpu->r[0],v=cpu->r[1],w=cpu->r[2],x=cpu->r[3],result=0;
    if(!strncmp(name,"BattleSystem_",13)||!strcmp(name,"CalcMoveDamage")||!strcmp(name,"CheckAbilityActive")||!strcmp(name,"CheckSortSpeed")||!strcmp(name,"GetBattlerLearnedMoveCount")||!strcmp(name,"ov12_0223AB0C")||!strcmp(name,"ov12_02251D28")||!strcmp(name,"ov10_0221BE20"))GF_ASSERT(a==AI_SYSTEM);
    if(!strcmp(name,"BattleMon_Get")||!strcmp(name,"GetBattlerAbility")||!strcmp(name,"GetBattlerHeldItemEffect")||!strcmp(name,"CurseUserIsGhost")||!strcmp(name,"GetItemVar")||!strcmp(name,"GetNaturalGiftType")||!strcmp(name,"GetHeldItemFlingPower")||!strcmp(name,"ov12_02252054"))GF_ASSERT(a==AI_CONTEXT);
#define AI_CALL(symbol,call) if(!strcmp(name,#symbol)){result=(call);goto done;}
    AI_CALL(BattleSystem_GetBattleContext,AI_CONTEXT)
    AI_CALL(CalculateTypeEffectiveness,CalculateTypeEffectiveness(a,v,w))
    if(!strcmp(name,"Battler_GetRandomOpposingBattlerId")){GF_ASSERT(a==AI_SYSTEM&&v==AI_CONTEXT);result=Battler_GetRandomOpposingBattlerId(s,ctx,w);goto done;}
    AI_CALL(BattleSystem_Random,BattleSystem_Random(s))
    AI_CALL(BattleSystem_GetBattleType,BattleSystem_GetBattleType(s))
    AI_CALL(BattleSystem_GetFieldSide,BattleSystem_GetFieldSide(s,v))
    AI_CALL(BattleSystem_GetPartySize,BattleSystem_GetPartySize(s,v))
    AI_CALL(BattleSystem_GetMaxBattlers,BattleSystem_GetMaxBattlers(s))
    AI_CALL(BattleSystem_GetBattlerIdPartner,BattleSystem_GetBattlerIdPartner(s,v))
    AI_CALL(BattleSystem_GetBattlerFromBattlerType,BattleSystem_GetBattlerFromBattlerType(s,v))
    AI_CALL(MaskOfFlagNo,MaskOfFlagNo(a))
    AI_CALL(DamageDivide,DamageDivide((s32)a,(s32)v))
    AI_CALL(GetBattlerAbility,GetBattlerAbility(ctx,v))
    AI_CALL(GetBattlerHeldItemEffect,GetBattlerHeldItemEffect(ctx,v))
    u32 *stack=ds_translate(&cpu->memory,cpu->r[13],32);GF_ASSERT(stack);
    AI_CALL(CheckAbilityActive,CheckAbilityActive(s,ctx,w,x,*stack))
    AI_CALL(CheckSortSpeed,CheckSortSpeed(s,ctx,w,x,*stack))
    AI_CALL(GetBattlerLearnedMoveCount,GetBattlerLearnedMoveCount(s,ctx,w))
    AI_CALL(CurseUserIsGhost,CurseUserIsGhost(ctx,v,w))
    AI_CALL(GetMonBaseStat,GetMonBaseStat(a,v))
    if(!strcmp(name,"BattleSystem_GetParty")){GF_ASSERT(v<4);result=AI_PARTY_TOKEN+v*0x1000;goto done;}
    if(!strcmp(name,"BattleSystem_GetPartyMon")){GF_ASSERT(v<4&&w<(unsigned)Party_GetCount(s->trainerParty[v]));result=AI_MON_TOKEN+v*0x1000+w*0x100;goto done;}
    if(!strcmp(name,"Party_GetCount")){result=Party_GetCount(ai_party(s,a));goto done;}
    if(!strcmp(name,"Party_GetMonByIndex")){Party *party=ai_party(s,a);GF_ASSERT(v<(unsigned)Party_GetCount(party));result=AI_MON_TOKEN+(a-AI_PARTY_TOKEN)+v*0x100;goto done;}
    if(!strcmp(name,"GetMonData")){GF_ASSERT(!w);result=GetMonData(ai_mon(s,a),v,NULL);goto done;}

    AI_CALL(CalcMoveDamage,CalcMoveDamage(s,ctx,w,x,stack[0],stack[1],stack[2],stack[3],stack[4],stack[5]))
    AI_CALL(GetItemVar,GetItemVar(ctx,v,w))
    AI_CALL(GetNaturalGiftType,GetNaturalGiftType(ctx,v))
    AI_CALL(GetHeldItemFlingPower,GetHeldItemFlingPower(ctx,v))
    AI_CALL(ov12_0223AB0C,ov12_0223AB0C(s,v))
    if(!strcmp(name,"ov12_02251D28")){
        u32 *flag=ds_translate(&cpu->memory,stack[3],4);GF_ASSERT(flag);
        result=ov12_02251D28(s,ctx,w,x,stack[0],stack[1],stack[2],flag);goto done;
    }
    if(!strcmp(name,"ov12_02252054")){
        u32 *flag=ds_translate(&cpu->memory,stack[4],4);GF_ASSERT(flag);
        ov12_02252054(ctx,v,w,x,stack[0],stack[1],stack[2],stack[3],flag);goto done;
    }

    if(!strcmp(name,"BattleMon_Get")){GF_ASSERT(v<4);result=BattleMon_Get(ctx,v,w,0);goto done;}
    if(!strcmp(name,"_s32_div_f")){GF_ASSERT(v);result=(u32)((int64_t)(s32)a/(s32)v);cpu->r[1]=(u32)((int64_t)(s32)a%(s32)v);goto done;}
    if(!strcmp(name,"_u32_div_f")){GF_ASSERT(v);result=a/v;cpu->r[1]=a%v;goto done;}
    if(!strcmp(name,"ov10_0221BE20")){GF_ASSERT(v==AI_CONTEXT);ov10_0221BE20(s,ctx,w,x);goto done;}
    {char error[128];snprintf(error,sizeof(error),"trainer AI import %s",name);battle_boundary_failed(error);}
#undef AI_CALL
done:;
    /* Preserve guest pointers; host helpers operate only on native records. */
    u32 script=g->unk_2134,work=g->trainerAIData.unk14,item=g->trainerAIData.itemData;ai_copy(b,g,1);g->unk_2134=script;g->trainerAIData.unk14=work;g->trainerAIData.itemData=item;
    cpu->r[0]=result;return 1;
}
static unsigned ai_execute(HGBattle *b,u32 entry,u32 argument){
    _Static_assert(sizeof(hg_ai_image)<AI_SYSTEM-0x02000000u,"AI image overlaps host handles");
    _Static_assert(sizeof(HGAIBattleContext)<0x10000,"AI context overlaps guest stack");
    if(!b->ai_ram){b->ai_ram=calloc(1,AI_RAM_SIZE);GF_ASSERT(b->ai_ram);memcpy(b->ai_ram,hg_ai_image,sizeof(hg_ai_image));}
    DSThumb cpu={0};cpu.memory=(DSMainRAM){b->ai_ram,AI_RAM_SIZE};cpu.code_start=0x02000000;cpu.code_size=HG_AI_CODE_SIZE;cpu.return_address=0x023fffff;cpu.boundary=ai_boundary;cpu.boundary_user=b;
    HGAIBattleContext *g=ds_translate(&cpu.memory,AI_CONTEXT,sizeof(*g));GF_ASSERT(g);memset(g,0,sizeof(*g));ai_copy(b,g,1);
    u32 *system_context=ds_translate(&cpu.memory,AI_SYSTEM+0x30,4);GF_ASSERT(system_context);*system_context=AI_CONTEXT;
    cpu.r[0]=AI_SYSTEM;cpu.r[1]=argument;cpu.r[13]=0x0203fff0;cpu.r[14]=cpu.return_address;cpu.r[15]=entry;
    DSThumbResult status=ds_thumb_run(&cpu,1000000);
    if(status!=DS_THUMB_RETURN){char error[160];snprintf(error,sizeof(error),"trainer AI guest status %d pc %08x address %08x instruction %04x",status,cpu.r[15],cpu.fault_address,cpu.fault_instruction);battle_boundary_failed(error);}
    b->ai_calls++;b->ai_steps+=cpu.steps;ai_copy(b,g,0);return cpu.r[0];
}
u8 ov10_0221BF44(BattleSystem *s,BattleContext *ctx){GF_ASSERT(s==&running_battle->system&&ctx==s->ctx);return ai_execute(running_battle,HG_AI_ov10_0221BF44,AI_CONTEXT);}
u8 ov10_0221C038(BattleSystem *s,BattleContext *ctx){(void)s;(void)ctx;battle_boundary_failed("double trainer AI");}
static unsigned native_trainer_move(HGBattle *b,unsigned side){
    unsigned move=ov10_0221BEF4(&b->system,side);GF_ASSERT(move<4);return move;
}

static unsigned native_trainer_command(HGBattle *b,unsigned side){
    unsigned command=ai_execute(b,HG_AI_ov10_022205BC,side);
    if(command==BATTLE_INPUT_BAG)battle_boundary_failed("trainer medicine application");
    GF_ASSERT(command==BATTLE_INPUT_FIGHT||command==BATTLE_INPUT_POKEMON);return command;
}

static unsigned native_trainer_party(HGBattle *b,unsigned side){
    GF_ASSERT(side==1&&b->system.battleType&BATTLE_TYPE_TRAINER);
    unsigned slot=ai_execute(b,HG_AI_ov12_02258BA0,side);
    if(slot==6)slot=ai_execute(b,HG_AI_ov12_02258800,side);
    if(slot==6){ /* Original ov12_0225F8AC fallback after both selectors. */
        for(slot=0;slot<(unsigned)Party_GetCount(&b->parties[side]);slot++){
            Pokemon *mon=Party_GetMonByIndex(&b->parties[side],slot);
            if(slot!=b->system.ctx->selectedMonIndex[side]&&GetMonData(mon,MON_DATA_HP,NULL)&&!GetMonData(mon,MON_DATA_IS_EGG,NULL))break;
        }
    }
    GF_ASSERT(slot<(unsigned)Party_GetCount(&b->parties[side])&&slot!=b->system.ctx->selectedMonIndex[side]&&GetMonData(Party_GetMonByIndex(&b->parties[side],slot),MON_DATA_HP,NULL));
    return slot;
}
