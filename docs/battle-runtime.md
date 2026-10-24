# Original battle execution checkpoint

Current checkpoint: wild field battles and the visible first-rival cutscene now
complete. All three starters pass from the original opening, through the rival
fight and back through its original post-battle departure. Both win and allowed
loss resume. Earlier sections record historical checkpoints; see the latest
sections for current evidence and limits.

2026-10-20: a headless single wild battle now completes using the original native
battle controller, move-script interpreter, damage/status logic, fainting and EXP
task. This is not a playable field battle or a completed rival fight.

Run `./build/heartgold_battle_probe` for its messages and result. The current
scenario creates a level-5 Cyndaquil against a level-2 Rattata and returns the
original victory outcome. Enemy creation is a probe setup, not original field
encounter selection. Run `ctest --test-dir build -R original_battle_execution
--output-on-failure` for the connected regression scenario.

## Connected services

- Original overlay selection/presentation phase is retained in `unk2445`.
  Native modules remain resident; no DS executable overlay loading is claimed.
  Original assembly `ov12_02237ED0` establishes the phase transition contract.
- Each battle owns an original SysTask queue, party copy, message-format buffers,
  input history and tracked battle allocations. Cancelling destroys the private
  task arena and outstanding allocations. Original helper calls exchange global
  LCRNG state at the owner boundary, restoring the caller's RNG on every exit.
- Original message-side adjustment and tag-to-parameter dispatch run unchanged.
  Native providers convert original nicknames, read name banks and expand the
  supported UTF-8 resource placeholders. Unknown controls fail. The tested path
  has real encounter, stat-change, fainting and EXP text, not message numbers.
- Direct EXP-task messages use real acknowledgement state separately from the
  original controller packet queue. Original task completion is polled through
  the native printer boundary. No global field task queue is borrowed.
- Original emitted party updates retain HP, status and PP; original EXP/EV
  routines update the owned Pokémon. Original record-increment packets use the
  original statistics helper and controller-side condition. Hit effects use the
  original effectiveness-to-sound mapping; spatial sound panning is not supplied.
- The wild move-selection boundary follows the original opponent assembly's
  uniform selection among legal moves. Trainer move selection explicitly rejects
  missing original trainer AI; it does not reuse this wild behavior.

## Evidence and limits

`tests/test_battle.c` executes two interleaved battles for each starter, checks
matching results, victory, enemy fainting, EXP gain, PP consumption, unchanged
caller party/RNG, invalid input rejection and cancellation while the original EXP
task owns a message. This caught and prompted correction of global RNG leakage.
The new test passed in 0.46 seconds under the parallel full suite; all 40 tests
passed in 12.41 seconds. The native field UI smoke also passed.

ASan reports no memory errors in the battle scenario, including EXP-task
cancellation. UBSan reports original signed-shift issues: existing Pokémon and
Pokédex cases plus newly exercised battle code using signed high-bit flags and
EXP masks. These remain unresolved; the sanitizer run is not clean. Original
upstream sources remain unmodified.

The probe acknowledges visual packets without rendering battle surfaces. It does
not establish working sprites, animations, party/bag menus, capture, fleeing,
loss/blackout, level-up UI, move learning, real trainer setup/AI, field admission
or result publication into the live story. Those branches must be exercised
before admission. There is a frame cap and test timeout; full per-instruction
budgeting and malformed battle-script validation remain incomplete.

## Level-up checkpoint

The connected regression now places each starter one experience point below
level 6. The unchanged original EXP task completes level-up, friendship/mood
updates, stat recalculation, both acknowledgement-driven stat pages and learning
the original level-6 move into an empty slot. Tests check increased maximum HP,
the actual learned move against the original learnset, both pages, matching
interleaved results, caller input/RNG preservation and cancellation while the
previous-level stat allocation and summary surface are live.

The native summary adapter captures the original window/row lifecycle and frame,
palette and layer requests. It emits text pages in the headless backend; it does
not load those requests into DS VRAM or render their original frame graphics.
Unknown layouts fail. Original message callbacks start the level-up fanfare;
its wait control rejects acknowledgement until completion. A real sequence
renderer test observes 194 blocked frames, without opening an audio device.
The caller advances the borrowed audio renderer once per simulation frame.

The focused battle test passes in 0.05 seconds on the latest run. All 40 tests
pass (13.02 seconds total), and the native field UI smoke passes. ASan reports
no memory errors; the previously recorded original signed-shift UBSan findings
remain. This extends the earlier checkpoint: active-battler level-up and move
learning into an empty slot are now exercised. Bench-mon nameplates, replacing
one of four moves, battle sprites/animation and a visible battle UI remain absent.

Next shared dependencies: original trainer setup/AI, then the left battle/right
controls view and field suspend/return.

## Session setup and first-rival data checkpoint

`hg_battle_new_setup` now copies the real party, player profile, Bag, options,
Pokédex and game statistics into its private owner. Wild setup can take the
original encounter-generated opponent party directly, preserving its complete
Pokémon records. The species/level constructor remains a headless probe helper.

Trainer records and parties are generated using the original JSON/templates and
original record types. All 738 records are prepared build-locally. Original
`CreateNPCTrainerParty`, gender/PID generation, IV calculation, moves, form,
friendship and capsule routines execute unchanged. The existing original trainer
message-table archives are reused. Preparation is not a claim that every trainer
is admitted: doubles and long/compressed names still reject explicitly.

`tests/test_battle_setup.c` creates trainer 496 against Chikorita, 497 against
Cyndaquil and 495 against Totodile. It checks their actual level-5 opposing
starter, zero difficulty-derived IVs, original friendship and reproducible party
records. Each first-rival introduction uses the original trainer message and
name bindings and reaches the explicit `original trainer AI` boundary. A stricter
test caught and eliminated an erroneous wild announcement in the native adapter.
No rival turn, victory or loss is claimed.

The wild session scenario checks preservation of the original opponent party and
that a Pokémon belonging to the supplied profile receives ordinary EXP rather
than the traded-Pokémon bonus. `hg_battle_commit` publishes party/profile/Bag/dex/
statistics together only after completion and task/message quiescence. It checks
the initial caller snapshots and rejects stale state, unfinished/failed battles
and repeated publication. A stale-Bag scenario verifies rejection leaves the
caller's party unchanged. Audio remains borrowed and advanced by the caller.

All 41 tests pass (11.59 seconds total on this run). Both battle scenarios run
under ASan without memory-error reports; the recorded original UBSan signed
shifts remain. The existing native field UI smoke passes. Upstream remains
unmodified; generated game data stays ignored.

The live field does not yet launch battles. The next connection must suspend its
exact session, use original encounter generation or original trainer setup,
present the battle on the left and controls on the right, publish once, restore
field music and resume that session. Original trainer AI is still required before
admitting the rival path. These tests do not replace that playable acceptance.


## Live grass encounters and field return

The native field now calls the unchanged original walking-encounter eligibility,
rate, ability/item modifiers, time-slot and Pokémon-generation helpers after a
completed step and after coordinate events. Original encounter JSON/templates
produce the build-local encounter archive. The opening integration uses daytime
land encounters; radio, swarm, repel, flute, water and fishing are not connected.

The battle suspends the exact field owner. A native two-screen view displays
original front/back Pokémon artwork, names, HP, moves, PP and messages, with
Fight/Run and move selection on the right. W/S changes selection; Enter/Space
confirms or acknowledges. The original controller executes damage, escape, EXP,
level-up and empty-slot move learning. Finished battles publish their private
snapshots once and resume the same field session with map music restored.
Artwork is static first-frame presentation with a native background; original
battle animations, capture, Bag/party menus and alternate forms are not admitted.
Defeat reaches an explicit blackout-recovery boundary and rolls back the battle
with a visible error; it does not silently heal or claim a completed blackout.
The rival remains blocked on original trainer AI and field admission.

`field_wild_battle_return` walks actual Route 29 grass through application input,
renders both screens, fights, checks increased EXP and verifies the same story
owner and map after return. `original_battle_execution` also checks the original
Run command and escape message. The complete 42-test suite passes, including
the opening route for all three starters (12.55 seconds on this run). Earlier
headless-only descriptions above record previous checkpoints.

The native field-window smoke also passes. Sanitized field-battle, battle-engine
and session-setup scenarios report no AddressSanitizer errors. UBSan still reports
signed shifts in original Pokémon, Pokédex and battle code; the new encounter
path also exercises the same signed-shift pattern in original `sys_vars.c:171`.
These are recorded findings, not a clean UBSan result. Upstream remains unchanged;
generated resources and visual evidence remain ignored.

## What required adaptation for native battles

The original C controller, battle-script interpreter, damage/status rules,
fainting, EXP, level-up and move learning compile and execute unchanged. Their
platform calls still need working providers; compiling a function alone does not
make the called services available.

* Graphics: DS main/sub 2D engines use VRAM banks, background tilemaps, palettes,
  sprite OAM, windows and blending. For example, original `battle_system.c`
  copies from address `0x06010000`, and `battle_input.c` calls
  `GXS_SetVisibleWnd`. The host view presents the battle state in native RGBA
  surfaces; it does not implement complete original battle graphics/animations.
* Input: original touch/button polling now consumes acknowledged native choices.
* Audio: original sequence/effect IDs go through the shared sequence renderer and
  native output rather than the DS sound service. Full cry fidelity is incomplete.
* Timing and memory: original SysTask callbacks run in an owned frame queue;
  heap/resource/message providers replace SDK services. Native 64-bit owners
  must respect original record layouts and avoid guest-pointer assumptions.
* Overlays and instruction set: `ov12_02237ED0` originally swaps overlays 7/10.
  Native modules stay resident while preserving the phase contract. ARM/Thumb
  assembly cannot be compiled as Apple Silicon instructions by the C compiler.
  The remaining trainer decision path is assembly, not a missing special-purpose
  DS AI chip. Likewise, save ownership, field return and blackout recovery are
  game integration dependencies rather than hardware components.

The native field Bag/Pokémon panels borrow the actual story inventory/party.
The existing original list-menu inventory reader now accepts a borrowed Bag,
without demo seeding or transferring ownership. Connected testing checks pause,
pocket contents, post-battle EXP refresh and safe panel destruction. Native UI
smoke checks the Bag button, Pokémon shortcut and Escape close behavior.

## Palette and viewport correction

Battle sprite PNGs contain indexed artwork, but their embedded palette is not
always the palette for that view. Original `tools/scripts/make_pokegra.py` takes
the normal palette from the first nonempty female/male front PNG and the shiny
palette from the corresponding back PNG. The native resource preparation now
follows that contract for both directions and genders, preserving original
compressed indices and transparency. The battle-mon view exposes the original
`shiny` flag, and the sprite cache key includes it. Previously, ordinary back
sprites incorrectly displayed shiny colors.

`original_battle_sprite_palettes` compares all 3,616 nonempty generated variants
against the original palette sources and checks unchanged artwork/transparency.
Connected Chikorita normal/shiny and Totodile scenarios also exercise actual
battle-state palette selection. The battle viewport now scales to available
width with nearest-neighbor sampling, retaining both screens' aspect ratio.
The 800x600 viewport is checked for full-width content rather than the previous
512x192 integer-scale island. Native health panels and ground platforms improve
framing, but these remain host presentation, not original scene backgrounds.

Next trainer dependency: `src/battle/trainer_ai.c` already provides original C
initialization (`ov10_0221BE20`) and the selection dispatcher (`ov10_0221BEF4`).
Single/double scoring continues into `ov10_0221BF44` / `ov10_0221C038` in original
`asm/overlay_10_trainer_ai.s`. The remaining work must connect that original
scoring path and controller choices; a random trainer-move substitute would not
satisfy rival admission. The AI is partly C, partly assembly.

## Original trainer AI execution

`prepare_trainer_ai.py` assembles and relocates the unchanged original
`asm/overlay_10_trainer_ai.s` (62,244 bytes including data). It uses the existing
`ds_thumb_run` engine. This adds no CPU emulator or replacement trainer strategy.
The original C `ov10_0221BE20` initializer and `ov10_0221BEF4` dispatcher compile
unchanged. Original `ov10_022205BC` chooses the command; original `ov10_0221BF44`
runs the single-battle scoring scripts and tie-breaking selection.

A bounded per-battle guest arena translates the original 32-bit BattleContext
and TrainerAIData records. Pointer fields are explicit guest addresses or
validated party/Pokémon tokens, never host pointers. Generated field transfers
check widths, critical offsets and original BattleMon stride. New ambiguous
fields/bitfields reject. Native helper boundaries run the existing original
ability, held-item, damage and type-effect code. Each call has a million-Thumb-
instruction limit, and cancellation releases the arena with its battle owner.
Unsupported imports fail by name; trainer switch/item applications and doubles
remain unsupported. Assembly preparation does not establish all AI branches.

Trainer speech uses the original trainer/message table and original message
bank 728, including the first rival's half-HP and defeat messages. Unbound
message controls reject instead of displaying placeholder text.

`original_battle_session_setup` now completes trainers 496/497/495 against their
matching starters. Each executes five command selections and five scored move
selections (102,803 / 102,803 / 103,438 Thumb instructions in the recorded run).
Two interleaved owners produce identical messages, final parties and instruction
counts, while preserving the caller RNG. Victory publishes EXP and party changes
through the existing one-time snapshot boundary. The original level-5 opponents,
IVs and deterministic setup are still checked. Wild battle/session checks remain
in place.

The full 46-test suite passes at this checkpoint. Sanitized trainer and wild
battle scenarios report no AddressSanitizer errors; the original-code UBSan
signed-shift findings remain. No upstream sources are modified.

**Remaining playable-rival dependency:** original script opcode 213 still rejects
atomically. It needs an asynchronous battle application boundary that suspends
the exact script candidate, presents the battle, publishes into that candidate
once and resumes the original win/loss branch. The engine test does not claim
this cutscene, blackout recovery or the post-rival sequence works.

## Rival field/script application connection

Unchanged original `ScrCmd_TrainerBattle` and `ScrCmd_CheckBattleWon` now execute.
The admitted setup boundary accepts the three first-rival IDs, a single opposing
trainer and the original `BATTLE_TYPE_11` allowed-loss option. Other trainer/task
configurations still reject before the cutscene. The script yields while the
shared native battle view owns presentation/input. Battle snapshots come from
that script's private candidate, including its real party/profile/Bag/dex/stats.

On completion, results publish into the candidate once; the real battle outcome
feeds original `CheckBattleWon` and `VAR_BATTLE_RESULT`. A loss in this original
encounter type runs original `HealParty`, matching `Task_StartEncounter`; this
does not implement general blackout recovery. Original trainer-battle statistics
are retained once. Cancellation/failure marks the script failed and discards its
candidate instead of leaving a half-published cutscene.

Admission previews both win/loss continuations privately. Those previews do not
publish outcomes, heal the live party or start battles. The live result always
comes from the battle engine. Original follower movement 56, needed for the
rival's shove, now executes through its already-assembled original entry, alongside
movements 48/55; no replacement follower choreography was added.

The connected opening test now covers all three starters through the visible
rival fight and original departure. It checks owner revision/EXP/statistics stay
unpublished during battle and checks a single trainer-stat increment at script
completion. `connected_checkpoint_resume` loads all nine generated checkpoints,
continues Route 29 through the tour, wins all three resumed rival fights, checks
post-rival does not retrigger, and exercises an actual loss/heal/resume. Native
window smoke loads a checkpoint through its picker, renders the real fight,
finishes the cutscene and restores the exact prior owner with Back.

All 47 tests pass (20.09 seconds in the recorded full run). The sanitized opening
and checkpoint scenarios report no AddressSanitizer errors. Original signed-shift
UBSan findings remain; this is not a clean UBSan result. Upstream is unmodified.

## Ordinary wild capture and storage publication

Ordinary wild field battles show Fight, Poké Ball and Run. Selecting Poké Ball
without balls or available party/PC space shows the reason; confirmation leaves
the battle at its command menu. Trainer battles retain Fight/Run. Throwing
requires a ball in the live Bag and room for the catch. The unchanged original item controller,
ball calculation and `Task_GetPokemon` execute. The native boundary supplies
animation timing, Pokedex-entry acknowledgement and keeping the original name.
Other ball/item selection and entering a Pokemon nickname are not connected.
Unsupported nickname input rejects; the native view explicitly offers keeping
the original name rather than silently selecting a result.

The battle copies PC storage alongside its party, Bag, profile, Pokedex and
statistics. `hg_battle_commit_storage` checks every initial record before copying
any result. The older publication API rejects a battle that requires storage.
Original PC insertion chooses the active/next available box, restores PP and
sets box-modified flags. A private original Bag cursor owns the item controller's
last-used-ball state. It is temporary application state, not a cartridge record.

The field supplies its actual map and player position. The existing original
terrain selector and capture memo code produce met location, terrain, trainer,
ball and date metadata. Wild-encounter statistics accrue in the same private
result. Original Bill-full-PC and Oak-dex-progress notifications execute at the
next idle field boundary, including before Save; changed notification records
advance the story revision before another script can publish.

`original_wild_capture` checks party insertion, full-party PC insertion, overflow
from a full active box, first/already-owned Pokedex branches, ball consumption,
private-state isolation, stale/duplicate publication rejection, and original
save/write/reopen retention. `field_wild_capture_return` and its other-starter
variants load the actual after-tutorial checkpoints. With their original starter
and five gifted balls, they walk into grass, render and finish a catch, retain
the same field owner, and verify party, met location/terrain, OT, ball and dex.
The shared travel driver still selects Run when passing through grass.

General battle Bag/party menus, nickname entry, blackout recovery, broader
trainer applications and the later gym routes remain unfinished. Capture adapters
are native presentation, not the original DS capture/Pokedex graphics pipeline.

## Blackout recovery checkpoint

Ordinary wild defeat now executes the original loss script, including prize-money
loss and its two white-fade requests. Original message-format slots persist across
messages: the final `TAG_NONE` blackout announcement deliberately reuses the
player-name binding. Clearing those slots per message was a native boundary bug.
The native renderer supplies the fade; no DS palette hardware runs.

`Task_Blackout` now executes unchanged against a complete private save. Original
Thumb routines resolve the death and special-spawn records from the existing
spawn table. The original local-field relocation function updates current and
previous positions. Original healing and follower-flag/number clearing run too.
The host supplies timed audio, a white blackout message surface, acknowledgement
and resource reconstruction. The requested original standard script must pass
private preparation before replacing the field owner. Saving remains inhibited
during recovery; current IGT carries into the replacement owner.

A real defeat from the Route 29 checkpoint now returns home, completes Mom's
original movement, dialogue, fades and healing fanfare, then regains field
control. Money and the healed party survive an original save/reopen. Native
AppKit smoke exercises this loss with keyboard input and renders the blackout
and restored field. The task scenario separately checks all 30 original spawn
selectors, cancelled/private state, both home/Center destinations and special
warp records, healing/PP, and original follow-up script selection.

Center recovery now completes the original `std_whited_out_to_pokecenter` script
for all three starter checkpoints, including avatar pose changes, give/receive
movement waits, nurse turns/bow timing, party-count-driven healing-machine task,
dialogue and return to field control. Original save/reopen retains the healed
party and original money loss. The former negative Center scenario is now a
positive connected recovery scenario; it no longer claims rollback coverage.

The original avatar transition dispatcher and selected walking/Rocket/healing
callbacks run in the existing movement guest. Unsupported bits reject before
mutation. The host loads the original sprite/model selected by the avatar;
ordinary player graphics no longer remain fixed to model 69. Both genders'
walking/healing/Rocket resources are checked. Character presentation currently
uses a directional pose: the original give/receive and nurse-bow controllers
execute their timing/state changes, but their complete character animation
frames/effects are not yet reproduced.

The unchanged `PokecenterAnimCreate` / `PokecenterAnimRun` instructions execute
in a separate bounded guest arena, reusing the existing Thumb engine and the
same assembly/relocation implementation as movement. The original matrix-offset
helper and party-lead selector run too. Typed host boundaries provide validated
models, native prop instances, audio and the existing original prop-animation
owner. Guest dictionary proxies contain only offsets/tokens, never native
pointers. The original controller owns the six ball offsets, placement intervals,
animation start/completion, fanfare wait, removal and completion.

Healing uses NSBTP texture/palette animation rather than NSBCA joints. The shared
bounded reader now validates its original sparse keyframes and material bindings;
the renderer stages texture changes before publication and restores base
materials when animation ends. Texture-only updates reuse geometry. The healing
scenario checks one through six balls, changing machine presentation, original
animation/fanfare waits, clone cancellation and restored rendering. No original
assets or generated guest bytes are tracked.

## Normal Pokémon Center interaction

The original map nurse entry and `std_nurse_joy` now execute the greeting,
Yes/No confirmation, avatar give/receive waits, follower handoff, healing-machine
animation/fanfare, HP/status/PP restoration and closing dialogue. Refusal retains
the party byte for byte. All three starters, a fainted lead with another healthy
party member, the Pokérus notification/phone trigger, durable Save/Continue and
immediate interaction after Continue or Center blackout recovery are checked.
The native UI smoke now interacts with Nurse Joy after real Center recovery.

The original Trainer Card star routine executes in a separate bounded query
arena with this session's real flags, Pokédex and Frontier record. Its original
Frontier statistic reader and native Pokédex completeness closure run too;
query fixtures check the saved star conditions, including five stars, without
changing route progression.
Time-of-day comes from the original saved-clock function and hourly table, with
all 24 hours checked. Nonurgent phone triggers retain their original save logic;
urgent call scheduling remains rejected at this script boundary.

`ScrCmd_599`, its original task initializer, callback, direction selector, held
movements and eight-frame follower arc execute unchanged. Its copyable 72-byte
environment belongs to the script scene, including menu preflight clones. The
native renderer supplies follower shrink/return presentation using the original
task and Receive counters; the DS NNS retraction effect is not executed. Full
character give/receive and nurse-bow frame selection is still incomplete, as
noted above. A restored field can rebuild the follower through the existing
original initializer when healing changes the first healthy party member.

This work also removes two shared lifecycle restrictions: dialogue/choice waits
now pause bytecode while scene movement continues, and a stopped script retains
its scene until outstanding movement completes. A direction queued with native
confirmation is preserved for menu selection. Co-located followers after field
entry, Continue or recovery remain recalled until walking separates them, so an
immediate nurse interaction observes a hidden follower rather than asserting
that it is adjacent. These changes apply beyond the nurse script.

The full suite passes 73 CTests. Focused nurse/time/healing/door checks and all
three Center recovery→nurse cases pass with ASan/UBSan instrumentation; existing
original signed-shift UB remains, so this is not a clean UBSan claim. Evidence
logs are ignored under `build/nurse-complete-tests.log`,
`build-sanitize/nurse-final-tests.log` and `build/nurse-final-ui-smoke.log`.

General trainer battles still need party switching and battle item applications;
their admission remains restricted. Contest defeat uses a different original
task and remains explicitly unsupported.


Battle/Mart input correction: shop frames consume queued directions without
clearing physical key history. Holding W/S or a quantity direction therefore
moves once until release, as the other native menus do. The connected Mart
scenario holds keys for 120 frames, checks every frame, then verifies release
and repress. Capture scenarios now throw through directional input plus Enter
queued in the same simulation frame; battle confirmation preserves that input.
Ordinary no-ball field battles retain the visible, unavailable Poké Ball option
and reject throws. This does not grant balls or bypass the original tutorial.

Failed-catch presentation: the original `BATTLE_COMMAND_SHOW_POKEMON` emitted by
`Task_GetPokemon` on breakout now clears the native capture phase when presented.
Previously it was acknowledged without restoring the opponent sprite, leaving
the ball visible through later turns. `original_wild_capture` now also executes
two deterministic failed throws, compares the opponent's rendered sprite region
with its pre-throw pixels after each breakout, retries and runs, then verifies
two balls consumed with no party insertion. The isolated low-catch-rate opponent
has Splash so its original Whirlwind cannot end this recovery scenario. Original
catch calculations and breakout sequencing are unchanged.


## Battle Bag, party selection and multi-Pokemon trainers

The native command menu now adds Bag and Pokemon while retaining the Poke Ball
shortcut and Run. Ordinary Bag selection lists HP/status/PP medicines present in
the private battle Bag. Choose a Pokemon and, for a single-move PP restorative,
a move. Esc returns through target/Bag/command layers. A no-effect selection
stays in the menu, consumes nothing and spends no turn. X items, other ball
selection and unsupported item applications are not admitted by this interface.

Unchanged `BattleSystem_RecoverStatus` applies medicine effects to both original
party records and the active `BattleMon`, including volatile status and
transformed/mimicked PP rules. This is the original battle item application
boundary, not a second field-item implementation. The adapter consumes one item
on a successful effect, records the original Bag cursor selection and returns
`BattleItemUse` to the original controller. The controller chooses action order
and executes the enemy response. Native text acknowledges application; full
original Bag/party application graphics and item-use animation remain unported.
The native HUD now displays sleep/poison/burn/freeze/paralysis badges from the
original battler status rather than rejecting the status-icon presentation packet.

Original party-menu messages supply the switch restriction, current party slots
and mandatory-replacement mode. Active, fainted and Egg selections are refused;
forced replacement cannot be cancelled. Original Yes/No prompts cover using the
next Pokemon and switching before a trainer's next Pokemon. Original controller
scripts handle switch state, PP, turn cost and field result publication. Native
single-battle message bindings retain original return/send-out text and HP
thresholds; full DS switch animation remains unported.

The existing Thumb AI image now also includes unchanged `ov12_02258BA0`,
`ov12_02258800` and `ov12_02258BB4`. These original selectors and their original
fallback choose enemy replacements through the same typed guest/native context
boundary. Falkner's actual original two-Pokemon team now completes at the engine
boundary, with AI, replacement, victory and prize-money publication. No badge,
trainer-defeated flag or field story event is injected; the connected Violet gym
route remains to be implemented. Trainer-used medicines still reject explicitly.
Original trainer-party resources admit validated zero alignment padding, including
odd-sized 10/18-byte party records; Bugsy's original three-Pokemon member now
loads, without claiming a completed Bugsy battle or story route.

`original_battle_menus` uses a separate six-Pokemon early-Violet feature roster:
Chikorita 11, Mareep 11, Hoothoot 10, Wooper 10, Geodude 11 and Gastly 10. It uses
original level-up learnsets/abilities/stats. The Falkner engine fixture uses
Geodude 13, Mareep 12 and Chikorita/Hoothoot/Wooper/Gastly 11. These level-scaled,
varied fixture teams supplement the real progression checkpoints; they do not
establish legal acquisitions or replace a chapter's predecessor save.

Evidence covers actual enemy PP spent after medicine, zero turns spent on
cancellation/no-effect, paralysis cure, bench revival, PP restoration, active
party rejection, voluntary switching, faint/mandatory-replacement recovery,
private-state isolation, original trainer victory/rewards, and exact party/Bag
retention through durable save/reopen. Actual grass-capture scenarios for all
starters open/cancel Bag, target and party menus before catching. Native AppKit
smoke opens/renders Bag/party, cancels with Escape and still completes the
original defeat/recovery routes. Existing upstream signed-shift UBSan reports
remain; instrumented scenario completion is not a clean UBSan claim.

## Trainer event services, 2026-10-22

The gameplay-produced Cyndaquil Violet save reaches Sprout Tower 2F and Sage
Nico's standard trainer script. Grouped preflight now executes original actor
movement queries, the trainer identity/phone metadata lookup, original encounter
music selection, original trainer/message table selection and the original
static-facing reset callbacks. The next rejection is ordinary `TrainerBattle`
(opcode 213, bank 953 byte 3123). This is still a failing probe: preflight withholds
the live event, so no trainer dialogue/battle victory or badge is claimed.

The native reset reads its callback/type choices from the original tables. It
admits static movements 14–17 and ordinary object types, runs the original empty
teardown, facing initializer, previous-position synchronization and type initializer,
and preserves the entire object on failure. Dynamic trainer effects, double
trainers and DS autonomous movement callback scheduling remain unconnected.
The original overlay-26 reset helper executes inside checked native module
registration/load/unload. Scene binding now supplies the actual selected NPC ID.

Trainer text uses the unchanged original `GetTrainerMessageByIdPair` traversal;
its native String boundary returns a message-bank/member reference to the existing
UTF-8 formatter and acknowledgement owner. Tests check every valid original table
pair, missing-message clearing and invalid-output preservation. Encounter music
uses the original class/region table and selector. Native audio retains the paused
BGM voice, including track position, across scene clones; failed preparation leaves
playback unchanged. Restoration is checked sample-for-sample against uninterrupted
playback. This adapts the original field sound save/resume boundary; DS sound-handle
and NNS calls are not executed.

Actor service fixtures cover private preflight, all 16 facing-movement pairs,
unsupported callback/type rejection, retry, cartridge Save/load and actor
reconstruction. These are feature fixtures, separate from the continuous route.
Ordinary trainer acceptance still requires battle/result ownership, original
WhiteOut into existing blackout recovery, the saved candle predicate, post-win
flag publication, defeated dialogue and durable gameplay-produced victory.

Verification for this service group: all 77 CTests pass, as do nine dependency
analysis tests. Actor/text/audio scenarios finish under ASan/UBSan with the
previously recorded upstream Pokedex/sys-vars signed-shift warnings and no
AddressSanitizer errors. The native AppKit smoke check passes; the recovered
field/follower scene was inspected. The compiler-resolved trainer-message
closure contains 82 nodes and no open call edges, with resource traversal checked
by the table-pair scenarios. The full trainer event closure remains open.
