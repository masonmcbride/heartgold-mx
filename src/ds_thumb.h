#pragma once
#include "ds_memory.h"
/* Bounded ARMv5 Thumb-1 execution slice. No host pointers in guest registers.
 * No MMIO, ARM state, exceptions or hardware timing. External calls require
 * an explicitly supplied host boundary; no implicit host symbol lookup. */
typedef enum { DS_THUMB_RETURN, DS_THUMB_BAD_MEMORY, DS_THUMB_BAD_INSTRUCTION,
               DS_THUMB_BUDGET, DS_THUMB_BOUNDARY } DSThumbResult;
typedef struct DSThumb DSThumb;
/* Return 1 when handled, 0 for unmapped address, -1 for boundary failure.
 * Called only outside executable code; successful calls return through LR. */
typedef int (*DSThumbBoundary)(DSThumb *,void *);
struct DSThumb {
    DSMainRAM memory;
    uint32_t r[16];
    uint32_t code_start,code_size,return_address;
    unsigned n,z,c,v;
    size_t steps;
    uint32_t fault_address;
    uint16_t fault_instruction;
    DSThumbBoundary boundary;
    void *boundary_user;
};
/* PC is an even guest instruction address. LR/return_address are Thumb-tagged.
 * Caller supplies stack/arguments; execution never allocates guest memory. */
DSThumbResult ds_thumb_run(DSThumb *,size_t budget);

/* Named machine leaves shared by original guest islands; 0 means unhandled. */
int ds_thumb_math_boundary(DSThumb *,const char *);
