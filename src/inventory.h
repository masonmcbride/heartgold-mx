#pragma once
#include <stdint.h>
typedef struct HGInventory HGInventory;
enum HGAction { HG_UP,HG_DOWN,HG_PREVIOUS_POCKET,HG_NEXT_POCKET,HG_ADD,HG_REMOVE,HG_CONFIRM,HG_PAGE_UP,HG_PAGE_DOWN };
typedef struct { uint16_t id,quantity; char name[128],description[1024]; } HGItem;
HGInventory *hg_inventory_new(void);
struct Bag;
/* Borrow the live inventory; caller retains ownership and must keep it alive. */
HGInventory *hg_inventory_new_bag(struct Bag *);
void hg_inventory_free(HGInventory *);
int hg_inventory_action(HGInventory *,enum HGAction);
int hg_inventory_pocket(const HGInventory *);
const char *hg_inventory_pocket_name(unsigned);
unsigned hg_inventory_count(const HGInventory *);
unsigned hg_inventory_selected(const HGInventory *);
unsigned hg_inventory_scroll(const HGInventory *);
int hg_inventory_item(const HGInventory *,unsigned,HGItem *);
int hg_item_details(uint16_t id,HGItem *);
void hg_inventory_select(HGInventory *,unsigned);
int hg_item_icon(uint16_t id,uint8_t rgba[4096]);
int hg_validate_catalog(void);
