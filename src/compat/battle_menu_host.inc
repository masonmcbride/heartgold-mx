/* Native selection boundary. Original RecoverStatus updates both the party and
 * active BattleMon; original controller owns action order and enemy response. */
static int native_battle_medicine(unsigned item){
    if(!item||item>=ITEMS_COUNT)return 0;
    unsigned pocket=GetItemAttr(item,ITEMATTR_BATTLE_POCKET,HEAP_ID_BATTLE);
    return (pocket&(BTLPOCKET_HP_RESTORE_F|BTLPOCKET_STATUS_RESTORE_F|BTLPOCKET_PP_RESTORE_F))&&
        GetItemAttr(item,ITEMATTR_PARTY_USE,HEAP_ID_BATTLE)&&!GetItemAttr(item,ITEMATTR_REVIVE_ALL,HEAP_ID_BATTLE);
}
static int native_menu_state(const HGBattle *b){
    static const int states[]={HG_BATTLE_RUNNING,HG_BATTLE_COMMAND,HG_BATTLE_MOVES,HG_BATTLE_NICKNAME,HG_BATTLE_BAG,HG_BATTLE_ITEM_TARGET,HG_BATTLE_ITEM_MOVE,HG_BATTLE_PARTY,HG_BATTLE_YES_NO};
    GF_ASSERT(b->menu<NELEMS(states));return states[b->menu];
}
static int native_party_allowed(const HGBattle *b,unsigned slot){
    if(slot>=(unsigned)Party_GetCount(&b->parties[0]))return 0;
    Pokemon *mon=Party_GetMonByIndex((Party *)&b->parties[0],slot);
    return !b->party_cannot_switch&&slot!=b->system.ctx->selectedMonIndex[0]&&slot!=(unsigned)b->party_excluded&&
        GetMonData(mon,MON_DATA_HP,NULL)&&!GetMonData(mon,MON_DATA_IS_EGG,NULL);
}
static void native_menu_complete(HGBattle *b){b->menu=0;b->input_pending=1;b->menu_hint[0]=0;finish_event(b);}
static int native_use_medicine(HGBattle *b,unsigned move){
    if(b->item_target>=(unsigned)Party_GetCount(&b->parties[0])||move>=4||!native_battle_medicine(b->chosen_item)||!Bag_GetQuantity(&b->bag,b->chosen_item,HEAP_ID_BATTLE))return 0;
    Pokemon *mon=Party_GetMonByIndex(&b->parties[0],b->item_target);
    if(GetMonData(mon,MON_DATA_IS_EGG,NULL)){snprintf(b->menu_hint,sizeof(b->menu_hint),"It won't have any effect.");return 0;}
    Party saved_party=b->parties[0];BattleContext saved_ctx=*b->system.ctx;
    if(!BattleSystem_RecoverStatus(&b->system,0,b->item_target,move,b->chosen_item)){
        b->parties[0]=saved_party;*b->system.ctx=saved_ctx;
        snprintf(b->menu_hint,sizeof(b->menu_hint),"It won't have any effect.");return 0;
    }
    GF_ASSERT(Bag_TakeItem(&b->bag,b->chosen_item,1,HEAP_ID_BATTLE));
    unsigned pocket=GetItemAttr(b->chosen_item,ITEMATTR_BATTLE_POCKET,HEAP_ID_BATTLE);
    BattleItemUse use={.item=b->chosen_item,.category=(pocket&(BTLPOCKET_HP_RESTORE_F|BTLPOCKET_PP_RESTORE_F))?BTLPOCKETLIST_HP_PP_RESTORE:BTLPOCKETLIST_STATUS_RESTORE,.target=(u8)b->item_target};
    BagCursor_Battle_SetLastUsedItem(&b->bag_cursor,use.item,use.category);
    BattleController_EmitSelectedBagItem(&b->system,0,use);native_menu_complete(b);
    const u8 *name;size_t size;GF_ASSERT(native_resource_member(NARC_msgdata_msg,222,use.item,&name,&size));
    snprintf(b->text,sizeof(b->text),"Used %s on %s.",(const char *)name,hg_pokemon_species_name(GetMonData(mon,MON_DATA_SPECIES,NULL)));
    b->waiting=b->direct_message=1;return 1;
}
int hg_battle_menu_cancellable(const HGBattle *b){return b&&b->menu>=4&&b->menu<=7&&(b->menu!=7||!b->party_forced);}
static int native_menu_input(HGBattle *b,unsigned choice){
    if(b->menu==8){if(choice!=1&&choice!=255)return 0;b->system.ctx->battleBuffer[0][0]=(u8)choice;native_menu_complete(b);return 1;}
    if(choice==255){
        if(!hg_battle_menu_cancellable(b))return 0;
        b->menu_hint[0]=0;
        if(b->menu==5){b->menu=4;return 1;}
        if(b->menu==6){b->menu=5;return 1;}
        b->system.ctx->battleBuffer[0][0]=255;native_menu_complete(b);return 1;
    }
    if(!choice)return 0;unsigned row=choice-1;
    if(b->menu==4){if(row>=b->bag_count)return 0;b->chosen_item=b->bag_items[row];b->menu=5;b->menu_hint[0]=0;return 1;}
    if(b->menu==5){if(row>=(unsigned)Party_GetCount(&b->parties[0]))return 0;b->item_target=row;
        if(GetItemAttr(b->chosen_item,ITEMATTR_PP_RESTORE,HEAP_ID_BATTLE)){b->menu=6;b->menu_hint[0]=0;return 1;}
        return native_use_medicine(b,0);
    }
    if(b->menu==6)return row<4&&native_use_medicine(b,row);
    if(b->menu==7){if(!native_party_allowed(b,row))return 0;BattleController_EmitPartyMenuResult(&b->system,0,choice);native_menu_complete(b);return 1;}
    return 0;
}
unsigned hg_battle_menu_count(const HGBattle *b,int state){
    if(!b)return 0;
    if(state==HG_BATTLE_YES_NO)return 2;
    if(state==HG_BATTLE_COMMAND)return hg_battle_command_count(b);
    if(state==HG_BATTLE_BAG)return b->bag_count;
    if(state==HG_BATTLE_ITEM_TARGET||state==HG_BATTLE_PARTY)return (unsigned)Party_GetCount(&b->parties[0]);
    if(state==HG_BATTLE_MOVES||state==HG_BATTLE_ITEM_MOVE)return 4;
    return 0;
}
unsigned hg_battle_choice_code(const HGBattle *b,int state,unsigned row){
    if(!b||row>=hg_battle_menu_count(b,state))return 0;
    if(state==HG_BATTLE_YES_NO)return row?255:1;
    if(state==HG_BATTLE_COMMAND){
        if(row==0)return 1;
        if(b->contest)return row==1?(hg_battle_can_throw(b)?2:0):4;
        if(!(b->system.battleType&BATTLE_TYPE_TRAINER)){
            if(row==1)return hg_battle_can_throw(b)?2:0;
            row--;
        }
        return row==1?5:row==2?3:4;
    }
    if(state==HG_BATTLE_PARTY&&!native_party_allowed(b,row))return 0;
    if(state==HG_BATTLE_MOVES&&!hg_battle_move_allowed((HGBattle *)b,row))return 0;
    return row+1;
}
void hg_battle_menu_label(const HGBattle *b,int state,unsigned row,char *out,size_t size){
    if(!out||!size)return;out[0]=0;if(!b||row>=hg_battle_menu_count(b,state))return;
    if(state==HG_BATTLE_YES_NO){snprintf(out,size,"%s",b->yesno_type==3?(row?"Run":"Use next Pokemon"):(row?"Keep battling":"Switch Pokemon"));return;}
    if(state==HG_BATTLE_COMMAND){
        const char *label="Fight";
        if(row){if(b->contest)label=row==1?"Sport Ball":"Run";
            else if(!(b->system.battleType&BATTLE_TYPE_TRAINER)&&row==1)label="Poke Ball";
            else {unsigned index=row-(!(b->system.battleType&BATTLE_TYPE_TRAINER)?1:0);label=index==1?"Bag":index==2?"Pokemon":"Run";}}
        snprintf(out,size,"%s",label);return;
    }
    if(state==HG_BATTLE_BAG){const u8 *name;size_t bytes;unsigned item=b->bag_items[row];if(native_resource_member(NARC_msgdata_msg,222,item,&name,&bytes))snprintf(out,size,"%s x%u",name,Bag_GetQuantity((Bag *)&b->bag,item,HEAP_ID_BATTLE));return;}
    if(state==HG_BATTLE_ITEM_TARGET||state==HG_BATTLE_PARTY){Pokemon *mon=Party_GetMonByIndex((Party *)&b->parties[0],row);
        snprintf(out,size,"%s Lv%u HP %u/%u%s",hg_pokemon_species_name(GetMonData(mon,MON_DATA_SPECIES_OR_EGG,NULL)),GetMonData(mon,MON_DATA_LEVEL,NULL),GetMonData(mon,MON_DATA_HP,NULL),GetMonData(mon,MON_DATA_MAX_HP,NULL),state==HG_BATTLE_PARTY&&row==b->system.ctx->selectedMonIndex[0]?" (out)":"");return;}
    if(state==HG_BATTLE_ITEM_MOVE||state==HG_BATTLE_MOVES){
        unsigned move,pp;if(state==HG_BATTLE_MOVES){move=b->system.ctx->battleMons[0].moves[row];pp=b->system.ctx->battleMons[0].movePPCur[row];}
        else {Pokemon *mon=Party_GetMonByIndex((Party *)&b->parties[0],b->item_target);move=GetMonData(mon,MON_DATA_MOVE1+row,NULL);pp=GetMonData(mon,MON_DATA_MOVE1_PP+row,NULL);}
        const u8 *name;size_t bytes;if(move&&native_resource_member(NARC_msgdata_msg,750,move,&name,&bytes))snprintf(out,size,"%s PP %u",name,pp);else snprintf(out,size,"--");
    }
}
const char *hg_battle_menu_hint(const HGBattle *b,int state,unsigned row){
    if(!b)return "";if(b->menu_hint[0])return b->menu_hint;
    if(state==HG_BATTLE_BAG&&!b->bag_count)return "No supported medicines. Esc: back";
    if(state==HG_BATTLE_COMMAND&&row==1&&!(b->system.battleType&BATTLE_TYPE_TRAINER)&&!hg_battle_can_throw(b))return hg_battle_ball_unavailable(b);
    if(state==HG_BATTLE_PARTY&&!native_party_allowed(b,row))return b->party_cannot_switch?"You cannot switch right now.":"Choose a healthy Pokemon not in battle.";
    return hg_battle_menu_cancellable(b)?"W/S choose  Enter select  Esc back":"W/S choose  Enter select";
}
