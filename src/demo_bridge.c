#include "bag.h"
/* Keep the game's 32-bit BOOL ABI out of Objective-C's one-byte BOOL ABI. */
int hg_add_ball_demo(void) {
    Bag *b=Save_Bag_New(HEAP_ID_DEFAULT);
    int ok=Bag_AddItem(b,ITEM_ULTRA_BALL,1,HEAP_ID_DEFAULT);
    Heap_Free(b);return ok;
}
