#pragma once
#include "compat/pokemon.h"
#include "compat/apricorn.h"
#include "gymmick.h"
#include "misc_records.inc"
_Static_assert(sizeof(SAVE_MISC_DATA)==0x2e0,"Original misc save layout");
SAVE_MISC_DATA *hg_story_save_misc(SaveData *);
