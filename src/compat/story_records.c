#include "compat/story_records.h"
#include "save_vars_flags.h"
#include HG_ORIGINAL_SAVE_FLAGS
_Static_assert(sizeof(sTempFlags)==8,"Original temporary flag storage changed");
void hg_story_temp_exchange(uint8_t bytes[8]) {
    u8 previous[8];memcpy(previous,sTempFlags,8);memcpy(sTempFlags,bytes,8);memcpy(bytes,previous,8);
}
