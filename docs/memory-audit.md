# Memory correspondence audit — 2026-10-24

The **v4 rollback and observer follow-up** at the end records the current work.
The v2 snapshot below records the preceding refactor. The **v3 lifetime follow-up**
at the end supersedes its permanent-fixed-tombstone, spare-slot shrink and
unobserved-caller-lifetime policies. Dynamic placement remains unproved.

## Evidence and scope

Inspected the actual dirty tree before editing. AGENTS.md and CMakeLists.txt
already had tracked changes; original_loader.c, ds_mapping.h, mapping tests,
memory-contract.md and many related files were untracked existing work. They
were preserved. No `extend_heartgold_memory_contracts_v2.py` was found by the
repository file inventory. The starting header/contract were v1; script execution
history cannot be inferred from an absent script.

History: ccf4707 introduced DSNativeRegion lookup ahead of RAM in ds_memory.h;
956a79d introduced RAM translation. The untracked loader and mapping header have
no tracked Git history to audit. Read current-work.md, johto-playthrough.md,
original-engine-ownership-plan.md and the initial memory contract.
compatibility-semantics.md was absent. FieldMap LOAD remains partial; Main,
enabled-input and actual FieldMap Exit are not advanced by this work.

Search scope: repository src and tests (including .inc), not generated resource
bytes or proprietary assets. Reproducible inventories:

```
rg -n 'DSNativeRegion|region_count|borrowed_regions' src tests
rg -n 'ds_translate|ds_mapping_|hg_original_map_native|hg_original_object_work' src tests
rg -n 'l->next|Heap_Free|Heap_Destroy|Heap_Realloc' src/compat/original_loader.c src/compat/original_raw_resources.inc
rg -n 'guest|uintptr_t|DSMainRAM' src tests
```

All direct region-table mutations found by the first search are grouped below.
Pointer conversions are likewise grouped by owner. Indirect callback lifetime
closure and every native cast in the entire engine are not claimed proved.

## Region and conversion inventory

| Owner / sites | Classification and lifetime / identity |
| --- | --- |
| ds_memory.h `ds_translate` | Explicit views in table order, then guest RAM. No copying. Positive access bounded within one span. |
| ds_mapping.h | Borrow registration, inverse conversion, dynamic reservation, view retirement and backing retirement. Borrowed metadata does not transfer ownership. |
| original_loader resource context | Heap-owned guest code/BSS/stack RAM until context deletion. RAM guest base 0x02000000. Dynamic arena starts 0x02800000, ends before 0x03000000. |
| Loader `Heap_Alloc`, `Heap_AllocAtEnd`, archive reads and GfGfxLoader results | Native-owned heap allocations directly exposed as bytes. region_heaps records owner. Nonmoving shrink narrows accessibility. Guest heap placement is a compatibility policy, not a proof of original arena placement semantics. |
| Loader `NNS_G3dAllocAnmObj` | Native-owned 12-byte backing with mapped frame [0,4) and borrowed resource [8,12); +4 remains unsupported. Typed SDK sidecar owns other state. These are disjoint views, not a complete ARM32 SDK layout. |
| original_raw_resources.inc `raw_map_prefix` | Separately owned copied prefix from original raw manager, new guest identity, archive metadata shared with immutable backing. Not an alias of full live payload bytes. |
| Loader effect member read/native free | Caller allocation borrowed for original resource read, then explicitly freed by native owner path. Existing map reused for interior/exact identities. |
| Loader effects_new | Direct borrowed fixed mappings of field_manager_identity and field_effect_identity scalars at 0x0310003c / 0x03100044. Native field itself is a synthetic identity resolved by accessors. |
| Loader objects_graphics_new | Direct fixed borrowed manager opaque ABI storage at 0x03102000 and caller ids at 0x03103000. Owner must keep both alive; free does not currently retire these fixed views. |
| Loader object_work / facing_vector / pending flag | Checked fixed borrowed pointer-free views; object slot indexing determines address. Lifetime follows canonical native object/field. No general slot generation policy yet. |
| Loader map_track_target | Dynamic borrow, original ARM call, verifies original pointer field and copied vector. Target must outlive guest references; no external release API yet. |
| Loader default_free_functions | Two borrowed native scalar function-pointer words at original imported global guest addresses inside RAM. Intentional RAM overrides; RAM bytes at those locations are not authoritative backing. |
| Loader 0x04000060 | Device/MMIO shadow in native video state, excluded from ownership free. Ordinary byte lookup alone cannot establish device side effects. |
| Loader local terrain installer | Synchronous borrowed immutable code image (also RAM identity) plus local stack region; tables die with the call. Duplicate RAM view agrees on bytes. |
| original_transition_bridge.inc | Synchronous RAM projection plus borrowed Location and pointer-free player record. SAVE/FLAGS/ROAMERS/LOCAL are checked synthetic tokens, not mapped native structs. |
| tests/test_original_field_constructor.c | Synchronous object work/follower-tail borrowed regions and task-state word; explicit exclusions of native pointers. Loader callbacks resolve canonical owners through typed accessors. |
| tests/test_platform.c / test_ds_mapping.c | Local owned arrays with borrowed views; mapping tests also construct explicit coherent aliases. |
| tests/test_thumb.c | Guest RAM fixtures; no registered native owners. |
| motion_guest.c | Separate per-call guest RAM and explicit scalar/record projections. FIELD/MANAGER/OBJECT/SAVE/TASK/CAMERA/EFFECT handles are synthetic tokens, not host addresses. |
| compat/healing.c | Per-call guest RAM plus checked TASK/PROPS/SAVE/PARTY/MONS/MODEL/SURFACE tokens. Explicit writes into projected records. |
| compat/save_records.c | Persistent scratch guest RAM, bounded record/heap/stack partitions, copy-in/copy-out records; 0x03000000 identifies active save owner. Not zero-copy correspondence. |
| compat/trainer_ai_host.inc | Per-battle RAM and generated field projections with size checks; party/mon tokens resolve native owners. Native structs are not globally mapped. |
| ds_thumb.c | Central read/write fetches call ds_translate; width/alignment/code-write checks remain CPU obligations. |
| ds_scene_graphics.c / ds_video.h | Graphics resource offsets and device storage, not independent DSNativeRegion registries. Their resource/view relations need separate graphics proofs. |

Loader native-to-guest conversions outside the registry include model and
animation record offsets within registered archive payloads, texture/header
associations, raw_guest_address's exact-base lookup, and array-index tokens for
NARC/props/tasks/raw objects/SDK keys. These remain distinct: resource byte offsets
preserve a backing span, while tokens are validated by service-specific tables.
They must not be collapsed into pointer casts. No host-pointer truncation to a
32-bit guest pointer was found in the inspected correspondence paths.

## Findings and disposition

**Confirmed implementation defects repaired:**

- Dynamic borrowing attempted only q and failed on an otherwise avoidable live
  or retired collision. Ordinary heap, archive, SDK and prefix allocation paths
  bypassed the mapping collision/limit checks altogether. One widened, aligned
  reservation primitive now skips RAM and registered extents for all dynamic
  loader allocation paths. Failed borrow registration rolls the cursor back.
- Free paths erased extents (`size=0`), allowing fixed bindings into retired
  interiors. They now preserve extents as tombstones. Legacy zero-length
  identities reserve one byte against overlapping new bindings.
- Heap destruction invalidated mappings by heap tag only; borrowed interior
  views need not carry that tag. Retirement now asks the native heap for the
  full allocation span and invalidates all intersecting views before release.
  This also supplies the common primitive for the split SDK animation mapping.
- Nonmoving Heap_Realloc narrowed a view without reserving its discarded tail
  or retiring aliases into that tail. The shared shrink primitive now records a
  tail tombstone and invalidates intersecting aliases. Equal-size requests are
  idempotent; growth, borrowed-view shrink and table exhaustion reject before
  mutation. Upstream heap.c and native_heap.c establish the nonmoving behavior.
- Translation could accept an explicit span crossing 2^32 given malformed
  metadata and could perform null RAM pointer arithmetic. Those requests now
  fail. Bind/inverse reject wrapping host spans.

The collision and stale-interior defects follow directly from source and are
covered by decisive tests; no gameplay collision occurrence is asserted.

**Unproved behavior / remaining boundaries, not silently generalized:**

- Direct owned registration still initializes loader table entries after checked
  reservation. Native malloc uniqueness and existing heap accounting supply host
  disjointness; there is no independent C lifetime/provenance proof. Registry
  capacity is finite (1024), and tombstones deliberately consume entries.
- Fixed field/manager/ids views still bypass checked bind. Constructor failure
  and repeated lifecycle paths can retain views; a future repair must trace
  original consumers and choose session/slot retirement rules. Rebinding a
  fixed retired address conflicts with strict context-wide no-reuse, so a
  generation/owner decision is needed before changing these lifetimes.
- Object-slot views and map-track targets lack a general external retirement
  API. Callers must retain backing until context destruction. This is an owner
  integration gap; do not expose arbitrary frees while these references remain.
- `unbind` retires a view only; freeing backing must call retire_native with
  its full allocation size. RAM and device lifetimes are outside that primitive.
- Default free-function globals override RAM intentionally. Their host words
  cannot be treated as evidence for inverse identity of the shadowed RAM bytes.
- Zero-sized allocation pointer identity is supported, not byte access. Legacy
  zero-length one-past translation is distinct from positive access validity.
- Synthetic tokens may reuse slots after release. They are outside the raw-byte
  isomorphism and have service-specific validity checks, not epoch guarantees.
- The 0x04000060 raw shadow remains an MMIO modeling boundary. Side effects,
  masking, interrupt timing and Metal publication are not established here.
- ABI projections require per-layout evidence. Source-level arithmetic proofs
  and passing tests establish neither ARM execution nor Metal refinement.

## Minimal change and validation

The implementation adds address reservation and backing retirement to the
existing header, plus owned-view shrink with tail tombstones; it adds no allocator, engine, gameplay function, dependency,
region representation, ownership table or generic alias system. Repeated cursor
arithmetic and repeated extent-erasing retirement are removed from the loader
and raw-prefix transport. Effect member reads reuse existing pointer borrowing.

Focused mapping and original-field tests passed after the coherent refactor.
The occupancy oracle is independent of production interval helpers and examines
success/failure, all guest bytes, inverse identity and host reuse. Explicit
legacy aliases test canonical priority, coherence and joint backing retirement.
See the final execution report for full-build, CTest and sanitizer results.
No Lean toolchain was available; there are no machine-checked proof claims.

Final execution checkpoint:

- Complete `cmake --build build -j 8`: passed.
- Full `ctest --test-dir build --output-on-failure -j 8`: **82/82 passed**,
  37.89 seconds, after the shrink change.
- ASan/UBSan build of test_ds_mapping and test_original_field_constructor:
  passed; both selected CTest tests passed (1.66 seconds), with
  `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`.
  LeakSanitizer is unsupported on this platform; enabling detect_leaks aborted
  before either test executed. No leak-check result is claimed.
- `git diff --check`: passed. No upstream files changed.
- Logs retained locally under ignored `build/evidence/memory-correspondence/`.

Compared with the inspected starting files (including existing untracked work),
implementation changes add 152 lines and remove 47 across ds_memory.h,
ds_mapping.h, original_loader.c and original_raw_resources.inc. Mapping tests
add 230 lines. Documentation is separate. These counts do not include the user's
pre-existing changes and are not a claim about the repository-wide Git diff.

The next memory integration blocker is explicit lifetime/retirement for fixed
field/object/caller views, including failure and slot reuse. These need original
owner/callback tracing; no gameplay boundary should be advanced on the strength
of these registry tests alone.


## V3 lifetime follow-up

### Original observable reuse versus host safety

A numeric ARM pointer is an address in the *current* machine state, not an
allocation identity. Host diagnostics may retain allocation/binding identities;
putting them into guest pointers or universally rejecting a reused raw address
would change guest behavior.

Direct source evidence:

- `upstream/pokeheartgold/src/heap.c:181–236` adds the original MemoryBlock
  header and calls `NNS_FndAllocFromExpHeapEx` with alignment +4 / -4 for
  front/end allocation. `:238–282` returns storage to the expandable heap;
  `:306–323` shrinks a block without moving its pointer. The native budget
  wrapper omits the original headers, placement, direction and fragmentation.
  The exact Nitro free-list selection has **not** been executed or proved in
  this audit. Monotone compatibility placement and permanent dynamic tombstones
  are not established as equivalent; globally forbidding reuse is unsuitable as
  a DS specification. No replacement heap algorithm was invented here.
- `src/map_object.c:176–185,570–586,685–687` in upstream allocates one object
  array, chooses the first inactive slot and clears records in place.
  `MapObject_Remove:279–287` runs callbacks, cancels its task, and clears it;
  manager deletion `:145–148` eventually frees the whole array. Logical actor
  recreation can therefore reuse exactly the same address while the storage
  allocation remains live. Do **not** retire mappings on ACTIVE-bit clear.
- `upstream/.../src/poke_overlay.c:119–164` checks live overlaps using each
  overlay's original ram_address/ram_size/bss_size. Reload uses those same
  bounds; unloaded ranges cannot be permanently forbidden. Current native
  overlay hooks have explicit load/unload/retry lifetimes but do not map actual
  overlay images. The generated loader image's residence is not a proof of
  original overlay overlap, BSS reinitialization or address-reuse semantics.
- `upstream/.../src/field/fieldmap.c:299–310` tears down object graphics before
  effect managers and preload IDs. FieldSystem outer deletion in
  `src/field_system.c:161–173` later frees field owners/heaps. This ordering,
  including callback quiescence, remains required by the bridge.

The implementation keeps the existing dynamic placement policy pending an
original-allocator/arena refinement. It adds explicit reuse only to owner-scoped
fixed mappings whose numeric identity already denotes stable field/slot storage.
A DS instruction sees the current bytes at a reused address. A native retained
`DSMappingRef(space,slot,generation)` rejects a retired/reused incarnation. Both
facts are tested, including reuse of the same host backing address. References
must not outlive their address space; no global nonce scheme was introduced.

### Fixed and caller mapping lifetime trace

| View / root | Initialization and consumers | End / failure / reuse |
| --- | --- | --- |
| 0x0310003c, 0x03100044 field scalars | Effect construction maps context-owned ARM32 scalar projections; the canonical native field is observed as the callback root. Original FieldEffectManager_New retains the synthetic field identity. | Original effect destructor runs with views live, then both retire; the next initialization rebinds their existing slots with new generations. Freeing the canonical field while effects remain live is an ownership violation and rejects before native release. |
| 0x03102000 manager opaque storage | `canonical_manager_storage` returns the actual manager's 0x10c byte-compatible tail. Original `ov01_021F91F8 -> ov01_021F944C` initializes it, retains manager identity and creates nested resource owners. Native owner observation covers interior storage. | Original `ov01_021F9250 -> ov01_021F94A0` destroys children and closes NARC, then the view retires. Repeated constructor/destructor at the same address is tested. Pre-dispatch bind failure retires any newly installed scope view without freeing native manager storage. |
| 0x03103000 caller IDs | Original FieldMap passes MapObjectsToPreload_GetIDs and count+3 through `ov01_021F944C -> ov01_021F9A8C`; the complete preload allocation remains owned by native field code. | Keep the view through graphics teardown, then retire it. Upstream frees preload only after graphics/effects teardown. Heap notifications cover native release or shrink; count zero exposes no bytes. Different live backing cannot silently replace a binding. |
| 0x03210000+i*0x100 work; 0x03220000+i*0x100 facing | Generated scalar/vector accessors return byte-compatible spans within the original native object array; original callbacks store numeric guest addresses. | Removal/recreation clears the same slot without freeing backing, so mapping and numeric identity remain. Array free/heap destruction retires every contained view before host free. Effect scope teardown also retires these exposed views after original effect callbacks are destroyed. Rebinding a retired slot is explicit. |
| 0x031000f9 pending flag | Borrowed byte inside native FieldSystem; original supported zero branch is synchronous. Nonzero branch still explicitly rejects the missing ABI. | Native field release/shrink notification or effect scope teardown retires it. Repeated calls with live same field are idempotent; a new field owner needs retired-view reuse. |
| MapLoadManager target | `MapLoadManager_TrackTarget` at upstream asm/overlay_01_021F4704.s:3738 copies the vector and retains its numeric address at +0xdc. Other code dereferences it later; it is not a temporary argument. | Keep the backing view beyond the call. Original ForgetTrackedTarget at :3756 clears +0xdc; native release retires backing but does **not** synthesize Forget or change that original guest field. Caller must quiesce/forget before destroying target. Stale raw integers are not allocation IDs. Exact heap-address reuse remains under the unresolved placement policy. |
| Effect member buffers | Native caller allocates; original NARC read writes into its borrowed view; model/render associations can retain it. | Owner notification and existing native-effect free wrapper retire all aliases/model views before native release. A failed read leaves ownership with caller, whose later Heap_Free/heap destroy still invalidates the view. |
| Default free-function globals and MMIO | Context-owned scalar words override two RAM globals; video shadow is device-owned. | Context lifetime for globals, external device lifetime for shadow. They do not acquire per-field allocation generations or a generic device byte-isomorphism claim. |
| Local terrain / transition / test projections | Local RAM/stack tables and pointer-free Location/player/task/work views execute synchronously; original calls return before local tables/backing leave scope. | Their whole address space retires at call return. No asynchronous retain is supported by these local transports. Layout and indirect-callback closure beyond the inspected functions remain explicit obligations. |
| Synthetic native roots | Field/manager/player/object/task/NARC/props/raw resource tokens resolve through existing checked service tables, not native struct casts. Save/matrix/pool roots retain their original parent ownership. | Field/graphics scopes and original destructors govern admission; task cancellation/queue teardown precedes context deletion. Save and other parent owners must outlive the child resource context. Byte-view generations are not inserted into these original guest records. |

One native allocation supports one address-space observer. Repeated views in the
same context share it; conflicting observers explicitly reject. Supporting
multiple independent guest contexts borrowing one mutable native owner would need
an owner-specific sharing contract, not a silent overwrite of its observer.
Untracked malloc/stack backing now rejects at the public long-lived loader borrow
boundary; RAM and known context-owned scalars keep their existing identities.

### Failure and consistency audit

- Single bind/rebind failure leaves the registry unchanged. Reuse requires a
  retired borrowed reference with matching context/generation and available next
  generation; its conflict check precedes mutation. Native observer attachment
  may remain after a bind rejection, but owns no storage, safely tolerates no
  mapped views and detaches before context deletion.
- Native allocation ownership has a fresh identity in the existing allocation
  list. Release and successful nonmoving shrink notify mapped borrowers before
  freeing/narrowing storage. Native free validates explicit heap ownership before
  notification. Distinct dependent frees can occur during notification; heap
  unlinking reacquires the current list link to avoid a dangling predecessor.
  Recursive release/resize of the notifying block is rejected.
- Loader Heap_Alloc preflights native budget, table capacity and guest reservation.
  Realloc checks actual native size before changing mapped size. Shrink preserves
  the original reservation in its own entry and therefore succeeds even with a
  full registry; the previous artificial spare-tombstone-slot failure was removed
  with a stronger full-table success test. No original successful path is claimed
  to require that old failure.
- GfGfx helper allocations are freed when guest reservation or native allocation
  metadata lookup rejects, restoring ownership/accounting rather than losing the
  result. Host malloc or allocation-metadata OOM still aborts; a recoverable native
  OOM protocol was not added. Original Heap_Alloc also invokes AllocFail on OOM;
  this is not evidence that their full error/reset traces are equivalent.
- Effect constructor closure is directly inspected: two private blocks plus one
  NARC (`overlay_01_021F1348.s:20–46, ov01_021F147C`), no task publication.
  Rejection closes newly opened NARCs, releases the two blocks, retires scalar
  views and clears native field association. Native budgets return to their old
  values; cursor/tombstone diagnostics can advance. A test deliberately rejects
  the bridge's NARC_New-on-heap-3 restriction after those allocations, then retries
  successful original construction/free. This is a compatibility-failure test,
  not a claim that original NARC_New rejects heap 3.
- Object graphics construction can open/publish NARC, SDK/resource owners and
  callbacks through `ov01_021F944C`'s helper graph. An unsupported rejection after
  dispatch is marked failed, retains valid owners/views, and rejects normal
  retry or pretending the complete effect destructor is safe. **There is no
  generic resumable rollback or completed-abort cleanup for that partially
  constructed graph.** Such execution must stop/reset; supplying the missing
  owner-specific abort contract remains an explicit boundary. Preflight failures
  and the supported complete construct/free/reconstruct route are tested.
- Native callbacks still carry numeric guest data and follow original queues.
  Generation checks protect retained native mapping references, not original
  guest pointers. Valid source owners must cancel callbacks or forget retained
  targets before release; arbitrary misordered frees are not silently emulated
  as successful original teardown. Original FieldMap Exit remains unexecuted.

### Proof and test status

The v3 contract specifies allocation identities, binding generations, ownership,
reserved/accessible extents, observer/quiescence preconditions, and preservation
obligations for every transition. Its algebraic round-trip and generation
arguments are conditional mathematical reasoning. No Lean toolchain was
available; there is no machine-checked C/lifetime/ARM/Metal equivalence proof.

Focused tests pass for explicit fixed reuse and stale native references,
cross-context rejection, generation exhaustion, 2048 repeated fixed lifecycles,
full-table/zero-size shrink, aliases, native free/reuse/shrink/heap destruction,
observer detach/conflicts and nested dependent frees. Original-engine tests add
follower removal/recreation at the same slot, repeated real graphics setup/free,
constructor rejection with unchanged native heap accounting, and successful
subsequent original effect construction. Existing randomized allocation/borrow/
alias/free tests remain enabled. Final full-suite and sanitizer results follow.

Remaining DS-observable boundaries: exact original heap numeric placement,
header/fragmentation/accounting/direction/reuse; real overlay image reuse and BSS;
raw stale-pointer accesses after deallocation/reuse; callback quiescence outside
the inspected supported paths; partial graph abort; full ARM32 layout, MMIO and
Metal semantics. These are not proved by memory safety or the regression suite.
No gameplay boundary or upstream file was changed.

Final v3 execution checkpoint:

- Full build: passed.
- Full CTest suite: **82/82 passed**, 37.35 seconds.
- ASan/UBSan: mapping, native heap/module lifetimes and original field
  construction **3/3 passed**, 1.88 seconds, with leak detection disabled because
  this platform does not support LeakSanitizer. No leak-check claim is made.
- Added-line whitespace checks (including existing untracked source files) and
  `git diff --check` passed. Upstream and unrelated user changes were preserved.
- Logs: ignored `build/evidence/memory-lifetimes/`.

Original Heap_Realloc checks the SDK's physical block extent, whereas the native
owner tracks requested logical bytes. Padding and any resize permitted within
that SDK extent are part of the unresolved original allocator refinement; the
supported logical-shrink tests do not prove those limits equivalent.


## V4: partial graphics rollback and concrete address observers

### Constructor closure and recovery

Inspected the dirty tree and preceding contracts before edits; upstream and the
existing user changes remain intact. No Nitro allocator or gameplay function
was reimplemented. Source anchors below refer to `upstream/pokeheartgold`.

A mechanical direct-call/tail-literal inventory from `ov01_021F91F8` contains 89
labels, including its imported services. It found no missing direct functions or
unresolved register-tail targets in that limited parse, and no BLX in the
collected ARM bodies. Local generated inventory:
`build/audit/graphics-constructor-closure.json`. This is a bounded call inventory,
not a proof of ARM execution or of arbitrary native callback behavior. The
native raw resource C functions and their typed SDK transports were read too.

| Original acquisition / mutation | Source / retaining owner | Abort action |
| --- | --- | --- |
| Model NARC and native manager publication | `asm/overlay_01_021F8D80.s:648–685` calls FldObjSys_OpenMModelNarc before other construction; native setter publishes it outside opaque ARM bytes. | Reject an already owned native manager NARC before entry. Close only this attempt's tagged NARC, then clear its publication. Existing effect/map NARCs remain. |
| Existing original model-pool slot | `asm/unk_02023694.s:207–269,370–399` selects an inactive 0xe0 slot in the array rooted at _021D2208. Original creation writes the slot before all its children exist. | Call the original selector before dispatch and retain this slot's exact preimage. Release its private blocks/SDK descriptor and restore bytes; do not call the complete pool destructor against partial state. |
| Pool backing/allocator/auxiliary records | sub_020237EC, sub_0202068C and their helpers allocate original ARM work blocks; HeapExp_FndInitAllocator retains a native lease for an ARM descriptor. | Tag all native acquisitions made by constructor imports. Remove owned allocator leases and dependent model views before releasing remaining tagged blocks. |
| Manager opaque state | `asm/overlay_01_021F944C.s:18–55` stores the manager identity and the helper owners in the real 0x10c native storage. | Snapshot and restore only that borrowed byte span. Retire its fixed guest view after cancellation/release; retain normal fixed-view generations for later rebinding. |
| Child heaps 63/64 and their ARM resource tables | `asm/overlay_01_021F944C.s:528–604 -> asm/overlay_01_021FC4C4.s:18–63`; the latter creates a child heap before allocating/publishing its table. | Journal successful Heap_Create calls. Release their acquired blocks, then destroy only those child heaps. An already active external child heap causes rejection before native creation and survives rollback. |
| Raw manager, copied prefix/full resource header and VRAM keys | `src/unk_02025534.c:154–236`, called through original loader raw transport. Original C can transfer backing ownership into a raw object. | Journal successful manager publication. Run its native original destructor, which understands those transfers and key releases. Clear only dependent owned raw/texture/key metadata; release remaining tagged blocks after structured destruction. |
| Four asynchronous graphics tasks | ov01_021FA854, `asm/overlay_01_021F944C.s:2904–2979`, creates two Main, one VBlank and one VWait task before returning. | Journal each attempted task slot, cancel live owned tasks first, and restore overwritten free-slot diagnostic counters. Do not cancel pre-existing effect tasks or run Main/VWait to finish construction. |
| Preload resources | ov01_021F9980 / ov01_021FA470 / ov01_021FA61C can read mmodel resources and publish raw objects/keys. | The same acquisition ownership covers resource buffers, prefixes, keys, native sidecars and dependent mappings. The nonempty preload test requires a real raw object, not just an ignored sprite ID. |
| Final initialized flag | original ov01_021F91F8 sets manager bit 1 only after helper completion. | The post-return fault case clears only that bit; other manager flags are preserved. |

The C heap allocation list now carries an optional acquisition tag. This is the
only additional attribution mechanism: no second heap, general snapshot engine,
or timestamp-based “free everything newer” scheme. Tags are active only in the
constructor's native import at its interpreter call depth, or imports of its
journaled tasks. IRQ service starts with capture suspended; existing tasks and
other owners remain outside the tag. Its own VBlank tasks retain the tag for
any acquisitions while construction is pending. The original display IRQ timing and callbacks are retained.
The VBlank queue-capacity wait likewise suspends capture. Native allocation
identities continue monotonically across failed attempts.

Why attribution is necessary was observed during testing: an existing effect
VBlank callback allocated two native model views (240 bytes total) during a
failed attempt. Those are external acquisitions, not constructor leaks. Exact
budget comparisons therefore first finish those pending external acquisitions,
then keep display IRQs active through fault tests. A separate heap test
interleaves two capture owners and an untagged IRQ allocation and verifies that
rollback frees only its own blocks and invalidates their aliases.

On recoverable rejection, cleanup cancels owned tasks before freeing backing,
uses complete native raw-object destructors, releases owned dependent views,
closes owned NARCs, frees remaining tagged allocations and destroys journaled
child heaps. Borrowed manager/pool bytes are restored from preimages. No zero
initializer is substituted for original data. The previous permanently failed
object-graphics state is removed; safe retry is exercised.

Only unpublished trailing guest mappings are reclaimed, with every parallel
metadata array cleared. q rewinds only without unrelated interleaved guest
mappings; otherwise their positions/reservations survive. This relies on the
source-checked no-escape boundary: parent publications are undone, callback
queues are cancelled, no native DSMappingRef to those transient regions was
issued, and the chosen inactive original pool slot has no other writer.
Committed allocation/free policy and fixed-view generations are unchanged.
Immutable native archive-cache warming belongs to its existing external owner;
it is not rolled back as if it were a game allocation.

### Concrete observers: mandatory versus implementation-specific

Searches covered original source/assembly and actual loader import boundaries,
including pointer casts/masks, heap queries, SDK heap calls, object/pool slots and
overlay range checks. This was not exhaustive semantic classification of every
ARM instruction in every overlay.

| Observer and execution path | Evidence | Required semantics / current scope |
| --- | --- | --- |
| Alignment faults of actual ARM memory operations | `src/ds_thumb.c:6–15` checks a%width for every interpreted load/store. Loader Heap_Alloc uses 16-byte guest alignment; SDK animation records reserve 32-byte addresses and expose +0/+8 only. | Effective-address alignment and valid ABI offsets are mandatory. Extra alignment alone does not prove address-mask equivalence. General unaligned ARM behavior remains a CPU obligation. |
| Integer alignment of allocated buffers | `src/overlay_mic_test.c:759–775` allocates 0x120 bytes then rounds its data pointer with `(p+31)&~31`; `src/unk_02037C94.c:1778–1783` rounds a Heap_Alloc result before DWC_Init. | Required aligned interior span, offset/ownership and any observable address residues must match. These mic/network paths are not in the current loader constructor closure; they refute a universal “only opaque pointers” assumption. |
| SDK allocation direction, headers and resize bounds | `src/heap.c:181–236,238–282,306–323` adds the original 16-byte header, uses +4/-4 allocation direction, subtracts the header on free and queries physical block size before resize. | Preserve the contract at any admitted caller. The bridge replaces those services with registered native ownership/logical sizes; the inspected constructor does not otherwise read a Nitro block header. Exact fragmentation, padding, direction and failure decisions are not proved equivalent. |
| Heap free-size assertions | `src/scrcmd_c.c:4329–4347` (ScrCmd_682) snapshots FIELD2/FIELD1/FIELD3 free sizes and later asserts restored FIELD2/FIELD3 values. `src/heap.c:286–294` queries the SDK free-size value. | The observed scalar/equality and failure outcome are mandatory, not an incidental malloc detail. Native requested-byte budgets can test restoration but are not an exact Nitro free-size implementation. This command is not an import in the current loader image; its original script path is not validated by these constructor tests. |
| Direct SDK heap clients | `asm/unk_020192D0.s:299–333` (sub_02019520/548) passes requested size/alignment to an expandable heap. `asm/overlay_00_thumb.s:1645–1680` can fall back to another heap after allocation failure; `:3251–3279` compares total free-size values. | Allocation success/fallback, alignment, free-size checks and retained identities are concrete observers. These network/overlay clients are outside the current admitted constructor closure. They cannot be silently served by an unverified generic heap replacement. |
| Stable object and model-pool slots | `src/map_object.c:176–185,279–287,570–586,685–687` and `asm/unk_02023694.s:370–399`. | The first inactive record is reused inside a still-live array, preserving its numeric slot address. This is a demonstrated identity/reuse requirement, already supported by live byte views and explicit fixed reincarnations; no Nitro free-list reconstruction is required for it. |
| Pointer/offset relationships in the graphics constructor | ov01_021FA854 partitions a single allocation into arrays at +0x28 and size-derived offsets; ov01_021FC4C4 places its table at +0x18; pool/list helpers retain pointers and null/slot identities. | Intra-allocation offsets, bounds, pointer equality and topology are mandatory. No cross-allocation numeric ordering dependency was found in the inspected constructor closure. This is bounded evidence, not proof that such observers are absent elsewhere. |
| Retained pointer and future dereference | MapLoadManager_TrackTarget, `asm/overlay_01_021F4704.s:3738–3765`, copies the vector and retains its guest pointer at +0xdc. | Preserve identity until original Forget or owner teardown. Numeric freed/reused pointer comparisons would constrain reuse if reached. No whole-game stale-address-independence proof exists. |
| Absolute overlay placement | `src/poke_overlay.c:43–59,119–164` reads original ram_address, classifies ITCM/DTCM/Main destinations and rejects overlap with other live images; load/start then uses those headers. | Those addresses, bounds, overlap decisions and BSS lifetimes are directly observed. Native overlay hooks preserve lifecycle admission only; they do not establish original image placement or RAM reuse. No replacement overlay/heap algorithm was added. |
| Opaque native associations and physical host placement | Actual loader uses region lookups, resource interior offsets, checked NARC/task/raw-object tokens and native heap allocation identities. | Host malloc ordering is not automatically a DS observation. It may vary only when all guest-visible relations and reached scalar/device observations remain matched; a native pointer may never leak into an ARM32 field. |

The allocation algorithm's internal free-list shape is incidental **only to the
extent it cannot change an admitted observation**. Header layout can be hidden
behind a service boundary if no reached consumer inspects it; allocation
ordering can differ if no reached computation distinguishes it. Conversely,
address bits, ordering, reuse, free-size values, OOM/fallback and fixed overlay
ranges become mandatory as soon as a reached observer uses them. Exact returned
ARM9 register/pointer bits are themselves mandatory under an architectural trace
observation. The current arena is not proved equivalent under that strong
observation. This work does not weaken AGENTS.md by silently replacing that
specification with a gameplay-only projection.

The v4 contract now gives explicit pathwise equivalence conditions parameterized
by Obs and a bounded allocation correspondence rho. It requires equal logical
allocation outcomes, commuting address operations, matched reuse observations,
correct boundaries and an Obs invariant under rho. If Obs includes raw heap
addresses, rho must be identity on them. The conditional induction argument does
not discharge those premises for the entire original engine. Existing guest
allocations are never remapped to simplify a bridge.

### Verification and remaining obligations

Fault injection is compiled only for the constructor test target. The opening
path has 58 import/return rejection points; the nonempty preload path has 64 and
is required to own a real raw graphics object. Every point is tested through
progressively retried construction in the normal-completion session, ending in
successful original construction/destruction. The second parent session keeps the preceding
pending-IRQ cancellation regression without draining its external acquisitions. Tests assert restored heap bytes/blocks/child reservations for four
heaps; absence of created child heaps; preservation of existing Main/VWait tasks;
no leftover VBlank tasks; unchanged texture budgets and external effect objects;
restored manager bytes/flag/NARC; retired fixed and transient guest addresses;
registry/cursor restoration; and preserved external buffer contents. Additional
real-rejection tests preserve an externally owned child heap/mapping and reject
an already owned native NARC. Native heap tests cover independent capture tags,
interleaved untagged allocations, commit, rollback and dependent aliases.

Still unproved:

1. The pathwise allocation-equivalence premises for the whole game, including
   exact guest heap addresses, inter-allocation comparisons, pointer-bit uses,
   stale references/reuse, original OOM/fallback and free-size observations.
2. Original SDK header/fragmentation/padding/direction/physical-resize behavior;
   native budgets and widened native SDK layouts are not its exact model.
3. Actual overlay image overlap/reload/BSS and relocated-code address semantics.
4. Rollback outside the inspected constructor ownership closure: arbitrary game
   callbacks, reentrant Main/VWait, another writer to the selected inactive pool
   slot/manager bytes, or escaped transient addresses/native references.
5. Recovery from process-terminating native OOM/assertions inside a native import;
   fault injection covers recoverable import/instruction-boundary ownership
   states, not arbitrary C instruction failures. Corrupted raw-object invariants
   fail explicitly rather than report a completed rollback.
6. Cross-context lifetime after context destruction, multi-context observation
   of one native owner, unsupported ARM32 layouts and CPU instruction behavior.
7. Full MMIO/interrupt/Metal/VRAM observational refinement, including observations
   of unallocated/freed backing. Passing memory tests does not prove these.

The conditional algebraic arguments remain unmechanized; no Lean or C-refinement
proof is claimed. FieldMap_Init, Main/input/Exit progress and upstream are unchanged.


V4 execution checkpoint: the complete build and CTest suite passed **82/82**
(37.21 seconds). The targeted ASan/UBSan suite passed **3/3** (2.92 seconds):
memory correspondence, native heap/module lifetimes and original field
construction. LeakSanitizer was disabled because this platform does not support
it; no leak-check result is claimed. The loader also passed a syntax check with
the test-hook definition removed; that check was not a separate production link
or execution. Whitespace checks passed and upstream has no working-tree edits.
Logs and the bounded constructor-closure inventory are retained locally under
ignored `build/evidence/memory-rollback/`.
