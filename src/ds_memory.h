#pragma once
#include <stdint.h>
#include <stddef.h>
/* DS addresses are values, never native pointers. Main RAM only for now. */
typedef struct { uint8_t *ram; size_t size; } DSMainRAM;
static inline void *ds_translate(DSMainRAM *m, uint32_t address, size_t length) {
    if (address < 0x02000000u) return NULL;
    size_t offset = address - 0x02000000u;
    if (offset > m->size || length > m->size - offset) return NULL;
    return m->ram + offset;
}
