#include "compat/pokemon.h"
#include "field_menu.h"
#include "compat/item_use.h"
#include "inventory.h"
#include "game_font.h"
#include "compat/resources.h"
#include <stdarg.h>
struct HGFieldMenu { SaveData *save;unsigned section,item,move;int stage,changed;char message[256];struct Bag *bag;Party *party;HGInventory *inventory;HGFont *font;int kind;unsigned selected,swap_from;HGFieldPartySwap swap;void *swap_owner; };
void hg_field_menu_set_party_swap(HGFieldMenu *m,HGFieldPartySwap swap,void *owner){if(m){m->swap=swap;m->swap_owner=owner;}}
static void append(char *out,size_t size,const char *fmt,...){size_t used=strlen(out);if(used>=size)return;va_list args;va_start(args,fmt);vsnprintf(out+used,size-used,fmt,args);va_end(args);}
HGFieldMenu *hg_field_menu_new(SaveData *save,unsigned section){
    if(!save)return NULL;struct Bag *bag=hg_story_save_bag(save);Party *party=hg_story_save_party(save);HGFieldMenu *m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->save=save;m->section=section;m->bag=bag;m->party=party;m->font=hg_font_new();if(!m->font){free(m);return NULL;}return m;
}
void hg_field_menu_free(HGFieldMenu *m){if(m){hg_inventory_free(m->inventory);hg_font_free(m->font);free(m);}}
int hg_field_menu_kind(const HGFieldMenu *m){return m?m->kind:0;}
void hg_field_menu_open(HGFieldMenu *m,int kind){
    if(!m)return;hg_inventory_free(m->inventory);m->inventory=NULL;m->selected=0;m->kind=kind;m->stage=0;m->swap_from=6;m->message[0]=0;
    if(kind==1){m->inventory=hg_inventory_new_bag(m->bag);if(!m->inventory)m->kind=0;}
}
void hg_field_menu_input(HGFieldMenu *m,uint16_t keys){
    if(!m||!m->kind)return;
    if(m->kind==1&&m->stage){
        unsigned count=m->stage==2?4:(unsigned)Party_GetCount(m->party);if(m->stage==3||!count)return;
        unsigned *selected=m->stage==2?&m->move:&m->selected;
        if(keys&(0x40|0x20))*selected=(*selected+count-1)%count;
        if(keys&(0x80|0x10))*selected=(*selected+1)%count;
    }else if(m->kind==1){
        if(keys&0x20)hg_inventory_action(m->inventory,HG_PREVIOUS_POCKET);
        if(keys&0x10)hg_inventory_action(m->inventory,HG_NEXT_POCKET);
        if(keys&0x40)hg_inventory_action(m->inventory,HG_UP);
        if(keys&0x80)hg_inventory_action(m->inventory,HG_DOWN);
    }else {unsigned count=Party_GetCount(m->party);if(!count)return;
        if(keys&(0x40|0x20))m->selected=(m->selected+count-1)%count;
        if(keys&(0x80|0x10))m->selected=(m->selected+1)%count;
    }
}
int hg_field_menu_take_changed(HGFieldMenu *m){if(!m)return 0;int result=m->changed;m->changed=0;return result;}
void hg_field_menu_confirm(HGFieldMenu *m){
    if(m&&m->kind==2){
        unsigned count=Party_GetCount(m->party);if(m->selected>=count||count<2)return;
        if(m->swap_from==6){m->swap_from=m->selected;m->message[0]=0;return;}
        int canceled=m->swap_from==m->selected;
        int ok=!canceled&&m->swap&&m->swap(m->swap_owner,m->swap_from,m->selected);
        snprintf(m->message,sizeof(m->message),"%s",canceled?"Switch canceled.":ok?"Party order changed.":"Couldn't switch. Your party is unchanged.");
        m->swap_from=6;return;
    }
    if(!m||m->kind!=1)return;
    if(!m->stage){HGItem item;if(!hg_inventory_item(m->inventory,hg_inventory_selected(m->inventory),&item))return;
        m->item=item.id;m->selected=m->move=0;
        if(!hg_item_use_supported(item.id)||!Party_GetCount(m->party)){snprintf(m->message,sizeof(m->message),"This item needs an application that is not available yet.");m->stage=3;}else m->stage=1;return;
    }
    if(m->stage==3){unsigned pocket=hg_inventory_pocket(m->inventory);hg_inventory_free(m->inventory);m->inventory=hg_inventory_new_bag(m->bag);if(!m->inventory){m->kind=0;return;}for(unsigned i=0;i<8&&hg_inventory_pocket(m->inventory)!=pocket;i++)hg_inventory_action(m->inventory,HG_NEXT_POCKET);m->stage=0;return;}
    if(m->stage==1&&(GetItemAttr(m->item,ITEMATTR_PP_RESTORE,HEAP_ID_FIELD2)||GetItemAttr(m->item,ITEMATTR_PP_UP,HEAP_ID_FIELD2)||GetItemAttr(m->item,ITEMATTR_PP_MAX,HEAP_ID_FIELD2))){m->stage=2;return;}
    int result=hg_item_use(m->save,m->item,m->selected,m->move,m->section);m->changed|=result==HG_ITEM_USE_APPLIED;
    snprintf(m->message,sizeof(m->message),"%s",result==HG_ITEM_USE_APPLIED?"The item was used.":result==HG_ITEM_USE_NO_EFFECT?"It won't have any effect.":"The item could not be used.");m->stage=3;
}
void hg_field_menu_text(HGFieldMenu *m,char *out,size_t size){
    if(!out||!size)return;out[0]=0;if(!m||!m->kind)return;
    if(m->kind==1&&m->stage){
        HGItem item;if(!hg_item_details(m->item,&item))return;append(out,size,"BAG / %s\n\n",item.name);
        if(m->stage==3){append(out,size,"%s\n\nEnter continue",m->message);return;}
        if(m->stage==1){append(out,size,"Choose a Pokemon\nW/S select  Enter use\n\n");for(unsigned i=0;i<(unsigned)Party_GetCount(m->party);i++){Pokemon *mon=Party_GetMonByIndex(m->party,i);append(out,size,"%c %s HP %u/%u\n",i==m->selected?'>':' ',hg_pokemon_species_name(GetMonData(mon,MON_DATA_SPECIES_OR_EGG,NULL)),GetMonData(mon,MON_DATA_HP,NULL),GetMonData(mon,MON_DATA_MAX_HP,NULL));}return;}
        Pokemon *mon=Party_GetMonByIndex(m->party,m->selected);append(out,size,"Choose a move\nW/S select  Enter use\n\n");for(unsigned i=0;i<4;i++){unsigned move=GetMonData(mon,MON_DATA_MOVE1+i,NULL);const u8 *name;size_t n;if(move&&native_resource_member(NARC_msgdata_msg,750,move,&name,&n))append(out,size,"%c %s PP %u\n",i==m->move?'>':' ',name,GetMonData(mon,MON_DATA_MOVE1_PP+i,NULL));}return;
    }else if(m->kind==1){
        append(out,size,"BAG / %s\nA/D pockets  W/S items\n\n",hg_inventory_pocket_name(hg_inventory_pocket(m->inventory)));
        unsigned count=hg_inventory_count(m->inventory),selected=hg_inventory_selected(m->inventory),start=selected/6*6;
        if(!count)append(out,size,"This pocket is empty.\n");
        for(unsigned i=start;i<count&&i<start+6;i++){HGItem item;if(hg_inventory_item(m->inventory,i,&item))append(out,size,"%c %s x%u\n",i==selected?'>':' ',item.name,item.quantity);}
        HGItem item;if(hg_inventory_item(m->inventory,selected,&item))append(out,size,"\n%s\n\nEnter: use on a Pokemon",item.description);
    }else {
        unsigned count=Party_GetCount(m->party);append(out,size,"POKEMON / %u in party\nW/S or arrows: select\nEnter: pick up / place\n",count);
        if(!count){append(out,size,"No Pokemon yet.\nChoose a starter at Elm's lab.");return;}
        for(unsigned i=0;i<count;i++){Pokemon *p=Party_GetMonByIndex(m->party,i);append(out,size,"%c%u %s Lv%u%s\n",i==m->selected?'>':' ',i+1,hg_pokemon_species_name(GetMonData(p,MON_DATA_SPECIES_OR_EGG,NULL)),GetMonData(p,MON_DATA_LEVEL,NULL),i==m->swap_from?" *":"");}
        append(out,size,"%s\n\n",m->swap_from<6?"Choose destination; Enter to switch.":m->message);
        Pokemon *mon=Party_GetMonByIndex(m->party,m->selected);unsigned species=GetMonData(mon,MON_DATA_SPECIES_OR_EGG,NULL),level=GetMonData(mon,MON_DATA_LEVEL,NULL);
        append(out,size,"%u/%u  %s Lv%u\nHP %u/%u %s EXP %u\n",m->selected+1,count,hg_pokemon_species_name(species),level,GetMonData(mon,MON_DATA_HP,NULL),GetMonData(mon,MON_DATA_MAX_HP,NULL),GetMonData(mon,MON_DATA_STATUS,NULL)&7?"Sleep":GetMonData(mon,MON_DATA_STATUS,NULL)&0x88?"Poison":GetMonData(mon,MON_DATA_STATUS,NULL)&16?"Burn":GetMonData(mon,MON_DATA_STATUS,NULL)&32?"Frozen":GetMonData(mon,MON_DATA_STATUS,NULL)&64?"Paralysis":"OK",GetMonData(mon,MON_DATA_EXPERIENCE,NULL));
        append(out,size,"Atk %u  Def %u  Speed %u\nSp.Atk %u  Sp.Def %u\n\n",GetMonData(mon,MON_DATA_ATK,NULL),GetMonData(mon,MON_DATA_DEF,NULL),GetMonData(mon,MON_DATA_SPEED,NULL),GetMonData(mon,MON_DATA_SP_ATK,NULL),GetMonData(mon,MON_DATA_SP_DEF,NULL));
        for(unsigned i=0;i<4;i++){unsigned move=GetMonData(mon,MON_DATA_MOVE1+i,NULL);const uint8_t *name;size_t length;
            if(move&&native_resource_member(NARC_msgdata_msg,750,move,&name,&length))append(out,size,"%s PP %u\n",(const char *)name,GetMonData(mon,MON_DATA_MOVE1_PP+i,NULL));
        }
        unsigned held=GetMonData(mon,MON_DATA_HELD_ITEM,NULL);HGItem item;append(out,size,"Held item: %s",held&&hg_item_details(held,&item)?item.name:"None");
    }
}
int hg_field_menu_render(HGFieldMenu *m,uint8_t *pixels,unsigned width,unsigned height){
    if(!m||!m->kind||!pixels||width<256||height<384)return 0;
    enum{W=256,H=384};uint8_t panel[W*H*4];
    for(unsigned i=0;i<W*H;i++){panel[i*4]=234;panel[i*4+1]=241;panel[i*4+2]=245;panel[i*4+3]=255;}
    char text[4096];hg_field_menu_text(m,text,sizeof(text));hg_font_draw(m->font,text,panel,W,H,10,10,236,0x182838,0);
    hg_font_draw(m->font,"Esc / B / P close",panel,W,H,10,362,236,0x384858,0);
    unsigned scale=height/H;if(scale>width/2/W)scale=width/2/W;if(!scale)scale=1;
    unsigned left=width-W*scale,top=(height-H*scale)/2;
    for(unsigned y=0;y<H*scale;y++)for(unsigned x=0;x<W*scale;x++)memcpy(pixels+((size_t)(top+y)*width+left+x)*4,panel+((y/scale)*W+x/scale)*4,4);
    return 1;
}
