#pragma once
#include "compat/script.h"
#include "field_script.h"
#include "save_vars_flags.h"
#include "player_data.h"
#include "party.h"
#include "options.h"
#include "constants/global.h"
#include "constants/sprites.h"
#include "story_types.inc"
#include "constants/species.h"
#include "constants/game_stats.h"
#include "pokemon_records.inc"
#define POKEHEARTGOLD_POKEDEX_H
#define SCRIPT_COMPARISON_RESULT_LESS 0
#define SCRIPT_COMPARISON_RESULT_EQUAL 1
#define SCRIPT_COMPARISON_RESULT_GREATER 2
u16 *GetVarPointer(FieldSystem *,u16);
u16 FieldSystem_VarGet(FieldSystem *,u16);
BOOL FieldSystem_VarSet(FieldSystem *,u16,u16);
u16 FieldSystem_VarGetObjectEventGraphicsId(FieldSystem *,u16);
BOOL FieldSystem_FlagCheck(FieldSystem *,u16);
void FieldSystem_FlagSet(FieldSystem *,u16);
void FieldSystem_FlagClear(FieldSystem *,u16);
void PlayerProfile_SetVersion(PlayerProfile *,u8);
void SetLCRNGSeed(u32);
u32 GetLCRNGSeed(void);
u16 LCRandom(void);
/* Explicit native save-owner handle, not a full original SaveData. */
void hg_story_bind_field(HGStoryState *,FieldSystem *);
/* Native scene services clone their objects for both preflight and replay.
 * The script owner never mutates the source scene during validation. */
typedef struct {
    const void *source;
    void *(*clone)(const void *);
    void (*destroy)(void *);
    void (*bind)(void *,FieldSystem *,int preview);
    int (*apply)(void *,unsigned,const uint8_t *,size_t);
    int (*tick)(void *);
    unsigned (*active)(const void *);
    int (*busy)(const void *);
    int (*command)(void *,ScriptContext *,unsigned,BOOL *);
    int (*face_player)(void *);
} HGScriptSceneServices;
HGFieldScript *hg_field_script_scene_new(HGStoryState *,unsigned map,unsigned entry,const HGScriptSceneServices *,char *,size_t);
const void *hg_field_script_scene(const HGFieldScript *);

int hg_script_scene_command(ScriptContext *,unsigned,BOOL *);
HGFieldScript *hg_field_script_scene_bytes(HGStoryState *,const uint8_t *,size_t,unsigned,const HGScriptSceneServices *,char *,size_t);
Party *hg_story_save_party(SaveData *);

GameStats *hg_story_save_stats(SaveData *);
Pokedex *hg_story_save_dex(SaveData *);
PlayerProfile *hg_story_save_profile(SaveData *);
Options *hg_story_save_options(SaveData *);

struct Bag;struct Bag *hg_story_save_bag(SaveData *);

struct SavePokegear;struct SavePokegear *hg_story_save_pokegear(SaveData *);

SysInfo *hg_story_save_sys_info(SaveData *);
u16 *hg_story_save_blackout_spawn(SaveData *);
u32 hg_story_rng_get(const HGStoryState *);
void hg_story_rng_set(HGStoryState *,u32);
void hg_story_touch(HGStoryState *);
