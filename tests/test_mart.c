#include "compat/pokemon.h"
#include "compat/mart.h"
#include "compat/item_use.h"
#include "compat/stats.h"
#include "compat/delivery.h"
#include "compat/save_c_records.h"
#include "field_app.h"
#include "field_data.h"
#include "bag.h"
#include <unistd.h>
static char error[256];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
#include "field_routes.h"
static void step(HGFieldApp *app){hg_field_app_step(app);if(hg_field_app_error(app)[0])snprintf(error,sizeof(error),"%s",hg_field_app_error(app));CHECK(!hg_field_app_error(app)[0]);}
static void confirm(HGFieldApp *app){hg_field_app_interact(app);step(app);}
static void key(HGFieldApp *app,u16 keys){hg_field_app_input(app,keys);step(app);hg_field_app_clear_input(app);}
static void menu(HGFieldApp *app){for(unsigned t=0;t<2048;t++){step(app);if(hg_field_app_menu_count(app)==3)return;if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);}CHECK(0);}
static void choice(HGFieldApp *app,unsigned index){CHECK(hg_field_app_menu_count(app)==3&&hg_field_app_menu_choice_supported(app,index));for(unsigned i=0;i<index;i++)key(app,0x80);confirm(app);}
static HGShop *shop(HGFieldApp *app){for(unsigned i=0;i<64;i++){HGShop *s=hg_field_app_shop(app);if(s)return s;step(app);}CHECK(0);return NULL;}
static unsigned find(HGShop *s,unsigned item){for(unsigned i=0;i<hg_shop_count(s);i++)if(hg_shop_item(s,i)==item)return i;return UINT32_MAX;}
static void buy_ten(HGFieldApp *app,HGShop *s){CHECK(hg_shop_select(s,find(s,ITEM_POTION)));confirm(app);CHECK(hg_shop_stage(s)==HG_SHOP_QUANTITY);key(app,0x10);key(app,0x80);CHECK(hg_shop_quantity(s)==10);confirm(app);CHECK(hg_shop_stage(s)==HG_SHOP_CONFIRM);confirm(app);CHECK(hg_shop_stage(s)==HG_SHOP_RECEIPT);confirm(app);CHECK(hg_shop_stage(s)==HG_SHOP_LIST);}
static void domain_cases(void){
    HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-5.hgcheckpoint",error,sizeof(error));CHECK(app);FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);
    Bag *bag=hg_story_save_bag(field.saveData);PlayerProfile *profile=hg_story_save_profile(field.saveData);Pokemon *mon=Party_GetMonByIndex(hg_story_save_party(field.saveData),0);
    MysteryGiftSave *gifts=SaveArray_Get(field.saveData,SAVE_MYSTERY_GIFT);CHECK(hg_delivery_gift_tag(field.saveData)==0);gifts->gifts[3].tag=MG_TAG_ITEM;CHECK(hg_delivery_gift_tag(field.saveData)==MG_TAG_ITEM);gifts->gifts[3].tag=0;CHECK(hg_delivery_gift_tag(field.saveData)==0);
    const u16 stock[]={ITEM_POKE_BALL,ITEM_POTION,0xffff};HGShop *s=hg_shop_new(&field,stock,0);CHECK(s&&find(s,ITEM_POKE_BALL)!=UINT32_MAX);
    unsigned balls=Bag_GetQuantity(bag,ITEM_POKE_BALL,HEAP_ID_FIELD2),bonus=Bag_GetQuantity(bag,ITEM_PREMIER_BALL,HEAP_ID_FIELD2),money=PlayerProfile_GetMoney(profile);
    CHECK(hg_shop_select(s,find(s,ITEM_POKE_BALL)));hg_shop_input(s,0,1,0);hg_shop_input(s,0x10,0,0);hg_shop_input(s,0x80,0,0);CHECK(hg_shop_quantity(s)==10);hg_shop_input(s,0,1,0);hg_shop_input(s,0,1,0);hg_shop_input(s,0,0,1);hg_shop_input(s,0,0,1);
    CHECK(Bag_GetQuantity(bag,ITEM_POKE_BALL,HEAP_ID_FIELD2)==balls+10&&Bag_GetQuantity(bag,ITEM_PREMIER_BALL,HEAP_ID_FIELD2)==bonus+1&&PlayerProfile_GetMoney(profile)==money-2000);
    /* Check affordability again at confirmation, including external changes. */
    CHECK(hg_shop_select(s,find(s,ITEM_POTION)));hg_shop_input(s,0,1,0);hg_shop_input(s,0,1,0);PlayerProfile_SubMoney(profile,PlayerProfile_GetMoney(profile));Bag before=*bag;hg_shop_input(s,0,1,0);CHECK(!memcmp(&before,bag,sizeof(before))&&PlayerProfile_GetMoney(profile)==0);hg_shop_free(s);
    PlayerProfile_AddMoney(profile,3000);CHECK(Bag_AddItem(bag,ITEM_POTION,999-Bag_GetQuantity(bag,ITEM_POTION,HEAP_ID_FIELD2),HEAP_ID_FIELD2));s=hg_shop_new(&field,stock,0);CHECK(s&&hg_shop_select(s,find(s,ITEM_POTION)));before=*bag;hg_shop_input(s,0,1,0);hg_shop_input(s,0,1,0);hg_shop_input(s,0,1,0);CHECK(!memcmp(&before,bag,sizeof(before))&&PlayerProfile_GetMoney(profile)==3000);hg_shop_free(s);
    u32 full=GetMonData(mon,MON_DATA_MAX_HP,NULL);SetMonData(mon,MON_DATA_HP,&full);Pokemon unchanged=*mon;before=*bag;
    CHECK(hg_item_use(field.saveData,ITEM_POTION,0,0,0)==HG_ITEM_USE_NO_EFFECT&&!memcmp(&unchanged,mon,sizeof(*mon))&&!memcmp(&before,bag,sizeof(before)));
    CHECK(hg_item_use(field.saveData,ITEM_RARE_CANDY,0,0,0)==HG_ITEM_USE_UNSUPPORTED&&hg_item_use(field.saveData,ITEM_SACRED_ASH,0,0,0)==HG_ITEM_USE_UNSUPPORTED&&!memcmp(&before,bag,sizeof(before)));
    CHECK(Bag_AddItem(bag,ITEM_ANTIDOTE,1,HEAP_ID_FIELD2));u32 status=8;SetMonData(mon,MON_DATA_STATUS,&status);CHECK(hg_item_use(field.saveData,ITEM_ANTIDOTE,0,0,0)==HG_ITEM_USE_APPLIED&&!GetMonData(mon,MON_DATA_STATUS,NULL));
    CHECK(Bag_AddItem(bag,ITEM_REVIVE,1,HEAP_ID_FIELD2));u32 zero=0;SetMonData(mon,MON_DATA_HP,&zero);CHECK(hg_item_use(field.saveData,ITEM_POTION,0,0,0)==HG_ITEM_USE_NO_EFFECT);CHECK(hg_item_use(field.saveData,ITEM_REVIVE,0,0,0)==HG_ITEM_USE_APPLIED&&GetMonData(mon,MON_DATA_HP,NULL)>0);
    CHECK(Bag_AddItem(bag,ITEM_ETHER,1,HEAP_ID_FIELD2));SetMonData(mon,MON_DATA_MOVE1_PP,&zero);CHECK(hg_item_use(field.saveData,ITEM_ETHER,0,0,0)==HG_ITEM_USE_APPLIED&&GetMonData(mon,MON_DATA_MOVE1_PP,NULL)>0);hg_field_app_free(app);
}
int main(void){
    domain_cases();
    HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-3.hgcheckpoint",error,sizeof(error));CHECK(app);
    hg_test_portal(app,68);hg_test_travel(app,68,80);CHECK(hg_field_app_map(app)==68);hg_test_approach(app,1);hg_field_app_interact(app);menu(app);
    choice(app,0);HGShop *s=shop(app);CHECK(find(s,ITEM_POTION)!=UINT32_MAX&&find(s,ITEM_POKE_BALL)==UINT32_MAX&&find(s,ITEM_GREAT_BALL)==UINT32_MAX);
    char held[1024],again[1024];hg_field_app_input(app,0x80);step(app);hg_shop_text(s,held,sizeof(held));
    for(unsigned tick=0;tick<120;tick++){hg_field_app_input(app,0x80);step(app);hg_shop_text(s,again,sizeof(again));CHECK(!strcmp(held,again));}
    hg_field_app_input(app,0);hg_field_app_input(app,0x80);step(app);hg_shop_text(s,again,sizeof(again));CHECK(strcmp(held,again));hg_field_app_clear_input(app);
    CHECK(hg_shop_select(s,find(s,ITEM_POTION)));confirm(app);hg_field_app_input(app,0x40);step(app);CHECK(hg_shop_quantity(s)==2);
    for(unsigned tick=0;tick<120;tick++){hg_field_app_input(app,0x40);step(app);CHECK(hg_shop_quantity(s)==2);}hg_field_app_clear_input(app);hg_field_app_shop_cancel(app);step(app);
    FieldSystem live={0};hg_story_bind_field(hg_field_app_story(app),&live);unsigned money=PlayerProfile_GetMoney(hg_story_save_profile(live.saveData));
    Bag before=*hg_story_save_bag(live.saveData);char path[]="/tmp/hg-mart-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));CHECK(!hg_field_app_save(app,path,error,sizeof(error)));error[0]=0;
    CHECK(hg_shop_select(s,find(s,ITEM_POTION)));confirm(app);confirm(app);hg_field_app_shop_cancel(app);step(app);CHECK(hg_shop_stage(s)==HG_SHOP_LIST);
    buy_ten(app,s);
    /* The live owner is unchanged until the original clerk script returns. */
    CHECK(!memcmp(&before,hg_story_save_bag(live.saveData),sizeof(before))&&PlayerProfile_GetMoney(hg_story_save_profile(live.saveData))==money);
    hg_field_app_shop_cancel(app);menu(app);choice(app,2);hg_test_settle(app);CHECK(!hg_field_app_interaction_busy(app));
    hg_story_bind_field(hg_field_app_story(app),&live);Bag *bag=hg_story_save_bag(live.saveData);
    CHECK(PlayerProfile_GetMoney(hg_story_save_profile(live.saveData))==money-3000);
    CHECK(Bag_GetQuantity(bag,ITEM_POTION,HEAP_ID_FIELD2)==Bag_GetQuantity(&before,ITEM_POTION,HEAP_ID_FIELD2)+10);
    CHECK(GameStats_GetCapped(hg_story_save_stats(live.saveData),GAME_STAT_CURRENCY_SPENT)>=3000);
    hg_field_app_interact(app);menu(app);choice(app,1);s=shop(app);CHECK(hg_shop_select(s,find(s,ITEM_POTION)));confirm(app);confirm(app);confirm(app);confirm(app);hg_field_app_shop_cancel(app);menu(app);choice(app,2);hg_test_settle(app);
    hg_story_bind_field(hg_field_app_story(app),&live);bag=hg_story_save_bag(live.saveData);CHECK(PlayerProfile_GetMoney(hg_story_save_profile(live.saveData))==money-2850);
    Pokemon *mon=Party_GetMonByIndex(hg_story_save_party(live.saveData),0);u32 hp=1;SetMonData(mon,MON_DATA_HP,&hp);hg_story_touch(hg_field_app_story(app));unsigned potions=Bag_GetQuantity(bag,ITEM_POTION,HEAP_ID_FIELD2),max=GetMonData(mon,MON_DATA_MAX_HP,NULL);
    CHECK(hg_field_app_inspect(app,1));char text[4096];for(unsigned p=0;p<8;p++){hg_field_app_inspect_text(app,text,sizeof(text));if(strstr(text,"> Potion"))break;key(app,0x10);}CHECK(strstr(text,"> Potion"));confirm(app);confirm(app);
    CHECK(Bag_GetQuantity(bag,ITEM_POTION,HEAP_ID_FIELD2)==potions-1&&GetMonData(mon,MON_DATA_HP,NULL)==(max<21?max:21));CHECK(hg_field_app_inspect(app,0));
    CHECK(hg_field_app_save(app,path,error,sizeof(error)));HGFieldApp *continued=hg_field_app_continue(path,error,sizeof(error));CHECK(continued);FieldSystem restored={0};hg_story_bind_field(hg_field_app_story(continued),&restored);
    CHECK(!memcmp(bag,hg_story_save_bag(restored.saveData),sizeof(*bag)));CHECK(!memcmp(hg_story_save_party(live.saveData),hg_story_save_party(restored.saveData),sizeof(Party)));CHECK(PlayerProfile_GetMoney(hg_story_save_profile(restored.saveData))==money-2850);
    hg_field_app_free(continued);CHECK(!unlink(path));hg_field_app_free(app);puts("Connected original clerk: buy/cancel/sell and bonus/failure cases, Bag Potion use, transaction ownership and durable Save/Continue pass");
}
