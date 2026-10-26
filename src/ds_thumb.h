#pragma once
#include "ds_memory.h"
/* Bounded ARMv5 Thumb-1 execution slice. No host pointers in guest registers.
 * No implicit MMIO, ARM state, exceptions or hardware timing. External calls require
 * an explicitly supplied host boundary; no implicit host symbol lookup. */
typedef enum { DS_THUMB_RETURN, DS_THUMB_BAD_MEMORY, DS_THUMB_BAD_INSTRUCTION,
               DS_THUMB_BUDGET, DS_THUMB_BOUNDARY } DSThumbResult;
typedef struct DSThumb DSThumb;
/* Return 1 when handled, 0 for unmapped address, -1 for boundary failure.
 * Called only outside executable code; successful calls return through LR. */
typedef int (*DSThumbBoundary)(DSThumb *,void *);
/* Optional data-only DS I/O dispatch, separate from pointer correspondence.
 * Called only for a naturally aligned 1/2/4-byte access disjoint from RAM/views
 * and wholly in [0x04000000,0x05000000). Return 1 only for an implemented
 * register/width;
 * any other result rejects the access. Rejection must have no device effects.
 * Writes carry only the low width bytes; reads are masked to that width.
 * Instruction fetch never calls these hooks. No timing/interrupt behavior is
 * supplied by the CPU itself. */
typedef int (*DSThumbDeviceRead)(void *,uint32_t,unsigned,uint32_t *);
typedef int (*DSThumbDeviceWrite)(void *,uint32_t,unsigned,uint32_t);
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
    DSThumbDeviceRead device_read;
    DSThumbDeviceWrite device_write;
    void *device_user;
};
/* PC is an even guest instruction address. LR/return_address are Thumb-tagged.
 * Caller supplies stack/arguments; execution never allocates guest memory. */
DSThumbResult ds_thumb_run(DSThumb *,size_t budget);

/* Named machine leaves shared by original guest islands; 0 means unhandled. */
int ds_thumb_math_boundary(DSThumb *,const char *);
