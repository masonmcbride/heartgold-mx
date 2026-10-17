#include "bag.h"
#include "compat/scheduler.h"
int main(void) {
    Bag *bag = Save_Bag_New(HEAP_ID_DEFAULT);
    if (!Bag_AddItem(bag, ITEM_POTION, 5, HEAP_ID_DEFAULT) || !Bag_TakeItem(bag, ITEM_POTION, 2, HEAP_ID_DEFAULT)) return 1;
    printf("HeartGold MX: pointer=%zu bits; bag=%u bytes; Potion quantity=%u; price=%d; TM01 move=%u\n", sizeof(void *)*8, Save_Bag_sizeof(), Bag_GetQuantity(bag, ITEM_POTION, HEAP_ID_DEFAULT), GetItemAttr(ITEM_POTION, ITEMATTR_PRICE, HEAP_ID_DEFAULT), TMHMGetMove(ITEM_TM01));
    Heap_Free(bag);
    if(!hg_scheduler_init())return 1;
    for(unsigned frame=0;frame<60;frame++)if(!hg_scheduler_step())return 1;
    printf("Original SysTask queues: %llu native frames dispatched\n",(unsigned long long)hg_scheduler_ticks());
    return hg_scheduler_shutdown()?0:1;
}
