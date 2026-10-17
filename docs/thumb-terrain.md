# Original ARM terrain execution on Apple Silicon

The original BDHC height solver now executes through a bounded Thumb interpreter. This extends the project beyond original C: the algorithm remains original ARM instructions, while the interpreter itself compiles natively for Apple Silicon.

```sh
cmake --build build -j
./build/test_thumb
./build/heartgold_field --terrain 60
ctest --test-dir build --output-on-failure
```

The terrain diagnostic executes the original solver at each tile center. New Bark reports 1,024 hits, zero misses and a collision-local Q12 height range of 32768..294877. This is static geometry querying, not player movement or full field collision resolution.

## Assembly and execution boundary

`scripts/prepare_thumb.py` assembles the unchanged original `asm/overlay_01_021FAD1C.s` and `asm/unk_02020B8C.s` through Apple Clang's ARM backend. It removes build-specific include/function-marker directives and adapts GNU divided syntax to LLVM unified spelling, including explicit flag-setting suffixes. It does not translate the game's algorithms into C. Instruction offsets in the terrain unit must match the original function-address layout.

The generated ELF object is linked at an explicit guest address by resolving Thumb BL relocations. The enabled 832-byte image contains the complete terrain unit and the first two functions of the second unit. Other assembled functions are excluded from the executable image and are not claimed as linked/executed. Unexpected relocation kinds or external symbols fail preparation. No SDK library or ROM is needed for this slice.

Generated assembly, object, executable bytes and `terrain_thumb_provenance.json` stay under ignored build directories. The provenance records source/object/code hashes, compiler version, enabled functions, relocations and native arithmetic targets. Address-layout agreement is not a claim that bytes were compared against a ROM.

`ds_thumb.c` is shared platform code with no Pokémon identifiers or resource assumptions. Guest registers contain 32-bit addresses; guest memory access goes through bounded translation rather than pointer casts. Fetches are restricted to the enabled code range, writes to code fail, and accesses require natural alignment. Runs have an instruction budget. Invalid addresses, unsupported encodings and arithmetic-boundary failures return explicit statuses. Register state may be partially advanced on failure; callers discard failed runs.

The implemented Thumb-1 subset includes the arithmetic/flags, signed/unsigned conditional branches, loads/stores, stack and multiple transfers, high-register operations and BL/BX used by these functions. Unsupported encodings, ARM-state transitions and BLX fail. It is not a complete ARM9 emulator: no MMIO, exceptions, interrupts, cycle fidelity, MMU or DS hardware model is implemented. Unaligned DS load behavior is deliberately unsupported. Passing these scenarios does not establish general instruction-set conformance.

`terrain_guest.c` is the game integration boundary. It validates BDHC bytes, copies them into isolated 64 KiB guest RAM, constructs the original 32-bit table descriptor, supplies register/stack arguments and reads the successful output. No serialized address is interpreted as a host pointer. Each query currently allocates a fresh guest context; pooling is a future optimization, not required for correctness.

Two explicit native arithmetic boundaries implement modulo-64-bit multiplication and synchronous rounded fixed-point division. Division computes a signed Q32 quotient, adds 2^19 and arithmetically shifts by 20 to Q12. Divide-by-zero and unrepresentable results fail. These are arithmetic contracts, not execution of proprietary SDK/compiler routines, and hardware division timing/edge-case equivalence is not established.

## What actually executes and is checked

| Original function | Validation |
| --- | --- |
| `ov01_021FAD1C` | Inclusive rectangle membership, endpoint reversal, signed extrema/overflow-sensitive comparisons |
| `ov01_021FAD6C` | Both bounding points for every original collision plane |
| `ov01_021FAD9C` | Every original plane's normal vector |
| `ov01_021FADBC` | Every original plane's constant |
| `ov01_021FADD4` | All ten candidate records initialized, adjacent bytes preserved |
| `ov01_021FADEC` | Empty/singleton strips and signed sorted boundaries for 2..16 entries |
| `ov01_021FAE50` | Synthetic overlapping surfaces, nearest/highest/lowest selection, misses and New Bark geometry |
| `sub_02020B8C`, `sub_02020B94` | Original min/max helpers called by nearest-surface selection |

Accessor tests compare guest results against independent little-endian views for **14,356 planes across all 676 land resources**. This is not full height-solver coverage for all maps.

For New Bark, an independent floating-point oracle scans the plane rectangles and evaluates their equations. At all 1,024 tile centers, original highest/lowest results agree within two Q12 units. Every sampled center hits. Synthetic tests additionally cover a miss, a nearest-surface choice on each side, invalid input and division failure. Guest helpers preserve callee-saved registers and restore the stack. Memory, instruction and budget traps are exercised.

`hg_field_resources_height` connects the original solver to the persistent resource context. It accepts matrix-global tile coordinates and queries the tile center. The coordinate relationship is verified against the inspected original `sub_02054654`: subtract the chunk center from world coordinates, with 16 model units per tile. That original outer function remains unexecuted. Dynamic terrain, moving objects and its other resolution paths are not included. Returned heights remain in the chunk collision coordinate system; they must not be presented as complete avatar collision results.

## Next integration

The original terrain-algorithm gate now has a working execution path. Next work can bind object/avatar updates to static terrain while retaining explicit boundaries for dynamic terrain, occupancy, event scripts and transition state. The same interpreter and guest-memory contract can admit further ARM-only routines incrementally, with new instruction semantics and calls requiring tests before being claimed supported. No second game's code is yet exercised.
