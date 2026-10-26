#pragma once
#include <stdint.h>
#include <stddef.h>
/* DS addresses are values, never native pointers. Explicit views precede RAM. */
typedef struct {
    uint32_t address; uint8_t *bytes; size_t size;
    /* Host metadata only: reservation extent and binding incarnation. Neither
     * changes numeric ARM pointers. Legacy aggregate initializers use zero. */
    size_t reserved_size;
    uint64_t generation;
} DSNativeRegion;
typedef struct { uint8_t *ram; size_t size;
    const DSNativeRegion *regions; size_t region_count;
} DSMainRAM;
static inline void *ds_translate(DSMainRAM *m, uint32_t address, size_t length) {
    if (!m || (m->region_count && !m->regions) ||
        length > (uint64_t)UINT32_MAX + 1 - address) return NULL;
    for (size_t i=0;i<m->region_count;i++) {
        const DSNativeRegion *r=&m->regions[i];
        if(r->bytes && address>=r->address && (size_t)(address-r->address)<=r->size &&
           length<=r->size-(size_t)(address-r->address))return r->bytes+(address-r->address);
    }
    if (!m->ram || address < 0x02000000u) return NULL;
    size_t offset = address - 0x02000000u;
    if (offset > m->size || length > m->size - offset) return NULL;
    return m->ram + offset;
}
