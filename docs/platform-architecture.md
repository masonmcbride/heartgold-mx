# DS platform and game integration

The goal is reusable DS support on Apple Silicon, with HeartGold as the first integration. Only HeartGold is currently exercised. Support for another game, general ARM execution and whole-system DS compatibility are not established. A bounded [original Thumb terrain slice](thumb-terrain.md) now executes.

## Ownership boundaries

| Layer | Current code | Contract |
| --- | --- | --- |
| DS data and graphics | `ds_resources`, `ds_model`, `ds_texture`, `ds_transform`, `ds_raster` | Bounded byte views, explicit host graphics state; no Pokémon IDs or gameplay rules |
| Native host | Cocoa viewers, Metal inventory frontend | Window/input/presentation; no claim of DS GPU timing or register fidelity |
| Game compatibility | `src/compat`, resource preparation | Original symbol interfaces and game resource selection; native handles explicitly distinguished from SDK layouts |
| Original game | Unchanged upstream translation units | Original algorithms; native pointer layouts audited per interface |
| Execution evidence | Scenario manifests, compiler instrumentation, dependency reports | Compile, link, observed execution and behavior assertions reported separately |

Keep field matrices, Pokémon metatile meanings, event banks and save semantics in game integration. Do not move them into `ds_platform` simply because other games also have maps. Conversely, BMD0/TEX0 decoding and affine composition belong in shared support. A second independently developed game must exercise a shared component before cross-game compatibility can be claimed.

Serialized DS addresses must never become host pointers. Existing decoded structures use host pointers only after bounds validation. General binary execution will require a separate guest address space and explicit address translation; the native SDK scalar header does not provide that mechanism.

## HeartGold field relationships verified in source

`src/field/fieldmap.c` supplies the field overlay lifecycle. `FieldMap_Main` compares avatar coordinates with the current location, updates map loading and changes zone when necessary, then runs its flag-selected update work. `FieldMap_ChangeZone` resolves a map header through the matrix and connects the changed location to events, object reconciliation, music, weather and map-name display. These functions are inspected, **not compiled or executed in the current target**.

This makes a map-header boundary more than a geometry change: it changes event/script and object context. Walking into a neighboring chunk must not be implemented as a camera translation alone. A matrix cell's altitude likewise is not a substitute for collision-plane height.

The existing original prop traversal proves only placement/render submission. Its scene capture frees its original prop manager afterward. It is not a persistent object simulation. The original system-task queues execute in their separate tested integration; this does not mean the original field overlay is scheduled by the viewer.

## Persistent resource context

`HGFieldResources` is a host-owned game integration object, not a replacement declaration of the original `FieldSystem`. It retains an original `MapMatrix`, lazily validates land and BDHC tables per queried cell, caches success/failure and returns map-header/member/altitude/attribute plus borrowed land and collision views together. All map identity/altitude/member selection uses original matrix getters. Resource bytes live in the existing process-lifetime cache; freeing a context frees its matrix and metadata, not those bytes.

Coordinates are signed matrix-global tile coordinates. Negative/out-of-bounds requests fail without changing output. Neighboring map headers are returned rather than filtered to the starting map. Contexts are single-threaded and immutable after creation; original matrix mutations are not exposed, avoiding stale cached cells. A future mutation API must invalidate affected cells explicitly. Memory is bounded by matrix dimensions; each queried cell is decoded at most once per context.

`heartgold_field --terrain 60` now uses this context for its repeated tile queries. Tests compare three complete passes over New Bark with direct resource attributes, require one cached cell, then cross both neighboring chunk boundaries and exercise a separate interior context, invalid coordinates and destruction. Collision tables are validated; the later [Thumb integration](thumb-terrain.md) also executes original static height selection.

## Next integration gates

1. **Static terrain execution achieved in a bounded slice:** establish and expand an executable path for original ARM-only terrain routines. `asm/overlay_01_021FAD1C.s` contains BDHC access/query routines; `overlay_01_021FAC44.s` manages associated buffers. The BDHC accessors and complete static height solver now execute through the shared Thumb interpreter; buffer-management routines remain unexecuted. See [the tested execution boundary](thumb-terrain.md). Rewriting a guessed plane solver would not satisfy that requirement.
2. Bind original object/avatar state and field tasks to persistent resources. Verify one complete input-to-object update without rendering, then verify its resulting state in the viewer.
3. Exercise an actual map-header crossing with original event/object reconciliation, followed by an interior transition. Keep save/script services explicit when unavailable.
4. Validate shared DS components with another game's independently obtained resources/code. Keep original assets in ignored build inputs and report unsupported format/ABI cases explicitly.

The gates describe dependencies, not completed functionality. Resource lifetime, bounded original Thumb terrain/movement execution, native viewer walking and limited original background interaction are implemented; general ARM execution, the original full field overlay and cross-game compatibility remain pending. See [player movement](player-movement.md) and [field interaction](field-interaction.md) for current execution boundaries.

The [original transition integration](original-transitions.md) now connects original event parsing, local-field records and seven unchanged source-level C function slices to native scene replacement across New Bark’s eight-map network. Current movement uses original walk/run commands within this admitted static network. Outdoor map-header changes, original object reconciliation, map-init scripts and complete transition-task execution remain integration gates.
