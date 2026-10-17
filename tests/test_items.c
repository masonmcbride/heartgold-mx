#include "bag.h"
#include "constants/moves.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
int main(void) {
    Bag *b = Save_Bag_New(HEAP_ID_DEFAULT);
    CHECK(sizeof(ItemSlot)==4); CHECK(sizeof(ItemData)==36);
    CHECK(!Bag_PocketNotEmpty(b, POCKET_MEDICINE));
    CHECK(GetItemAttr(ITEM_POTION, ITEMATTR_PRICE, HEAP_ID_DEFAULT)==300);
    CHECK(GetItemAttr(ITEM_POTION, ITEMATTR_HP_RESTORE_PARAM, HEAP_ID_DEFAULT)==20);
    CHECK(Bag_AddItem(b, ITEM_POTION, 998, HEAP_ID_DEFAULT));
    CHECK(Bag_AddItem(b, ITEM_POTION, 1, HEAP_ID_DEFAULT));
    CHECK(!Bag_AddItem(b, ITEM_POTION, 1, HEAP_ID_DEFAULT));
    CHECK(!Bag_TakeItem(b, ITEM_POTION, 1000, HEAP_ID_DEFAULT));
    CHECK(Bag_GetQuantity(b, ITEM_POTION, HEAP_ID_DEFAULT)==999);
    CHECK(Bag_AddItem(b, ITEM_ANTIDOTE, 2, HEAP_ID_DEFAULT));
    CHECK(Bag_TakeItem(b, ITEM_POTION, 999, HEAP_ID_DEFAULT));
    CHECK(b->medicine[0].id==ITEM_ANTIDOTE); CHECK(b->medicine[1].id==0);
    CHECK(Bag_AddItem(b, ITEM_TM02, 99, HEAP_ID_DEFAULT));
    CHECK(!Bag_AddItem(b, ITEM_TM02, 1, HEAP_ID_DEFAULT));
    CHECK(Bag_AddItem(b, ITEM_TM01, 1, HEAP_ID_DEFAULT));
    CHECK(b->TMsHMs[0].id==ITEM_TM01 && b->TMsHMs[1].id==ITEM_TM02);
    CHECK(TMHMGetMove(ITEM_TM01)==MOVE_FOCUS_PUNCH);
    CHECK(MoveIsHM(MOVE_SURF)); CHECK(!MoveIsHM(MOVE_FOCUS_PUNCH));
    CHECK(BallToItemId(ItemToBallId(ITEM_ULTRA_BALL))==ITEM_ULTRA_BALL);
    CHECK(ItemIdIsBerry(ITEM_CHERI_BERRY)); CHECK(!ItemIdIsBerry(ITEM_POTION));
    CHECK(ItemToMailId(MailToItemId(0))==0);
    CHECK(Bag_TryRegisterItem(b, ITEM_BICYCLE)==REG_ITEM_SLOT1);
    CHECK(Bag_TryRegisterItem(b, ITEM_OLD_ROD)==REG_ITEM_SLOT2);
    CHECK(Bag_TryRegisterItem(b, ITEM_GOOD_ROD)==REG_ITEM_FAIL);
    Bag_UnregisterItem(b, ITEM_BICYCLE); CHECK(Bag_GetRegisteredItem1(b)==ITEM_OLD_ROD);
    Bag copy; Save_Bag_Copy(b,&copy); CHECK(memcmp(b,&copy,sizeof(copy))==0);
    CHECK(Bag_AddItem(b, ITEM_CHESTO_BERRY, 1, HEAP_ID_DEFAULT));
    CHECK(Bag_AddItem(b, ITEM_CHERI_BERRY, 1, HEAP_ID_DEFAULT));
    CHECK(b->berries[0].id==ITEM_CHERI_BERRY);
    MoveItemSlotInList(b->berries,0,1,POCKET_BERRIES,HEAP_ID_DEFAULT);
    CHECK(b->berries[1].id==ITEM_CHERI_BERRY);
    for(unsigned i=0;i<NUM_BAG_ITEMS;i++) { b->items[i].id=ITEM_REPEL; b->items[i].quantity=1; }
    CHECK(!Bag_AddItem(b, ITEM_ESCAPE_ROPE, 1, HEAP_ID_DEFAULT));
    CHECK(Bag_GetPocketSlotN(b,POCKET_ITEMS,NUM_BAG_ITEMS)==NULL);
    CHECK(GetItemIndexMapping(ITEM_TM01,ITEMNARC_PARAM)==306);
    CHECK(GetItemAttr(ITEM_TM01,ITEMATTR_FIELD_POCKET,HEAP_ID_DEFAULT)==POCKET_TMHMS);
    Heap_Free(b); puts("Original item/bag tests passed"); return 0;
}
