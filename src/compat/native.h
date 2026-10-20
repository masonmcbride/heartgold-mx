#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Suppress SDK-dependent header bodies, retaining original gameplay headers. */
#define POKEHEARTGOLD_GLOBAL_H
#define POKEHEARTGOLD_HEAP_H
#define POKEHEARTGOLD_STRING_H
#define POKEHEARTGOLD_FILESYSTEM_H
#define POKEHEARTGOLD_MSGDATA_H
#define POKEHEARTGOLD_SAVE_H
#define POKEHEARTGOLD_FIELD_TYPES_DEF_H
#define POKEHEARTGOLD_BAG_VIEW_H
#include <nitro/types.h>
#define NELEMS(a) (sizeof(a)/sizeof((a)[0]))
#define GF_ASSERT(x) do { if (!(x)) { fprintf(stderr,"Assertion failed: %s:%d: %s\n",__FILE__,__LINE__,#x);abort(); } } while (0)
#include "constants/heap.h"
#include "constants/save_arrays.h"
typedef struct String String;
typedef struct MsgData MsgData;
typedef struct SaveData SaveData;
typedef struct PlayerAvatar PlayerAvatar;
typedef struct FieldSystem FieldSystem;
typedef struct BagView BagView;
void *Heap_Alloc(enum HeapID, u32);
void *Heap_AllocAtEnd(enum HeapID, u32);
void Heap_Free(void *);
void Heap_FreeExplicit(enum HeapID, void *);
void MI_CpuClear8(void *, u32);
void MI_CpuClear16(void *, u32);
void MI_CpuCopy8(const void *, void *, u32);
void *SaveArray_Get(SaveData *, int);
BagView *BagView_New(u8);
struct ItemSlot;
void BagView_SetItem(BagView *, struct ItemSlot *, u8, u8);
/* Use original archive IDs; the local bag alias identifies its real archive. */
#include "filesystem_files_def.h"
#define NARC_native_bag NARC_a_0_1_5
typedef struct NARC NARC;
NARC *NARC_New(NarcId,enum HeapID);
void NARC_Delete(NARC *);
void NARC_ReadFromMember(NARC *,u32,u32,u32,void *);
void NARC_ReadFromAbsolutePos(NARC *,u32,u32,void *);
void NARC_GetMemberImageStartOffset(NARC *,u32,u32 *);
/* Original MsgDataLoadType values; direct loading remains unsupported. */
enum { MSGDATA_LOAD_DIRECT = 0, MSGDATA_LOAD_LAZY = 1 };
void *AllocAndReadWholeNarcMemberByIdPair(int, int, enum HeapID);
void *AllocAndReadFromNarcMemberByIdPair(int, int, enum HeapID, u32, u32);
MsgData *NewMsgDataFromNarc(int, int, int, enum HeapID);
void ReadMsgDataIntoString(MsgData *, int, String *);
void DestroyMsgData(MsgData *);
String *NewString_ReadMsgData(MsgData *, int);
/* Original list-menu drawing calls notify a native view adapter. */
#define POKEHEARTGOLD_BG_WINDOW_H
#define POKEHEARTGOLD_TEXT_H
typedef struct Window { u32 revision; u16 width, height; u8 cursor_x, cursor_y; } Window;
#include "system.h"
/* Input updates must preserve the original System's task queue pointers. */
static inline void native_set_keys(u16 pressed,u16 repeated) {
    gSystem.newKeys=pressed;gSystem.newAndRepeatedKeys=repeated;
}
#define REG_PAD_KEYINPUT_A_MASK 1
#define REG_PAD_KEYINPUT_B_MASK 2
#define REG_PAD_KEYINPUT_RIGHT_MASK 16
#define REG_PAD_KEYINPUT_LEFT_MASK 32
#define REG_PAD_KEYINPUT_UP_MASK 64
#define REG_PAD_KEYINPUT_DOWN_MASK 128
#define REG_PAD_KEYINPUT_R_MASK 256
#define REG_PAD_KEYINPUT_L_MASK 512
#define MAKE_TEXT_COLOR(fg,sh,bg) (((fg)<<16)|((sh)<<8)|(bg))
#define TEXT_SPEED_NOTRANSFER 255
void FillWindowPixelBuffer(Window *, u8);
void CopyWindowToVram(Window *);
void FillWindowPixelRect(Window *,u8,u16,u16,u16,u16);
void ScrollWindow(Window *,u8,u8,u8);
u16 GetWindowWidth(Window *);
u16 GetWindowHeight(Window *);


String *native_string_new(void);
void native_string_free(String *);
const char *native_string_utf8(const String *);

#define POKEHEARTGOLD_GF_GFX_LOADER_H
#include "constants/charcode.h"
enum GFPalLoadLocation { NATIVE_PAL_LOCATION_UNUSED };
enum GFPalSlotOffset { NATIVE_PAL_SLOT_UNUSED };
#include "font.h"
#include "sys_task_api.h"
#define MAX_TEXT_PRINTERS 8
#define TEXT_SPEED_INSTANT 0
void GenerateFontHalfRowLookupTable(u8,u8,u8);
void DecompressGlyphTile(const u8 *,u8 *);
u8 AddTextPrinterParameterizedWithColorAndSpacing(Window *,int,String *,u32,u32,u32,u32,u32,u32,PrinterCallback_t);
u16 *String_cstr(String *);
typedef struct { void *pRawData; } NNSG2dCharacterData;
void *GfGfxLoader_GetCharData(NarcId,s32,BOOL,NNSG2dCharacterData **,enum HeapID);
void MI_CpuCopy32(const void *,void *,u32);
void BlitBitmapRectToWindow(Window *,const void *,u16,u16,u16,u16,u16,u16,u16,u16);
