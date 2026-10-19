#pragma once
#include "compat/story.h"
#include "item.h"
#include "move.h"
#include "constants/abilities.h"
#include "constants/balls.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/map_sections.h"
#include "constants/moves.h"
#include "constants/trainer_class.h"
#include "constants/mail.h"
#include "constants/johto_dex.h"
#include "constants/events.h"
#include "map_section.h"
#include "constants/easy_chat.h"
/* Original encoded String records are distinct from the inventory UTF-8 owner. */
#define String HGMonString
#define String_New hg_mon_string_new
#define String_Delete hg_mon_string_delete
#define String_Copy hg_mon_string_copy
#define String_Compare hg_mon_string_compare
#define PlayerProfile_GetPlayerName_NewString hg_mon_profile_name
#define PlayerName_FlatToString hg_mon_profile_to_string
#define CopyStringToU16Array hg_mon_string_to_array
#define CopyU16ArrayToString hg_mon_string_from_array
#include "pokemon_string_type.inc"
#include "pokemon_macros.inc"
#include "pokemon_decls.inc"
extern const u8 gGameLanguage,gGameVersion;
void _MonEncryptSegment(u16 *,u32,u32);
void _MonDecryptSegment(u16 *,u32,u32);

void MI_CpuClear8(void *,u32);

typedef RTCDate HGMonDate;
typedef struct {
    void *owner;
    BOOL (*location_compatible)(void *,unsigned);
    int (*map_location)(void *,unsigned,unsigned);
    void (*date)(void *,HGMonDate *);
} HGPokemonServices;
HGPokemonServices hg_pokemon_services_exchange(HGPokemonServices);

const char *hg_pokemon_species_name(unsigned);
