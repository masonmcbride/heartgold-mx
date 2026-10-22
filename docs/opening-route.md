# Original opening route

The active target is the original player-bedroom start through the first rival
battle and catching tutorial, with the required original events, people, menus,
items and battles present. This target is **not complete**. The current connected
route now completes Elm's farewell, crosses Route 29, completes Cherrygrove's
guided tour and map-card gift, receives the Apricorn Box on Route 30 and reaches
Mr. Pokemon's house. Its complete introduction now gives the Mystery Egg, heals
the party and enables the Pokedex. Elm's incoming call now completes through the shared application lifecycle. The
return trip now runs the visible first-rival battle and completes the original
post-battle dialogue, shove and departure for all three starters. Both victory
and the original allowed-loss/healing route resume the cutscene. Returning to
Elm now executes the original police conversation, rival-name confirmation,
friend/officer departure and Mystery Egg handoff for all three starters. The catching
tutorial now executes its original automatic battle and five-Ball gift for all
starters. Its capture graphics use native adapters; ordinary Poke Ball capture now executes through party/PC publication. Pokemon
nickname entry, other battle items and later opening events remain unfinished. See
[validated checkpoints](checkpoints.md) to resume later stages.

Walking in grass now admits original land encounter checks and generation, then
a visible wild battle with Fight/Run/Poke Ball, damage, EXP and field return. See
[battle runtime](battle-runtime.md) for the executed scenario and limits. General trainer applications, blackout recovery and full battle menus remain missing; the complete opening target
remains unfinished.

## Connected execution

The default field viewer now starts in the original upstairs bedroom at (6,6),
facing south, through unchanged `Location_SetToPlayerRoom` and its original
location constant. Running shoes are absent. Original profile/variable setters
establish the starting 3,000 money, fishing record and new-game flag. The host
still supplies the inspection name Ethan, gender and deterministic session RNG;
original boot, character naming and full save-array initialization are unported.

`original_background_interaction` starts there for each of the three starters.
Its route finder uses original collision/portal decisions and actual application
input to descend the stairs, run Mom's original introductory frame event,
receive the Bag/trainer-card/save/options flags, leave the house, run the first
friend/Marill scene, enter Elm's lab, complete the introduction, choose the
starter with its original name, receive five Potions, and leave with it following.
The post-starter friend/Marill frame event then completes. Lab re-entry preserves
the same party/bag, and returning to Mom gives the Pokegear. Both answers to her
phone question run across these scenarios. No progression flags are planted to
skip events or acquisition.

The west approach completes the original farewell event with rendered lab-door
cycles and Elm's phone registration. Ordinary movement then crosses Route 29's
header, retaining the party, bag, follower, contacts and step statistics.
Cuttable trees, item balls and Apricorn trees have original template/mesh
bindings. The original Apricorn selector executes with retained fruit state;
the Route 29 tree uses the green variant and its original no-Box dialogue runs.

Cherrygrove's complete original tour runs, including follower choreography,
its house-door animation and Running Shoes gift. The shoes are absent before
the tour and survive subsequent travel. The north-exit event returns the guide
and registers the original Pokegear Map Card. Route 30's original Apricorn-man
event then opens/closes his door, gives the Apricorn Box and sets its condition.
Original movement commands 69/70 now provide object invisibility/visibility.

Mr. Pokemon's full original introduction now runs, including actual brightness
fades, party healing, Mystery Egg/Pokedex acquisition, Mystery Gift enablement,
Oak's phone registration/departure and the original rival setup. The blackout
destination is retained; blackout execution is not connected. All three starter
scenarios reach this checkpoint without planted progression flags. Leaving the
house runs Elm's original panic call and returns through the original fade/field
restore sequence. The return trip reaches Cherrygrove's rival trigger; private
preflight rejects the battle command and rolls back the triggering step. See
[Elm's call](elm-phone.md). See [Mr. Pokemon](mr-pokemon.md),
[the Route 29/Cherrygrove checkpoint](route29-cherrygrove.md) and
[prop animation](prop-animation.md) for boundaries and evidence.

## Shared lifecycle and ownership

The starter picker now uses the original application launcher and the same
TaskManager/OverlayManager frame driver as other admitted applications. Its
separate application-status substitute and fade timer are removed; scene fades
and starter fades execute the same original brightness controller. See
[shared application lifecycle](application-lifecycle.md). Elm's scripted phone call now uses the same application runtime. General
General Pokegear browsing, trainer battles and saving remain unported.

* Transition and load hooks run against private story state before incoming
  objects are prepared. Resume hooks use a private scene after object/resource
  preparation on every admitted map, including New Bark; the Elm-only gate is
  removed. Resume-produced player/avatar/equipment state survives publication.
* Header frame conditions run before field input, through original
  `GetMapSceneScriptId`. A successful frame script must clear its own condition
  before publication. Rejected automatic scripts report once per story revision
  rather than opening a new error every frame.
* Original coordinate selection runs after completed steps on every admitted
  map. A rejected mandatory event restores a same-map step. If a rejected event
  follows a committed zone change, only reversing that crossing is admitted.
* NPC, background and metatile interactions retain original priority. Unchanged
  `GetInteractedMetatileScript` and its coordinate helpers execute with typed
  native resources and original Thumb surf/rock-climb predicates. Selecting a
  field-move script does not implement that field move. Safari placement remains
  explicitly unsupported.
* Original show/hide/lock/release handlers use the existing native object pool,
  event initializer, terrain settling and sprite readers. Deletion runs original
  flag semantics; resource and pose rollback uses the existing retained owners.
  Missing actor resources reject travel rather than silently omitting people.
* Scene clones own their avatar equipment record. Root publication applies it
  only after successful completion; preflight and rejected scenes cannot mutate
  the live local-field record.

The host schedules these boundaries in the original transition → load → resume
→ frame/input order. Whole original `FieldMap_Main`, `FieldInput_Process` and
`Task_RunScripts` are still unconnected. Original NPC AI, encounters and concurrent
dialogue/movement are not established by these scenarios.

## Shared menus, sound requests and resources

Yes/No menus now validate each answer by forking the actual live continuation,
including its private scene, acquired Pokemon, variables, formatting, code arena
and child contexts. Supported answers are enabled by behavior, rather than a
nickname-specific UI exception. Mom's answers both work; nickname Yes remains
disabled because its naming application fails admission. A synthetic unsupported
branch also checks that probing/rejecting an answer cannot publish state or RNG.

Original BGM and sound-command handlers now execute against the shared native
SDAT renderer and macOS output. Effect/fanfare waits follow actual playback;
fanfares pause map music and preserve the original follow-up wait. Private scene
snapshots keep preflight silent. See [audio evidence and limits](audio.md).
Ordinary walking, walking-in-place, jump and delay families share the existing
original command table and guest services; unsupported imports still reject.

Resource preparation follows the entire original script manifest: 497 script
banks and 468 headers assemble, retaining all 965 member IDs. All 828 message
banks use original JSON order in the existing private UTF-8 adapter. This is
resource coverage, not execution coverage. Assembly runs in a bounded pool and
reuses objects only when compiler identity and dependency contents match. Failed
banks retain explicit diagnostics/empty slots; the current manifest has none.

Outdoor header changes stage story, actors, rendering and collision against
both populations. Route 29, Cherrygrove and Route 30 crossings now execute.
The crossing step updates statistics in the private destination owner; previously
updating the source statistics lost that step when destination state published.
Rejected/cancelled crossings restore the source owner and discard destination
updates. Scenario routing now queries the complete original header footprint
instead of assuming every map fits in one 32x32 chunk.

## Mom’s gift presentation

Mom’s first conversation now shows four native acknowledgment boxes: “You
received the Bag!”, “You received your Trainer Card!”, “Save button unlocked!”
and “Options button unlocked!”. Each is triggered by the original flag’s first
unset-to-set transition. An immediately following original fanfare starts with
the notification, and acknowledgment uses the same pager as ordinary dialogue.
A grant without a fanfare still displays its notification before the next
original command. Already-owned unlocks do not produce duplicate receipts.

The boxes belong to the private script candidate: preflight stays invisible,
cancellation publishes no gift, and snapshot/revision ownership is unchanged.
These are native notifications added for this port, not messages in the original
Mom script. Original menu-button graphics and full menu applications remain
unported. Ordinary items and Pokégear retain their original receipt messages.
The Pokégear receipt is original message 8 in bank 545.

`test_unlock` checks all four notices, fanfare and no-fanfare grants, input waits,
preflight/cancellation isolation and duplicate grants. The connected opening
scenario checks all four rendered receipt pages for each of the three starters.

## Original Pokegear record

The complete unchanged `save_pokegear.c` now compiles through a typed save-array
boundary into the retained story owner. Original initialization installs Mom as
the first contact and initializes the phone/rematch/savings record. Original
registration, duplicate detection and phone queries execute through script
handlers; original card registration and map-unlock setters are also admitted.
The full record belongs to each private candidate, including menu probes and
map transitions. Tests verify registration, independent sessions, travel, failed
preflight, invalid operands, stale publication and menu-branch isolation. The
connected route now registers Elm through the completed original farewell.
These are in-memory records. Elm's scripted call executes; general Pokegear UI,
other call controllers and disk saving remain unported.

## Presentation acceptance

Completing an event is not visual/audio acceptance. The opening route still
requires broader original prop animations, the original reaction-effect graphic
and palette (the current exclamation is a gold font placeholder), original menu
presentation and audio fidelity against original hardware. These remain explicit
requirements alongside gameplay; native playback now works, while font placeholders
do not satisfy visual acceptance.

## Remaining route dependencies

Wild encounters and battle/field return now execute; original trainer AI and
blackout recovery are the next battle dependencies.
The rival event currently rejects TrainerBattle opcode 213. General Pokegear
browsing and other call controllers remain unported; Pokedex browsing and full
menu presentation are also incomplete. Apricorn harvesting/animation and daily RTC updates remain unported;
owning the Box does not establish working harvesting. Saving, full audio integration and
original startup are still separate missing systems.

Build-local evidence is written with
`HG_ACCEPT_EVIDENCE=build/evidence/opening-route ./build/test_interaction` after
creating that ignored directory. No original source, assets, generated banks or
screenshots belong in Git.

## Verification

All 36 CTests pass in normal and ASan/UBSan builds at this checkpoint. The full three-starter connected scenario is an integration
check; focused tree/state tests remain the short iteration path. The full connected opening
scenario is driven headlessly for all three starters. The native-window smoke
test also passes: actual AppKit keyboard/rendering, automatic Mom introduction,
travel, paging, original conversations and rival shove/jump are exercised. Its
conversation routing now shares the C scenario planner instead of maintaining
separate hard-coded walking strings. It does not claim a complete native-window
run to the first battle/tutorial.

The default bedroom frame was generated by the native viewer and visually
inspected, alongside ignored route evidence. Upstream remains unmodified;
`git diff --check` passes. Targeted story/interaction checks are used during
iteration; full parallel suites run at the integration checkpoint.

## Return to Elm

`original_elm_return` resumes each actual post-rival checkpoint, walks back across
Route 29 into New Bark and enters the lab. Original script `scr_seq_T20R0101_002`
owns the movements, dialogue, rival naming, confirmation/retry, Egg removal and
quest flags. The native naming panel accepts original-font characters (seven for
the rival), Enter confirms and Esc returns to the original retry. The original
`CallTask_NamingScreen` / `Task_NamingScreen` / `SetName` and original misc-save
rival-name setter execute through the existing task/application lifecycle.
The DS keyboard artwork/overlay is replaced by native text entry; this is not a
claim that the full original naming overlay executes. Player/nickname entry
paths are not admitted by this milestone; keep No at the starter nickname prompt.

The shared name codec serves both the native inspection profile and naming
input. Original misc-save storage now owns both the rival name and existing
Apricorn trees. Other fields in that record do not imply their systems have been
initialized or ported. The checkpoint schema is now version 2; regenerate old
snapshots with `heartgold_checkpoints`.

The scenario checks all three starters, cancel/retry, invalid/empty/overlong
input rejection, no name publication while the private script is pending,
original confirmation text using the name, Egg removal, Elm scene 4, officer
flag, follower retention and lab re-entry without retrigger. A separate pending
naming-task destruction check verifies the original cleanup/result path. ASan
reported no memory errors; existing upstream signed-shift UBSan reports remain.
Native AppKit smoke types Silver through key events, renders the panel and
completes the Egg handoff before checking exact Back-session restoration.

The catching tutorial now connects original `SetupAndStartTutorialBattle`,
`Task_TutorialBattle`, `BattleSetup_New_Tutorial`, tutorial input callbacks and
capture calculations to the existing battle owner. `original_catching_tutorial`
checks all three starters, the automated Marill/Rattata encounter and capture,
five-Ball gift, retained player records and no retrigger. Native capture animation
handles stand in for the original graphics services. Ordinary player capture now
executes separately through the same original task, with party/PC publication;
full battle item menus remain unported. See [battle runtime](battle-runtime.md).

## Center recovery shared-service checkpoint

A real wild defeat from the post-tutorial checkpoints now returns to the original
registered Center for every starter, executes its recovery standard script and
healing-machine task, then saves/reopens successfully. The machine uses original
ball placement/timing and NSBTP texture animation through the shared prop owner.
The avatar now selects original walking/healing/Rocket graphics. Full character
action animation and ordinary nurse greeting/follower handling remain open. See
[battle runtime](battle-runtime.md) for original/native boundaries.
