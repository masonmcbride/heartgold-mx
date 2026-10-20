# Original battle execution checkpoint

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
