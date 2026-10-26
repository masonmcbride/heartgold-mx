# Current connected outcome
- Goal: original FieldMap_Init LOAD completion, actual Main/controller frames and original Exit.
- Canonical new-game owner, save, 64-object manager/player/follower and original tasks remain preserved.
- RESET completes; LOAD returns all 23 original effect constructors (mask 0x007fffff).
- Renderer 14 unchanged C executes as ARM32; four 20-byte holders and four 84-byte embedded render objects retain real models.
- Existing model/texture/animation/raster services supply native associations; upstream remains unchanged.
- Original VBlank IRQ drains capacity-32 queue during LOAD; main/VWait phases remain separate.
- Original object graphics manager construction/destruction now executes on directly mapped canonical opaque state.
- Original sprite tables and 17 JSON-generated mmodel control members restore missing data dependencies.
- Separate original raw-manager owners share existing transport with checked distinct manager/object handles.
- ov01_022057DC, FieldEffect_InitRenderObject and original PlayerAvatar_InitMapFeatures now execute.
- Shared-root audit: A canonical ABI/callbacks, B environment owners, C graphics state, D frame lifecycle, E script/input.
- Priority A typed accessors now initialize original ov01_02200900 with canonical player identity; next owner is sub_0205E520.
- Repeated constructor/callback/identity rejection probe passes (0.242 s); character/control-table test passes (0.133 s).
- Preserved loader: 59 models, eight props, 260 land triangles and four resource ticks per repeated session.
- LOAD partial, runningFieldMap false; paused Control only; zero Main/Exit or enabled-input frames.
- No full suite, sanitizers, UI smoke or unrelated benchmark workloads run.


## Batched field-initialization frontier checkpoint

The current-tree baseline already passed object activation and rejected at
`ov01_021E6580`; older next-blocker notes above are superseded by executed
inspection. The original parent probe now executes display enabling, ARM model
attributes/default configuration and fog allocation plus original save camera
type reads, then explicitly rejects at `FieldCamera_Create`. Model/fog original
component frees, repeated lifetimes and guest-view invalidation are tested.
LOAD remains partial; Main, enabled input, whole original Exit and a complete
visible field frame remain unexecuted.

[The frontier report](field-init-frontier.md) describes the conservative
machine-readable initialization/callback/teardown graph, current compiler and
generation cross-references, runtime trace distinctions and capability batches.
The next coherent batch is retained camera target/history/projection and its
teardown, followed by environment resources and asynchronous lifecycle owners.

Final checkpoint validation: complete build, 82/82 CTests, 3/3 targeted ASan/UBSan
checks and 11/11 audit Python tests passed. LeakSanitizer was disabled; no UI smoke
or rendered-frame equivalence claim is made.


## Camera and area-light integration checkpoint

The original field-camera constructor, retained target/history, active preset,
per-frame component update and destruction now execute through the shared ARM32
runtime. The existing raster camera consumes original view/projection arguments.
Parent field pointer bytes survive component destruction as original code leaves
them; lifetime references reject retired targets and light attributes. Rollback
preserves external owners and the persistent object-graphics VBlank task.

Original area-light construction/update/free succeeds for the two terminated
resources. Three bare-EOF resources expose a read beyond the requested allocation;
they explicitly reject and roll back pending evidence about original physical
RAM/heap backing. No synthetic terminator or speculative allocator was added.
The opening parent now reaches original weather construction and rejects at
`GF_InitG2dRenderer`; its diagnostic prelude rolls back its own allocations.
The next coherent capability is ARM32 2D renderer/sprite-resource/OAM ownership
and task teardown. Weather itself remains unsupported.

See [the technical frontier report](field-init-frontier.md) and ignored
`build/audit/original-field/frontier.json` for refreshed static/runtime evidence
and remaining obligations. LOAD is partial; FieldMap_Main, enabled input, whole
original FieldMap Exit and a visible field frame remain unexecuted. Component
owner replacement is tested; a real map warp is not.

Final validation processes exited 0: complete build and 82/82 CTests (38.40 s),
3/3 targeted ASan/UBSan checks (3.96 s), and 12/12 audit tests. LeakSanitizer was
disabled. Exact ROM trigonometric-table equality, Nitro matrix rounding/cache
layout, bare-EOF physical-memory behavior and CPU/device/Metal refinement remain
unproved. Upstream was not edited.


## Weather/Window/message and texture-descriptor checkpoint (2026-10-25)

Original parent `FieldMap_Init` now executes weather construction, original
map-name Window/String/lazy message owners, inactive signpost initialization,
texture catalogue loading and resource-name lookup, saved-weather selection,
the no-gymmick branch and field VBlank callback publication. It rejects at
`FieldBGM_PlayEffectiveForMapHeader`. **FieldMap_Main has not been reached.**

Original SDK renderer/surface/ARM32 sprite owners and full OAM algorithms execute
through the existing Thumb runtime. Checked data-only MMIO and bounded OAM copy
leaves reuse canonical DSVideo storage. Opaque empty OAM emulation was removed.
Original field Windows own real pixel buffers; encoded strings and encrypted
message banks are distinct from preview Window/UTF-8 services. Upstream msgenc
regenerates all 829 banks; original lazy decryption of every bank 279 message is
checked. The opening map matches zero of nine texture-animation descriptors;
occupied animation uploads/teardown are not covered.

Owner-capture rollback tracks tasks/archive leases alongside heap acquisitions,
preserving external owners and persistent VBlank tasks. Tests cover construction
interruption, retries, pointer invalidation, OAM entry/affine publication/reset,
original partial reservation failure and all 16 window fill colors. The MMIO
intermediate-pointer UBSan defect is fixed and its sanitizer check exited 0.

Completed final checkpoint: full build and 82/82 CTests (38.53 s), 4/4 targeted
ASan/UBSan tests (5.40 s), 12/12 audit tests and final constructor trace exited 0.
LeakSanitizer was disabled. See [the updated frontier report](field-init-frontier.md)
and ignored `build/audit/original-field/frontier.json` for evidence and obligations.
Native SDK/source tools and generated resources remain ignored; upstream and
unrelated user work are preserved.

Remaining boundaries include original BGM/soundplate sequence ownership,
HBlank/bottom-screen lifecycle, recurring field VBlank BG/VRAM/object flush,
active sprites/weather and map-name drawing, whole Main/Exit, heap/address reuse
and compiler/instruction/device equivalence. Three bare-EOF light resources still
reject; no backing bytes were fabricated. ROM trig-table equality, Nitro matrix
rounding/cache semantics and Metal presentation equivalence remain unproved.

## BGM policy / recurring publication checkpoint (2026-10-25)

Partial integration: original `FieldMap_Init` now executes original native BGM
selection and the original ARM scene prefix, rejecting at
`NNS_SndPlayerSetAllocatableChannel(7, 0xA7FE)`. **BGM playback and FieldMap_Main
are not reached.** The existing sequence player does not yet implement original
SDK channel reservation, capture, handles, sound-heap/archive lifetime or scene
transitions. No success/no-op sound adapter was added. Private sound-policy
metadata restores on rejection so retry does not suppress playback as “scene
already active”; it is not a persistent shared audio owner.

The actual original native `FieldMap_VBlankCallback` completes five explicitly
invoked publications per session, in two reconstructed sessions: scheduled BG3
scroll/tilemap bytes reach canonical DSVideo, empty VRAM transfer handling and
original OAM apply/reset execute, and the original object-pool state is
acknowledged. Persistent graphics VBlank tasks survive, and retired pool views
reject. These are callback tests, not recurring IRQ-driven interactive frames.
Original Main, player input/movement, active scene rendering and Metal
publication remain distinct unverified milestones.

An enlarged guest image exposed a real fixed-scratch/model overlap; temporary
terrain/area projections now follow the aligned image with separate stack space.
Image/import overlap rejects during assembly and RAM/import overlap at compile
time. Registered guest allocations and native heap budgets were not remapped or
relaxed. Sound ARM32 layout assertions compile; no new Lean proof was checked.

Final full build and 82/82 CTests completed (exit 0, 52.17 s); targeted ASan/UBSan
4/4 completed (exit 0, 5.45 s, LeakSanitizer disabled); audit checks 13/13 and final
trace exited 0. The existing frontier graph is reused and refreshed, with
`initialization_checkpoint` recording these separate milestones and remaining
capability groups. See [the report](field-init-frontier.md) and ignored
`build/evidence/field-bgm/`. The three bare-EOF light resources, ROM trig equality,
Nitro matrix/cache equivalence and complete device/presentation refinement remain
open. Upstream and unrelated user changes are preserved.

## Shared sound-channel checkpoint (2026-10-25)

Original `FieldMap_Init` now executes the shared NNS mask service and rejects at
`NNS_SndCaptureStopReverb`. Field-requested BGM playback, complete Init, Main entry,
and an interactive frame are **all still unverified/unreached**. Standalone
original SDAT music produces nonzero PCM; that fixture is not original PlayBGM.

The existing audio owner now has one physical 16-channel domain across rendered
voices. Original SDK allocation, priority/volume selection, locking, free/drop and
initialization helper bodies execute natively through generated private records;
the old per-voice allocator was removed. The original NNS setter changes future
player policy, not reservations or active tracks. SDAT player IDs, priorities,
zero volume and archive player masks are retained. Weak locks, strong requests,
conflicts, release/retry, clones and real PCM are checked. Native SDK records are
never cast into ARM memory; full SDK handles/sequence arbitration remain missing.

The loader leases the caller's audio engine and snapshots admitted sound state on
an incomplete scene request. Rejection restores masks/locks/notes/cursors/PCM;
loader deletion preserves the external owner. Tests cover repeated field rejection,
invalid snapshots/binding, unrelated import fallback, actual teardown and two
loader sessions. A direct terrain-query regression protects image bytes at the
old scratch address 0x0204B000 and retains the land-model checks.

The remaining groups are capture/handle/heap/archive sound lifecycle, HBlank and
poison buffers/tasks, bottom-screen overlay/task lifecycle, then Main/input/render
scheduling. The opening fixture verifies mapLoadType=0 → bottomScreenType=1 →
overlay27; the table indexes bottomScreenType, not mapLoadType. Empty callback rows
cannot replace that selected owner. See [the report](field-init-frontier.md).

Full build and 83/83 CTests completed (exit 0, 50.33 s). After the final selector
assertion, the traced constructor and 6/6 targeted ASan/UBSan checks completed
(exit 0, 6.20 s, leak detection disabled). Audit checks pass 13/13. The existing
frontier graph is refreshed; evidence is ignored under build/evidence/sound-channels.
No new Lean theorem, ARM7/audio-output equivalence or Metal refinement is claimed.
The bare-EOF lights, trig/matrix and other existing obligations remain open.
Upstream and unrelated user work are preserved.

## Original sound software owners (2026-10-25)

Partial integration: original FieldMap_Init now rejects at
NNS_SndArcGetSeqParam(1018). Original capture checks, handle/stop/pause logic,
radio stop and Chatot cleanup run through original ARM code. Capture/player/heap,
frame-heap/list/resource-manager software units are compiled unchanged, with a
rejecting device boundary and no speculative allocator. Original handle bytes are
mapped; the archive and embedded heap remain absent. Rejected scenes restore all
compiled sound statics and external audio. The former native NNS-mask import
adapter is removed; policy now runs in original SDK records.

A dedicated software-owner test covers heap exhaustion/save failures/reuse,
16 slots and handle detach/free/retry, capture conflicts/alarm reuse/fader math,
active-device rejection rollback, stale backing and external-owner survival.
Thumb SBC and bounded abs are shared execution primitives with independent tests.
This does not complete the requested sound-scene lifecycle: persistent
InitSoundData/archive/player heaps, callback-backed disposal fences, actual
StartSeq/PCM transport and active capture remain open. Original field BGM PCM,
Init completion, Main execution and an interactive frame remain four separate
unverified milestones. See the latest [frontier report](field-init-frontier.md).

Validation completed: full build exit 0; final full regression 84/84 (49.92 s),
focused checks 7/7, ASan/UBSan 7/7 (6.72 s; leak detection disabled), audit checks
14/14. Both parent and independent owner traces exit 0. The existing conservative
frontier is refreshed; ignored evidence is under build/evidence/sound-lifecycle/.
No new machine-checked proof or original field PCM claim. Upstream stays clean.

## Original ARM9 sound-command milestone

Original ARM9 PrepareSeq, allocation/reservation, mask/priority producers and
flushing execute with the 256-entry ARM32 SDK pool. The live sequence1018 fixture
checks PREPARE_SEQ(0, 0x0212007C, 0, 0x02191E60), original sequence-heap ownership,
SDAT bytes and bank publication. PXI submission attempts reject before acceptance;
reply waits and pool exhaustion are bounded, with no acknowledgment or recycling.
Pool initialization uses a verbatim software span from SND_CommandInit, not a fake
ARM7 handshake or completed shared-work bootstrap. Sound transactions restore
heap/static queue state, file cursors, IRQ state, output and external PCM on failure.

Original field initialization advances to sub_020378CC. Active capture rejection
and rollback remain covered. Field BGM PCM, Init completion, Main and an interactive
frame remain unverified. No PCM transport, HBlank or unrelated driver work was
added. See the latest [frontier report](field-init-frontier.md).

Validation: completed full build; 86/86 CTests (57.04 s), focused 4/4,
ASan/UBSan 4/4 (4.97 s; leak detection disabled), and audit 14/14; all exit 0.
The field and command traces also exit 0. Evidence: build/evidence/sound-command/.
No new machine-checked proof or ARM7/PCM completion claim. Upstream stays clean.


## Communication-query frontier checkpoint

The real repeated parent initialization now passes original `sub_020378CC` and
stops at `PXI_SendWordByFifo: ARM7 sound consumer unavailable` during transactional
sound flushing. The function is a communication-mode/overlay-flag query: three
unchanged Thumb routines and the original ARM32 C getter now execute. No native
sound or networking substitute was introduced. All 256 modes, flag values,
conditional invalid backing and owner retirement are tested alongside preserved
sound queue, IRQ, active-device rejection, retry and external PCM assertions.

See [the frontier report](field-init-frontier.md) and ignored
`build/evidence/communication-query/` for source ownership, execution evidence,
validation and unresolved obligations. Further parent progress requires real
sound-command consumption/acknowledgment. Original field playback, Init completion,
Main execution and an interactive frame remain separate unachieved milestones.

Final verification: complete build exited 0; **86/86 CTests** (52.58 s),
**4/4 focused tests**, **4/4 ASan/UBSan checks** (5.29 s; LeakSanitizer disabled),
and **14/14 audit tests** passed. Final parent and command trace runs exited 0;
upstream remained clean.


## Original ARM7 sound-command consumer checkpoint

The actual parent frontier is now `FieldSystem_ProcessSoundplate: canonical
parent contract`. Both repeated new-game runs completed original ARM7 BGM
preparation, published shared completion and recycled all 256 ARM9 nodes.
The original dispatcher, sequence/track algorithms, software driver initialization
and OS message queue execute through the existing Thumb runtime. Native PXI
transport preserves queued versus executed versus recycled state, bounded capacity
and independent CPU IRQ state; it adds no native sound algorithm or PCM playback.

Tests separately execute SHARED_WORK, PREPARE_SEQ, ALLOCATABLE_CHANNEL,
PLAYER_PARAM and START_PREPARED_SEQ. Hardware publication and complete scheduled audio remain
explicitly unsupported: original periodic parsing, instrument lookup, note
creation and channel parameters execute in component probes, but hardware
publication reaches `arm7_SND_StopChannel` and rolls back. Full RTOS driver startup
rejects `OS_CreateThread` and restores its init guard/state on retry. The existing native SSEQ renderer and external audio owners remain separate.
Full FieldMap_Init, FieldMap_Main, original audible field PCM and an interactive
frame remain unverified. See [the frontier report](field-init-frontier.md).

Final build and trace exits are 0; 86/86 regressions, 5/5 focused checks, 5/5
ASan/UBSan checks (LeakSanitizer disabled), and 15/15 audit tests passed.
Production changes add 181 handwritten lines, conservatively including generator
code, comments, blanks and test hooks, below the 199-line limit.
