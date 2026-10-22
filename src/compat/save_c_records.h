#pragma once
#include "compat/pokemon.h"
#include "compat/misc.h"
#define POKEHEARTGOLD_POKEMON_H
#define POKEHEARTGOLD_SAV_SYSTEM_INFO_H
#include "daycare.h"
#include "save_palpad.h"
#include "save_link_ruleset.h"
#include "save_trainer_card.h"
#include "sav_chatot.h"
#include "save_special_ribbons.h"
#include "roamer.h"
#include "save_rankings.h"
#include "save_wifi_history.h"
#define POKEHEARTGOLD_CAMERA_H
#include "save_c_types.inc"
#include "mystery_gift.h"
#include "palPark_migration.h"
#include "pokeathlon/pokeathlon_save.h"
#include "easy_chat.h"
#include "unk_0203170C.h"
#include "save_follow_mon.h"
#include "safari_zone.h"
#include "photo_album.h"
#include "pokewalker.h"
#include "save_trainer_house.h"
#include "pokemon_storage_system.h"
#include "hall_of_fame.h"
void ReadMsgDataIntoU16Array(MsgData *,u32,u16 *);
void MI_CpuFill8(void *,u8,u32);
void MI_CpuClear32(void *,u32);
void SaveSubstruct_UpdateCRC(int);
void GF_RTC_CopyDateTime(RTCDate *,RTCTime *);
s32 RTC_ConvertDateToDay(const RTCDate *);
s64 RTC_ConvertDateTimeToSecond(const RTCDate *,const RTCTime *);
#define BERRY_POT_GROWTH_STAGE_NONE 0
u32 MTRandom(void);

typedef struct WiFiList WiFiList;
typedef struct DWCUserData DWCUserData;
DWCUserData *sub_0202C08C(WiFiList *);
BOOL DWC_CheckUserData(const DWCUserData *);
void DWC_CreateUserData(DWCUserData *,u32);
void DWC_ClearDirtyFlag(DWCUserData *);

/* Scoped sample shared by every original RTC consumer in one transaction. */
typedef struct HGSaveRTC { RTCDate date;RTCTime time; } HGSaveRTC;
const HGSaveRTC *hg_save_rtc_exchange(const HGSaveRTC *sample);
