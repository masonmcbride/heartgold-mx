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
/* Execute the admitted original New Bark sign bytecode via original script.c.
 * Native UI command adapters, no general story/script support. */
int hg_sign_script(unsigned script_id,char *message,size_t capacity);
