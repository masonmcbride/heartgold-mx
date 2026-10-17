# Native original field-data milestone

Current extension: [original Thumb terrain execution](thumb-terrain.md) now supplies static height queries. Earlier milestone limits below describe their original scope; movement remains pending.

The new `heartgold_field` executable compiles, links and executes original `src/map_header.c`, `src/map_matrix.c` and `src/metatile_behavior.c`. Upstream files and algorithms remain unchanged. This is field metadata loading and terrain-behavior classification, not a movement engine or a rendered map.

```sh
./build/heartgold_field              # New Bark Town, map 60
./build/heartgold_field --map 63     # Player's house, first floor
./build/heartgold_field --map 33     # Route 29
./build/heartgold_field --tile 2     # Original tall-grass behavior predicates
```

The tool uses original map-header getters to select a matrix, and original `MapMatrix_Load` to read it through a validated resource boundary. Original getters return its dimensions, map headers, model IDs and altitudes. It reports only cells carrying the requested map header. `--tile` inspects a supplied behavior byte. `--terrain` now reads actual land attributes and executes original predicates on their low bytes.

Observed original data:

| Map | Matrix | Dimensions in chunks | Matching cell/model |
| --- | ---: | --- | --- |
| New Bark Town (60) | 0 | 47×17 | (21,12), model 0 |
| Player's house 1F (63) | 71 | 1×1 | (0,0), model 216 |
| Route 29 (33) | 0 | 47×17 | (18,12)–(20,12), models 1–3 |

These are map-chunk coordinates and model references, not player-tile coordinates or rendered geometry.

## Resource preparation and compatibility

`prepare_field.py` reads the actual upstream manifests to generate private map-matrix, event, script, message and encounter archive indices under `build/generated`. It packs unchanged matrix files into a local NARC. Matrix/script numeric order is checked against the manifest; full message-bank names and indices come from `NARC_msg.txt`. Encounter indices follow the original encounter array. Events, scripts and encounter binaries are not built or executed by this milestone; their indices allow the original typed map-header table to compile and answer metadata queries.

The native runtime now imports original `filesystem_files_def.h`, retaining original archive IDs rather than its earlier private ID enumeration. Only selected archives are served by the native resource layer. The bag alias refers to its real `a/0/1/5` archive. Other original filesystem services remain unimplemented.

`hg_field` has its own compatibility preinclude. It imports original non-graphics game constants while withholding unsupported graphics aliases. This avoids broad changes to the existing inventory interface. Original map-header data compiles as typed C initializers; no copy of the original table belongs in our repository.

Before original `MapMatrix_Load` sees a matrix blob, `hg_matrix_validate` checks:

- a complete five-byte prefix, nonzero dimensions, and no more than 799 cells;
- section-presence flags of zero or one;
- a name length no greater than 16 bytes;
- the exact required size of the name, optional map-header/altitude tables, and model table.

Then the boundary returns an owned copy to the original parser and provides the missing fill/copy services. It does not rewrite the parser. The loader uses little-endian native reads/copies on this tested Apple Silicon target; this is not a big-endian port.

Calendar/save-dependent Lake of Rage selection and Safari Zone placement are not enabled. Their external boundaries fail with diagnostics. The small calendar declaration in the scoped header exists only to compile those unenabled callers; no RTC SDK implementation or calendar ABI compatibility is claimed. The independent matrix mutation functions for lake levels and the Mahogany antenna tree do execute in tests.

## Tests and limitations

`original_field_data` verifies:

- all 288 original matrix resources pass size/structure validation;
- original loading and getters for every map ID 0–539, covering 280 distinct matrices referenced by map headers;
- every loaded cell's header, model and altitude against the bounded resource bytes, including absent-section fallback behavior;
- New Bark's known world cell, interior/outdoor metadata, original lake-level patches and antenna-tree mutation, and their no-op behavior on a different intended matrix;
- known grass, water, door, ice, waterfall, magma and ledge predicates, with table-backed predicates exercised over all 256 behavior bytes;
- truncated blobs, oversized dimensions, invalid flags and excessive name lengths are rejected before the original parser runs.

A separate CTest case verifies that the CLI rejects an out-of-range map ID. All ten CTest checks pass normally and with AddressSanitizer/UndefinedBehaviorSanitizer. The existing inventory's graphical smoke check also passes.

Original `MapMatrix.matrix_id` is a byte although archive IDs reach 287. The original stored getter therefore truncates IDs above 255. This behavior has not been silently changed; tests distinguish the full map-header matrix ID from the stored byte. The CLI displays the full ID from the original map header. Do not apply special matrix-ID-based mutations to arbitrary high-ID matrices without auditing that assumption.

Eight matrix resources are validated but are not reached through the 540 original map headers in this test. They are not claimed as loaded by original functions. Save-dependent variants, field overlays, tile collision, player movement, scripts, encounters and NSBMD map-model rendering remain unimplemented.

## Audit and next step

The compile audit uses the field preinclude for the same three original field sources as the CMake library. With new resource indices and the retained original archive IDs, 32 of the 399 game C units now emit ARM64 objects. Relative to the initial 27-file baseline, these are `sys_task_api.c`, `map_header.c`, `map_matrix.c`, `location_gmm_dat.c` and `unk_02068F84.c`. The last two remain compile-only candidates, not integrated/behavior-tested features.

The land resource and terrain milestone below now connects actual tile attributes to original predicates. Geometry and collision decoding remain next.

## Land chunks and actual terrain attributes

```sh
./build/heartgold_field --terrain 60
./build/heartgold_field --terrain 33
```

The land milestone reads the original `files/a/0/6/5` NARC, copied unchanged into the ignored build resource directory. All 676 members pass bounded section decoding. The resource cache allows 32 MiB specifically for this 19.5 MB archive; other archives retain the 16 MiB cap.

`src/land_data.c` is a resource boundary decoder, not a replacement game loader. Inspection of original `ov01_021F49FC`, `ov01_021F4AAC`, `ov01_021F4AE4`, and their call sequence in `asm/overlay_01_021F4704.s` establishes four size words, followed by a four-byte prefix whose high halfword gives the extra payload size. The decoder checks the observed `0x1234` prefix marker, consumes that payload, and then returns bounded views of attributes, props, BMD0 model, and collision sections. It requires a 2,048-byte attribute grid, exact total length, and a BMD0 signature/file-size match. It also validates the little-endian version-2 BMD0 container, single block table, and bounded MDL0 signature/length. Props and prefix payload remain opaque. The subsequent [geometry/collision milestone](model-geometry.md) now validates model/mesh and BDHC tables; bounds validation is not complete semantic validation.

New Bark's member 0 has an 88-byte prefix payload, 816 bytes of props, 16,604 bytes of model and 658 bytes of collision. Its actual attributes therefore begin at offset 108. The original C `terrain_attributes.c` helper reads at fixed offset 20 and has a 225-cell matrix-index table and 16-block cap. It is unsuitable for the 799-cell world matrix and is **not compiled or executed** here. The original assembly overlay loader is inspected, not executed or translated into a replacement gameplay implementation.

`hg_land_tile` decodes little-endian attribute words at local x/z coordinates. `hg_field_attribute` loads a matrix using original functions, selects its land member using original `MapMatrix_GetMapModelNo`, and queries matrix-global tile coordinates. For New Bark these are x=672..703, z=384..415. Coordinates may select neighboring map headers in a shared matrix; the API does not constrain them to the requested town's header.

Original `GetMetatileBehavior` assembly in `unk_02054648.s` extracts the low byte of the attribute word. The boundary uses that same representation when calling the already linked original C metatile predicates. It preserves the whole word for future collision work and does not interpret upper bits as a walkability rule. New Bark's grid classifies 15 tiles as surfable and zero as tall grass. The text diagnostic also marks door predicates, with x increasing right and z down. This is a terrain diagnostic, not original map graphics or a movement simulation.

Tests decode all 676 chunks and query all 692,224 attribute words, execute original grass/water predicates, verify New Bark section sizes and classifications, compare all its world-coordinate queries with direct reads, check neighboring chunks and coordinate limits, and reject every truncation of New Bark plus malformed sizes, excessive prefix lengths, corrupt BMD0/MDL0 container fields and trailing bytes. All ten CTest checks pass normally and with ASan/UBSan after integration. No additional upstream C unit is claimed as executed or compile-only coverage.

The subsequent [model geometry milestone](model-geometry.md) implements bounded BMD0/MDL0 meshes, CPU wireframe/solid diagnostics, the supported single-root scene transforms and BDHC collision-table validation. Materials/textures and collision-height semantics remain necessary before full rendering and movement.
