# Field initialization dependency frontier

Current executed parent boundary: **FieldSystem_ProcessSoundplate: canonical parent contract**.
The original ARM7 command service now processes the real field BGM preparation
and publishes completion; original ARM9 reply handling recycles the commands.
The ARM7 checkpoint below supersedes the earlier PXI rejection. Audible original
field playback, Init completion, Main and an interactive frame remain unverified.

This checkpoint uses the actual working tree, not the older progress note. Before
these changes the parent already executed object graphics activation and rejected
at `ov01_021E6580`. Both baseline and resulting runtime logs are retained under
ignored `build/evidence/field-init-frontier/`. Upstream is unchanged.

## Reproducible inventory

Use the existing source graph and generation manifests:

```sh
python3 scripts/audit_original_field.py --frontier \
  --output build/audit/original-field/frontier.json \
  --native-calls build/audit/frontier-native-calls.json \
  --trace build/evidence/field-init-frontier/runtime.log
```

`frontier-native-calls.json` was refreshed using the existing
`dependency_closure.native_calls(project, build)` compiler-analysis function after
the complete build. It records current Clang-emitted calls and open native
indirect dispatch, not just matching C symbol names. Original-to-native candidate
links are included in the frontier; their names do not prove ABI equivalence.

The JSON retains the conservative graph rooted at `FieldMap_Init`,
`ov01_021E6580`, `FieldMap_Exit`, the VBlank callback and original update/draw
owners. Each node includes callers, call/tail-branch versus address/data
relationships, generated native-body provenance, guest exports/imports, native
provider candidates, original ABI evidence, reachability classification and
shared capability obligations. Direct MMIO literals and symbolic register
references are a separate hardware review list; a literal is not proof of an
access. `--trace` adds execution observations without changing static reachability.

This is a large source supergraph, not a list of functions that LOAD must execute.
Conditional branches, all macro alternatives, callback tables, ambiguous
same-named definitions, scripts and application dispatch expand it beyond the
opening path. Roots are statically confirmed; other source dependencies remain
conditionally reachable and missing/indirect edges unresolved. Exported code is
assembled, not necessarily supported transitively. Native spelling and generated
bodies do not establish ABI compatibility, linkage or execution. The grouped
missing-or-unverified lists are review queues, not counts of missing algorithms.

## Shared capability batches

| Capability | Original dependencies and consumers | Status / necessary contract |
| --- | --- | --- |
| Pointer-free environment records and SDK global state | `ov01_021E6580 -> ModelAttributes_Init -> FieldMap_InitModelAttributes -> Fog_New`; original Exit frees both | Implemented prefix below. Same live guest bytes, original ARM32 scalar layout, typed destruction, SDK software state distinct from command submission. |
| Camera, history and projection | `FieldCamera_Create`, all 17 templates, original camera C/history, `ov01_021EAC4C/6C`, preset update and `FieldCamera_Delete` | Original field-camera path now executes, including an active preset transition and destruction. Renderer projection consumes original arguments. Exact SDK matrix/table and pixel equivalence remain unproved; see latest checkpoint. |
| Time/light/weather and environment resources | `AreaLightManager_New/UpdateActiveTemplate/Free`, flag-96A archive choice, `WeatherManager_New/SetWeather/Delete`, map-name/signpost, texture slots and Gymmick | Reuse existing archive/save/RTC/texture transport; original constructors, callback tables, update and teardown need explicit layouts and failure/publication contracts. Some ARM units are already assembled; that is not a connected owner. |
| IRQ and asynchronous lifecycle | `Main_SetVBlankIntrCB(FieldMap_VBlankCallback)`, HBlank owners, normal type-1 bottom screen / overlay 27, completion, Main and Exit | Existing queue/IRQ transport is reusable. Original VBlank callback's BG/OAM and model-pool leaves, HBlank retained work, overlay task completion/cancellation and phase ordering are distinct obligations. Do not select empty bottom-screen handlers or publish DONE early. |
| Scripts, sound and canonical object callbacks | LOAD weather/BGM/soundplate/resume scripts; Main/input/movement and object descriptors | Existing canonical slots, guest callbacks, scalar accessors and resource services cover parts of this graph. Nonempty async scripts, enabled input, sound handles and arbitrary graphics/movement callbacks remain conditional or unresolved. |

The first batch crosses display enabling, model allocation, original default
configuration/global publication, fog allocation and existing original save camera
type accessors together. It supplies a complete component teardown rather than
adding successful substitutes for each rejecting game function.

## Implemented prefix and runtime evidence

`prepare_field_loader.py` assembles the unchanged original model-attribute unit
and compiles the original field configuration and fog lifecycle/read-accessor C
for ARM946E-S Thumb using the existing assembler/relocator. Fog's ARM record is
compile-time checked at 0x38 bytes; model assembly uses 0x48 bytes. Local compiler
labels are namespaced when combining the two existing generated C fragments.
`prepare_original_parent.py` emits unchanged original environment entry and
VBlank callback bodies; unresolved leaves remain explicit rejecting imports.

The loader uses existing `DSMappingRef` identities for the two environment
owners. Shared bytes are returned to the native canonical field, while ARM keeps
guest addresses. Destruction validates the current owner incarnation, writes an
ARM32 pointer into a scoped guest-RAM cell, executes original pointer-to-pointer
free, verifies its null publication and clears the native field. Native stack
pointer widths never cross that ABI. Foreign allocations of the same size,
interior pointers and duplicate live construction are rejected.

The shared memory import now supports bounded, aligned `MI_CpuClear32`, exposed
by actual fog execution. Native display adapters admit the already configured
opening graphics mode and retain unrelated display bits. `NNS_G3dGlbPolygonAttr`
retains packed SDK **software global state**; it does not claim to have submitted
a hardware command or rendered fog/lighting. Model/material mutation, global
cache/flush behavior and projection/raster refinement remain open.

The executed parent now rejects at `FieldCamera_Create`, after the actual model
and fog allocations and camera-type read. Tests independently check all initial
record bytes and the polygon value 0x001f8080, exact heap restoration, dependent
interior guest-view invalidation, foreign-owner preservation, duplicate and stale
rejection, and three additional construct/free cycles per parent session.
They tear down only the completed components; whole `FieldMap_Exit` is not run on
the partial aggregate. A rejected LOAD is not resumed from its beginning.

`HG_TRACE_ORIGINAL=1` records optional JSON events for guest entry/return,
import requests and task callbacks with queue and guest data identities. The
inventory matches numeric entries against the current generated images. A result
of 0 is `DS_THUMB_RETURN`; failed return results remain separately counted.
Internal direct calls are not instrumented, and aliases of one PC cannot be
disambiguated by this trace. Other resource probes and injected-failure runs in
the same executable are included; their events are not labeled completed LOAD.
Instrumentation can alter host timing; this is not cycle-accurate IRQ evidence.

The trace resolves the indirect default texture/palette free-service targets to
existing checked SDK-key release boundaries (synthetic callable identities,
not byte-compatible RAM). It also records actual queued guest callback targets.
These observations constrain callback admission; they do not close the remaining
static indirect edges or prove that all source-reachable callbacks execute.

## Prior checkpoint obligations

Camera/history/projection is the next blocking shared capability. Light/weather,
texture/UI/Gymmick owners, VBlank/HBlank leaves and type-1 bottom lifecycle remain
required before DONE. Main, enabled input, combined asynchronous Exit and visible
field-frame equivalence remain unexecuted. Variants, resource failures, arbitrary
callbacks and partial aggregate recovery need their own execution evidence.
The memory contract's address/reuse/SDK accounting and device/CPU/Metal proof
obligations remain unchanged. No machine-checked equivalence proof is claimed.


## Prior validation checkpoint

The final complete build and CTest suite passed **82/82** (38.11 seconds).
Targeted ASan/UBSan checks passed **3/3** (2.98 seconds): memory correspondence,
native heap/module lifetimes and original field construction. LeakSanitizer is
disabled on this unsupported platform; no leak-check result is claimed. The
existing audit Python suite passed **11/11**, including new cross-reference and
trace-separation assertions. The loader passed a syntax check with test hooks
removed; that check is not a separate production link/execution.

Logs and a machine-readable validation summary are in ignored
`build/evidence/field-init-frontier/`; the full annotated graph is in
`build/audit/original-field/frontier.json`, with current native compiler calls in
`build/audit/frontier-native-calls.json`. Whitespace checks passed and upstream
has no working-tree changes. Existing unrelated user changes were preserved.


## Camera and area-light checkpoint

The starting inventory was reused, not rediscovered. `--refresh-frontier` retains
its original nodes and dependency edges while refreshing generated exports,
current compiler providers and runtime observations. Upstream must be unchanged
for this mode; it is unchanged here. The new audit test rejects any attempt to
rediscover the graph during refresh. Reproduce the current report with:

```sh
python3 scripts/audit_original_field.py --frontier \
  --refresh-frontier build/evidence/camera-light/frontier-before.json \
  --native-calls build/audit/frontier-native-calls.json \
  --trace build/evidence/camera-light/runtime.log \
  --output build/audit/original-field/frontier.json
```

### Original execution and ownership

The existing generator compiles unchanged `src/camera.c` for ARM946E-S Thumb and
assembles the complete original field-camera unit and its 17 templates. Camera
and history records are explicitly ARM32 (0x68 and 0x28 bytes); their pointer
fields contain guest addresses. The canonical native field retains opaque
backing pointers and must not dereference them using a widened native `Camera`
layout. Original allocation, target/distance/angle initialization, clipping,
offsets, static camera selection, history, per-frame look-at, preset transition
math and deletion execute through the existing interpreter.

The player-position vector is borrowed through the existing mapping subsystem;
no position snapshot is substituted. A `DSMappingRef` plus bounded offset tracks
that binding incarnation. Updates reject a freed/shrunk target before guest
execution; destruction still works without dereferencing the retired target.
Retargeting executes original `Camera_SetFixedTarget`, including its original
history behavior. Camera/history/vector/transition allocations retire through
original frees and the existing native-heap observer.

Camera destruction deliberately leaves the numeric camera/transition values in
the projected **live FieldSystem** fields, as the original assembly does. Those
parent-owned words remain readable even though their pointees have retired.
Recreation overwrites the same field view; final FieldSystem retirement invalidates
that view. Failed construction restores its field/global preimages. The partial
parent fixture separately releases native publications after checking these guest
values; it does not turn the original destructor into a nulling operation.

Synchronous construction uses the existing heap acquisition tag around only its
own imports. IRQ service runs outside that tag. Failure retires tagged allocations
and restores owned preimages; external allocations and the persistent object-
graphics VBlank task survive. The camera's published target lifetime and the
area-light manager's borrowed attributes lifetime are checked independently from
numeric pointers. Retiring/recreating attributes cannot silently reattach an old
light manager. Component frees remain legal after their borrowed dependency retires.

### Mathematics and renderer boundary

Original camera algorithms execute with explicit Q12 arithmetic transports:
rounded signed multiply (arithmetic floor after adding 0x800), the existing
synchronous FX division, bounded vector operations and indexed angle reduction.
The Q12 trig table is generated mathematically at 70 decimal digits; no SDK table
or new dependency was imported. **Full equality to the original ROM's lookup
table remains unverified.** Tests compare composed source calculations against
independent long-double trig/rounding and signed integer reference arithmetic,
including cardinal/noncardinal angles, negative products, wrap and discarded low
angle bits. This is conditional SDK-contract evidence, not a binary differential
proof of Nintendo's table.

Original NNS look-at, perspective and orthographic arguments feed a matrix mode
added to the existing `DSCamera`/CPU raster primitives. Original camera records
retain their integer inputs. Projection and an untextured triangle are exercised
through the actual raster code; no second gameplay camera was added. The old
viewer camera remains the zero/default mode. This double-matrix transport is
**not** verified against SDK Q12 matrix rounding, normalization/cache internals or
DS pixels. Frustum-crossing geometry rejects instead of silently disappearing;
full clipping and perspective-correct texture interpolation remain open. No
Metal equivalence, native-window result or complete field frame is claimed.

### Area-light resource group and observed bounds gap

The original area-light constructor/update/free and model-attribute setters now
execute. The existing file transport serves five byte-identical original CRLF
resources, with original ASCII parsing, file allocation and RTC-to-seconds code
compiled for ARM. The RTC adapter uses the existing save/clock owner, not another
clock. SDK light/material arguments are retained as software state; visible
lighting/model-material refinement remains open.

Two resources (`area01light.txt`, `area02light.txt`) have terminated EOF lines and
complete construction, update and teardown. Three (`area00light.txt`,
`dun20_01light.txt`, `dun20_02light.txt`) end in bare `EOF`. Executing the original
`Ascii_GetDelim` on those files reads past the requested file allocation while
searching for CR/NUL. The current bounded mapping rejects at the first uncovered
byte and construction rolls back. This is a concrete gap between the current
allocation-view coverage and original DS RAM/heap backing, not evidence that
Nintendo's allocator is unnecessary or that its padding is zero. No terminator
was appended, no asset was changed, and no speculative Nitro allocator/padding
policy was introduced. Original backing contents and any adjacent-byte observation
must be established before admitting these variants.

The opening-map resource is supported. Its parent proceeds through camera and
area light, then executes the original weather constructor prelude. The unresolved
`GF_InitG2dRenderer` import rejects before any weather task/resource publication;
the two private allocations are retired. The diagnostic probe asserts this known
admission boundary so adding later services requires reviewing ownership again.
Weather itself is not implemented by this probe.

### Executed tests and next capability

The constructor test exits successfully with explicit checks for:

- All 17 camera templates, retained vector movement and Y-only history behavior;
  the original ring first overrides after its initial seven reads.
- An active preset transition over its original frame count, with signed integer
  angle interpolation and the final offset checked against original table inputs.
- Perspective/orthographic numerical projection, negative fixed-point rounding,
  interior/retired references, target retargeting, owner replacement/recreation,
  duplicate construction, original frees and injected heap-budget failures.
- Parent-owned dangling numeric field values, unchanged persistent graphics
  VBlank ownership, and exact component heap restoration.
- Independent parsing of all five light resources, successful lifecycles for two,
  explicit bounds rejection/rollback for three, original half-second time keys
  and one-template-per-update advancement after a clock jump.
- Light attribute-owner invalidation and replacement without stale reattachment;
  constructor failure preserves that external owner.

These are component-level movement/map-owner replacement tests, not an executed
map warp or autonomous FieldMap_Main frame. The second parent session retains its
pre-existing pending-IRQ cancellation scenario; it is not drained by the expanded
fixtures. During the first session the tests deliver bounded IRQ passes because
the original graphics VBlank task is persistent, not an empty-queue completion gate.

The next coherent missing group is **ARM32 2D renderer/sprite-resource/OAM
ownership**, not a successful replacement for one weather function.
`WeatherManager_New -> ov01_021EB64C -> ov01_021EB4B8` requires renderer/surface
setup, four original 2D resource managers, four resource headers, a 64-entry sprite
list, original Main task dispatch/cancellation and matching destruction. Later
weather callbacks additionally require sprite creation/animation, fog/BG/OBJ/VRAM
publication and original field-pointer ABI views. Existing native scene widgets
are not proven binary-compatible SDK records. Empty records or no-op renderers
would not establish this contract. Map-name/signpost and bottom-screen lifecycle
remain subsequent owners; DONE/Main/input/whole Exit remain unexecuted.

Remaining proof obligations include the ROM trig table, exact SDK matrix and raw
`NNS_G3dGlb` representation, unadmitted camera interfaces/ABI returns (including
position-derived initialization requiring vector magnitude/normalization), the
bare-EOF heap/RAM observation, arbitrary callback/reentrant constructor schedules,
process-fatal native allocation failures, exact guest address/overlay observations,
and full device/CPU/Metal refinement. No machine-checked equivalence proof is claimed.


Final validation: the complete build and full CTest process exited 0 with
82/82 passing tests (38.40 s). The targeted ASan/UBSan process exited 0 with
3/3 passing tests (3.96 s); LeakSanitizer was disabled. All 12 audit Python tests
passed. The final traced constructor process also exited 0 before frontier
refresh. No UI smoke, ROM-table differential, whole field Exit/Main or Metal
rendered-frame equivalence test was performed.

The refreshed inventory includes `checkpoint` and `checkpoint_evidence` fields
linking `build/evidence/camera-light/checkpoint.json`: validation exit codes,
capability-specific limitations, the current boundary and unexecuted paths.
Static reachability remains separate from observed entry/import events.


## Weather, field Window/message and texture-descriptor checkpoint

The existing graph was refreshed from
`build/evidence/weather/frontier-before.json`; no broad graph discovery was
repeated. The current inventory is `build/audit/original-field/frontier.json`.
The constructor trace is `build/evidence/weather/runtime.log`. Entry/import/task
observations remain distinct from static reachability: interpreter return-status
0 means execution returned, **not** that a guest function returned value zero.
Internal direct calls are not individually instrumented. Component state checks
and source control flow supply additional evidence, not instruction-trace proof.

### Admitted original code and shared services

| Capability | Original code and observed scope | Remaining admission boundary |
| --- | --- | --- |
| ARM32 2D owners | Original `GF_InitG2dRenderer`, surface setup, four `Create2DGfxResObjMan`/raw managers, `GF2DGfxResHeader_Init`, `SpriteList_Create`, NAN relocation, empty-list update and matching destructors execute. Renderer is 0x98, surface 0x48, Sprite 0x104, list 0x124; SDK headers enforce layouts. | Active cell drawing/animation, uploads and renderer callbacks remain rejecting imports. Empty-list traversal is original behavior, not a replacement renderer. |
| OAM | Full original SDK initialization, reservation, attribute entry, affine entry, hardware publication and reset execute. Checked copy/cache leaves publish into the existing `DSVideo.oam`. Native opaque empty-instance emulation was removed. | Fast DMA reset and software 3D drawing remain unsupported. OAM bytes are device state, not a claim of Metal presentation. |
| Weather ownership | Original root/sub-owner construction, resource headers, 64 sprite slots, default animation bank, persistent Main task, original same-weather selection, callback trampoline and complete component deletion execute. | Animated/nonzero weather variants and their resource/VRAM transfer owners remain unverified. No full Main queue or field frame is claimed. |
| Window and encoded messages | Original native field Window allocation/fill/removal, map-name and inactive signpost constructors/destructors, original encoded String owner, original lazy MsgData allocation/decryption/free execute. Native field records have explicit native layouts and never enter ARM32 storage. | Direct message loading, text printing, name animation and complete Window-to-device publication are not admitted. The menu preview Window/UTF-8 MsgData is never cast to these records. |
| Texture descriptors | Original `FieldTextureManager_Init/LoadTexture/FreeAllSlots/Destroy`, original ASCII resource-name conversion and SDK dictionary lookup execute. The NARC139 descriptor catalogue is retained without fabricated members; selection is checked independently using the existing bounded texture-name decoder. | Recurring texture animation/VRAM task enqueue and flush remain unsupported. Descriptor construction does not establish uploads. |
| Following initialization | Original saved-weather accessor, original `Gymmick_Init` no-gymmick branch and `Main_SetVBlankIntrCB(FieldMap_VBlankCallback, field)` execute. | Parent rejects at `FieldBGM_PlayEffectiveForMapHeader`. Nonzero gymmick callbacks remain explicit rejecting original-overlay boundaries. |

The SDK software algorithms compile for the existing Thumb runtime from the exact
NitroSDK/NitroSystem revisions already pinned by upstream. No SDK material or game
resources are added to tracked source. Native image-proxy preview records remain
separate from ARM32 SDK records. Construction does not admit active rendering:
the four retained renderer callback identities explicitly reject, and admitting
them requires compiling private renderer matrix/cache state together with the
active renderer operations.

Window word filling uses the existing unsigned-packing adaptation mechanism for
the original promoted `fillValue << 24`; tests exercise all 16 4bpp colors under
UBSan. The MMIO adapter now subtracts numeric addresses before pointer addition,
fixing the sanitizer-detected invalid intermediate pointer. Optional Thumb device
hooks accept only aligned, bounded, unmapped data requests; instruction fetch
never uses them. The loader admits only audited halfword BG0CNT/BG2CNT/BLDCNT
operations. Other registers/widths reject; no general hardware emulation is implied.

### Resources and lifetimes

Upstream's unchanged `msgenc` and its existing patched yyjson dependency generate
all **829** encrypted banks in original manifest order. The original `jsonproc`
and trainer-name template supply bank729. Generated banks remain in ignored
`original_messages.narc`, separate from UTF-8 preview archives. Each bank is cached
by source, tool and charmap hashes. Every message in bank279 is read by the
original lazy decryptor and compared with independently mapped JSON/UTF-8 glyphs;
other banks are encoded, not all runtime-decoded. ROM-byte equality is unproved.

Synchronous acquisition tags now track task and NARC publications as well as heap
blocks. Rejection cancels only tagged tasks, releases only tagged archive leases,
retires mapped backing and frees private blocks, in that order. IRQ tasks are
outside the tag. Successful commits clear the tags. Weather rejection points
include after its Main task and archive acquisition; persistent object-graphics
VBlank tasks and external owners survive. Texture-load failure restores the
original owner preimage and retires only its acquisitions. Native map-name
interruption at the archive transport rolls back original partial allocations;
its complete-object destructor is never called on an interrupted aggregate.
Native process-fatal malloc/budget assertions remain outside recoverable rollback.

Original OAM failure semantics are deliberately retained: an affine reservation
collision can return FALSE **after** committing attribute reservations. Tests
check that partial mutation instead of imposing a transactional safety policy.
Original `FieldTextureManager_FreeAllSlots` also leaves dangling numeric table
words; their mappings retire without nulling original bytes. Retired test backing
and duplicate destruction/initialization reject at the admitted adapter boundary.

The original field VBlank callback is newly registered at this initialization
prefix. Its scheduled BG/VRAM/object flush path has not executed. The aborted-
parent fixture checks that publication, then removes this field callback before
component fixtures tear down its retained inputs. Persistent queue tasks and the
IRQ pump remain live. This is explicit abort cleanup, not a successful original
FieldMap Exit or a bypass of the unimplemented callback. Full field callback,
Main, enabled input, bottom-screen ownership and whole Exit remain open.

### Build inputs and remaining proof obligations

Configuration performs no downloads. Set `HG_NITROSDK` and `HG_NITROSYSTEM` to
read-only checkouts at the revisions in upstream's wrap files; their HEADs are
checked and used source/header hashes are recorded. `HG_YYJSON` points to upstream's
0.12.0 source with its packagefile C/header patches. The downloaded archive SHA256
was checked against the upstream wrap:
`b16246f617b2a136c78d73e5e2647c6f1de1313e46678062985bdcf1f40bb75d`.
These are upstream's existing build inputs, needed here to retain original
renderer layouts/software operations and the original message codec. SDK sources,
tools, compiler products, resources and evidence stay under ignored build paths;
`upstream/pokeheartgold` is unchanged.

Unproved obligations remain: general ARM/Thumb execution equivalence, compiler/ABI
and implicit condition-code observations at original assembly/C interfaces,
malformed SDK resource behavior, arbitrary constructor/callback interleavings,
unbounded restart/resource-cache capacity and original address/heap/overlay reuse,
process-fatal native allocation failures, active sprite/VRAM/OAM device timing,
SDK renderer matrix/cache state and pixel refinement. No new machine-checked
proof is claimed. Existing numerical/ownership tests establish the stated cases,
not global equivalence. The three bare-EOF light resources still reject at bounds;
no backing bytes were invented. ROM trig-table equality and exact Nitro matrix
rounding remain unverified, and Metal equivalence is not claimed.


The opening map has **zero matching animation slots out of nine descriptors**,
confirmed independently against bounded texture dictionary enumeration. Thus
catalogue loading/name lookup/selection and repeated empty-slot teardown execute;
populated texture-animation payload allocation, upload and occupied-slot teardown
are not covered by this map. Nonzero weather, visible map-name animation and
other maps must not inherit an equivalence claim from these prefix tests.

Final completed checkpoint: full build and **82/82 CTests** (38.53 s), **4/4**
targeted ASan/UBSan tests including Thumb device dispatch (5.40 s), **12/12** audit
tests, and final traced constructor all exited **0**. LeakSanitizer was disabled.
The earlier MMIO fix also independently passed the sanitizer constructor test
before further integration. No unfinished process is counted as passing.
Upstream status and `git diff --check` are clean. No UI smoke, original Main entry,
full queue frame, bottom-screen lifecycle, whole original field Exit or Metal
pixel equivalence is claimed.


## Original BGM policy and field publication checkpoint (2026-10-25)

This is partial integration. Original `FieldMap_Init` still stops in LOAD; the
original BGM request does **not** play a sequence. Original `FieldMap_Main`, input
movement and a complete interactive frame have **not** been reached.

Unchanged native original `FieldBGM_GetEffective/GetForMapHeader/GetOverride`,
`FieldBGM_SetOverride`, `FieldBGM_PlayEffectiveForMapHeader`, `sub_02055198`,
`IsNighttime`, `GetOverriddenMapMusic` and `CheckFlag99A` now use the canonical
save, RTC, map headers and player. The existing original ClearOverride remains
shared with the transition unit. Tests check all 24 hours, contest map overrides,
saved overrides, surf priority and the flag suppressing surf music, then restore
all fixture state. These functions are native original bodies, not ARM execution.

The existing guest image now contains whole original `unk_02004A44.s` and
`unk_02005D10.s`, and original `sound.c` compiled with pinned SDK declarations.
The executed ARM prefix is `Sound_SetFieldBGM`, `sub_02005CF4`, `sub_02004EB4`,
`Sound_SetSceneAndPlayBGM`, `Sound_SetScene`, `sub_02005AF8`, and
`GF_SndSetAllocatableChannelForBGMPlayer`, including original attribute lookup.
Internal direct-call execution is established by this fixed source path and its
observed SDK request; root entry/import tracing does not instrument each internal
call. `PlayBGM` is compiled and linked, **not executed**.

`SND_WORK` retains its checked ARM32 size 0xBEC88 and original field offsets.
Only its 0xEC-byte metadata tail at offset 0xBEB9C is mapped. The source global
storage declaration becomes extern; the loader owns that partial byte-compatible
view. This private staging state is not a persistent global sound owner; a future
shared audio owner must survive map-loader destruction/reconstruction. There is
no fabricated SDK heap/archive/handle storage and no claim that
`InitSoundData` ran. Rejected scene requests restore private policy bytes before
returning failure; the native bridge clears the request guard and rejects.
Repeated requests must reach the same device rejection, rather than treating a
partially written scene 4 as successfully active. An independently seeded
same-scene fixture verifies original FALSE suppression without claiming playback.

### Remaining initialization and recurring dependencies, analyzed together

| Capability and source path | Confirmed dependencies and required semantics | Evidence/status |
| --- | --- | --- |
| Shared sound lifecycle: `field_bgm.c` → scene 4 → `sub_0200508C` → `PlayBGM` | Player-7 channel mask 0xA7FE; capture stop/status; handles 0/7 and effects; pause/stop; field/radio bank selection; original sound-heap save/load/disposal and archive lifetimes | The channel-mask request is executed and rejects. Remaining effects are statically conditional. Existing `DSAudio` renders sequences, but has no SDK heap, player-handle/capture/channel-reservation contract. Routing the whole scene to `ds_audio_play` would bypass original state/ownership and is not admitted. |
| Environment sound: `FieldSystem_ProcessSoundplate` → coordinate lookup → activation table | Map-load/collision-selected soundplate records; save flags; sound-effect handle ownership, volume ramps and stop/fade | Statically reachable after the BGM result/environment-state branch. Original source/table exists; not executed here. |
| Resume scripts: `TryStartMapScriptByType(INIT_SCRIPT_ON_RESUME)` | Original header/type selection, task ownership, script-command closure | Source-confirmed following edge; this LOAD invocation has not executed. Existing original script machinery is reused, not evidence of this branch. |
| HBlank and poison-effect owner: `HBlankSystem_New/Start` → `ov01_021FB5D4` | ARM32 0x1C owner with two callback slots; actual callback registration/IRQ mask; 0x328 poison owner and `sub_02014A08` double-buffer transport; cancellation before free | Original assembly inspected, callbacks and teardown identified. Existing native IRQ services preserve refusal/masking. Guest retained-callback adaptation and poison transfer are not integrated. |
| Bottom screen: `FieldSystem_InitBottomScreen` → `ov01_022066DC` | Six four-slot callback rows indexed by bottomScreenType 1..6 (guest offset 0x18), derived from the selected map-load mode. Row 1/3 constructs overlay27 through `ov01_021F6894`, retaining a SysTask/environment and canonical FieldSystem. Row 2 contains original empty callbacks, not permission to use them for row 1. Destroy/poll must retain overlay until complete. | Exact original table inspected; selection and overlay27 task execution remain unverified at runtime. Dummy-running check has null callback rows and returns TRUE; that does not establish bottom-screen readiness/publication. |
| Recurring Main: `FieldSystem_Main` → Control → task frame → parent Main; `FieldMap_Main` → location synchronization/zone change → `ov01_021E5FC0` | Player movement gating/input, save/bug-contest clock, light and camera update, signpost, texture upload, prop animation, map tick, render submission. `unkBC=15` enables all four update/render groups. Zone changes additionally reload events/objects, fade BGM, weather/name animation. | Original source-confirmed, conditional branches retained; Main and movement have not run. Standalone loader ticks/camera tests are separate evidence. |
| VBlank publication: `FieldMap_VBlankCallback` | Scheduled BG scroll/tilemap transfers, VRAM transfer tasks, OAM apply/reset, object graphics pool acknowledgement | **Entire original native callback returns five times per session**, twice reconstructed. Test schedules real BG3 bytes/scroll, checks canonical DSVideo VRAM/registers, and exercises the original pool 2→0 transition. VRAM queue is empty. Retained graphics VBlank task survives; stale pool publication after graphics teardown rejects. |
| Presentation and frame orchestration | Original Main/Print/VBlank/VWait ordering, IRQ phase, input sampling, original draw lists/geometry submission and swap/publication, audio advancement | Existing scheduler explicitly describes host phases, not a complete DS main-loop port. Direct callback calls are not recurring IRQ-driven gameplay frames. Populated transfer queues, active sprite rendering, and Metal pixel/audio refinement remain open. |

The last VBlank operation is original `sub_02023910`, which acknowledges a pool
state byte (2→0); it is **not** a geometry flush or proof of rendered pixels.
Two bounded bridges retain the original getter and acknowledgement and reject
retired owner storage. Original scheduled BG code now extends the same existing
BG closure, using existing native register/VRAM transports; affine operations
remain rejecting boundaries. No second renderer or scheduler was introduced.

The enlarged original guest image exposed a real pre-existing fixed-scratch
collision: terrain projection at 0x0204B000 zeroed original model backing. Scratch
is now located after the aligned image, separate from eight reserved call-stack
intervals. A synthetic return marker is outside that storage. Assembly rejects
images overlapping import identities; loader compilation rejects RAM/stack
placement reaching the import range. The independent overlap test and existing
retained-land triangle checks cover this regression. No original guest allocation
was remapped, and no heap budget/test assertion was relaxed.

Machine-readable evidence reuses `build/audit/original-field/frontier.json` and
`build/evidence/field-bgm/{runtime.log,checkpoint.json,frontier-before.json}`.
Guest exports take precedence over generic provenance-body labels in inventory
status, avoiding the previous misclassification of generated ARM bodies as native.
The graph stays conservative and incomplete; aliases of an entry PC are not
individually disambiguated. Source inspection and fixture evidence do not close
unobserved indirect dispatch.

Validation results and remaining proof obligations are recorded in the checkpoint
JSON and current-work note. No new Lean theorem was checked. In addition to the
sound/IRQ/Main obligations above, the three bare-EOF light resources, ROM trig
lookup equality, Nitro Q12 matrix/cache behavior, nonempty graphics transfer
ownership, general allocator/address observations and Metal refinement remain
unproved. Upstream and prior user changes are preserved.


## Shared channel services checkpoint (2026-10-25)

The original field request now executes the NNS player-mask service and rejects at
`NNS_SndCaptureStopReverb`. Four milestones remain separate: field-requested BGM
playback **no**, complete FieldMap_Init **no**, FieldMap_Main execution **no**, and
interactive frame **no**. Standalone original New Bark music (SDAT sequence 1018)
produces checked nonzero PCM through the existing renderer; that fixture is not
execution of Nintendo's `PlayBGM` or success of the field request. The repeated
original VBlank callbacks remain verified as before, distinct from IRQ-driven
whole-frame scheduling and presentation.

### Source evidence and implementation

Pinned NitroSystem `snd/src/player.c:NNS_SndPlayerSetAllocatableChannel` only
stores a mask for the next start. `NNSi_SndPlayerStartSeq` sends it to all prepared
tracks only when nonzero. NitroSDK `ARM7/snd_seq.c:SND_PrepareSeq` preallocates the
SSEQ track set; `SND_SetTrackAllocatableChannel` sets its explicit-mask flag, and
NoteOn passes that flag as the strong request. Masks do not reserve channels or
stop active notes. NitroSystem `resource_mgr.c` separately refuses overlapping
exclusive channel locks. NitroSDK `snd_exchannel.c` filters hard/weak locks,
orders by priority and encoded volume, drops a victim, then initializes the new
channel. Strong requests bypass weak locks only. `SND_FreeExChannel` is callback
detachment, not a stop. These distinctions are exercised in focused tests.

The existing audio implementation gave each voice an independent 16-channel
array and discarded SDAT player/channel-priority fields. All rendered voices now
use one physical domain in their existing DSAudio owner. The old private
allocation algorithm was removed from Player::ChannelAlloc; a missing shared
owner rejects explicitly. Unchanged original SDK allocator/lock/free/helper
bodies and the NNS setter are compiled as private native methods from the existing
pinned sources. Their native pointer-bearing records never enter ARM32 storage.
The existing DSP remains the renderer; there is no second sound engine or heap.
Generated SDK bodies and provenance remain ignored build products.

The SDAT transport now retains channel priority, player priority and player ID,
reads the existing PLAYER records for initial masks, and preserves encoded zero
volume rather than replacing it with full volume. Sequence 1018 is player 1,
channel priority 30, player priority 64, volume 80; its archive mask is 0xA7FE.
SE1500 uses player 4, channel priority 104 and archive mask 0xD800. The field scene's
player-7 mask request is not an instruction to relabel field BGM as player 7.
Logical sequence-count/handle-priority arbitration is still a separate missing
SDK contract; stored metadata is not evidence that those algorithms execute.

The DSP runs voices against a common sample cursor, so a later voice cannot steal
a channel whose entire victim audio block was already published. Native SDK state
is projected from current DSP priority and volume registers before allocation;
the original drop callback disables a stolen DSP note. Stop, completion,
replacement, clone and preview restoration retain one physical owner per channel.
A separate inspector scans all active notes for duplicate/unregistered ownership
and hard-lock violations. Sample/event phase equality with ARM7 and exact envelope,
volume/mixer/capture output remain unproved.

The loader borrows the caller's existing audio engine; loader deletion does not
stop or free it. Counted leases reject owner misuse, and independent snapshots
make incomplete field-scene requests transactional over the admitted audio state.
Restore preserves root identity and leases and uses a compiled no-throw move,
rebinding native pointers without allocating. Tests repeatedly execute the shared
mask request, stop at capture, and compare the external owner's subsequent PCM to
an independent preimage. No capture operation is silently accepted.

### Remaining coherent groups

1. **Shared sound initialization/scene ownership:** original capture initialization,
   stop/status and any lock/stream callbacks; complete canonical sound work and
   handle/sequence-player ownership; original sound-heap save/load/archive disposal;
   radio/field bank selection, pause/stop and actual start through existing PCM
   transport. Source `SND_PauseSeq(TRUE)` releases/detaches track channels, so the
   preview API that saves an entire music cursor cannot serve as its SDK adapter.
   Full SDK records require valid guest placement/lifetimes beyond the current
   partial sound-work view. This mandatory group still blocks LOAD before
   soundplates and resume scripts. Adding a successful capture-stop stub would
   not supply it.
2. **HBlank/poison lifecycle:** retain the original 0x1C two-slot callback owner,
   hook canonical IRQ/refusal/mask semantics with ARM callback identities, construct
   the 0x328 poison owner and its double-buffer transport together, then cancel
   tasks/IRQ users before freeing buffers. Creation, active callbacks and deletion
   form one capability; original source and existing IRQ services were identified
   in the preceding inventory, not rediscovered as new implementation.
3. **Bottom-screen tasks:** preserve the six original four-function table rows,
   exact bottomScreenType selection, overlay27's retained FieldSystem/task environment,
   and asynchronous destroy/poll before overlay release. The real empty row must
   not be substituted for a selected nonempty row. Source and fixture checks
   establish the opening path: CallFieldTask_NewGame sets mapLoadType=0;
   sub_02053284 selects sMapLoadModes[0], which sets bottomScreenType=1. That
   selects ov01_021F6894/overlay27, not an empty callback. This corrects the earlier
   conflation of mapLoadType with the assembly's field at guest offset 0x18.
   The dummy running predicate
   does not establish visible bottom-screen readiness.
4. **Main/input/render scheduling:** complete mandatory initialization first, then
   observe original Main and Control/task/update ordering. Enabled movement needs
   actual input and original player/object updates, plus the four `unkBC` update
   groups, original draw/geometry submission, queued transfers and publication.
   Five direct VBlank callback invocations are not a complete recurring frame.
   Camera/loader component ticks and preview input remain separate evidence.

These groups reuse the existing graph and its unresolved indirect edges; no new
framework or speculative Nitro heap was added. The next batch is the whole sound
owner/scene contract, rather than isolated rejecting sound leaves.

### Direct scratch regression and verification

The constructor fixture now snapshots the actual guest image at old scratch
address 0x0204B000, invokes real terrain/height/behavior queries, and checks those
bytes and retained land triangles. It verifies that the enlarged image covers the
old address and current scratch starts after the complete image. This fails if
the former fixed terrain scratch is restored. Both sessions execute it under
normal and sanitized builds; the earlier independent assembler overlap guard
remains. Registered guest allocations are unchanged.

Final completed counts and trace evidence are stored in
`build/evidence/sound-channels/checkpoint.json` and the reused
`build/audit/original-field/frontier.json`. No new Lean theorem or whole-machine
proof is claimed. Remaining assumptions include main-thread transaction isolation,
SDK handle/heap/capture lifecycle, original ARM7 command timing and track/sequence
capacity, decoder/mixer/envelope refinement, general allocator/address/overlay
observations, and whole Main/input/render integration. Three bare-EOF light
resources, ROM trig lookup equality, exact Nitro matrix/cache behavior, nonempty
VRAM queues and Metal equivalence remain open. Upstream and prior user work are
preserved.

## Original sound software-owner checkpoint (2026-10-25)

This is a partial sound-scene integration, not a completed audio lifecycle.
Original field initialization now rejects at `NNS_SndArcGetSeqParam(1018)`.
The original scene prefix traverses capture-stop/query, handle query, stopped
sequence/pause handling, radio stop, and Chatot cleanup. Its call path reaches
PlayBGM's player-parameter query; this statement combines the traced SDK import
and its guest LR with the original direct call graph. It is not a trace of every
internal function entry. Root-level original SDK calls are independently traced
by the software-owner test. No successful original StartSeq or field PCM is claimed.

The existing generator compiles pinned original capture, fader, player, resource
manager, sound heap, frame heap, heap-list and common-heap units as ARM32. Original
radio and Chatot source units are also compiled. Private symbols retain their
translation-unit identity. Native SDK pointers are not cast into ARM records.
The SDK's warning-only weak SND_SetTrackMuteEx fallback is excluded: the caller
retains a rejecting device import. Provenance and emitted SDK code remain ignored
build inputs; upstream is unchanged. A shared Thumb SBC operation and bounded
C abs boundary were needed to execute the original routines. SBC flags are tested
against independent signed and unsigned arithmetic, including borrow and overflow.

Binding audio initializes original software player lists, capture state, resource
locks and the nine original handles through original functions. This context is
**loader-owned staged software state**, not the game's persistent InitSoundData
owner. Only handles and metadata of SND_WORK are mapped. The archive, heap pointer
and embedded heap arena remain inaccessible. The caller's DSAudio identity, leases,
preview sequences and notes remain external. Original SDK mask policy is now in
its original ARM records; the former per-import native setter bridge is removed.
The existing DSAudio allocator remains the standalone PCM service. Connecting
these two player domains is still required before admitting SDK driver commands.

Rejected scene requests restore all compiled sound static objects, the partial
sound-work view, original control words, and the external audio snapshot. The
static-object inventory is emitted from compiler object sizes rather than a
manually maintained list. Tests compare the entire original 32-player table and
an explicit active-capture fixture across retries; SND_StopTimer rejects before
an external effect. The fixture exercises rollback and original fade arithmetic,
not an actual reverb backend. Active capture setup, timers, alarms, callbacks,
thread/sleep effects and ARM7 command processing remain rejecting/unverified.

Independent software-owner tests execute:

- Sound-heap construction/destruction, aligned allocation, exhaustion, save/load,
  both small-heap save failures, clear, and repeated reuse of identical guest bytes.
- Sixteen SDK sequence slots, low-priority refusal, handle/sequence-number queries,
  release without stop, free/detachment and reuse. Allocation alone is not playback.
- Capture resource conflicts, original reverb-start failure before device setup,
  capture buffer clearing, all eight alarm identities, release/retry, original
  fader truncation and unsupported active-device rollback.
- Native backing retirement and preservation of the external audio owner, with
  equal native heap usage after teardown.

The callback-backed sound-heap teardown path still requires the real sound command
fence/driver owner. Original heap callbacks can invalidate archive files, banks,
waves and player subheaps; copying a heap frontier or pretending commands finished
cannot replace those effects. Tests of heaps without callbacks do not close that
obligation. Full InitSoundData also initializes microphone, archive PLAYER heaps,
radio heap, initial group/bank loads and saved levels. None is replaced by an
empty initializer or a synthetic level. A file-backed archive and persistent
boot-to-shutdown owner must precede the field scene's successful admission.

The next shared initialization groups remain soundplate, resume scripts, HBlank
and poison tasks, selected bottom-screen overlay27, then Main/input/render phases.
They have not been bypassed. Existing bare-EOF light resources and trig/matrix,
ARM7/mixer, IRQ timing and Metal refinement obligations remain open.

Validation: full build exit 0; final 84/84 CTests exit 0 (49.92 s); focused 7/7;
ASan/UBSan 7/7 exit 0 (6.72 s, leak detection disabled); audit 14/14 exit 0.
The parent trace and dedicated owner trace both finish exit 0. The existing
frontier is refreshed, with separate compiled/executed/unsupported entries in
initialization_checkpoint. Evidence: build/evidence/sound-lifecycle/ (ignored).
No new formal proof was checked. Nintendo SDK/game bytes are generated build
inputs only. The upstream checkout and unrelated user changes are preserved.

## Original ARM9 sound-command checkpoint

The generator now selects original SND_PrepareSeq, track allocation-mask and
player channel-priority producers, PushCommand_impl, allocation/free-list and
reservation operations, counters, and flushing/fence algorithms. Original
ARM32 SNDCommand has 24 bytes (next, ID, four arguments); the pool has 256 nodes.
Original IDs, pointer arithmetic, linked-list order and epoch operations remain
in ARM code. Generated provenance lists the selection and hashes the verbatim
pool-initialization span from SND_CommandInit. That span runs once on a fresh
owner. InitPXI and the shared-work bootstrap are **not** claimed to have run;
there is no ARM7-ready indication or synthetic bootstrap acknowledgment.

The native OS_IsRunOnEmulator boundary selects the retail DS path rather than
an Ensata development register. Existing IRQ/cache services retain their roles.
PXI_SendWordByFifo rejects explicitly without returning a negative FIFO result
that blocking original SDK loops would retry forever. SND_RecvCommandReply remains
an unsupported import. No native command queue or ARM7 consumer was added.

The state distinction is observable in tests:

- Allocated nodes enter the original reserved list in producer order.
- Flushing attempts PXI submission; transport rejection leaves the list reserved.
- The SDK waiting-list state, completion epoch and acknowledgment state do not
  advance. No node returns to the free list through a fictional reply.
- Both a future-epoch wait and a blocking exhausted-pool allocation reject at a
  named boundary. Nonblocking exhaustion returns NULL through original code.

The fixture executes original sub_020051A4 bank/wave loading and
NNS_SndArcPlayerStartSeqEx for sequence 1018. The first command is PREPARE_SEQ:
player=0, sequence base=0x0212007C, offset=0, bank=0x02191E60. These literals appear
only in tests. The sequence lives in the original player heap, not FAT.mem: tests
compare its complete SSEQ bytes with a POSIX read of the original SDAT file range,
verify ownership through NNS_FndFindContainHeap, and validate its baseOffset. The
bank pointer resolves through the original SDAT record and FAT lookup. The next
commands are ALLOCATABLE_CHANNEL and PLAYER_PARAM, with original player policy.
The original flush reaches a bounded PXI rejection with this exact live payload.
This is a **submission attempt**, not submission acceptance, execution, PCM or
acknowledgment. Raw queue tests preserve these reserved nodes across retries.

The existing sound snapshot is reused as one transaction helper. Its preimage
contains the entire SND_WORK (including native-backed guest heaps), every generated
sound static object (including the pool, queue heads and epochs), control words,
file cursor records, external audio and IRQ enable state. On failed admission these
restore and caller output stays untouched. A staged SDK request that returns with
queued commands must attempt original flushing before admission commits. This
additional admission step does not claim a completed original sound frame.
Failure at an earlier original leaf also restores the queue. The native mutex
unwind remains intact. Tests repeat both cases and compare heap/pool bytes and
external PCM. The rejection payload retained under test hooks is diagnostic
history, not live driver state or a second queue.

This relation requires exclusive synchronous admission and no already accepted
external effects. A future actual PXI consumer must specify submitted-list
ownership, asynchronous acknowledgment and rollback/commit boundaries before any
accepted submission is allowed. Software snapshots cannot retract ARM7 execution.
No formal ARM7, DSP, timing or PCM refinement proof is added here.

The actual field request passes SND_PrepareSeq and rejects at sub_020378CC.
The earlier SND_PrepareSeq rejection expectation changed only after this traced
advance. Active-device SND_StopTimer still rejects, and its original capture-state
rollback checks remain. No HBlank, input, Main, or additional driver operation was
implemented. The bare-EOF lights and trig/matrix/Metal obligations remain open.

Validation completed: full build exit 0; 86/86 CTests exit 0 (57.04 s), focused
4/4 exit 0, ASan/UBSan 4/4 exit 0 (4.97 s; LeakSanitizer disabled), audit 14/14
exit 0. Both final traced fixtures completed successfully. Evidence is ignored
under build/evidence/sound-command/. These are execution/property tests and ARM32
layout assertions, not a new machine-checked equivalence theorem. Upstream and
unrelated user changes are preserved.


## Communication-query admission checkpoint

`asm/unk_02035900.s:sub_020378CC` calls the mode getter `sub_0203993C`,
tests the mode using `sub_02034044`, and conditionally returns
`ov00_021E7080()`. The selected modes are 19–25, 29, and 33–36. Other modes
return zero without touching overlay state. This is communication-state software:
it has no MMIO, interrupt, allocation, callback or queue side effects.

The original C getter returns `_021D4150->unk4E`, or mode 1 when absent.
`sub_02037C98` owns allocation/initialization of that communication context;
`sub_02037D2C` frees it and clears the pointer. Communication setup paths select
the mode (including 19, 23, 29, 33–36). The classifier is also used by
`sub_02039998`; the getter has independent callers in Main and battle setup.
Overlay-00's `ov00_021E5900` allocates its separate aligned owner; the admitted
original `ov00_021E7080` reads its word at offset 0x109C, returning zero if absent.
`ov00_021E5AE8` destroys its internal heaps, frees its backing and clears that
global (then clears the DWC error state).
Both globals retain their original null initialization. Their active constructors,
network services and destruction are outside this admission and remain rejecting
imports; populated-owner tests are ABI fixtures, not working communication sessions.

The loader generator assembles the three original Thumb bodies unchanged and
compiles the unchanged original C getter with the original owner declaration.
The SDK random-context member needs explicit four-byte alignment to retain the
original ARM layout under Clang AAPCS; a compiled assertion pins the mode to 0x4E.
Source hashes and exports remain in the existing build-local provenance files.
No native success adapter, sound-driver change or new device service was added.

Sound callers `sub_02005464` and `sub_02005774` use this query to divide volume
by five when the returned flag equals one. The real new-game field constructor
now passes that query and reaches the existing original `SND_FlushCommand`
through sound admission. PXI explicitly rejects before accepting any command;
this is the next mandatory boundary. No ARM7 consumption, acknowledgment,
completion, recycling or original field PCM is demonstrated. Later HBlank,
bottom-screen and Main dependencies remain blocked behind that admission.

Focused tests enumerate all 256 mode values independently, exercise absent and
populated overlay owners with flags 0, 1 and 0xDEADBEEF, preserve owner bytes,
check unsigned out-of-range classification, and verify conditional invalid-pointer
access and IRQ preservation. Borrowed fixture backing is retired after clearing
both globals. Real parent construction repeats twice; sound-scene rollback,
active capture rejection, 256-command ownership, exact live sequence/bank addresses,
caller PCM and failed-admission retry retain their assertions. The sound-owner
fixture now supplies the real field BGM selection and field-load guard so its
negative assertion requires an actual pending playback request.

Evidence lives under ignored `build/evidence/communication-query/`; the existing
`build/audit/original-field/frontier.json` is refreshed rather than rediscovered.
These are executed tests and compiled layout checks, not a machine-checked
compiler/ISA or communication-lifecycle equivalence proof. Active communication
owner lifetime/overlay residency, ARM7/PXI protocol and completion, full sound
startup, device timing, the three bare-EOF light resources, ROM trig/matrix equality
and Metal refinement remain unresolved. FieldMap_Init and FieldMap_Main have not
completed/executed respectively; no interactive player frame is claimed.

Final verification: complete build exited 0; **86/86 CTests** (52.58 s),
**4/4 focused tests**, **4/4 ASan/UBSan checks** (5.29 s; LeakSanitizer disabled),
and **14/14 audit tests** passed. Final parent and command trace runs exited 0;
upstream remained clean.


## Original ARM7 command-service checkpoint

The existing generator now compiles unchanged SDK `snd_command.c` for both CPUs,
ARM7 `snd_seq.c` and `snd_exchannel.c`, and original work/util/data/bank/main/message units.
ARM7 sound symbols, including unresolved calls, receive a separate namespace so
an ARM7 operation cannot accidentally invoke an ARM9 command producer. Targets
are ARM7TDMI Thumb and ARM946E-S Thumb respectively. Compiler object inventory
now includes ARM7 driver RAM, message queues, sequence cache, private statics and
both shared-work pointers. Function-local static normalization was corrected so
none of those objects escapes rollback. SDK bytes stay build-local and ignored.

The native interface registers the two original sound callbacks and replaces the
physical PXI sound FIFO with delivery to the original ARM7 callback. Original
`OS_SendMessage`/`OS_ReceiveMessage` retain the eight-message ring and its ordering.
A data send enqueues a list; it does not execute it or advance completion. Driver
commands run at explicit admission barriers, process requests and pending-command
polls. Admission checks original blocking-flush success before dispatch/retirement;
a test with eight pending lists and a ninth reserved list verifies that none is
left unexecuted at commit. Empty thread-queue wakes have their original empty effect; actual waiters
and other unsupported OS operations reject. A future completion wait with no
pending work rejects rather than spinning or fabricating an epoch. ARM7 OS IRQ
state is independent of the existing ARM9 scheduler IRQ and restored on failure.

Initialization now executes original ARM7 ExChannel/Seq/Command initialization,
then the complete original ARM9 `SND_CommandInit`. The latter initializes shared
work, creates SHARED_WORK (29) and sends it. Original ARM7 `SND_CommandProc` binds
the shared pointer and increments `finishCommandTag` after the whole list; original
ARM9 `SND_RecvCommandReply` observes that counter and returns the list to its pool.
This replaces the former copied free-list initialization span. No native queue,
tag increment, free-list algorithm or successful sound-device substitute was added.

Live sequence 1018 executes original PREPARE_SEQ (2), ALLOCATABLE_CHANNEL (9) and
PLAYER_PARAM (6) in order. Tests independently locate its full original SSEQ bytes
in the live arena, resolve its bank through original archive/FAT algorithms, and
compare all PREPARE arguments: player 0, live sequence base, offset 0, live bank.
Current fixture addresses are 0x021200DC and 0x02191EC0, reflecting the generated
import layout; production contains neither address. Existing allocations are not
remapped. Preparation performs active-player teardown, original track allocation,
default initialization, retained cursors, initial MML ALLOC_TRACK parsing and
shared variable/status updates. Its original active bit is set and prepared bit
is clear; START_PREPARED_SEQ (3), verified separately, sets the prepared bit.
Tests prove neither PCM output nor periodic timing equivalence by these flags.

Independent tests cover submission before execution, completion before ARM9
recycling, exact masks/priority order, all 256 nodes, blocking allocation with
real processing, eight outstanding lists and ninth-list backpressure, original
track exhaustion/free/reuse, repeated active-player preparation, shared-work
visibility and logical source retirement. Hardware MASTER_VOLUME (23) is injected
after PREPARE and mask execution: its unimplemented ARM7 import rejects, restoring
the entire guest image's static state, complete SND_WORK/heap bytes, both IRQ
states, queues/tags/handles, caller output and external native PCM on three retries.
The existing active capture `SND_StopTimer` rejection tests remain intact.

A periodic original parser probe uncovered Thumb high-register MOV-to-PC. The
shared interpreter now implements its Thumb-preserving, low-bit-masking branch;
independent even/odd target, pipelined PC, return and condition-flag tests execute.
The original bank unit and original ARM7 mutex bodies now execute instrument
lookup and note creation; the live fixture produces an original software voice
whose sample pointer resolves inside the persistent sound arena. Original
`SND_ExChannelMain` computes its envelope/pitch/timer state. The shared compiler
runtime reuses the existing unsigned division and 64-bit multiply primitives
under their EABI names, and adds bounded unsigned 64-bit right shift. Independent
limb/carry/wrap/shift vectors and original octave/clamp timer checks pass.
Actual hardware publication rejects at **arm7_SND_StopChannel**, with full state
restoration on three retries. Full original `SND_Init` also executes its software
prefix but rejects at `OS_CreateThread`; its initialization guard and reset queues
restore on each retry. No native thread/device success stub was added.

Both real new-game parent runs now reach `FieldSystem_ProcessSoundplate`, with
shared player status 1, all 256 command nodes free, no waiting nodes and balanced
IRQs. The field request genuinely completes software command admission. Full
Init, Main and interactive input/rendering still do not execute.

This is a staged command service, not complete ARM7 firmware or the original
sound-thread loop. Hardware enable/output setup, alarms/capture/channels, periodic
interrupt scheduling and PCM transport remain unsupported. Instrument lookup,
note creation and channel parameter algorithms execute as component probes;
a complete scheduled driver iteration has not executed.
There is no claim that the native DSAudio preview renderer refines that driver.
Unimplemented ARM7 imports/device operations reject; unknown command IDs and a
foreign SHARED_WORK owner reject before delivery. The synchronous transaction
requires exclusive admission and no external accepted device effects. Full DS
thread/interrupt/timing equivalence, compiler/ISA refinement beyond tested paths,
three bare-EOF lights, exact trig/matrix equivalence and Metal refinement remain
unproved. No new Lean proof is claimed.

Conservatively counting all added handwritten production lines, including
comments, blank lines, generator changes and test hooks: **181 added, 47 removed**
across five production files. Reused SDK sources, generated bytes, tests and
reports are excluded. No new production function is compressed onto one line.
Final evidence is under ignored `build/evidence/arm7-sound/`; the existing frontier
inventory is refreshed in place.

Verification completed: build exit 0; **86/86 CTests** (50.64 s), **5/5 focused
checks**, **5/5 ASan/UBSan checks** (5.18 s; LeakSanitizer disabled), and **15/15 audit
tests**. Final parent and command trace runs exit 0. Upstream remains unchanged.


Files changed in this checkpoint:

- Production: `scripts/prepare_original_sound.py`, `scripts/guest_assembly.py`,
  `src/compat/original_loader.c`, `src/compat/original_loader.h`, `src/ds_thumb.c`.
- Tests: `tests/original_sound_command_tests.inc`,
  `tests/original_sound_owner_tests.inc`, `tests/test_original_field_constructor.c`,
  `tests/test_thumb.c`, `tests/test_original_field_audit.py`.
- Reports/contracts: `docs/current-work.md`, `docs/field-init-frontier.md`,
  `docs/memory-contract.md`.
- Ignored evidence: refreshed `build/audit/original-field/frontier.json` and
  `build/evidence/arm7-sound/`, including the conservative production diff/line count.
