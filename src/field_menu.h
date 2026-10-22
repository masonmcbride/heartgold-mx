#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct HGFieldMenu HGFieldMenu;
struct Bag;struct Party;struct SaveData;
HGFieldMenu *hg_field_menu_new(struct SaveData *,unsigned map_section);
void hg_field_menu_free(HGFieldMenu *);
void hg_field_menu_input(HGFieldMenu *,uint16_t keys);
void hg_field_menu_confirm(HGFieldMenu *);
int hg_field_menu_take_changed(HGFieldMenu *);
void hg_field_menu_text(HGFieldMenu *,char *,size_t);
int hg_field_menu_kind(const HGFieldMenu *);
void hg_field_menu_open(HGFieldMenu *,int kind);
int hg_field_menu_render(HGFieldMenu *,uint8_t *,unsigned,unsigned);
