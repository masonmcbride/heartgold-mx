#pragma once
#include "compat/native.h"
/* Native static-module identity, not a serialized overlay address. */
typedef s32 FSOverlayID;
#include "overlay_manager.h"
#include "poke_overlay.h"
/* Explicitly admit a linked native module before original load requests.
 * Main-thread-only; no dynamic code or DS overlay relocation is performed. */
int hg_overlay_register(FSOverlayID);
int hg_overlay_unregister(FSOverlayID);
unsigned hg_overlay_live(void);
