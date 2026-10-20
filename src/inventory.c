#include "inventory.h"
#include "bag.h"
#include "list_menu.h"
#include "ds_graphics.h"
#include "compat/resources.h"
struct HGInventory { Bag *bag;int owns_bag;BagCursor *cursor;ListMenu *menu;Window window;ListMenuItem entries[NUM_BAG_ITEMS];unsigned pocket,count; };
static const unsigned capacities[]={NUM_BAG_ITEMS,NUM_BAG_MEDICINE,NUM_BAG_BALLS,NUM_BAG_TMS_HMS,NUM_BAG_BERRIES,NUM_BAG_MAIL,NUM_BAG_BATTLE_ITEMS,NUM_BAG_KEY_ITEMS};
static const uint16_t defaults[]={ITEM_REPEL,ITEM_POTION,ITEM_POKE_BALL,ITEM_TM01,ITEM_CHERI_BERRY,ITEM_GRASS_MAIL,ITEM_X_ATTACK,ITEM_BICYCLE};
static unsigned selection(const HGInventory *v) { u16 index=0;ListMenuGetCurrentItemArrayId(v->menu,&index);return index; }
static void rebuild(HGInventory *v,unsigned preferred,int restore) {
    if(v->menu)DestroyListMenu(v->menu,NULL,NULL);
    v->count=0;
    for(unsigned i=0;i<capacities[v->pocket];i++) {
        ItemSlot *slot=Bag_GetPocketSlotN(v->bag,(u8)v->pocket,i);
        if(slot->id && slot->quantity)v->entries[v->count++]=(ListMenuItem){.text=NULL,.value=slot->id};
    }
    if(!v->count)v->entries[0]=(ListMenuItem){.text=NULL,.value=LIST_CANCEL};
    unsigned total=v->count?v->count:1,show=total<8?total:8;
    unsigned scroll=0,row=0;
    if(restore) {
        u8 r,s;BagCursor_Field_PocketGetPosition(v->cursor,v->pocket,&r,&s);scroll=s;row=r;
        if(scroll+row>=total || scroll>total-show || row>=show)scroll=row=0;
    } else {
        if(preferred>=total)preferred=total-1;
        scroll=preferred>=show?preferred-show+1:0;row=preferred-scroll;
    }
    ListMenuTemplate t={.items=v->entries,.window=&v->window,.totalItems=(u16)total,.maxShowed=(u16)show,.scrollMultiple=LIST_MULTIPLE_SCROLL_L_R,.cursorKind=0};
    v->menu=ListMenuInit(&t,(u16)scroll,(u16)row,HEAP_ID_DEFAULT);
}
HGInventory *hg_inventory_new(void) {
    HGInventory *v=calloc(1,sizeof(*v));if(!v)abort();v->owns_bag=1;v->bag=Save_Bag_New(HEAP_ID_DEFAULT);v->cursor=BagCursor_New(HEAP_ID_DEFAULT);v->window=(Window){.width=32,.height=24};
    static const uint16_t seeds[]={ITEM_REPEL,ITEM_SUPER_REPEL,ITEM_ESCAPE_ROPE,ITEM_POTION,ITEM_SUPER_POTION,ITEM_HYPER_POTION,ITEM_MAX_POTION,ITEM_FULL_RESTORE,ITEM_REVIVE,ITEM_MAX_REVIVE,ITEM_ANTIDOTE,ITEM_PARLYZ_HEAL,ITEM_AWAKENING,ITEM_BURN_HEAL,ITEM_ICE_HEAL,ITEM_FULL_HEAL,ITEM_ULTRA_BALL,ITEM_GREAT_BALL,ITEM_POKE_BALL,ITEM_TM02,ITEM_TM01,ITEM_CHERI_BERRY,ITEM_CHESTO_BERRY,ITEM_GRASS_MAIL,ITEM_X_ATTACK,ITEM_X_DEFENSE,ITEM_BICYCLE,ITEM_OLD_ROD};
    for(unsigned i=0;i<NELEMS(seeds);i++)if(!Bag_AddItem(v->bag,seeds[i],i%5+1,HEAP_ID_DEFAULT))abort();
    rebuild(v,0,0);return v;
}
HGInventory *hg_inventory_new_bag(Bag *bag) {
    if(!bag)return NULL;
    HGInventory *v=calloc(1,sizeof(*v));if(!v)return NULL;
    v->bag=bag;v->cursor=BagCursor_New(HEAP_ID_DEFAULT);v->window=(Window){.width=32,.height=24};
    rebuild(v,0,0);return v;
}
void hg_inventory_free(HGInventory *v) { if(v) { DestroyListMenu(v->menu,NULL,NULL);Heap_Free(v->cursor);if(v->owns_bag)Heap_Free(v->bag);free(v); } }
int hg_inventory_pocket(const HGInventory *v) { return (int)v->pocket; }
const char *hg_inventory_pocket_name(unsigned p) { static const char *names[]={"Items","Medicine","Balls","TMs & HMs","Berries","Mail","Battle items","Key items"};return p<8?names[p]:""; }
unsigned hg_inventory_count(const HGInventory *v) { return v->count; }
unsigned hg_inventory_selected(const HGInventory *v) { return selection(v); }
unsigned hg_inventory_scroll(const HGInventory *v) { u16 scroll;ListMenuGetScrollAndRow(v->menu,&scroll,NULL);return scroll; }
int hg_item_details(uint16_t id,HGItem *out) {
    if(!out||id>ITEM_MAX)return 0;memset(out,0,sizeof(*out));out->id=id;
    String *name=native_string_new(),*desc=native_string_new();
    GetItemNameIntoString(name,id,HEAP_ID_DEFAULT);GetItemDescIntoString(desc,id,HEAP_ID_DEFAULT);
    snprintf(out->name,sizeof(out->name),"%s",native_string_utf8(name));snprintf(out->description,sizeof(out->description),"%s",native_string_utf8(desc));
    native_string_free(name);native_string_free(desc);return 1;
}
int hg_inventory_item(const HGInventory *v,unsigned row,HGItem *out) {
    if(!out)return 0;memset(out,0,sizeof(*out));if(row>=v->count)return 0;
    u16 id=(u16)v->entries[row].value;if(!hg_item_details(id,out))return 0;
    out->quantity=Bag_GetQuantity(v->bag,id,HEAP_ID_DEFAULT);return 1;
}

int hg_inventory_action(HGInventory *v,enum HGAction action) {
    if(action==HG_PREVIOUS_POCKET || action==HG_NEXT_POCKET) {
        u16 scroll,row;ListMenuGetScrollAndRow(v->menu,&scroll,&row);
        BagCursor_Field_PocketSetPosition(v->cursor,v->pocket,(u8)row,(u8)scroll);
        v->pocket=(v->pocket+(action==HG_NEXT_POCKET?1:7))%8;
        BagCursor_Field_SetPocket(v->cursor,(u16)v->pocket);rebuild(v,0,1);return 1;
    }
    unsigned index=selection(v);
    if(action==HG_ADD || action==HG_REMOVE) {
        u16 id=v->count?(u16)v->entries[index].value:defaults[v->pocket];
        int ok=action==HG_ADD?Bag_AddItem(v->bag,id,1,HEAP_ID_DEFAULT):(v->count && Bag_TakeItem(v->bag,id,1,HEAP_ID_DEFAULT));
        if(ok)rebuild(v,index,0);return ok;
    }
    native_set_keys(0,0);
    switch(action) {
    case HG_UP:gSystem.newAndRepeatedKeys=REG_PAD_KEYINPUT_UP_MASK;break;
    case HG_DOWN:gSystem.newAndRepeatedKeys=REG_PAD_KEYINPUT_DOWN_MASK;break;
    case HG_PAGE_UP:gSystem.newAndRepeatedKeys=REG_PAD_KEYINPUT_L_MASK;break;
    case HG_PAGE_DOWN:gSystem.newAndRepeatedKeys=REG_PAD_KEYINPUT_R_MASK;break;
    case HG_CONFIRM:gSystem.newKeys=REG_PAD_KEYINPUT_A_MASK;break;
    default:return 0;
    }
    s32 result=ListMenu_ProcessInput(v->menu);native_set_keys(0,0);
    return action==HG_CONFIRM?result:(int)selection(v)!=(int)index;
}
void hg_inventory_select(HGInventory *v,unsigned index) {
    if(index>=v->count)return;
    while(selection(v)!=index) { unsigned old=selection(v);hg_inventory_action(v,old<index?HG_DOWN:HG_UP);if(selection(v)==old)abort(); }
}
int hg_item_icon(uint16_t id,uint8_t rgba[4096]) {
    if(id>ITEM_MAX)return 0;
    const uint8_t *g,*p;size_t gn,pn;
    if(!native_resource_member(NARC_itemtool_itemdata_item_icon,0,GetItemIndexMapping(id,ITEMNARC_NCGR),&g,&gn) || !native_resource_member(NARC_itemtool_itemdata_item_icon,0,GetItemIndexMapping(id,ITEMNARC_NCLR),&p,&pn))return 0;
    return ds_decode_icon(g,gn,p,pn,rgba);
}
int hg_validate_catalog(void) {
    uint8_t rgba[4096];String *name=native_string_new(),*desc=native_string_new();int ok=1;
    for(unsigned id=0;id<=ITEM_MAX;id++) {
        GetItemNameIntoString(name,id,HEAP_ID_DEFAULT);GetItemDescIntoString(desc,id,HEAP_ID_DEFAULT);
        if(!*native_string_utf8(name) || !hg_item_icon(id,rgba)) { fprintf(stderr,"Invalid item resource %u\n",id);ok=0;break; }
    }
    native_string_free(name);native_string_free(desc);return ok;
}
