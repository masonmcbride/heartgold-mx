# Memory correspondence contract (v4)

## Scope and state

The guest address space is authoritative. This contract concerns byte-compatible
RAM views, not arbitrary C objects, device registers, CPU execution, or rendering.
Addresses below denote mathematical integers; implementations must check their
representability before arithmetic. No guest integer is a host pointer.

A state S consists of guest RAM backing, native allocations A, an ordered view
registry R, a placement policy P, and host observers. Each native allocation has
an identity alpha, heap owner, base h, logical size n and live/retiring/dead state.
Registered heap allocations receive a fresh nonzero 64-bit identity, even when
malloc returns the same numeric base; exhaustion is terminal, not wraparound.
A view r has guest base g, host base h', accessible length n', reserved extent e,
binding generation gamma, and live/retired state. For a heap view, its parent
alpha is determined by the observed native owner containing its byte span. It is
not a second allocation: view retirement does not free the owner.

`DSNativeRegion` stores g, h', n', e, gamma. Null `bytes` marks retirement.
`reserved_size` retains a shrink's original extent without another table entry;
legacy zero fields mean max(size,1). `borrowed_regions` controls whether the
loader may free a view as an owned base; `region_heaps` checks explicit frees.
Native heap ownership and allocation identity remain in the existing allocation
list, not duplicated in every region. During a synchronous graphics constructor,
new native allocations also carry an acquisition tag tau in that same list.
The tag is installed only around that constructor's and its own tasks' native
imports, suspended for IRQ service of other owners and unrelated reentrant calls, and cleared on commit or release.
It selects ownership, not an allocation algorithm or a timestamp interval.
Context-owned scalar words, RAM and devices
have their existing owners instead of a registered heap observer.

For compatibility, **numeric guest addresses and lifetimes are different**.
ARM pointers contain g only, never (g,gamma) or (g,alpha). A native retained
reference `(address_space,slot,gamma)` identifies a binding incarnation and
resolves only while that incarnation is live and its requested span fits.
After explicit reuse, a stale numeric ARM address resolves the current binding;
a stale native reference fails. References must not outlive their address space;
address-space destruction/recreation at the same host address has no automatic
cross-context epoch guarantee.

Object-record identity is a further distinction: clearing/recreating a
LocalMapObject changes the logical record but does not free its array storage.
The slot's bytes and numeric address persist until the manager array is freed.
Do not invalidate byte mappings merely because its ACTIVE bit is cleared.

P currently uses a monotone cursor q and exclusive limit L for dynamically
assigned compatibility addresses, and explicit fixed-address rebinding for
owner-scoped views. This is an implementation placement policy, **not a theorem
about original Nitro heap placement/reuse**. Heap allocation direction, headers,
fragmentation and arena address observations remain unresolved. Fixed overlay
addresses likewise require their original load/unload policy, not permanent
never-reuse reservations. See the source audit before extending P.

Preconditions for the laws:

- Guest spans lie in [0,2^32); host spans are representable, valid allocated
  byte arrays for the entire access lifetime. Nonempty guest bindings exclude 0.
- Canonical registry bindings have disjoint guest and host byte spans, including
  RAM. Explicit legacy aliases and RAM overrides require the rules below.
- Region metadata and backing lifetime are changed only by the owning context,
  without concurrent access. The registry does not validate C allocation lifetime.
- Every native release invalidates all views into its full backing allocation
  before release. Borrowers cannot release storage through guest Heap_Free.
  Observed allocations support one address-space observer; conflicting observers
  explicitly reject. Notifications run synchronously before logical shrink/free;
  they cannot fail, allocate, or release/resize the notifying allocation. They may
  release distinct dependent allocations; the native heap reacquires its list
  link afterwards. All context observers detach before context destruction.
- Shared bytes have compatible representation; pointer-containing ARM32/native
  structures require a separately justified adapter.

`ds_mapping_valid` checks table shape, not these semantic preconditions. Direct
legacy table writes do not acquire a correctness guarantee merely by passing it.

## The relation

For a live byte-compatible region (g,h,n), define

    phi_S(g+k) = h+k,                 0 <= k < n.

RAM contributes (0x02000000, ram, size). Overlapping guest views must agree at
every shared guest byte. Let D be the guest bytes with an unambiguous compatible
backing, and define x ~ y iff phi_S(x) = phi_S(y). Let H = phi_S(D).
The induced map D/~ -> H is bijective. This is the partial isomorphism; raw guest
addresses need not be bijective. Choose c:H->D selecting one representative per
class. The implementation uses the first registered compatible view, then RAM.
Canonical guest bytes C = c(H) give the bijection phi_S|C:C->H.

For a nonempty span, T_S(g,l) is defined only if one live region or RAM contains
the entire span, g+l <= 2^32, and the region remains alive. Its result is h+k.
No access is stitched across adjacent mappings. A span's canonical inverse
A_S(h,l) additionally requires that the first containing native view contains the
whole span; unsupported cross-view spans fail. Canonicality is therefore specified
for admissible spans, not arbitrary unions of neighboring bytes.

## Laws and operations

1. **Identity and partial inverse.** For admissible canonical spans,
   A_S(T_S(g,l),l)=g and T_S(A_S(h,l),l)=h. On aliases the first equality is
   c(phi_S(g)); it need not equal g. Null/failure is outside the relation.
2. **Offsets.** Within the same region, T_S(g+k,l)=h+k for k>=0, l>0,
   k+l<=n. Interior borrowing returns the existing canonical guest identity;
   it never copies or moves an existing allocation.
3. **Coherence.** Guest and host byte stores update the same backing byte.
   If B is host backing and M the guest observation, M(x)=B(phi_S(x)). A
   store at x of v changes B at phi_S(x) to v; all aliases observe v.
   Concurrency, cache publication and asynchronous GPU visibility need additional
   synchronization relations; ordinary byte coherence does not establish them.
4. **Overlap consistency.** Shared guest bytes must have equal backing.
   Shared host bytes produce equivalent guest addresses. `bind` rejects both
   kinds of overlap, including RAM; only the exact same borrowed tuple is
   idempotent. It rejects a host range extending beyond an existing view.
5. **Bounds.** Positive-length accesses cannot overflow the guest address
   space or exceed a single accessible span. Mapping failure causes no access.
   Translation retains legacy zero-length/one-past lookup behavior; inverse and
   binding reject zero lengths. Zero-length results authorize no dereference.
6. **Ownership.** A mapped view does not confer ownership. Loader heap metadata
   identifies the native owner. Borrowing adds a view without allocation/copy.
   `unbind(g)` retires one borrowed view; it never frees backing or other views.
   It is not a backing-release operation.
7. **Invalidation.** `retire_native(h,n)` retires every intersecting explicit
   region, preserving its guest extent and ownership metadata. The owner must
   supply the full allocation span. Guest RAM cannot be retired by this
   operation and must remain alive until the address space is destroyed.
   Loader frees obtain the full size from the existing heap owner, including
   the animation allocation's two disjoint mapped fields.
8. **Allocation and reuse.** `reserve(n,a)` requires a power-of-two alignment a,
   rounds max(n,1) up to a, searches at or above aligned q, and skips RAM and
   every registered extent, live or retired. Success returns b>0 with
   b+rounded(n)<=L and sets q to that end. Failure leaves q unchanged.
   `borrow` also rolls back q if registration fails. Bare reservations consume
   cursor space but do not create views or establish native ownership.
   Dynamic reservations are currently not recycled during a context lifetime;
   this diagnostic policy is not a DS-observable equivalence claim.
   `shrink(g,n)` narrows an owned live view, records its discarded tail as a
   reservation in the same entry, and invalidates aliases intersecting that tail.
   Growth and borrowed-view misuse fail without changing mappings; shrink needs
   no spare registry slot. The owner must then perform the corresponding
   nonmoving native shrink. Owner notifications also bound borrowed prefix views
   and retire interior aliases touching the discarded tail before native mutation.
   Host storage may be reused after invalidation, with a fresh dynamic identity.
   A raw stale host pointer is indistinguishable from a new allocation at the
   same numeric address: callers must respect lifetime. Across context destruction
   neither guest nor host addresses carry an epoch; no cross-context stale-pointer
   guarantee is claimed. `bind` cannot implicitly reuse tombstones. `rebind`
   requires an explicitly retired borrowed-view reference, matching context,
   slot and generation, and a nonexhausted next generation. It reruns guest/host
   bounds and overlap checks before modifying the same table entry. Fixed
   addresses may recur while their binding generations differ. Live rebinding,
   stale retire references and generation exhaustion fail without mutation.
9. **ARM32 ABI.** Byte compatibility requires equal scalar encoding, offsets,
   widths, alignment where accessed, and pointer representation. Equal sizeof
   alone is insufficient. Shared pointer-free records need layout evidence;
   widened native owners use typed accessors or explicit ARM32 projections.
10. **Aliases and devices.** Legacy coherent aliases may be installed only by
    an owner that specifies canonical priority and invalidates all dependent
    views. No generic alias-construction API is added. Guest RAM overrides
    (import globals) deliberately choose different backing and are excluded
    from the RAM inverse laws at overridden bytes. MMIO is a device transition,
    not a byte isomorphism; exposing a device shadow as bytes proves no hardware
    side effects, ordering, or interrupt semantics.

## Validity and preservation obligations

Between public operations, let I(S) mean: native heap usage equals the sum of
its live logical allocation sizes plus child reservations; allocation identities
are unique; each live heap view lies within its live observed parent allocation;
no view retains freed backing; accessible n'<=reserved e; canonical overlaps
agree; view generations never wrap; native references resolve only their own
context/incarnation; active callbacks keep their owners alive or are quiesced
before release. Guest integers stored in RAM may be stale: I does not assert
that every integer that resembles a pointer denotes a live allocation.

| Transition | Preconditions / preservation obligation |
| --- | --- |
| bind | Valid live backing and owner; nonempty bounded span; no canonical overlap. Attach an owner observer before publishing heap views. Add one binding; do not change ownership. Exact borrowed rebind is idempotent. Failure publishes no view. |
| borrow | Return an existing admissible identity, including RAM, or bind a new span under P. Failed reservation/registration restores q. Loader rejects untracked backing and conflicting owner observers. Observer attachment alone may persist after a failed bind; it owns no bytes and detaches with the context. |
| allocate | Preflight heap budget and registry/guest arena capacity before native allocation where size is known. Commit one native owner and its mapping only after allocation succeeds. Native malloc/metadata OOM remains terminal, as do underlying native heap assertions; no recoverable OOM transaction is claimed. |
| shrink | Confirm actual registered native size and owned mapping bounds before guest mutation. Restrict views without moving backing or changing alpha/gamma. Retain e; notify all borrowed aliases before native logical size/accounting changes. No mapping-capacity failure is needed. |
| unbind | Retire one view, retain reservation and gamma. Parent remains alive. Other aliases remain valid. |
| free / heap destroy | Validate heap ownership first. Notify every observed owner before native free, retiring all views and dependent model views. Adjust native accounting and remove owner. No callback may free/resize that same owner recursively. |
| reuse | Require retired binding and explicit fixed-address authority. Reuse g, increment gamma, associate new backing/parent; old native references stay invalid, old guest numbers observe current bytes. This does not select DS heap placement. |
| constructor failure | Preserve I and external owners, allowing independent IRQ progress. The effect constructor releases its private blocks/NARC. The audited object-graphics constructor journals tasks, child heaps, raw managers, tagged native blocks and dependent views; it cancels its tasks, releases only its acquisitions and restores its two borrowed byte preimages before allowing retry. See the scope/no-escape conditions below. |

Effects/graphics teardown must execute original destructors with fixed views
still available. Only afterwards do their view scopes retire. Callbacks receive
original numeric guest data; no host generation is inserted into original ARM
records. The proof obligation for callback quiescence remains owner-specific.

## Object-graphics abort scope

The supported original constructor closure writes two pre-existing byte regions:
the canonical manager's 0x10c opaque tail and the selected inactive 0xe0 original
model-pool entry. It also publishes a model NARC pointer, manager flag bit 1,
tasks, heap children, native raw-manager handles and dependent mapping metadata.
The unchanged original selector chooses the pool entry; the bridge snapshots its
preimage, not a synthetic initializer. Admission rejects an already owned manager
flag/NARC. Main/VWait callbacks cannot interleave this synchronous call; original
VBlank callbacks continue normally and their independent acquisitions are not
captured. The selected inactive pool slot and manager bytes must have no other
writer during construction. No arbitrary game callback is admitted by the
inspected constructor closure.

For an attempt tau let A_tau be the allocations tagged tau and J_tau the
journaled handles. Failure cleanup first cancels the attempt's tasks; invokes
native raw-object destructors for its managers/headers/keys; retires dependent
model/texture/allocator views; closes its NARCs; releases remaining A_tau; and
destroys exactly its recorded child heaps. It then restores borrowed preimages,
clears its NARC publication/flag and retires the two fixed views. External heap
allocations, tasks, NARCs, buffers and IRQ progress are not rolled back. GPU/key
serials and native allocation identities are not rewound.

Unpublished dynamic mappings may be discarded on this abort only when every
live root to them has been removed, no native DSMappingRef to them was issued,
and no guest address escaped the attempt. Only trailing attempt-owned registry
entries are reclaimed, including all parallel metadata. q is restored only if
no unrelated mapping was installed after the checkpoint; otherwise that owner's
entries/reservation are preserved. Committed allocations retain the existing
monotone/tombstone policy. Fixed-view generations are never reset by abort.

This is a host recovery contract for a rejected compatibility operation, not a
claim that original Nintendo allocation-failure/reset behavior includes rollback.
Native fatal assertions/OOM do not return into this recovery path. Tests inject
rejection before every import and after return on two admitted paths (58 and 64
points), including a path owning a raw object. Other assets, callback schedules
and owner graphs still need admission evidence; no generic transaction facility
or whole-context memory snapshot is claimed.

## When allocation policy can be observationally equivalent

Specify an observation projection Obs before claiming equivalence. For matched
original/compatible executions, pair each live allocation event by label lambda,
logical size, owner and lifetime. Let b_D(lambda) and b_C(lambda) be its guest
bases. A time-indexed relation rho maps

    rho(b_D(lambda)+k) = b_C(lambda)+k,       0 <= k < n(lambda).

Pointer fields/registers are related through rho; nonpointer data are equal.
Fixed hardware addresses and any literal address observable at a boundary must
retain their required values; code relocation needs its own justified relation.
This does not permit moving an already registered guest allocation: rho relates
two executions' allocation choices, not a later remapping inside one execution.

Sufficient conditions for a pathwise commuting simulation are:

1. The paired allocation/free/shrink sequence has the same logical ownership,
   sizes, bounds, nonmoving shrinks and success/failure branches. Neither host
   capacity, diagnostic registry exhaustion nor different native ABI sizes cause
   a new observed failure. Native budget equality alone is not Nitro accounting.
2. All executed address operations commute with rho. Offset arithmetic,
   same-allocation differences/order, live identity and null tests can satisfy
   this with disjoint bounded regions. Cross-allocation ordering, numeric hashes,
   pointer serialization or address-bit tests require an additional constraint;
   they do not follow from byte correspondence. Rounding/tagging must preserve
   the observed residues/results, not merely meet a minimum alignment.
3. Stale guest integers are never used to distinguish allocator reuse, or the
   actual observed reuse/equality pattern is matched. Allocation lifetimes do
   not erase a raw integer's DS meaning; monotone addresses can distinguish a
   freed/reallocated address from original reuse. No universal nonreuse theorem
   is asserted.
4. Native/SDK adapters preserve all reachable layouts, aliasing, ownership,
   hardware-visible pointers, interrupt effects and publication. No code inspects
   original heap headers/fragmentation/arena placement unless that observation
   is independently modeled. Heap free-size queries and OOM decisions agree
   wherever observed. Overlay addresses/overlaps/BSS and callback code addresses
   obey their original observable rules.
5. Obs is invariant under this particular rho. A gameplay/output projection may
   be invariant on a path using pointers solely as opaque live identities and
   bounded offsets. If Obs includes literal ARM9 heap addresses or all register/
   pointer-word bits, rho must be the identity there: the current monotone arena
   is **not established equivalent under that stronger DS observation**.

Under these assumptions, each commuting step preserves the related state;
induction yields equal Obs traces. This is a conditional mathematical argument,
not a proof that the original game satisfies the assumptions. The source audit
records concrete counterexamples to assuming universal address independence
(alignment masks, free-size checks, overlay range decisions) and narrower
constructor paths for which no cross-allocation ordering dependency was found.
There is currently no demonstrated need to duplicate Nitro's free-list algorithm;
that does not make its exposed results irrelevant. A reached observer needs its
actual contract or original execution, not a speculative allocator.

## Central arguments and proof status

For a single valid region, translation is addition of the bounded offset.
The inverse subtracts the same base, so (h+k)-h=k and (g+k)-g=k establish
round trips and offset preservation. Disjoint canonical spans make lookup
unique. Compatible aliases quotient exactly the equal-backing relation, so
injectivity and surjectivity of the induced map follow by construction.
Coherence follows from the shared backing location, assuming valid C storage.
These are mathematical arguments, not a verified C refinement.

Reservation searches with widened arithmetic. Each collision moves the aligned
candidate strictly past a nonempty intersecting extent; candidates increase,
so the finite arena either supplies a disjoint span or rejects at L. Retired
extents participate identically to live extents. Together with monotone q this
prevents dynamic address reuse, including allocation padding, until teardown.
Native retirement removes every intersecting region from the domain without
altering guest reservations. An explicit rebind increments gamma, so an old
native reference fails generation equality even if both guest and host numeric
addresses recur. A logical object clear preserves its backing allocation, hence
does not itself invalidate its byte relation. These arguments assume well-formed input metadata.

Lean 4, lake and elan were not available on PATH or in the checked local elan /
Homebrew locations during this audit. No Lean dependency was installed, no Lean
proof was compiled, and no theorem is claimed machine-checked. In particular,
C pointer validity/provenance, all legacy registration paths, ARM32 layout,
allocator/renderer observers and the C implementation's refinement remain
unproved. Tests are independent evidence, not a proof of equivalence.

## Verification and deferred boundaries

`test_ds_mapping` covers round trips, interior pointers, shared bytes, bounds,
RAM identity, collision skipping, tombstones, allocation exhaustion, zero-byte
reservations, aliases and backing retirement. Its independent occupancy oracle
runs 128 seeds of 96 allocation/bind/borrow/retire/reuse operations and checks
all 512 guest bytes after each operation. Another 32 seeds of 96 operations
use real native allocations, dependent aliases and frees. Shrink tests cover
zero size, growth rejection, full-table success and tail-alias retirement.
Further tests exercise fixed generation reuse, cross-context rejection, native
owner notifications, original constructor rollback and original repeated teardown. Fault sweeps cover opening
and raw-preload constructors; native tests check interleaved acquisition owners,
commit, rollback and dependent alias invalidation. Alias fixtures exercise legacy views;
they do not establish a general hardware mirror policy.

See [the audit](memory-audit.md) for the migration boundary and lifetime gaps.
ARM execution needs a forward simulation preserving guest registers, memory,
exceptions, interrupts and observable timing. Metal needs an observational
refinement of DS geometry/raster/device operations and synchronization. Neither
is implemented or established here. FieldMap_Init must remain at its current
boundary pending memory validation and subsequent owner-specific work.


### Environment-prefix admission

The model-attribute and fog prefix uses the same live-byte relation and existing
binding-incarnation references. Each canonical native environment field has at
most one current allocation in its loader context; a native provider may expose
its bytes only after the original ARM allocation returns. The two layouts are
pointer-free ARM32 records (0x48 and 0x38 bytes). Numeric guest pointers remain
authoritative; native fields retain translated backing, not ARM pointers cast to
native pointers. A type-specific `DSMappingRef` identifies the owned allocation;
size equality alone is not ownership evidence.

Original pointer-to-pointer destruction receives a scoped 4-byte cell in guest
RAM above the interpreter stack. Preconditions are a live exact-base owner,
quiescent non-reentrant invocation and no callback escape of that temporary
cell. Success retires the owner and its dependent views, nulls the guest cell and
canonical native field, and drops the native binding reference. Foreign owners,
interior pointers, stale incarnations and duplicate live construction reject.
Native callers must still obey C object lifetimes; a raw native pointer cannot
encode a generation across backing-address reuse.

NNS global polygon attributes are software SDK state, distinct from submitted
geometry and device registers. Retaining that state establishes only this
prefix's state transition, not global-cache/flush or rendered-pixel refinement.
Display admission is limited to the already configured opening graphics mode.
Whole-parent restart/Exit after a later rejected constructor is not established
by independently retiring these completed components. See
[field initialization frontier](field-init-frontier.md) for executed scope.


### Camera and area-light dependency lifetimes

For a live original ARM32 camera, let `t` be its numeric retained target. The
native adapter records a binding incarnation `r` and offset `d` such that
`resolve(r,d,12) = phi(t)`. An admitted update requires that reference to remain
live. Camera records/history use original ARM32 layouts, not widened native
structs. Retargeting changes the numeric target via original code and acquires
the corresponding live reference. Invalidating the target prevents further
updates but does not own or prevent camera destruction. The analogous relation
holds between an area-light manager and its borrowed model-attribute record;
recreating the latter cannot validate an old reference by native address reuse.

The projected camera pointer words belong to the containing FieldSystem lifetime.
Freeing camera allocations invalidates their spans/references while preserving
the original dangling numeric values in those parent words. Only FieldSystem
retirement invalidates the field view; recreation writes new values into the
same parent binding. This distinguishes numeric address observations from live
allocation identity instead of replacing stale addresses with null.

Constructor acquisition tags apply only to imports at the synchronous owner's
call depth; unrelated IRQ owners are outside the capture. On rejection, tagged
allocations retire and owned field/SDK-state preimages are restored, without
freeing external owners or rewinding the guest address policy. Inputs and shared
preimage bytes require the audited no-other-writer schedule; arbitrary concurrent
or reentrant producers are not proved safe. Native process-fatal OOM remains
outside recoverable-import rollback.

The three bare-EOF original light files demonstrate an access beyond requested
allocation-view bounds. Current rejection is memory-contract enforcement, not
proof of DS observational equivalence: original physical RAM/padding contents
have not been established. Extending coverage or supplying a terminator requires
that evidence. Ordinary byte-correspondence laws also do not prove SDK matrix
rounding or rendered-pixel equivalence; current renderer projection remains an
explicit, tested but unproved graphics refinement boundary.


### Device transitions and synchronous owner publications

The byte relation `phi` excludes device requests. For an admitted register/width
pair `(a,w)`, a data read yields the specified projection `read_device(D,a,w)`;
a write produces `D' = write_device(D,a,w,v mod 2^(8w))`. Unhandled, misaligned,
wrapping, overlapping-view or out-of-window requests reject before device effects.
Instruction fetch uses executable RAM only. The current register adapter admits
only three audited video-control halfwords; it does not define general MMIO,
timing or presentation equivalence. Subtract integer addresses before forming
host pointers: an out-of-object intermediate pointer is invalid even if a later
subtraction would numerically return it to the object.

For a synchronous acquisition scope `s`, let `A_s`, `T_s`, `N_s` denote its heap
blocks, task publications and archive leases. Only imports at the scope's admitted
call depth acquire these tags; IRQ service runs outside the scope. Rejection
quiesces `T_s` before retiring backing in `A_s`, detaches `N_s`, then releases
private allocations. External owners and persistent queue tasks are preserved.
Commit erases acquisition tags, leaving original ownership protocols in force.
This obligation is checked for weather and texture construction under the audited
no-Main/VWait-interleaving schedule, not proved for arbitrary reentrancy.

Native field Window/MsgData records are explicit native ABI representations;
ARM32 sprite/renderer records retain actual guest pointer fields. Neither the
menu preview Window nor UTF-8 messages are byte-correspondent aliases of these
records. Original encoded String storage remains compatible. Lazy message
transport selects the regenerated original encrypted archive through an opaque
native NARC service, retaining original bank numbers in original owner records.
BG ownership must outlive window pixel operations; admitted map-name destruction
requires no active callback, as in the original caller protocol.

Safety rollback is not applied to successfully executed original operations that
specify partial mutation. In particular, original OAM registration may reserve OBJ
slots before rejecting an overlapping affine reservation. Numeric dangling words
left by original component frees also remain readable in live containing owners;
their pointee bindings retire. See the frontier checkpoint for tested scope and
remaining allocator, execution, device and graphics obligations.


## Staged sound policy and loader scratch (2026-10-25)

A partial ARM32 work record is not a fully initialized device owner. For the
original sound work identity s, the admitted correspondence is restricted to
[s+0xBEB9C,s+0xBEC88); the loader owns this metadata until context destruction.
The archive, sound heap, and handle prefix is outside the relation. No native
pointer-bearing SDK record is cast into this view. `InitSoundData` has not run;
a field scene request must reject at its first unsupported device operation.

For the current scene-4 prefix, let P be the admitted work tail plus original
0x10-byte scene-control state, and D external device/audio/task/heap state. A
rejected request satisfies P'=P and D'=D, provided every reached SDK sound leaf
rejects before effect. The native caller then clears its request guard before
propagating the rejection; the earlier original field-BGM selector store remains
visible. This is compatibility admission/rollback, not a claim about a DS
allocation failure. Admitting SDK effects requires extending this relation with
actual sound-heap/handle/capture ownership, success/failure behavior and disposal;
the current private-byte rollback must not undo successful external effects.

For guest image length L, let p=8192*ceil(L/8192). Image [0,L), scratch
[p,p+8192), and eight call-stack intervals [p+8192+8192j,p+8192+8192(j+1)),
0<=j<8, are disjoint. Coordinates are offsets from 0x02000000. The high interval
has an extra 4096-byte outgoing-argument area. Scratch fields are temporary
synthetic projections, not moved Nintendo allocations. The compiler checks the
RAM extent ends no later than the import range starting at 0x02100000; assembly
also checks image/import separation. The interpreter checks call depth <8.
Per-call stack consumption <=8192 and absence of reentrant scratch consumers
remain explicit execution preconditions, not proved by those placement checks.

An object graphics pool remains owned by original graphics lifetime; its native
byte view is usable only while that owner is live and resolves to the current
original pool identity. Publication executes the original state acknowledgement,
not a native replacement. Tests reject stale views after destruction. Raw DS
addresses and raw BSS pointers do not acquire generation tags; address reuse can
make a raw stale pointer indistinguishable from the new incarnation. Callers must
quiesce callbacks before retirement; stronger raw-pointer freshness is not claimed.

## Shared sound-channel projection and admission rollback (2026-10-25)

The channel implementation extends the existing DSAudio owner. It does not export
native SDK records as ARM32 memory. Distinguish 32 NNS policy player IDs, 16 ARM7
sequence-player slots, game NNSSndHandle references, and 16 physical channels.
Only the policy and physical-channel projection are supplied by this batch; SDK
sequence/handle arbitration, capture and sound-heap ownership remain unprovided.
The existing ten-voice preview transport is not a substitute for those SDK owners.

For one live audio domain A, let C={0,...,15}, and let N be its live DSP note
objects. The ownership relation is a partial function o:C ⇀ N; an active note in
the rendered voices must have exactly its corresponding channel as owner. Saved
preview snapshots are outside the rendered domain. Stealing invokes the original
SDK drop callback before assigning the new owner. Voice stop/destruction retires
its channel references before freeing backing. The independent inspector scans
all active DSP notes, not merely the ownership table, to check uniqueness and
absence of hard-locked active channels. Snapshots rebind internal pointers to the
new domain; they do not share mutable channel owners.

Let M:P→{0,...,65535} be the 32-player policy. The original NNS setter changes
M(p) only. At a new sequence start, zero means no override (default track mask
0xFFFF, mask flag FALSE); nonzero installs that mask and sets the mask flag TRUE.
The original SDAT PLAYER records initialize M. Active tracks retain their start
policy when M changes. An explicit mask flag also requests the original strong
allocation semantics: it bypasses weak locks, never hard locks.

For type mask T (PCM 0xFFFF, PSG 0x3F00, noise 0xC000), track mask m, hard locks H,
weak locks W and strong flag s, eligible channels are
B=(T∩m)\H\(s?∅:W). Original SND_AllocExChannel chooses the least existing priority,
then least scaled volume, then earliest original channel-order position among B.
It fails without a drop if B is empty or the request priority is less than the
chosen channel priority. For volume word v, the comparator uses
((v & 255)*16) >> shift[v >> 8], shift={0,1,2,4}, under the valid register encoding
precondition. Original SDK functions execute these rules; they are not rewritten
as a new native allocation algorithm. SDK FreeExChannel detaches its callback;
it does not itself stop the channel or reset priority. A voice teardown is a
separate stop/retirement operation. Exclusive resource locks reject conflicts;
invalid releases preserve state at the admitted native interface.

The loader borrows an existing audio owner through a counted lease. It cannot
free that owner; deleting the loader releases its lease and preserves playback.
Freeing a leased owner is invalid. For the currently incomplete original sound
scene, the earlier private-policy rollback is extended with an independent
snapshot of A. Rejection restores the policy/control bytes and A's masks, locks,
notes, cursors and PCM while retaining A's identity and lease count. Restore is a
no-throw move plus pointer rebinding (checked at compile time), rather than an
allocation attempted after failure. Invalid/self/leased-source restores reject.
This relation assumes main-thread exclusive sound mutation and no admitted SDK
commands with external queued effects. A future asynchronous sound or capture
backend needs its own command/disposal transaction; restoring software bytes
alone would not retract an already published hardware effect. Immutable archive
cache additions and the host output epoch are outside the DS-observable state.

The original sound work remains a partial staging view, not a persistent global
SND_WORK/heap/handle implementation. This batch admits the real mask service;
NNS_SndCaptureStopReverb still rejects. No implicit capture initialization or
successful sequence start is inferred. Tests establish concrete ownership,
allocation and PCM cases, not ARM7 timing, envelope, mixer or audio-output
refinement. The existing preview temporary-BGM API is not an SDK pause/resume
adapter; restored preview notes nevertheless obey current locks and priority.

### Original sound software-owner refinement (staged)

The latest sound checkpoint supersedes the earlier capture-import boundary.
Original SDK capture/player/heap algorithms are compiled for ARM32; original
handles plus metadata are mapped. The persistent archive/embedded-heap owner and
active device transport are still absent. No whole sound-scene equivalence is
claimed. The resource loader's software owner is separate from the external
DSAudio preview player domain until an explicit driver transport connects them.

For the admitted scene prefix, extend state by original static sound bytes Q,
original partial work W, control words C and borrowed audio A. Under exclusive
main-thread admission, no active SDK command/callback publication, and no mapped
archive/heap prefix, a failed scene preserves (Q,W,C,A), including allocation
policy, intrusive lists, handles, capture/fader/lock state and external PCM.
Successful scalar software operations follow original ARM code. The generated
static inventory covers every compiler object in the selected sound units; its
size checks and rollback tests are evidence, not a machine-checked proof that an
arbitrary future sound closure mutates no additional state. Once heap bytes or
external command effects are admitted this rollback precondition is insufficient.

An original sound-heap suballocation is a logical lifetime within a live guest
arena, not a separately allocated native region. Its 32-byte alignment, saved
levels and reuse are governed by the executed original sound/frame-heap algorithms.
Load/clear retire logical allocations and may reuse their **numeric guest
addresses**; ordinary DS RAM remains addressable throughout. Stale raw DS values
therefore may observe replacement data. Native borrowing of a suballocation must
end before its original disposal callback/load/clear. Permanently tombstoning
those interior guest addresses would change DS behavior. The arena's native
allocation identity and registry lifetime do not change when its original heap
frontier moves. Heap tests verify this reuse and save-failure behavior without
changing native allocator placement or inventing a Nitro heap implementation.

SDK handle release detaches a handle from a sequence slot and does not stop the
slot. Original shutdown/free removes lists, returns the player heap and invalidates
the attached handle. These laws require valid original lists and owner lifetime;
raw guest slot address reuse does not preserve the old sequence identity. Tests
exercise all 16 slots and original detachment/free/retry. They do not establish
rendered sequence arbitration, callbacks or asynchronous disposal. Original
callback-backed heap destruction requires a sound-command fence and archive
invalidation transport; it remains unproved and unadmitted, rather than returning
success for an empty emulated device.


### Original ARM7 command-service refinement (bounded admission)

This checkpoint supersedes the earlier absent-archive/transport preconditions.
Let the sound state be S = (W, Q9, Q7, H, C, T, I9, I7, A, F), where W is the
complete original SND_WORK (including its heap), Q9/Q7 are all compiled mutable
ARM9/ARM7 SDK objects, H is shared work, C is the original game control state,
T holds callback registrations, I9/I7 are independent IRQ enable states, A is the
borrowed native audio owner, and F contains the admitted filesystem cursors.
H is one coherent guest byte region: the original CPU-specific shared pointers
must resolve to that same lifetime, never two copied representations. Q7 includes
the original driver work, eight-message ring and sequence cache. The generated
object inventory and compiled layout assertions cover this admitted closure.

The 256 original nodes partition into free nodes, producer-held nodes between
Alloc/Push, reserved nodes, and submitted list nodes awaiting ARM9 retirement.
Original `SND_CountWaitingCommand` computes 256 minus free minus reserved; during
an Alloc/Push gap it therefore includes producer-held nodes, not only submitted
ones. The original outstanding-list count is separately bounded by eight; its
waiting-list ring has nine slots. Native code does not recreate these algorithms.

Under valid original queue/heap/ABI invariants, live read-compatible sequence/bank
backing, exclusive admission and no external device effects: delivery appends a
list to the original ARM7 queue without completing it. Original CommandProc
executes each command in linked order and only then advances H.finishCommandTag
once modulo 2^32 per list. ARM9's original reply algorithm subsequently advances
its local finished epoch and recycles that list. A sender cannot reuse nodes
between these stages. The original modular tag comparison is retained; at most
eight outstanding lists stay within its unambiguous serial-number window.

An unsupported command/import/device access fails admission. Restoring S restores
both CPUs' queues, allocation/track/handle/heap state, shared status/epochs, source
associations, IRQs and native PCM, preserving the native owner's identity/lease.
Result output is committed only after successful original blocking flush, dispatch
and retirement; a backpressured nonblocking flush cannot be treated as successful
admission merely because its ARM function returned normally. Polling
a future tag with no pending command is an explicit unsupported-progress boundary,
not a fabricated completion. Rollback cannot undo already accepted external
hardware/DMA/PCM effects; such effects remain unadmitted. This is a conditional
state-transition contract supported by executed tests, not an equivalence proof
of arbitrary SDK closures, concurrent/reentrant schedules or physical sound timing.

Command preparation retains the original sequence/bank guest addresses until the
original player/track teardown or invalidation. Original STOP frees track identities
and clears shared player status; it need not erase dead cursor bytes. Tests clear
active users before retiring borrowed fixture backing, and verify original track
reuse/exhaustion. An active stream's backing cannot be retired merely because its
PREPARE command node was recycled: driver cursor lifetime and transport ownership
are distinct. Full callback-driven sound-heap destruction, hardware initialization,
periodic sound-thread execution and device/PCM refinement remain proof/integration
obligations. No complete firmware initialization or audible field playback follows
from successful command admission alone.
