#include "ds_mapping.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t ram[128], a[32], b[32];
    DSNativeRegion regions[16];
    unsigned borrowed[16];
    DSMainRAM mem;
    uint32_t next;
} Fixture;

static DSMapping fixture_mapping(Fixture *f) {
    f->mem.ram = f->ram;
    f->mem.size = sizeof(f->ram);
    f->mem.regions = f->regions;
    f->next = 0x02800000u;
    return (DSMapping){&f->mem, f->regions, f->borrowed,
                       16, &f->next, 0x03000000u};
}

static void memory_laws(void) {
    Fixture f = {0};
    DSMapping m = fixture_mapping(&f);
    uint32_t g = ds_mapping_borrow(&m, f.a, sizeof(f.a));
    assert(g == 0x02800000u);
    assert(ds_translate(&f.mem, g, sizeof(f.a)) == f.a);
    assert(ds_mapping_address_of(&m, f.a, sizeof(f.a)) == g);
    assert(ds_mapping_borrow(&m, f.a, sizeof(f.a)) == g);
    assert(ds_mapping_borrow(&m, f.a+7, 12) == g+7);
    assert(ds_mapping_address_of(&m, f.a+7, 12) == g+7);
    assert(ds_translate(&f.mem, g+7, 12) == f.a+7);
    assert(!ds_translate(&f.mem, g+31, 2));
    assert(!ds_mapping_address_of(&m, f.a+31, 2));
    assert(ds_mapping_address_of(&m, f.ram+3, 2) == 0x02000003u);
    assert(ds_mapping_borrow(&m, f.ram+3, 2) == 0x02000003u);
    assert(ds_translate(&f.mem, 0x02000003u, 2) == f.ram+3);
    assert(ds_mapping_borrow(&m, f.ram+127, 2) == 0);

    f.a[5] = 42;
    assert(*(uint8_t *)ds_translate(&f.mem, g+5, 1) == 42);
    *(uint8_t *)ds_translate(&f.mem, g+6, 1) = 99;
    assert(f.a[6] == 99);

    assert(!ds_mapping_bind(&m, 0x02000020u, f.b, 4));
    assert(!ds_mapping_bind(&m, g+8, f.b, 4));
    assert(!ds_mapping_bind(&m, 0x03100000u, f.a+2, 3));
    assert(!ds_mapping_bind(&m, 0x03100000u, f.ram+2, 3));
    assert(!ds_mapping_bind(&m, UINT32_MAX-1, f.b, 4));
    assert(!ds_mapping_borrow(&m, f.a+31, 2));
    assert(!ds_mapping_borrow(&m, f.b, 0));

    uint32_t h = ds_mapping_bind(&m, 0x03100000u, f.b, 16);
    assert(h == 0x03100000u);
    assert(ds_mapping_bind(&m, h, f.b, 16) == h);
    assert(ds_mapping_borrow(&m, f.b+3, 3) == h+3);
    assert(!ds_mapping_bind(&m, h, f.b, 15));

    assert(ds_mapping_unbind(&m, g));
    assert(!ds_translate(&f.mem, g, 1));
    assert(!ds_mapping_address_of(&m, f.a, 1));
    assert(!ds_mapping_unbind(&m, g));
    assert(!ds_mapping_bind(&m, g, f.a, sizeof(f.a)));
    uint32_t replacement = ds_mapping_borrow(&m, f.a, sizeof(f.a));
    assert(replacement != 0 && replacement != g);
    assert(ds_translate(&f.mem, replacement, sizeof(f.a)) == f.a);
    assert(!ds_translate(&f.mem, g, 1));

    assert(ds_mapping_unbind(&m, h));
    assert(!ds_translate(&f.mem, h, 1));
    assert(!ds_mapping_bind(&m, h, f.b, 16));
}

static void capacity_and_limit(void) {
    Fixture f = {0};
    DSMapping m = fixture_mapping(&f);
    m.capacity = 1;
    assert(ds_mapping_borrow(&m, f.a, 4));
    assert(!ds_mapping_borrow(&m, f.b, 4));

    Fixture f2 = {0};
    DSMapping m2 = fixture_mapping(&f2);
    m2.next = NULL;
    assert(!ds_mapping_borrow(&m2, f2.a, 4));
    m2.next = &f2.next;
    f2.next = 0x02fffff0u;
    assert(!ds_mapping_borrow(&m2, f2.a, 32));
    assert(f2.next == 0x02fffff0u);
    assert(ds_mapping_borrow(&m2, f2.a, 16) == 0x02fffff0u);
}

static void property_trials(void) {
    for (unsigned trial = 0; trial < 256; ++trial) {
        Fixture f = {0};
        DSMapping m = fixture_mapping(&f);
        size_t n = 1 + trial % sizeof(f.a);
        uint32_t g = ds_mapping_borrow(&m, f.a, n);
        assert(g != 0);
        for (size_t off = 0; off < n; ++off) {
            f.a[off] = (uint8_t)(trial ^ off);
            assert(ds_translate(&f.mem, g+(uint32_t)off, 1) == f.a+off);
            assert(ds_mapping_address_of(&m, f.a+off, 1) == g+off);
            assert(*(uint8_t *)ds_translate(&f.mem, g+(uint32_t)off, 1)
                   == f.a[off]);
        }
        assert(ds_mapping_unbind(&m, g));
        assert(!ds_translate(&f.mem, g, 1));
    }
}

static void reservations_and_edges(void) {
    Fixture f = {0}; DSMapping m = fixture_mapping(&f);
    uint32_t start = f.next;
    assert(ds_mapping_bind(&m, start, f.a, 17));
    assert(ds_mapping_unbind(&m, start));
    assert(ds_mapping_borrow(&m, f.b, 4) == start + 32);
    assert(!ds_mapping_bind(&m, start + 8, f.a, 1));
    uint32_t saved = f.next;
    assert(!ds_mapping_reserve(&m, SIZE_MAX, 16));
    assert(!ds_mapping_reserve(&m, 4, 3));
    assert(!ds_mapping_reserve(&m, 4, 0));
    assert(f.next == saved);
    f.next = 0x02000000u;
    assert(ds_mapping_reserve(&m, 0, 32) == 0x02000080u);
    assert(f.next == 0x020000a0u);
    assert(ds_mapping_reserve(&m, 1, 32) == 0x020000a0u);
    /* A failed registration after a successful search must roll back next. */
    saved = f.next;
    assert(!ds_mapping_borrow(&m, f.b + 3, 2));
    assert(f.next == saved);
    assert(!ds_translate(NULL, 0, 1));
    DSMainRAM empty = {0};
    assert(!ds_translate(&empty, 0x02000000u, 0));
    DSNativeRegion edge = {UINT32_MAX, f.a, 2};
    empty.regions = &edge; empty.region_count = 1;
    assert(ds_translate(&empty, UINT32_MAX, 1) == f.a);
    assert(!ds_translate(&empty, UINT32_MAX, 2));
    /* Legacy zero-sized allocations still reserve an identity byte. */
    Fixture z = {0}; DSMapping zm = fixture_mapping(&z);
    z.regions[0] = (DSNativeRegion){0x02800010u, NULL, 0};
    z.mem.region_count = 1;
    assert(!ds_mapping_bind(&zm, 0x0280000fu, z.a, 2));
    assert(ds_mapping_reserve(&zm, 32, 16) == 0x02800020u);
    assert(ds_mapping_bind(&zm, UINT32_MAX, z.a, 1) == UINT32_MAX);
    assert(ds_translate(&z.mem, UINT32_MAX, 1) == z.a);
    assert(ds_mapping_address_of(&zm, z.a, 1) == UINT32_MAX);
    assert(!ds_mapping_bind(&zm, 0x03100000u,
                            (void *)(uintptr_t)(UINTPTR_MAX - 1), 4));
}

static uint32_t random_word(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

/* Independent byte occupancy oracle: no production range/intersection helper.
 * Each buffer has one current live identity; retired addresses stay occupied.
 * Generate borrow, fixed bind, interior lookup, overlap/alias rejection,
 * retirement and host-buffer reuse, checking every guest byte after each step.
 */
static void randomized_lifetimes(void) {
    enum { BASE = 0x02800000u, ARENA = 512, BLOCKS = 8, STEPS = 96 };
    for (uint32_t seed = 1; seed <= 128; ++seed) {
        uint8_t ram[16] = {0}, host[BLOCKS][16] = {{0}};
        DSNativeRegion regions[STEPS] = {0}; unsigned borrowed[STEPS] = {0};
        DSMainRAM mem = {ram, sizeof(ram), regions, 0}; uint32_t next = BASE;
        DSMapping m = {&mem, regions, borrowed, STEPS, &next, BASE + ARENA};
        int occupied[ARENA] = {0}, owner[ARENA];
        uint32_t live[BLOCKS] = {0}, cursor = BASE, rng = seed;
        for (unsigned i = 0; i < ARENA; ++i) owner[i] = -1;
        for (unsigned step = 0; step < STEPS; ++step) {
            unsigned block = random_word(&rng) % BLOCKS;
            unsigned operation = (random_word(&rng) >> 16) % 4;
            if (operation == 0 && live[block]) {
                if (step & 1) assert(ds_mapping_unbind(&m, live[block]));
                else assert(ds_mapping_retire_native(&m, host[block], 16));
                for (unsigned k = 0; k < 16; ++k)
                    owner[live[block] - BASE + k] = -1;
                live[block] = 0;
            } else if (operation == 1) {
                uint32_t candidate = BASE + ((random_word(&rng) >> 16) % 32) * 16;
                int allowed = !live[block];
                for (unsigned k = 0; k < 16; ++k)
                    if (occupied[candidate - BASE + k]) allowed = 0;
                uint32_t result = ds_mapping_bind(&m, candidate, host[block], 16);
                if (live[block] == candidate) assert(result == candidate);
                else if (!allowed) assert(!result);
                else {
                    assert(result == candidate); live[block] = result;
                    for (unsigned k = 0; k < 16; ++k) {
                        occupied[result - BASE + k] = 1;
                        owner[result - BASE + k] = (int)(block * 16 + k);
                    }
                }
            } else {
                uint32_t expected = live[block];
                if (!expected) {
                    uint32_t candidate = cursor;
                    while (candidate + 16 <= BASE + ARENA) {
                        int clear = 1;
                        for (unsigned k = 0; k < 16; ++k)
                            if (occupied[candidate - BASE + k]) clear = 0;
                        if (clear) { expected = candidate; break; }
                        candidate += 16;
                    }
                }
                assert(ds_mapping_borrow(&m, host[block], 16) == expected);
                if (expected && !live[block]) {
                    live[block] = expected; cursor = expected + 16;
                    for (unsigned k = 0; k < 16; ++k) {
                        occupied[expected - BASE + k] = 1;
                        owner[expected - BASE + k] = (int)(block * 16 + k);
                    }
                }
            }
            assert(next == cursor);
            for (unsigned i = 0; i < ARENA; ++i) {
                void *p = ds_translate(&mem, BASE + i, 1);
                if (owner[i] < 0) assert(!p);
                else {
                    unsigned b = (unsigned)owner[i] / 16, k = (unsigned)owner[i] % 16;
                    assert(p == host[b] + k);
                    assert(ds_mapping_address_of(&m, p, 1) == BASE + i);
                    *(uint8_t *)p = (uint8_t)step;
                    assert(host[b][k] == (uint8_t)step);
                    host[b][k] ^= 0x5a;
                    assert(*(uint8_t *)p == (uint8_t)(step ^ 0x5a));
                }
            }
            for (unsigned b = 0; b < BLOCKS; ++b) {
                assert(ds_mapping_address_of(&m, host[b], 16) == live[b]);
                if (live[b]) {
                    assert(ds_mapping_borrow(&m, host[b] + 5, 3) == live[b] + 5);
                    assert(!ds_mapping_bind(&m, 0x03100000u, host[b] + 5, 3));
                    assert(!ds_translate(&mem, live[b] + 15, 2));
                }
            }
        }
    }
}

/* Legacy explicit aliases: first registered view is canonical. This fixture
 * owns both views and retires the backing allocation before storage ends. bind refuses
 * to create such aliases; no general device/mirror lifetime policy is implied.
 */
static void explicit_aliases(void) {
    Fixture f = {0}; DSMapping m = fixture_mapping(&f);
    f.regions[0] = (DSNativeRegion){0x02800000u, f.a, 16};
    f.regions[1] = (DSNativeRegion){0x02900000u, f.a + 4, 8};
    f.borrowed[0] = f.borrowed[1] = 1; f.mem.region_count = 2;
    for (unsigned k = 0; k < 8; ++k) {
        uint8_t *p = ds_translate(&f.mem, 0x02900000u + k, 1);
        assert(p == ds_translate(&f.mem, 0x02800004u + k, 1));
        assert(ds_mapping_address_of(&m, p, 1) == 0x02800004u + k);
        *p = (uint8_t)k; assert(f.a[k + 4] == k);
    }
    assert(!ds_mapping_retire_native(&m, NULL, 16));
    assert(!ds_mapping_retire_native(&m, f.a, 0));
    assert(ds_mapping_retire_native(&m, f.a, 16));
    assert(!ds_mapping_retire_native(&m, f.a, 16));
    assert(!ds_mapping_bind(&m, 0x02900000u, f.a + 4, 8));
    assert(!ds_translate(&f.mem, 0x02900000u, 1));
    assert(!ds_mapping_address_of(&m, f.a + 4, 1));
}

/* Real backing releases under sanitizers, with explicit dependent aliases. */
static void allocated_lifetimes(void) {
    for (uint32_t seed = 1; seed <= 32; ++seed) {
        uint8_t ram[16] = {0}; DSNativeRegion regions[256] = {0};
        unsigned borrowed[256] = {0}; uint32_t next = 0x02800000u, rng = seed;
        DSMainRAM mem = {ram, sizeof(ram), regions, 0};
        DSMapping m = {&mem, regions, borrowed, 256, &next, 0x03000000u};
        void *storage[8] = {0}; size_t sizes[8] = {0};
        uint32_t guests[8] = {0}, aliases[8] = {0};
        for (unsigned step = 0; step < 96; ++step) {
            unsigned slot = (random_word(&rng) >> 16) % 8;
            if (storage[slot]) {
                assert(ds_mapping_retire_native(&m, storage[slot], sizes[slot]));
                free(storage[slot]); storage[slot] = NULL;
                assert(!ds_translate(&mem, guests[slot], 1));
                assert(!ds_translate(&mem, aliases[slot], 1));
            } else {
                sizes[slot] = 1 + (random_word(&rng) >> 16) % 64;
                storage[slot] = malloc(sizes[slot]); assert(storage[slot]);
                uint32_t g = ds_mapping_borrow(&m, storage[slot], sizes[slot]);
                assert(g && g != guests[slot]); guests[slot] = g;
                aliases[slot] = g + 0x01000000u;
                assert(mem.region_count < 256);
                size_t index = mem.region_count++;
                regions[index] = (DSNativeRegion){aliases[slot], storage[slot], sizes[slot]};
                borrowed[index] = 1;
            }
            for (unsigned i = 0; i < 8; ++i) if (storage[i]) {
                assert(ds_translate(&mem, guests[i], sizes[i]) == storage[i]);
                assert(ds_translate(&mem, aliases[i], sizes[i]) == storage[i]);
                assert(ds_mapping_address_of(&m, storage[i], sizes[i]) == guests[i]);
            }
        }
        for (unsigned i = 0; i < 8; ++i) if (storage[i]) {
            assert(ds_mapping_retire_native(&m, storage[i], sizes[i]));
            free(storage[i]);
            assert(!ds_translate(&mem, guests[i], 1));
            assert(!ds_translate(&mem, aliases[i], 1));
        }
    }
}

static void owned_shrink(void) {
    Fixture f = {0}; DSMapping m = fixture_mapping(&f);
    uint32_t g = ds_mapping_bind(&m, 0x02800000u, f.a, 32);
    assert(g); f.borrowed[0] = 0; /* Native owner, not a borrowed view. */
    f.regions[1] = (DSNativeRegion){0x03100000u, f.a + 12, 12};
    f.regions[2] = (DSNativeRegion){0x03100100u, f.a + 2, 4};
    f.borrowed[1] = f.borrowed[2] = 1; f.mem.region_count = 3;
    assert(!ds_mapping_unbind(&m, g));
    assert(!ds_mapping_shrink(&m, g, 33));
    m.capacity = 3;
    /* A nonmoving shrink must not need a spare mapping slot. */
    assert(ds_mapping_shrink(&m, g, 16));
    assert(f.mem.region_count == 3);
    assert(!ds_translate(&f.mem, g + 31, 1));
    m.capacity = 16;
    assert(ds_mapping_shrink(&m, g, 16));
    assert(ds_mapping_shrink(&m, g, 16));
    assert(!ds_translate(&f.mem, g + 16, 1));
    assert(!ds_translate(&f.mem, 0x03100000u, 1));
    assert(ds_translate(&f.mem, 0x03100100u, 4) == f.a + 2);
    assert(!ds_mapping_bind(&m, g + 20, f.b, 4));
    assert(ds_mapping_shrink(&m, g, 0));
    assert(!ds_translate(&f.mem, g, 1));
    assert(!ds_translate(&f.mem, 0x03100100u, 1));
    assert(!ds_mapping_bind(&m, g + 1, f.b, 4));
    assert(ds_mapping_retire_native(&m, f.a, sizeof(f.a)));
    assert(ds_mapping_borrow(&m, f.a, sizeof(f.a)) == g + 32);
}

static void fixed_lifetime_identity(void) {
    Fixture f = {0}; DSMapping m = fixture_mapping(&f);
    uint32_t g = ds_mapping_bind(&m, 0x03100000u, f.a, 16);
    DSMappingRef first = ds_mapping_ref(&m, g);
    assert(ds_mapping_resolve(&m, first, 3, 4) == f.a + 3);
    assert(!ds_mapping_rebind(&m, first, f.b, 16)); /* Live view. */
    assert(ds_mapping_unbind(&m, g));
    assert(!ds_mapping_resolve(&m, first, 0, 1));
    assert(!ds_mapping_bind(&m, g, f.b, 16)); /* No implicit reuse. */
    assert(ds_mapping_rebind(&m, first, f.b, 16) == g);
    DSMappingRef second = ds_mapping_ref(&m, g);
    assert(second.slot == first.slot && second.generation != first.generation);
    assert(!ds_mapping_resolve(&m, first, 0, 1));
    assert(ds_translate(&f.mem, g, 16) == f.b); /* Raw ARM address reused. */
    assert(ds_mapping_resolve(&m, second, 0, 16) == f.b);
    assert(!ds_mapping_resolve(&m, second, 15, 2));
    assert(ds_mapping_unbind(&m, g));
    assert(!ds_mapping_rebind(&m, first, f.a, 16)); /* Stale retire token. */
    /* The same host backing address can also belong to a new lifetime. */
    assert(ds_mapping_rebind(&m, second, f.b, 16) == g);
    DSMappingRef third = ds_mapping_ref(&m, g);
    assert(third.generation != second.generation);
    assert(!ds_mapping_resolve(&m, second, 0, 1));
    assert(ds_mapping_unbind(&m, g));
    assert(ds_mapping_bind(&m, g + 32, f.a, 16));
    size_t count = f.mem.region_count;
    assert(!ds_mapping_rebind(&m, third, f.a + 2, 4));
    assert(!ds_translate(&f.mem, g, 1) && f.mem.region_count == count);
    assert(f.regions[third.slot].generation == third.generation);
    f.regions[third.slot].generation = UINT64_MAX;
    assert(!ds_mapping_rebind(&m,(DSMappingRef){third.slot,UINT64_MAX,m.memory},f.b,16));
    /* Repeated init/teardown never consumes another fixed-view slot. */
    Fixture repeat = {0}; DSMapping rm = fixture_mapping(&repeat);
    uint32_t fixed = ds_mapping_bind(&rm, 0x03100000u, repeat.a, 16);
    assert(!ds_mapping_resolve(&rm, first, 0, 1));
    assert(!ds_mapping_rebind(&rm, third, repeat.a, 16));
    for (unsigned i=0;i<2048;i++) {
        DSMappingRef old = ds_mapping_ref(&rm, fixed);
        assert(ds_mapping_unbind(&rm, fixed));
        assert(ds_mapping_rebind(&rm, old, repeat.a, 16) == fixed);
        assert(!ds_mapping_resolve(&rm, old, 0, 1));
        assert(repeat.mem.region_count == 1);
    }
}

int main(void) {
    memory_laws();
    capacity_and_limit();
    property_trials();
    reservations_and_edges();
    randomized_lifetimes();
    explicit_aliases();
    allocated_lifetimes();
    owned_shrink();
    fixed_lifetime_identity();
    puts("DS canonical memory correspondence: roundtrip, offsets, coherence,"
         " bounds, conflicts, ownership, tombstones, exhaustion PASS");
    return 0;
}
