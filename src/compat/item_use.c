#include "compat/item_use.h"
#include "compat/pokemon_services.h"
#include "bag.h"
int hg_item_use_supported(unsigned item){
    return item>0&&item<ITEMS_COUNT&&GetItemAttr(item,ITEMATTR_PARTY_USE,HEAP_ID_FIELD2)==1&&
        !GetItemAttr(item,ITEMATTR_REVIVE_ALL,HEAP_ID_FIELD2)&&
        !GetItemAttr(item,ITEMATTR_LEVEL_UP,HEAP_ID_FIELD2)&&!GetItemAttr(item,ITEMATTR_EVOLVE,HEAP_ID_FIELD2);
}
typedef struct {HGMonDate date;unsigned hour;} ItemClock;
static BOOL item_night(void *clock){unsigned hour=((ItemClock *)clock)->hour;return hour>=20||hour<4;}
int hg_item_use(SaveData *save,unsigned item,unsigned slot,unsigned move,unsigned section){
    if(!save||item>=ITEMS_COUNT||move>=4||slot>=(unsigned)Party_GetCount(hg_story_save_party(save)))return HG_ITEM_USE_INVALID;
    if(!hg_item_use_supported(item))return HG_ITEM_USE_UNSUPPORTED;
    Bag *bag=hg_story_save_bag(save);if(!Bag_GetQuantity(bag,item,HEAP_ID_FIELD2))return HG_ITEM_USE_INVALID;
    Party *party=hg_story_save_party(save);Pokemon *mon=Party_GetMonByIndex(party,slot);if(GetMonData(mon,MON_DATA_IS_EGG,NULL))return HG_ITEM_USE_NO_EFFECT;
    ItemClock clock={hg_story_save_sys_info(save)->rtc_info.date,hg_story_save_sys_info(save)->rtc_info.time.hour};HGPokemonServices next=hg_mon_services(&clock.date);next.owner=&clock;next.is_night=item_night;
    HGPokemonServices before=hg_pokemon_services_exchange(next);Pokemon candidate=*mon;
    int usable=CanUseItemOnPokemon(&candidate,item,(s32)move,HEAP_ID_FIELD2);
    int applied=usable&&UseItemOnPokemon(&candidate,item,(u16)move,(u16)section,HEAP_ID_FIELD2);
    if(applied)ApplyItemEffectOnMonMood(&candidate,item);
    hg_pokemon_services_exchange(before);
    if(!applied)return HG_ITEM_USE_NO_EFFECT;
    if(!Bag_TakeItem(bag,item,1,HEAP_ID_FIELD2))return HG_ITEM_USE_INVALID;
    *mon=candidate;return HG_ITEM_USE_APPLIED;
}
