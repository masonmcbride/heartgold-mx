# Parallel implementation tree

The implementation target is the original new game through eight Johto badges,
Lance, Hall of Fame and first credits. Saving is the first acceptance gate. This
plan partitions implementation responsibility; it does not claim the chapters
already run or can be played independently.

## Two graphs, one ownership tree

Keep these relations separate:

1. **Implementation dependencies:** a script needs a command handler; that handler
   needs a battle/application/field service; that service needs original state
   and platform services. These determine shared implementation ownership.
2. **Progression dependencies:** an event requires badges, flags, items, a prior
   event or a valid predecessor save. These determine legal scenarios and the
   order of connected acceptance runs.

A map warp is a progression/navigation edge, not a reason to merge every map's
implementation. A shared mutable save record is a shared service boundary, not
permission for concurrent chapter scenarios to mutate the same save file.

The ownership tree is:

```text
Root: complete original persistent state + common execution services
  Violet / Falkner and opening
  Azalea / Bugsy
  Goldenrod / Whitney
  Ecruteak / Morty
  Cianwood / Chuck
  Olivine / Jasmine
  Mahogany / Pryce
  Blackthorn / Clair and Radio Tower
  League / ending
```

The root contains all implementations consumed by multiple children, plus their
transitive prerequisites. It also owns shared integration/build files. It need
not finish every service before chapter investigation and scenario preparation
can begin. A child is admitted for integration only after its required root
contracts execute successfully. The complete save gate precedes chapter gameplay
integration regardless of how many children currently reference a save record.

There are nine gameplay children because the League is a separate package after
the eight Gym/narrative packages. The table in [Johto playthrough](johto-playthrough.md)
defines their event ownership and cross-chapter boundaries.

## Algorithm

Start with authoritative original map headers, event manifests, script entries,
standard-script routing, commands and their implementation dependencies. Retain
source identity and input hashes for each extracted node/edge.

1. Assign each scoped event entry exactly one chapter owner. A bank shared by
   several chapters does not imply duplicate ownership of its entries. Resolve
   ambiguous and unassigned events before calling the partition exhaustive.
2. Build a directed implementation graph. A node can be original code, a resource
   bank or a host service. Label structural edges, conservative inferred edges
   and unresolved edges separately. Record progression/state dependencies in a
   separate graph.
3. Collapse strongly connected components of the implementation graph. A cycle
   is one ownership unit: splitting mutually dependent state/services between
   children does not create independence.
4. For each component, find all consuming chapters through transitive dependency
   traversal. Give a single-consumer component to that chapter. Hoist a component
   with several consumers to their lowest common ancestor. With this flat tree,
   that ancestor is the root. Always put the complete save lifecycle in root.
5. Close root over its prerequisites and repeat ownership propagation to a fixed
   point. A root service cannot depend on a private child implementation. If
   extraction reveals such an edge, hoist the prerequisite or correct an
   incorrectly classified progression edge; never suppress it to make the tree
   look independent.
6. Produce child worklists containing private implementations, owned event
   acceptance, required root contracts, predecessor-state contracts, unresolved
   references and exclusive writable files. Reject duplicate implementation or
   event ownership. A shared file is assigned one integrator even when its
   functions serve multiple children.
7. Recompute after integration. A newly discovered common dependency is promoted
   to root and removed from child implementation worklists. It becomes an
   explicit service request; children must not invent parallel adapters.

This yields disjoint *implementation owners*. The progression graph remains a
DAG with branches and joins where original predicates permit them; cycles in
navigation or repeatable events are not an error. A finite tree cannot remove
real story prerequisites or prove that arbitrary chapter saves are compatible.

## Extraction limits

Use the existing assembler/preprocessor path in `scripts/script_assembly.py` and
`scripts/prepare_interaction.py` for macro values, compiled resources and standard
script routing. Extend existing audits instead of introducing a second game
resource compiler. Bank-level command inventories conservatively overapproximate
all branches and all entries in a bank; they do not establish executable reachability.

Map/header references and explicitly resolved command operands are structural
evidence. Service-family classification is a work assignment aid, not proof that
a command's full dependency closure executes. Computed calls, dynamic script
selection, native callbacks, time/phone events and opcode semantics need explicit
resolution or an unresolved marker. Original compiled handlers may still import
unsupported SDK/native services. Neither source parsing nor successful assembly
is an acceptance test.

Until event ownership, indirect references and optional accessible content are
accounted for, reports must call the result a conservative partial partition.
Do not treat an empty unresolved list from a bounded extractor as proof that
nothing is missing beyond that extractor's declared domain.

## Executable ownership audit

Run:

```sh
python3 scripts/audit_chapters.py --require-assigned
python3 tests/test_chapter_partition.py
```

The audit reads the exact original map-header records, ordered script/event
manifests, script macro definitions, opcode-handler table, standard-script
registry and save registry. It emits ignored `build/audit/chapters.json` and
`chapters.md`. The JSON includes input hashes, ownership and dependency edges,
per-worker worklists, reserved writable paths, boundary contracts, navigation
edges and unresolved scope. `scripts/johto_chapters.json` is the reviewed scope
manifest; it is an ownership policy, not recovered game logic.

The current inventory accounts for all 540 original map headers: 316 have a
scoped or root-quarantine owner and 224 have an explicit exclusion reason. The
13 maps in the unresolved sections have a root investigation owner; they are not
silently treated as unreachable. The report includes all 48 registered save
entries, even where no child currently references their systems. Whitney's and
Morty's gym maps have explicit child ownership overrides within shared cities.
Multi-phase city/theater/tower events still need entry-level refinement.

The tests cover private leaves, shared dependency hoisting, dependency cycles,
root prerequisite closure, missing-node rejection and actual source accounting.
Every implementation edge is checked: a root node may depend only on root, and
a child may depend only on itself or root. Navigation edges are not fed into this
ownership algorithm. This validates isolation within the extracted graph; it does
not prove a complete function/import closure or executable chapter independence.

Child worklists currently reserve scenario/document files. They are preparation
jobs, not permission to edit shared runtime files or an assertion that all nine
chapters can now be implemented without root changes. Complete save and runtime
contracts remain explicit dispatch gates.

## Root acceptance and parallel dispatch

Complete persistent ownership follows [save inventory](save-inventory.md) and
[original save boundary](save-runtime-boundary.md): all 42 normal and 6 extra
records, original initialization/lazy creation, live getters, field export,
normal Save/Continue, durable transport, integrity/recovery, clock handling and
process restart. No placeholder records or chapter-specific save formats.

Root service contracts also cover field/player/follower movement and transitions;
script/task/application lifecycles; trainer/wild battle, Pokemon ownership and
recovery; common menus; resource loading; input/render/audio/timing. Each contract
states supported operations, original owner, explicit failure behavior and
executed evidence. Chapters depend only on the contracts they actually use.

Dispatch each worker with:

- Its event IDs and private implementation nodes, with explicit exclusions.
- Exclusive editable files and read-only shared interfaces; use isolated worktrees
  and separate build/output directories during implementation.
- Required root contracts and missing-service requests.
- Input/output predicates on original game state, and gameplay-produced predecessor
  saves identified by provenance. Give each run its own save file.
- Acceptance scenarios, failure/alternate-order cases and evidence obligations.

A child needing a shared change reports a contract gap to the root owner. The
root owner implements and validates it once, then supplies the change to every
consumer. Concurrent root work can itself be split by exclusive subsystem/file
ownership, but one integrator owns shared build and state integration files.

Integration acceptance is staged: private tests, chapter execution against root,
then composition from predecessor saves, and finally a continuous new-game-to-
credits run with save/restart points. Verify original predicates, one-time events,
loss recovery, retained state and player/follower presentation. Compilation,
execution and native-window observation remain separate evidence columns.

## Scheduling consequences

Save records/initializers, save-manager transport, field-save/Continue and
independent save acceptance can be investigated concurrently under their existing
contracts. Their complete integrated result is the first gate. Chapter workers
can inventory and prepare scenarios in parallel during that work.

Once required root services pass, private chapter implementation can proceed in
parallel. Connected gameplay integration still follows original progression:
Violet through Ecruteak, the western/eastern branch predicates and medicine
handoff, Radio Tower/Blackthorn prerequisites, then the League/ending. Do not
approximate these conditions as a forced Gym order. The original scripts decide
when each scenario becomes legal.

The gain from this method is measurable: one implementation of each shared
service, no duplicate adapters, explicit unresolved dependencies before dispatch,
and chapter changes that can be reviewed and integrated without overlapping
implementation ownership. Completing the graph is a planning result; completing
its contracts and connected runs is the gameplay result.

## Concrete queue for the available workers

With four agent slots, keep one root integrator and at most three workers active.
The nine children are queued jobs, not nine simultaneous writes to this checkout.
Use isolated worktrees/build directories for implementation; the current graph
and manifest describe ownership before creating those checkouts.

1. **Save gate:** dispatch record/initializer closure, original manager/flash
   boundary and independent save/continue acceptance to three workers. The root
   integrator owns migration of live story/field state and shared build files.
   Workers report interface requirements before changing shared declarations.
2. **Root service closure:** use each chapter's required root nodes and observed
   unsupported imports to group common gaps by subsystem. Give each shared
   implementation one worker; chapter workers may prepare event/scenario worklists
   concurrently. No extra chapter-specific backend is allowed.
3. **Private chapter jobs:** dispatch up to three admitted children at once.
   Each uses read-only root interfaces and its own output/save paths. A missing
   root service returns to the shared queue; it is not patched in a child.
4. **Composition:** run the actual predecessor-save chain and legal branch orders.
   The integrator accepts private changes only with their connected evidence;
   independent chapter tests do not replace the new-game-to-credits run.

This queue is intentionally gated by evidence rather than dates or an unsupported
one-iteration completion promise. The next implementation work remains the full
original save lifecycle, not parallel invention of nine different state owners.

## Current next dependency

The save lifecycle, capture, home recovery and Center blackout recovery now have
connected execution evidence. Center recovery reuses the original healing task
and shared prop texture-animation service. Ordinary nurse entry still needs
greeting/trainer-card queries and the original follower handoff; battle switching
and item applications remain shared blockers for general trainer admission.
Chapter preparation can proceed now. Admit implementation separately for each
child once its actual service contracts pass; a complete root is not required
for unrelated child preparation.
