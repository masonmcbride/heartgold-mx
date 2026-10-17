#pragma once
#include "compat/native.h"
#include "config.h"
/* Preserve original game constants while leaving unported graphics aliases out. */
#define POKEHEARTGOLD_CONSTANTS_GX_H
#define POKEHEARTGOLD_CONSTANTS_SDK_EXTRAS_H
#include "constants/global.h"
/* Calendar interfaces are declared only to compile the whole original matrix
 * unit. Save-dependent calendar/Safari entry points fail explicitly below. */
#define POKEHEARTGOLD_GF_RTC_H
typedef struct { u32 year,month,day,week; } RTCDate;
#define RTC_WEEK_WEDNESDAY 3
void GF_RTC_CopyDate(RTCDate *);
void MI_CpuFill16(void *,u16,u32);
void *AllocAtEndAndReadWholeNarcMemberByIdPair(NarcId,s32,enum HeapID);
