#include "compat/native_heap.h"

/* Extend the single native allocation service. Unregistered legacy heaps retain
 * their malloc/free behavior. Registered roots and descendants have explicit
 * budgets and destruction; pointers keep their normal native malloc ABI. */
enum { HG_HEAP_COUNT = 256 };
enum { UNUSED, ACTIVE, RETIRED };
typedef struct Allocation {
    void *pointer;
    size_t size;
    unsigned heap;
    uint64_t identity;
    const void *capture;
    HGHeapObserver observer; void *observer_context; int notifying;
    struct Allocation *next;
} Allocation;
static struct { HGHeapUsage usage; int state, parent; } heaps[HG_HEAP_COUNT];
static Allocation *allocations;
static uint64_t allocation_identity;
static const void *allocation_capture;

static unsigned index_of(enum HeapID heap) {
    GF_ASSERT((unsigned)heap < HG_HEAP_COUNT);
    return (unsigned)heap;
}
static size_t available(unsigned id) {
    HGHeapUsage *u = &heaps[id].usage;
    GF_ASSERT(u->allocated <= u->capacity && u->reserved <= u->capacity - u->allocated);
    return u->capacity - u->allocated - u->reserved;
}
BOOL hg_heap_register_root(enum HeapID heap, size_t capacity) {
    unsigned id = index_of(heap);
    if (!capacity || heaps[id].state == ACTIVE) return FALSE;
    heaps[id].usage = (HGHeapUsage){.capacity = capacity};
    heaps[id].parent = -1;
    heaps[id].state = ACTIVE;
    return TRUE;
}
BOOL hg_heap_usage(enum HeapID heap, HGHeapUsage *usage) {
    unsigned id = index_of(heap);
    if (heaps[id].state != ACTIVE || !usage) return FALSE;
    *usage = heaps[id].usage;
    return TRUE;
}
BOOL Heap_Create(enum HeapID parent, enum HeapID child, u32 size) {
    unsigned p = index_of(parent), c = index_of(child);
    GF_ASSERT(p != c && heaps[p].state == ACTIVE && heaps[c].state != ACTIVE && size);
    GF_ASSERT(size <= available(p));
    heaps[c].usage = (HGHeapUsage){.capacity = size};
    heaps[c].parent = (int)p;
    heaps[c].state = ACTIVE;
    heaps[p].usage.reserved += size;
    heaps[p].usage.children++;
    return TRUE;
}
BOOL Heap_CreateAtEnd(enum HeapID parent, enum HeapID child, u32 size) {
    /* Direction affects DS arena placement, not native allocation ownership. */
    return Heap_Create(parent, child, size);
}
void *Heap_Alloc(enum HeapID heap, u32 size) {
    unsigned id = index_of(heap);
    GF_ASSERT(heaps[id].state != RETIRED);
    if (heaps[id].state == ACTIVE) GF_ASSERT(size <= available(id));
    void *p = malloc(size ? size : 1);
    GF_ASSERT(p);
    if (heaps[id].state == ACTIVE) {
        Allocation *a = malloc(sizeof(*a));
        GF_ASSERT(a);
        GF_ASSERT(allocation_identity != UINT64_MAX);
        *a = (Allocation){.pointer=p,.size=size,.heap=id,
            .identity=++allocation_identity,.capture=allocation_capture,.next=allocations};
        allocations = a;
        heaps[id].usage.allocated += size;
        heaps[id].usage.blocks++;
    }
    return p;
}
void *Heap_AllocAtEnd(enum HeapID heap, u32 size) { return Heap_Alloc(heap, size); }
/* Original Heap_Realloc is a nonmoving shrink, not ISO realloc. Keep the
 * native pointer stable; release logical arena budget and bound guest views. */
void Heap_Realloc(void *pointer,u32 size) {
    for(Allocation *a=allocations;a;a=a->next)if(a->pointer==pointer) {
        GF_ASSERT(!a->notifying && size<=a->size);
        if(a->observer && size<a->size) {
            a->notifying=1;a->observer(a->observer_context,pointer,a->size,size,FALSE);
            a->notifying=0;
        }
        heaps[a->heap].usage.allocated-=a->size-size;a->size=size;return;
    }
    GF_ASSERT(!"Heap_Realloc requires a registered original allocation");
}
static void release(void *pointer, int explicit_heap) {
    if (!pointer) return;
    for (Allocation **at = &allocations; *at; at = &(*at)->next) {
        Allocation *a = *at;
        if (a->pointer != pointer) continue;
        GF_ASSERT(!a->notifying && (explicit_heap < 0 || a->heap == (unsigned)explicit_heap));
        if(a->observer) {
            a->notifying=1;a->observer(a->observer_context,pointer,a->size,0,TRUE);
            a->notifying=0;
            /* A dependent allocation release may have removed the predecessor
             * that owned at. Reacquire the link before unlinking this owner. */
            at=&allocations;
            while(*at!=a){GF_ASSERT(*at);at=&(*at)->next;}
        }
        heaps[a->heap].usage.allocated -= a->size;
        heaps[a->heap].usage.blocks--;
        *at = a->next;
        free(a);
        break;
    }
    free(pointer);
}
void Heap_Free(void *pointer) { release(pointer, -1); }
void Heap_FreeExplicit(enum HeapID heap, void *pointer) { release(pointer, (int)index_of(heap)); }
void Heap_Destroy(enum HeapID heap) {
    unsigned id = index_of(heap);
    GF_ASSERT(heaps[id].state == ACTIVE && !heaps[id].usage.children);
    for (;;) {
        Allocation *a=allocations;
        while(a&&a->heap!=id)a=a->next;
        if(!a)break;
        release(a->pointer,(int)id);
    }
    int parent = heaps[id].parent;
    if (parent >= 0) {
        GF_ASSERT(heaps[parent].state == ACTIVE);
        heaps[parent].usage.reserved -= heaps[id].usage.capacity;
        heaps[parent].usage.children--;
    }
    heaps[id].usage = (HGHeapUsage){0};
    heaps[id].state = RETIRED;
}
u32 HeapExp_FndGetTotalFreeSize(enum HeapID heap) {
    unsigned id = index_of(heap);
    GF_ASSERT(heaps[id].state == ACTIVE && available(id) <= UINT32_MAX);
    return (u32)available(id);
}

BOOL hg_heap_allocation_size(const void *pointer,size_t *size){
    if(!pointer||!size)return FALSE;
    for(Allocation *a=allocations;a;a=a->next)if(a->pointer==pointer){*size=a->size;return TRUE;}
    return FALSE;
}

BOOL hg_heap_allocation_info(const void *pointer,HGHeapAllocation *info) {
    if(!pointer||!info)return FALSE;
    uintptr_t p=(uintptr_t)pointer;
    for(Allocation *a=allocations;a;a=a->next) {
        uintptr_t base=(uintptr_t)a->pointer;
        if(p>=base && (p-base<a->size || p==base)) {
            *info=(HGHeapAllocation){a->pointer,a->size,a->heap,a->identity};
            return TRUE;
        }
    }
    return FALSE;
}
BOOL hg_heap_observe(const void *pointer,size_t size,HGHeapObserver observer,
                     void *context,HGHeapAllocation *info) {
    HGHeapAllocation allocation;
    if(!observer||!hg_heap_allocation_info(pointer,&allocation))return FALSE;
    size_t offset=(uintptr_t)pointer-(uintptr_t)allocation.base;
    if(offset>allocation.size||size>allocation.size-offset)return FALSE;
    for(Allocation *a=allocations;a;a=a->next)if(a->pointer==allocation.base) {
        if(a->observer && (a->observer!=observer || a->observer_context!=context))return FALSE;
        a->observer=observer;a->observer_context=context;
        if(info)*info=allocation;
        return TRUE;
    }
    return FALSE;
}
void hg_heap_unobserve(HGHeapObserver observer,void *context) {
    for(Allocation *a=allocations;a;a=a->next)
        if(a->observer==observer && a->observer_context==context) {
            a->observer=NULL;a->observer_context=NULL;
        }
}

const void *hg_heap_capture(const void *tag) {
    const void *previous=allocation_capture;allocation_capture=tag;return previous;
}
BOOL hg_heap_captured(const void *pointer,const void *tag) {
    if(!pointer||!tag)return FALSE;
    uintptr_t p=(uintptr_t)pointer;
    for(Allocation *a=allocations;a;a=a->next) {
        uintptr_t base=(uintptr_t)a->pointer;
        if(p>=base&&(p-base<a->size||p==base))return a->capture==tag;
    }
    return FALSE;
}
void hg_heap_finish_capture(const void *tag,BOOL commit) {
    GF_ASSERT(tag && allocation_capture!=tag);
    for(;;) {
        Allocation *a=allocations;
        while(a&&a->capture!=tag)a=a->next;
        if(!a)break;
        if(commit)a->capture=NULL;else release(a->pointer,(int)a->heap);
    }
}
