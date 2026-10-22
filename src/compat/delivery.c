#include "compat/delivery.h"
#include "compat/save_c_records.h"
/* The original global query pointer is rebound only for this synchronous call.
 * Script BEGIN/END ownership belongs to each private script save candidate. */
static MysteryGiftSave *sMysteryGiftSaveData;
#include "delivery_queries.inc"
unsigned hg_delivery_gift_tag(SaveData *save){
    GF_ASSERT(save);MysteryGiftSave *previous=sMysteryGiftSaveData;
    sMysteryGiftSaveData=SaveArray_Get(save,SAVE_MYSTERY_GIFT);
    unsigned tag=SaveMGDataPtr_GetTagByIndex(SaveMGDataPtr_GetFirstGiftIndex());
    sMysteryGiftSaveData=previous;return tag;
}
