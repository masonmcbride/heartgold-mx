#pragma once
#include "compat/props.h"
#define POKEHEARTGOLD_POKEDEX_H
typedef struct Pokedex Pokedex;
#include "game_stats.h"
BOOL Pokedex_CheckMonCaughtFlag(const Pokedex *,u16);
void MI_CpuClear32(void *,u32);
u32 OS_GetVBlankCount(void);
typedef s16 fx16;
typedef struct MtxFx22 MtxFx22;
/* Compile-only native CRC storage. No CRC SDK provider is linked or called. */
typedef struct { u16 entries[256]; } MATHCRC16Table;
#define FX32_SHIFT 12
#define FX_FX16_TO_F32(x) ((double)(x)/4096.0)
u16 FX_DEG_TO_IDX(fx32);
fx16 FX_SinIdx(int);
fx16 FX_CosIdx(int);
void MTX_Rot22_(MtxFx22 *,fx32,fx32);
void MTX_ScaleApply22(const MtxFx22 *,MtxFx22 *,fx32,fx32);
u16 MATH_CalcCRC16CCITT(const MATHCRC16Table *,const void *,u32);
void MATH_CRC16CCITTInitTable(MATHCRC16Table *);
