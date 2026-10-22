#pragma once
#include "compat/script.h"
typedef struct HGShop HGShop;
struct MartItem;
BOOL ScrCmd_MartBuy(ScriptContext *);
BOOL ScrCmd_MartSell(ScriptContext *);
BOOL ScrCmd_SpecialMartBuy(ScriptContext *);
enum {HG_SHOP_LIST,HG_SHOP_QUANTITY,HG_SHOP_CONFIRM,HG_SHOP_RECEIPT};
HGShop *hg_shop_new(FieldSystem *,const u16 *stock,int selling);
void hg_shop_free(HGShop *);
void hg_shop_input(HGShop *,uint16_t keys,int confirm,int cancel);
int hg_shop_open(const HGShop *);
void hg_shop_text(HGShop *,char *,size_t);
unsigned hg_shop_count(const HGShop *);
unsigned hg_shop_item(const HGShop *,unsigned row);
int hg_shop_select(HGShop *,unsigned row);
int hg_shop_stage(const HGShop *);
unsigned hg_shop_quantity(const HGShop *);
void hg_mart_start(TaskManager *,FieldSystem *,const u16 *,int,u8,int,const struct MartItem *);
