#pragma once
#include "compat/native.h"

/* Native bootstrap supplies the template heaps before original Heap_Create.
 * Uses the existing malloc transport; budgets account requested native bytes,
 * not Nitro allocator headers or DS arena addresses. Main-thread ownership. */
BOOL hg_heap_register_root(enum HeapID heap, size_t capacity);
typedef struct HGHeapUsage {
    size_t capacity, allocated, reserved, blocks;
    unsigned children;
} HGHeapUsage;
BOOL hg_heap_usage(enum HeapID heap, HGHeapUsage *usage);
BOOL Heap_Create(enum HeapID parent, enum HeapID child, u32 size);
BOOL Heap_CreateAtEnd(enum HeapID parent, enum HeapID child, u32 size);
void Heap_Destroy(enum HeapID heap);
u32 HeapExp_FndGetTotalFreeSize(enum HeapID heap);

void Heap_Realloc(void *,u32);

BOOL hg_heap_allocation_size(const void *,size_t *);

/* One address-space observer per allocation. Notification runs synchronously
 * before release/shrink and cannot fail or allocate. It may free distinct
 * dependent blocks, but must not free/resize the notifying allocation. The owner remains native.
 * Registered heaps only; interior views must fit in the current allocation. */
typedef void (*HGHeapObserver)(void *,const void *,size_t,size_t,BOOL);
typedef struct {
    void *base; size_t size; unsigned heap; uint64_t identity;
} HGHeapAllocation;
BOOL hg_heap_observe(const void *,size_t,HGHeapObserver,void *,HGHeapAllocation *);
void hg_heap_unobserve(HGHeapObserver,void *);
BOOL hg_heap_allocation_info(const void *,HGHeapAllocation *);

/* Attribute acquisitions in a synchronous import, not all allocations since a
 * timestamp. IRQ/other-owner calls must run with capture suspended. The tag
 * lives until finish; commit clears tags, rollback frees only tagged blocks. */
const void *hg_heap_capture(const void *);
BOOL hg_heap_captured(const void *,const void *);
void hg_heap_finish_capture(const void *,BOOL);
