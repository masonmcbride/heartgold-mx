#pragma once
#include "compat/motion.h"
#include "compat/stats.h"
#define POKEHEARTGOLD_MAP_EVENTS_INTERNAL_H
#define POKEHEARTGOLD_SCRIPT_H
#define POKEHEARTGOLD_SCRIPT_POKEMON_UTIL_H
/* Encounter loading remains unlinked; retain the original versioned archive
 * identity without requiring generated indices for unused encounter entries. */
#define POKEHEARTGOLD_ENCOUNTER_TABLES_NARC_H
#ifdef HEARTGOLD
#define ENCDATA_NARC NARC_fielddata_encountdata_g_enc_data
#else
#define ENCDATA_NARC NARC_fielddata_encountdata_s_enc_data
#endif
#include "wild_encounter.h"
#include "event_types.inc"
#include "map_events.h"
#include "save_local_field_data.h"
#include "constants/species.h"
#include "constants/sprites.h"
typedef struct Pokemon Pokemon;
typedef struct Party Party;
Pokemon *GetFirstAliveMonInParty_CrashIfNone(Party *);
Party *SaveArray_Party_Get(SaveData *);
u32 GetMonData(Pokemon *,int,void *);
u32 GetNarcMemberSizeByIdPair(NarcId,int);
void ReadWholeNarcMemberByIdPair(void *,NarcId,int);
