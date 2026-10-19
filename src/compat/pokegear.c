/* Whole original save-record implementation. Only its save-array lookup and
 * SDK memory helpers cross this native boundary; no phone UI is implied. */
#include "compat/native.h"
#include "save_pokegear.h"
#include "constants/phone_constants.h"
#define POKEHEARTGOLD_OVERLAY_02_GEAR_PHONE_H
#define POKEHEARTGOLD_MATH_UTIL_H
u16 LCRandom(void);
SavePokegear *hg_story_save_pokegear(SaveData *);
static void *pokegear_save_array(SaveData *save,int id) {
    GF_ASSERT(save && id==SAVE_POKEGEAR);
    return hg_story_save_pokegear(save);
}
static void pokegear_fill(void *dest,u8 value,u32 size) { memset(dest,value,size); }
#define SaveArray_Get pokegear_save_array
#define MI_CpuFill8 pokegear_fill
#include HG_ORIGINAL_POKEGEAR
_Static_assert(sizeof(SavePokegear)==0x658,"Original Pokegear record layout changed");
_Static_assert(sizeof(PhoneCallPersistentState)==0x154,"Original phone record layout changed");
