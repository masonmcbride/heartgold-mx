# Original field lifecycle harness and dependency evidence

The first executable preservation scenario now runs the unchanged original `src/field/map_prop_manager.c`. It is a prop lifecycle slice, not the complete field lifecycle. Prop rendering now works through the later integration; movement remains pending.

```sh
cmake --build build --target field_evidence
./build/heartgold_field_lifecycle
ctest --test-dir build --output-on-failure
```

The target produces `build/evidence/new-bark-props-v1/REPORT.md`, `evidence.json` and `trace.jsonl`. The manifest in `scenarios/new-bark-props.json` defines the scenario, required original execution, phase checkpoints, resource provenance, boundary contracts and next integration targets. Evidence artifacts stay under ignored build directories and contain metadata rather than original asset bytes or source excerpts.

## What actually executes

The harness uses original map-header/matrix functions to select New Bark land member 0. A native resource boundary reads its area model-list reference, loads the corresponding original `a/0/4/3` list and validates BMD0 containers from `a/0/4/0`. This does not execute the original assembly area manager.

Original prop-manager functions then:

1. Allocate and initialize 32 slots.
2. Load all 17 New Bark prop records from the actual land resource through a bounded sequential NARC boundary.
3. Expose model IDs, translations, rotations and scales, compared field-by-field against independent little-endian reads of all original records.
4. Mutate a translation, remove a prop and reuse its slot with original `LoadOne`.
5. Fill the remaining slots to the original 32-slot capacity.
6. Reset, load an empty stream, exercise the original missing-model fallback to model 0, reload the 17 original placements, reset and free.

Native render handles must return to zero at reset/destruction. The scenario passes normally and with ASan/UBSan. It does not call `LoadOne` on an already full manager: the original code asserts in that case. Original `LoadFromNARC` does not release prior render handles and `MapPropManager_Free` only frees its own allocation; callers must reset before reload/free. These behaviors are retained, not silently repaired.

**22 of the original unit's 24 functions execute in the passing scenario**, including static initialization helpers. The remaining two compile and link but are unexecuted: `MapPropManager_LoadFromSafariZone` and `ov01_021F3A3C`. The original `DrawModelShapewise` and `ov01_021F3C9C` now execute through the [shared prop-rendering boundary](prop-rendering.md). Calling unsupported Safari, view-culling or lighting services still aborts with a named diagnostic. This is function-entry evidence, not complete branch coverage or proof of every behavior in an observed function. For example, the scenario calls the culling setter but does not verify original culling/render behavior.

## Compatibility boundary

`hg_props` has a scoped preinclude, leaving inventory and existing field compilation interfaces intact. Original prop APIs remain declared by the original `field/map_prop.h`. Original `Field3dRenderObj` and `SafariObjectConfig` declarations and `MAP_PROP_MAX` are extracted unchanged from their upstream headers into ignored generated includes. No original algorithm is extracted, copied or replaced; the full original prop-manager C file is compiled directly.

The Q12 scalar and vector interfaces use exact signed 32-bit components. The tested resource record is 48 bytes on this host, as established by complete 17-record loading and field comparisons. This does not establish serialized-pointer or SDK ABI compatibility. Original live prop objects use native pointers.

`NNSG3dResMdl` and `NNSG3dRenderObj` in this scope are explicitly native resource/render handles, not guessed SDK layouts. They own bounded resource views and lifecycle state. The native resource registry implements allocation/release for these handles; the original `Field3dRenderObjManager` implementation is not compiled or executed. Its behavior is not claimed as ported. Safari configuration, view culling and original lighting attributes remain explicit unsupported boundaries. Shape metadata, transform state and graphics submission now have native implementations with the supported scope described in [prop-rendering.md](prop-rendering.md); original SDK and area-manager assembly remain unexecuted.

The borrowed sequential NARC stream validates every read. The caller validates prop lengths, 32-record capacity and model-ID range before entering the original parser. The runtime is main-thread-only; resources and borrowed byte views must outlive their handles.

## How evidence is produced

Only the original prop translation unit is compiled with `-finstrument-functions`. Compiler-generated entry/exit hooks record actual calls, including static helpers, as offsets relative to the Mach-O image. This avoids treating a hand-maintained list of API names as proof of execution. Buffer overflow fails evidence generation; it does not silently truncate the trace.

The evidence tool:

- Uses the real CMake compilation command to obtain Clang AST call edges for the selected original source.
- Resolves trace offsets against symbols in the actual executable and checks balanced entries/exits and expected lifecycle checkpoints.
- Separates compilation, linking and observed execution for each function.
- Fails for unknown boundary contracts, missing required execution, stale objects/executables or stale prepared archives.
- Records source, object, executable, scenario and resource hashes, upstream revision and the exact AST command.
- Computes transitive unsupported dependencies and ranks their fan-out into the chosen next targets.
- Finds provider hints in the original game C/ARM manifests, distinguishing available C from ARM/Thumb assembly. These textual hints are not compiler/link proof and do not estimate implementation effort.

The tool is scenario-driven and reusable; this first graph covers one original translation unit. It is not yet a complete game dependency graph. Boundary availability is an explicit contract input, not a conclusion inferred from a symbol linking successfully. The existing whole-source compile audit remains a separate measurement; its historical 32-file result has not been regenerated or inflated by this milestone.

Negative tests reject unbalanced traces, unknown/non-original addresses, incomplete lifecycle checkpoints and required functions that never execute. All 15 CTest checks pass normally and with ASan/UBSan.

## Integration frontier

The measured next drawing targets share graphics-submission and area/shape-metadata dependencies. The report identifies original C for `GF3dRender_DrawModel`, while area/shape helpers, culling and rotation providers are ARM/Thumb assembly. That is a concrete preservation constraint, not a missing-header issue.

The [prop rendering integration](prop-rendering.md) now connects original lifecycle/traversal to the viewer, including embedded resources and multi-node signs. The dependency report now leaves only Safari configuration, view culling and lighting attributes unsupported within this prop unit.

After that, expand the scenario toward field objects, original collision-height queries, movement, an interior transition and inventory return. Keep each observed original function and its unsupported dependencies visible in this same evidence pipeline. Source counts and images alone are not acceptance criteria.
