#pragma once
#include "compat/pokemon.h"
enum HGItemUseResult {HG_ITEM_USE_UNSUPPORTED=-2,HG_ITEM_USE_INVALID=-1,HG_ITEM_USE_NO_EFFECT=0,HG_ITEM_USE_APPLIED=1};
/* Original effects against this save owner. Consumption publishes only when
 * the original effect succeeds; unsupported applications never spend items. */
int hg_item_use(SaveData *,unsigned item,unsigned party_slot,unsigned move,unsigned map_section);
int hg_item_use_supported(unsigned item);
