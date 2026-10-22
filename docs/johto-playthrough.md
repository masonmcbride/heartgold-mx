# Save-first Johto playthrough

The target is a continuous new game through eight Johto badges, Lance, Hall of
Fame and the first credits, before postgame Kanto exploration. The League approach
crosses into Kanto geographically. Saving is the first implementation priority:
chapter work must use the same complete original persistent owner, not accumulate
chapter-specific snapshots or inject story flags to reach its starting point.

The refreshed root-service status and next implementation order are in
[the dependency plan](dependency-plan.md). The complete persistence kernel is
implemented; broader chapter-specific Continue behavior still needs execution
evidence. This is a proposed ownership partition, not evidence that these chapters execute.
The opening scenarios and development checkpoints do not establish general save,
map, trainer, inventory or later-story support. See [opening route](opening-route.md)
and [checkpoints](checkpoints.md) for their narrower guarantees.

## Foundation before chapter integration

Use the original `gSaveChunkHeaders` in `src/save_arrays.c`, the additional save
structures and save lifecycle in upstream `src/save.c` as the persistence authority.
Include all registered records and auxiliary save regions, including systems whose
UI or gameplay is not connected yet. An opaque retained region is not evidence
that its initialization, mutation or resume behavior works. Inventory those separately.
No record may disappear merely because the opening route never touches it.

The save acceptance gate needs original new-game initialization, supported normal
save-point preparation, disk write, process exit and load/resume, integrity and
recovery behavior, and failure without live-state loss. Verify original structure
layout and pointer boundaries rather than dumping native objects. Inventory the
original pre-save/post-load callbacks and field-object export/import as well as
record bytes. Separate durable game records from rebuildable render/task resources
and development-only continuation snapshots. Do not promise arbitrary mid-battle
or mid-script saving unless the original game supports that save point.

Shared runtime owners remain shared throughout all chapters:

| Owner | Responsibility |
| --- | --- |
| Save/state | Original records, new game, save/load, clock-derived updates, save preparation and resume |
| Field | Player/follower motion, ledges, terrain, portals, objects, puzzles and map lifecycle |
| Scripts/applications | Original command execution, tasks, menus, cancellation, private publication and unsupported-service failures |
| Battles/Pokemon | Encounter/trainer setup, AI, capture, items, switching, experience, moves, evolution, party/storage and blackout recovery |
| Presentation/platform | Input, graphics, audio, timing, resources and durable host file boundary |

A chapter requests a missing service from its owner. It must not implement its
own Bag, battle controller, save format, movement controller or story-specific
success stub. Save completeness is necessary for a valid game; it does not by
itself implement the producers and consumers of those records.

## Save work that can proceed concurrently

| Lane | Exclusive implementation ownership | Integration requirement |
| --- | --- | --- |
| Record owner and initialization | Original registry, all normal/extra record layouts and original size/init closures; migration from selective StoryData | Every registered record has an executed initializer and one live owner; original getters reach the persisted bytes |
| Save manager and flash transport | Whole original save.c boundary; bounded durable flash I/O, callbacks, cancellation, errors and locking | Original slot/CRC/counter/recovery behavior executes against the complete registry; file success means durable completion |
| Field save and continue | Original pre-save object/location synchronization, application save restrictions, normal Save/Continue UI and reconstruction | No detached traversal/warp/object state is lost; callbacks and temporary runtime resources are reconstructed safely |
| Independent acceptance | Per-record retention cases, crash/fault/corruption scenarios, process restart, clock handling and original new-game/continue scenarios | Checks validate actual original behavior and UI; fixtures never replace missing original initialization or progression |

These lanes have explicit dependencies. They can investigate and prepare changes
concurrently, but cannot declare integration complete independently. Shared save
interfaces are agreed from the original declarations before simultaneous edits;
one owner handles each integration file. Chapter scenario preparation may run
alongside them, while chapter admission waits for the save gate.

Saving is complete only when all 48 registered entries are accounted for, including
original lazy/extra-record creation semantics, all persistent nested state survives
legitimate saves and restart, and interrupted/failed writes preserve recovery.
A fresh game with no Pokemon, full party and boxes, mail/held items, coins/playtime,
field object state, offline clock updates and Hall of Fame are explicit cases.
Original restrictions on save locations/application state remain authoritative;
this target does not require an arbitrary instruction-level emulator snapshot.

## Chapter ownership

The sets below partition event and acceptance ownership, not all source functions.
Shared maps and helpers have one owner; other chapters declare a dependency.
Assign individual script entries when a map hosts several story phases, such as
Goldenrod's first visit and Rocket occupation. Do not assign whole banks twice.

| Set | Content owned | Incoming boundary | Outgoing boundary |
| --- | --- | --- | --- |
| Opening / Violet / Falkner | Existing opening, Route 30/31 continuation, Violet, Sprout Tower, first Gym and post-badge Egg-related events | Original new game | Zephyr Badge and original exit-route prerequisites acquired |
| Azalea / Bugsy | Route 32, Union Cave, Route 33, Slowpoke Well, Kurt, Azalea Gym, rival and Ilex Farfetch'd/Cut progression | Violet outgoing boundary | Hive Badge; original Ilex progression opens Route 34 |
| Goldenrod / Whitney | Route 34, Day Care access, first Goldenrod visit, Gym, SquirtBottle, Route 35/36 and Sudowoodo progression | Ilex exit | Plain Badge; route to Ecruteak opens |
| Ecruteak / Morty | Route 37, Burned Tower/rival/legendary release, Dance Theater's first event and Surf acquisition, Ecruteak Gym | Goldenrod outgoing boundary | Fog Badge; original prerequisites for western/eastern branches |
| Cianwood / Chuck | Western approach, sea crossing, Cianwood events/Gym, medicine acquisition and Fly gift | Ecruteak branch boundary; original Surf requirements | Storm Badge; medicine available for lighthouse continuation |
| Olivine / Jasmine | Route 38/39 and Olivine first visit, lighthouse/Jasmine setup, medicine delivery and Olivine Gym | Ecruteak branch boundary; medicine delivery depends on Cianwood content | Mineral Badge; lighthouse quest resolved |
| Mahogany / Pryce | Eastern approach, Lake of Rage, Red Gyarados, Rocket HQ and Mahogany Gym | Ecruteak branch boundary and original traversal prerequisites | Glacier Badge and HQ completion |
| Blackthorn / Clair | Radio Tower takeover and Underground, eastern progression/Ice Path, Blackthorn Gym and Dragon's Den | Actual original Rocket trigger and branch prerequisites | Rising Badge plus original post-Gym progression |
| League / ending | Elm's late gift, Kimono Girls, HeartGold Bell Tower/Ho-Oh progression, Routes 27/26 and required traversal, Victory Road/rival, League, Hall of Fame and credits | Eight badges and actual original narrative prerequisites | Reloadable completion state after first credits |

These are work packages, not an instruction to force one player's route. Cianwood,
Olivine and Mahogany overlap in availability; lighthouse setup precedes medicine
delivery, and obtaining medicine need not require defeating Chuck first. Validate
legal branch orders from the original script conditions. The Radio Tower trigger
must be extracted from the original flags/badge logic, not approximated as
“Pryce defeated.” Required field moves and their badge permissions likewise come
from original predicates. Optional caves, gifts, trades, daily events and accessible
side branches need explicit map/event ownership; absence from this table is not
permission to silently omit them.

## Generate the actual worklist

Extend existing resource/script preparation and audit tools rather than maintaining
a second handwritten function allowlist. Available authoritative inputs include:

- `include/constants/maps.h`: semantic map names and original map IDs.
- `files/fielddata/eventdata/zone_event/*.json`: actor, background, coordinate and warp references.
- `files/fielddata/script/scr_seq/*.s` and header scripts: actual entry points,
  command operands, flags/variables, battle calls, gifts and application requests.
- Original map-header tables and script manifests used by `scripts/prepare_interaction.py`.
- `scripts/audit_native.py` and its generated reports: compilation and provider
  evidence, explicitly separate from runtime execution.
- `scripts/audit_maps.py`: load/rasterization evidence, explicitly separate from
  reachability, collision and playable event execution.

Produce a build-local event/dependency inventory with one chapter owner per event,
shared-service edges, read/write flags and records, command imports, object/puzzle
controllers and required resources. Resolve macros/includes through the existing
assembler/preparation machinery; raw regex text matches cannot establish branch
conditions or executable reachability. Reject unresolved or multiply owned entries.
Classify optional and time-dependent events explicitly. Resource preparation across
all banks is not command implementation coverage.

Use that inventory to form parallel jobs by missing shared dependency and chapter
scenario, with each file's implementation owner clear. Several chapter tests can be
prepared in parallel; integration still follows the actual dependency graph. A
single parallel iteration cannot be claimed to complete the game before this
inventory and executable evidence exist.

## State contracts and acceptance

A chapter boundary consists of an actual gameplay-produced save plus assertions:
map/location/facing and object state; party/boxes and every Pokemon field; badges,
HM ownership/eligibility, Bag/key items, money; original quest variables/flags;
Pokedex, contacts and quest gifts; respawn location; elapsed/time-dependent records;
and all other original registered records retained by the save owner. The contract
must state changes made by the chapter and retained inputs. Avoid a hand-authored
“chapter number” that replaces original progression predicates.

For each chapter, run from its predecessor's saved output, exercise its required
original events and accessible shared services, save, terminate, reload and continue.
Check no repeated one-time gifts or battles, missing actors, stale puzzle state,
lost inventory/Pokemon changes or accidentally advanced flags. Add targeted loss,
blackout, full-party/full-Bag, cancellation and alternate-order scenarios where
applicable. Check player and follower together through terrain and transitions.
Use normal UI for the connected route; fixture-only entry tests remain supplementary.

Maintain three distinct evidence columns: original code compiled, code executed,
and observable behavior verified (including native-window presentation). Run
appropriate sanitizers and native UI checks at runtime integration gates. Preserve
upstream and keep proprietary/generated evidence ignored. Only a continuous
new-game-to-credits route with durable reloads can establish the expanded target;
independent chapter passes do not establish their composition.


## Curated chapter testing teams

Each chapter also gets a separate six-Pokemon test roster, scaled to its point
in progression. Choose a fun mix of types, roles, abilities and stage-appropriate
moves: physical/special attackers, useful status/utility and defensive options.
Species/evolutions, levels and moves must make sense for the chapter, with any
exception explicit. Use the team to exercise targeting, switching, varied moves,
status, fainting/revival and party/storage behavior, not just easy victories.
Record species, level, ability, moves and intended coverage in the chapter's
acceptance contract. These fixtures supplement the continuous gameplay-produced
route; they never stand in for legitimate acquisitions or progression flags.

Concrete species, levels and original level-up moves for all nine separate feature
rosters are recorded in [chapter-test-teams.md](chapter-test-teams.md) and exported
with each chapter worklist by `audit_chapters.py`. These are fixture contracts;
only Violet currently has a corresponding executed six-mon engine scenario.
