#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t *backgrounds,*objects,*warps,*coordinates;
    unsigned background_count,object_count,warp_count,coordinate_count;
} HGEventView;
/* Validate serialized lengths before passing any background array to guest code. */
int hg_event_decode(const uint8_t *,size_t,HGEventView *);
int hg_event_load(unsigned map,HGEventView *);
/* Legacy visible-sign adapter; uses the same original script runtime. */
int hg_sign_script(unsigned script_id,char *message,size_t capacity);
typedef struct { unsigned face_player,select_sound; } HGFieldMessageEffects;
/* Legacy single-message extraction with a private initialized story owner.
 * Shares original handlers; never publishes state. Persistent gameplay uses
 * field_script.h. Sound requests are captured; audio output is absent.
 * Error preserves both outputs. */
int hg_field_message(unsigned map,unsigned script,char *,size_t,HGFieldMessageEffects *);
