#pragma once
#include "compat/native.h"
/* Native static-module identity, not a serialized overlay address. */
typedef s32 FSOverlayID;
#include "overlay_manager.h"
#include "poke_overlay.h"
/* Explicitly admit a linked native module before original load requests.
 * Main-thread-only; no dynamic code or DS overlay relocation is performed. */
int hg_overlay_register(FSOverlayID);
/* Statically linked modules may own native initialization/teardown. Hooks must
 * roll back their own work on failure; residency publishes only after success.
 * NULL hooks mean the module has no dynamic module-local resources. */
typedef BOOL (*HGOverlayHook)(void *);
int hg_overlay_register_lifecycle(FSOverlayID, HGOverlayHook load, HGOverlayHook unload, void *context);
int hg_overlay_unregister(FSOverlayID);
unsigned hg_overlay_live(void);
enum { HG_PROCESSOR_ARM9 = 0 };
BOOL FS_LoadOverlay(int processor, FSOverlayID);
BOOL FS_UnloadOverlay(int processor, FSOverlayID);
