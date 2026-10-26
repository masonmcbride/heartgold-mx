#pragma once
/* Canonical, byte-compatible DS-to-native memory correspondence.
 * The original ARM CPU keeps 32-bit guest pointers; do not cast those values
 * to native pointers. The caller owns and retires the native allocations.
 * This registry borrows bytes, not object ABI/layout compatibility.
 */
#include "ds_memory.h"
#include <stdint.h>
#include <stddef.h>

/* Exposes the existing loader region array and monotonic guest allocator.
 * One mapping registry must be used per guest address space.
 */
typedef struct {
    DSMainRAM *memory;
    DSNativeRegion *regions;
    unsigned *borrowed;
    size_t capacity;
    uint32_t *next;
    uint32_t allocation_limit;
} DSMapping;

static inline int ds_mapping_valid(const DSMapping *m) {
    return m && m->memory && m->regions && m->borrowed &&
           m->memory->regions == m->regions &&
           m->memory->region_count <= m->capacity;
}

/* A reference retained by native code identifies one binding incarnation.
 * ARM instructions still use numeric addresses and observe the current binding.
 * The owning DSMapping/address space must outlive every reference. */
typedef struct { size_t slot; uint64_t generation; const DSMainRAM *space; } DSMappingRef;
static inline DSMappingRef ds_mapping_ref(const DSMapping *m, uint32_t guest) {
    if (ds_mapping_valid(m))
        for (size_t i = 0; i < m->memory->region_count; ++i)
            if (m->regions[i].address == guest && m->regions[i].bytes)
                return (DSMappingRef){i, m->regions[i].generation, m->memory};
    return (DSMappingRef){SIZE_MAX, 0, NULL};
}
static inline void *ds_mapping_resolve(const DSMapping *m, DSMappingRef ref,
                                      size_t offset, size_t size) {
    if (!ds_mapping_valid(m) || ref.space != m->memory || ref.slot >= m->memory->region_count) return NULL;
    const DSNativeRegion *r = &m->regions[ref.slot];
    if (!r->bytes || r->generation != ref.generation || offset > r->size ||
        size > r->size - offset) return NULL;
    return r->bytes + offset;
}
static inline size_t ds_mapping_extent(const DSNativeRegion *r) {
    size_t extent = r->reserved_size > r->size ? r->reserved_size : r->size;
    return extent ? extent : 1;
}

/* Subtraction-based intersection avoids host-pointer range overflow. */
static inline int ds_mapping_intersects(
    uintptr_t a, size_t as, uintptr_t b, size_t bs
) {
    if (!as || !bs) return 0;
    return a >= b ? a - b < bs : b - a < as;
}

/* For canonical addresses, this is the inverse of ds_translate().
 * Existing explicit DS aliases are resolved in first-registered order.
 */
static inline uint32_t ds_mapping_address_of(
    const DSMapping *m, const void *native, size_t size
) {
    if (!ds_mapping_valid(m) || !native || !size ||
        size - 1 > UINTPTR_MAX - (uintptr_t)native) return 0;
    uintptr_t p = (uintptr_t)native;
    for (size_t i = 0; i < m->memory->region_count; ++i) {
        const DSNativeRegion *r = &m->regions[i];
        if (!r->bytes || !r->size) continue;
        uintptr_t base = (uintptr_t)r->bytes;
        if (p >= base && p - base < r->size) {
            size_t offset = p - base;
            if (size > r->size - offset) return 0;
            if ((uint64_t)r->address + offset + size >
                (uint64_t)UINT32_MAX + 1) return 0;
            return r->address + (uint32_t)offset;
        }
    }
    if (m->memory->ram) {
        uintptr_t base = (uintptr_t)m->memory->ram;
        if (p >= base && p - base < m->memory->size) {
            size_t offset = p - base;
            if (size > m->memory->size - offset) return 0;
            if ((uint64_t)offset + size >
                (uint64_t)UINT32_MAX + 1 - 0x02000000u) return 0;
            return 0x02000000u + (uint32_t)offset;
        }
    }
    return 0;
}

/* Fixed-address binding for ARM ABI fields, globals, or canonical handles.
 * Cannot overlap ordinary guest RAM, another live guest range, or any
 * already-mapped native byte range. A matching repeated bind is idempotent.
 * Retired guest addresses are tombstones and cannot be rebound here.
 */
static inline uint32_t ds_mapping_install(
    DSMapping *m, uint32_t guest, void *native, size_t size, size_t reuse
) {
    if (!ds_mapping_valid(m) || !guest || !native || !size ||
        size > UINT32_MAX || !m->memory->ram ||
        size - 1 > UINTPTR_MAX - (uintptr_t)native) return 0;
    uint64_t end = (uint64_t)guest + size;
    if (end > (uint64_t)UINT32_MAX + 1) return 0;
    if (reuse != SIZE_MAX && (reuse >= m->memory->region_count ||
        m->regions[reuse].bytes || !m->borrowed[reuse] ||
        m->regions[reuse].address != guest ||
        m->regions[reuse].generation == UINT64_MAX)) return 0;
    uint64_t ram_end = 0x02000000ull + m->memory->size;
    if (guest < ram_end && end > 0x02000000ull) return 0;
    if (ds_mapping_intersects((uintptr_t)native, size,
                              (uintptr_t)m->memory->ram, m->memory->size))
        return 0;

    for (size_t i = 0; i < m->memory->region_count; ++i) {
        const DSNativeRegion *r = &m->regions[i];
        if (i == reuse) continue;
        if (r->address == guest && r->bytes == native && r->size == size &&
            m->borrowed[i]) return guest;
        /* A retired region keeps its old address/extent reserved. */
        if (r->address && (uint64_t)guest < (uint64_t)r->address + ds_mapping_extent(r) &&
            (uint64_t)r->address < end) return 0;
        if (r->bytes && ds_mapping_intersects(
                (uintptr_t)native, size, (uintptr_t)r->bytes, r->size))
            return 0;
    }
    size_t i = reuse;
    uint64_t generation = 1;
    size_t extent = size;
    if (i == SIZE_MAX) {
        if (m->memory->region_count == m->capacity) return 0;
        i = m->memory->region_count++;
    } else {
        generation = m->regions[i].generation + 1;
        if (ds_mapping_extent(&m->regions[i]) > extent)
            extent = ds_mapping_extent(&m->regions[i]);
    }
    m->regions[i] = (DSNativeRegion){guest, native, size, extent, generation};
    m->borrowed[i] = 1;
    return guest;
}
static inline uint32_t ds_mapping_bind(DSMapping *m, uint32_t guest,
                                      void *native, size_t size) {
    return ds_mapping_install(m, guest, native, size, SIZE_MAX);
}
/* Explicit fixed-address reuse after the old binding has retired. A stale host
 * reference remains invalid even when guest AND native numeric addresses recur.
 * Dynamic heap placement is deliberately not decided by this operation. */
static inline uint32_t ds_mapping_rebind(DSMapping *m, DSMappingRef retired,
                                        void *native, size_t size) {
    if (!ds_mapping_valid(m) || retired.space != m->memory || retired.slot >= m->memory->region_count) return 0;
    DSNativeRegion *r = &m->regions[retired.slot];
    if (r->bytes || !m->borrowed[retired.slot] ||
        r->generation != retired.generation || r->generation == UINT64_MAX) return 0;
    return ds_mapping_install(m, r->address, native, size, retired.slot);
}

/* Reserve a fresh guest identity, including zero-byte allocations. The cursor
 * never moves backwards. Registered live AND retired ranges and RAM are holes
 * in the allocation arena. This reserves addresses, not native ownership.
 * Failure leaves the cursor unchanged. alignment must be a power of two.
 */
static inline uint32_t ds_mapping_reserve(DSMapping *m, size_t size,
                                           uint32_t alignment) {
    if (!ds_mapping_valid(m) || !m->next || !alignment ||
        (alignment & (alignment - 1)) || size > UINT32_MAX) return 0;
    uint64_t mask = alignment - 1;
    uint64_t base = ((uint64_t)*m->next + mask) & ~mask;
    uint64_t extent = (((uint64_t)(size ? size : 1)) + mask) & ~mask;
    for (;;) {
        if (!base || base + extent > m->allocation_limit) return 0;
        uint64_t after = base;
        uint64_t ram_end = 0x02000000ull + m->memory->size;
        if (base < ram_end && base + extent > 0x02000000ull)
            after = ram_end;
        for (size_t i = 0; i < m->memory->region_count; ++i) {
            const DSNativeRegion *r = &m->regions[i];
            uint64_t end = (uint64_t)r->address + ds_mapping_extent(r);
            if (r->address && base < end &&
                (uint64_t)r->address < base + extent && end > after)
                after = end;
        }
        if (after == base) {
            *m->next = (uint32_t)(base + extent);
            return (uint32_t)base;
        }
        base = (after + mask) & ~mask;
    }
}

/* Dynamic zero-copy borrow. Shares the loader's guest allocator and never
 * reuses a retired dynamic guest address during a context's lifetime.
 */
static inline uint32_t ds_mapping_borrow(
    DSMapping *m, void *native, size_t size
) {
    if (!ds_mapping_valid(m) || !native || !size || !m->next ||
        size > UINT32_MAX || m->allocation_limit <= 0x02000000u) return 0;
    uint32_t existing = ds_mapping_address_of(m, native, size);
    if (existing) return existing;

    uint32_t previous = *m->next;
    uint32_t base = ds_mapping_reserve(m, size, 16);
    if (!base) return 0;
    uint32_t guest = ds_mapping_bind(m, base, native, size);
    if (!guest) *m->next = previous;
    return guest;
}

/* Before an owner frees backing storage, invalidate EVERY intersecting view,
 * including explicit aliases. This never frees storage and retains extents.
 * The owner must provide the full allocation span, not just one mapped view.
 */
static inline int ds_mapping_retire_native(DSMapping *m, const void *native,
                                            size_t size) {
    if (!ds_mapping_valid(m) || !native || !size ||
        size - 1 > UINTPTR_MAX - (uintptr_t)native) return 0;
    int retired = 0;
    for (size_t i = 0; i < m->memory->region_count; ++i) {
        DSNativeRegion *r = &m->regions[i];
        if (r->bytes && (r->bytes == native || ds_mapping_intersects(
                (uintptr_t)native, size, (uintptr_t)r->bytes, r->size))) {
            r->bytes = NULL;
            retired = 1;
        }
    }
    return retired;
}

/* Narrow an owned view without reusing its discarded guest tail. Aliases
 * touching discarded bytes retire in full; unaffected prefix views survive.
 * The native owner performs the corresponding nonmoving shrink afterwards.
 */
static inline int ds_mapping_shrink(DSMapping *m, uint32_t guest, size_t size) {
    if (!ds_mapping_valid(m) || !guest) return 0;
    for (size_t i = 0; i < m->memory->region_count; ++i) {
        DSNativeRegion *r = &m->regions[i];
        if (r->address != guest || !r->bytes || m->borrowed[i]) continue;
        if (size > r->size) return 0;
        if (size == r->size) return 1;
        size_t tail = r->size - size;
        uint8_t *native = r->bytes + size;
        for (size_t j = 0; j < m->memory->region_count; ++j) {
            DSNativeRegion *view = &m->regions[j];
            if (j != i && view->bytes && ds_mapping_intersects(
                    (uintptr_t)native, tail, (uintptr_t)view->bytes, view->size))
                view->bytes = NULL;
        }
        r->reserved_size = ds_mapping_extent(r);
        r->size = size;
        return 1;
    }
    return 0;
}

/* Notification from a native owner after a successful nonmoving-shrink
 * preflight and before its logical extent changes. Prefix views narrow; views
 * starting in the discarded tail retire. This transition cannot allocate/fail. */
static inline void ds_mapping_native_shrink(DSMapping *m, const void *native,
                                            size_t old_size, size_t new_size) {
    if (!ds_mapping_valid(m) || !native || new_size >= old_size) return;
    uintptr_t base = (uintptr_t)native;
    for (size_t i = 0; i < m->memory->region_count; ++i) {
        DSNativeRegion *r = &m->regions[i];
        uintptr_t p = (uintptr_t)r->bytes;
        if (!r->bytes || p < base || p - base >= old_size) continue;
        size_t offset = p - base;
        r->reserved_size = ds_mapping_extent(r);
        if (offset == 0) { if (r->size > new_size) r->size = new_size; }
        else if (offset >= new_size || r->size > new_size - offset) r->bytes = NULL;
    }
}

/* Retire one borrowed view without freeing the native allocation. The region
 * stays as a tombstone to prevent dangling guest pointers being rebound.
 */
static inline int ds_mapping_unbind(DSMapping *m, uint32_t guest) {
    if (!ds_mapping_valid(m) || !guest) return 0;
    for (size_t i = 0; i < m->memory->region_count; ++i) {
        DSNativeRegion *r = &m->regions[i];
        if (r->address == guest && r->bytes && m->borrowed[i]) {
            r->bytes = NULL;
            return 1;
        }
    }
    return 0;
}
