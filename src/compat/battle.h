#pragma once
#include "compat/pokemon.h"
#include "bag.h"
#include "bag_cursor.h"
#include "pokemon_storage_system.h"
#include "constants/battle_menu.h"
#include "constants/battle_subscript.h"
#include "constants/move_effects.h"
#include "constants/message_tags.h"
#include "constants/battle_script.h"
#include "constants/opcode.h"
#include "constants/trainers.h"
#include "trainer_data.h"
#include "sys_task.h"
#include "wild_encounter.h"
#include "bug_contest_internal.h"
#include "constants/abilities.h"
#include "constants/weather.h"

#define ALPH_PUZZLE_MAX 4
#include "encounter_records.inc"
u32 PlayerAvatar_GetUnk8(PlayerAvatar *);
/* Native-only setup view consumed by unchanged trainer party generation. */

/* Native-only opaque presentation handles; never serialized or passed to ARM. */
typedef struct BgConfig BgConfig;
typedef struct BattleHpBar BattleHpBar;
typedef struct MessageFormat MessageFormat;
typedef struct PaletteData PaletteData;
typedef struct BagCursor BagCursor;
typedef struct SOUND_CHATOT SOUND_CHATOT;
typedef struct PokepicManager PokepicManager;
typedef struct SpriteSystem SpriteSystem;
typedef struct SpriteManager SpriteManager;
typedef struct BattleNumberPrinter BattleNumberPrinter;
typedef struct ManagedSprite ManagedSprite;
typedef struct BattleSystem BattleSystem;
typedef struct { u8 unk94[256];u8 battlerType;u8 native_controller,unk1A8;struct UnkBallData *ballData;ManagedSprite *managedSprite; } OpponentData;
typedef struct Bag Bag;
typedef struct Pokepic Pokepic;
typedef struct PokepicAnimScript PokepicAnimScript;
typedef void (*PokepicCallback)(Pokepic *,void *);
typedef struct PokedexData PokedexData;
typedef struct ObjCharTransferTasksManager ObjCharTransferTasksManager;
typedef struct MenuInputStateMgr MenuInputStateMgr;
typedef struct TextOBJ TextOBJ;
typedef int PMLCDTarget;
typedef struct UnkBallData UnkBallData;
typedef struct UnkStruct_134 UnkStruct_134;
typedef struct UnkStruct_50C UnkStruct_50C;
#define NewString_ReadMsgData hg_battle_NewString_ReadMsgData
#define FontID_String_GetWidth hg_battle_FontID_String_GetWidth
#include "string_util.h"

typedef int PaletteBufferId;
#define PLTTBUF_MAIN_BG 0
#define GF_BG_LYR_MAIN_1 1
#define GF_BG_LYR_MAIN_2 2
#include "battle_records.inc"
typedef struct SaveWiFiHistory SaveWiFiHistory;
typedef struct SavePalPad SavePalPad;
typedef struct EvolutionTaskData EvolutionTaskData;
#include "tutorial_setup_record.inc"

#include "dex_measure_ids.inc"
#define TRAINER_MALE 0

#include "constants/sndseq.h"
#include "battle_message_ids.inc"
#define PAD_BUTTON_A 1
#define PAD_BUTTON_B 2
#define PAD_BUTTON_X 1024
#define PAD_BUTTON_Y 2048
#define RGB_BLACK 0
#define PM_LCD_TOP 0
#define PM_LCD_BOTTOM 1
#define POKEPIC_X 0
extern const OverlayManagerTemplate gOverlayTemplate_NamingScreen;
void CP_SetSqrt32(u32);
void CP_WaitSqrt(void);
u32 CP_GetSqrtResult32(void);
void MI_CpuClear32(void *,u32);

#include "screen_fade_types.inc"
void sub_02074F9C(BattleSystem *,int,int,void *,u8);

void G2_SetBG0Priority(int);

#undef OFFSET_OF
#define OFFSET_OF(type,field) offsetof(__typeof__(*((type)0)),field)
u8 ov10_0221BF44(BattleSystem *,BattleContext *);
u8 ov10_0221C038(BattleSystem *,BattleContext *);
