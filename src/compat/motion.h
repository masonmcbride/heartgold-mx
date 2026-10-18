#pragma once
#include "compat/props.h"
#include "compat/field_tasks.h"
#undef POKEHEARTGOLD_FIELD_TYPES_DEF_H
#include "field_types_def.h"
#include "motion_types.inc"
#define POKEHEARTGOLD_OVERLAY_00_THUMB_H
#define POKEHEARTGOLD_OVERLAY_01_H
#define POKEHEARTGOLD_OVERLAY_01_021FE780_H
#define POKEHEARTGOLD_OVERLAY_01_021F944C_H
#define POKEHEARTGOLD_PLAYER_DATA_H
#define POKEHEARTGOLD_SCRCMD_H
#define POKEHEARTGOLD_UNK_02034354_H
#define POKEHEARTGOLD_UNK_02035900_H
#include "player_avatar.h"
#include "map_object.h"
#include "sys_task_api.h"
typedef struct PlayerProfile PlayerProfile;
/* Original provider signatures needed to type-check entire units. Definitions
 * are not supplied unless a reachable integration path actually requires them. */
#define POKEHEARTGOLD_ASSERT_H
#include "constants/scrcmd.h"
int ov00_021E6EBC(void);
PlayerProfile *sub_02034818(u32);
u16 sub_0203769C(void);
u32 ov01_021FE7DC(LocalMapObject *,u32,u32,u32,u32);
void ov01_021F9FB0(MapObjectManager *,void *);
u16 FieldSystem_VarGetObjectEventGraphicsId(FieldSystem *,u16);
u16 FieldSystem_VarGet(FieldSystem *,u16);
BOOL FieldSystem_FlagCheck(FieldSystem *,u16);
void FieldSystem_FlagSet(FieldSystem *,u16);
