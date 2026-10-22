#pragma once
/* The manager has the original pointer-free SaveData layout, including every
 * record and its original integrity/async bookkeeping. */
#include "native.h"
#undef POKEHEARTGOLD_SAVE_H
#include "save.h"
#ifndef HG_SAVE_REGISTRY_TYPES
#define HG_SAVE_REGISTRY_TYPES
/* Avoid importing unrelated overlay headers just to describe the registry ABI. */
#define POKEHEARTGOLD_SAVE_ARRAYS_H
typedef u32 (*SAVESIZEFN)(void);
typedef void (*SAVEINITFN)(void *);
struct SaveChunkHeader { int id; u32 block; SAVESIZEFN sizeFunc; SAVEINITFN initFunc; };
struct ExtraSaveChunkHeader { int id; u32 sector; SAVESIZEFN sizeFunc; SAVEINITFN initFunc; };
extern const struct SaveChunkHeader gSaveChunkHeaders[];
extern const int gNumSaveChunkHeaders;
extern const struct ExtraSaveChunkHeader gExtraSaveChunkHeaders[];
extern const int gNumExtraSaveChunkHeaders;
#endif
void hg_save_manager_init_ram(SaveData *save);
void hg_save_manager_release(SaveData *save);
int hg_save_manager_validate_ram(const SaveData *save);
/* Select the native authoritative owner and reload original guest BSS mirrors.
 * Runtime guest mutations must be committed to owner bytes before switching. */
void hg_save_manager_bind(SaveData *save);
/* Scoped consumers restore this returned owner before releasing candidates. */
SaveData *hg_save_manager_exchange(SaveData *save);
/* Scoped operations keep the live owner unchanged on transport/integrity failure.
 * Caller opens ds_save_flash first; load requires an existing valid game. */
int hg_save_manager_write(SaveData *live, char *error, size_t error_size);
int hg_save_manager_load(SaveData *out, char *error, size_t error_size);
/* Execute up to max_steps of the original asynchronous manager. Cancel and
 * retain live bytes if unfinished; completed receives 0 (cancelled) or 1. */
int hg_save_manager_write_steps(SaveData *live,unsigned max_steps,int *completed,char *error,size_t error_size);
/* Bounded allocation-failure injection, disabled by negative count. */
void hg_save_manager_fault_alloc_after(int successful_allocations);
/* MIRRORED uses original Read/WriteExtraSaveChunk (Hall of Fame and wipe).
 * JOURNALED uses original sub_020284A4/sub_02028230 (records1..5), including
 * normal-save misc generation references. Persist the normal save after writing
 * a journaled extra record. Payload size is exactly its registry sizeFunc();
 * footer storage is owned internally. Outputs remain unchanged on failure. */
typedef enum { HG_SAVE_EXTRA_MIRRORED, HG_SAVE_EXTRA_JOURNALED } HGSaveExtraMode;
int hg_save_manager_write_extra(SaveData *live,int id,void *payload,size_t payload_size,HGSaveExtraMode mode,char *error,size_t error_size);
int hg_save_manager_read_extra(SaveData *live,int id,void *payload,size_t payload_size,HGSaveExtraMode mode,int *recovered,char *error,size_t error_size);
