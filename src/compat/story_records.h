#pragma once
#include <stdint.h>
/* Swap one owner's eight temporary-flag bytes with the original unit's static
 * array, then swap back after original execution. Main-thread boundary only. */
void hg_story_temp_exchange(uint8_t bytes[8]);
