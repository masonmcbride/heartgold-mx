#include "inventory.h"
#include "bag.h"
#include "list_menu.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static unsigned callbacks;
static void changed(ListMenu *m,s32 value,u8 init) { (void)m;(void)value;(void)init;callbacks++; }
int main(void) {
    /* Execute the original menu with a header, scrolling, confirmation and cancel. */
    Window w={.width=32,.height=24};ListMenuItem entries[14];
    for(int i=0;i<14;i++)entries[i]=(ListMenuItem){.text=NULL,.value=i};entries[1].value=LIST_HEADER;
    ListMenuTemplate t={.items=entries,.window=&w,.totalItems=14,.maxShowed=4,.moveCursorFunc=changed,.scrollMultiple=LIST_MULTIPLE_SCROLL_L_R};
    ListMenu *m=ListMenuInit(&t,0,0,HEAP_ID_DEFAULT);CHECK(callbacks==1);
    native_set_keys(0,REG_PAD_KEYINPUT_DOWN_MASK);CHECK(ListMenu_ProcessInput(m)==LIST_NOTHING_CHOSEN);
    u16 selected;ListMenuGetCurrentItemArrayId(m,&selected);CHECK(selected==2);
    for(int i=0;i<30;i++)ListMenu_ProcessInput(m);
    ListMenuGetCurrentItemArrayId(m,&selected);CHECK(selected==13);CHECK(m->cursorPos==10);CHECK(callbacks>2);
    native_set_keys(REG_PAD_KEYINPUT_A_MASK,0);CHECK(ListMenu_ProcessInput(m)==13);
    native_set_keys(REG_PAD_KEYINPUT_B_MASK,0);CHECK(ListMenu_ProcessInput(m)==LIST_CANCEL);
    native_set_keys(0,0);DestroyListMenu(m,NULL,NULL);
    HGInventory *v=hg_inventory_new();CHECK(hg_inventory_pocket(v)==0);
    HGItem item;CHECK(hg_inventory_item(v,0,&item));CHECK(!strcmp(item.name,"Repel"));CHECK(*item.description);
    unsigned before=item.quantity;CHECK(hg_inventory_action(v,HG_ADD));CHECK(hg_inventory_item(v,0,&item));CHECK(item.quantity==before+1);
    CHECK(hg_inventory_action(v,HG_REMOVE));CHECK(hg_inventory_item(v,0,&item));CHECK(item.quantity==before);
    for(unsigned i=0;i<1100;i++)hg_inventory_action(v,HG_ADD);
    CHECK(hg_inventory_item(v,0,&item));CHECK(item.quantity==999);CHECK(!hg_inventory_action(v,HG_ADD));
    hg_inventory_action(v,HG_NEXT_POCKET);CHECK(hg_inventory_pocket(v)==1);CHECK(hg_inventory_count(v)>8);
    hg_inventory_select(v,hg_inventory_count(v)-1);CHECK(hg_inventory_scroll(v)>0);unsigned last=hg_inventory_selected(v);
    hg_inventory_action(v,HG_NEXT_POCKET);hg_inventory_action(v,HG_PREVIOUS_POCKET);CHECK(hg_inventory_selected(v)==last);
    CHECK(hg_inventory_item(v,last,&item));CHECK(hg_inventory_action(v,HG_CONFIRM)==item.id);
    while(hg_inventory_count(v))CHECK(hg_inventory_action(v,HG_REMOVE));
    CHECK(!hg_inventory_action(v,HG_REMOVE));CHECK(!hg_inventory_item(v,0,&item));CHECK(hg_inventory_action(v,HG_ADD));
    CHECK(hg_inventory_item(v,0,&item));CHECK(item.id==ITEM_POTION && item.quantity==1);
    hg_inventory_action(v,HG_NEXT_POCKET); /* Balls */
    int unicode_found=0;
    for(unsigned i=0;i<hg_inventory_count(v);i++) {
        CHECK(hg_inventory_item(v,i,&item));
        if(item.id==ITEM_POKE_BALL) { CHECK(!strcmp(item.name,"Poké Ball"));unicode_found=1; }
    }
    CHECK(unicode_found);
    void *graphic=LoadItemDataOrGfx(ITEM_ULTRA_BALL,ITEMNARC_NCGR,HEAP_ID_DEFAULT);
    CHECK(!memcmp(graphic,"RGCN",4));Heap_Free(graphic);
    CHECK(hg_validate_catalog());hg_inventory_free(v);
    puts("Original list menu, inventory mutations, pocket memory, and full item catalog passed");return 0;
}
