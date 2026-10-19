# Persistent original script decisions

Current lifecycle and connected opening evidence are maintained in [the opening route](opening-route.md). That checkpoint supersedes the older bank-count, Elm-only scheduling and menu restrictions below.

## Shared standard-script routing

Resource preparation now reads all 30 ordered entries of original
`sScriptBankMapping`, validates every row, and assembles their 29 distinct
script banks alongside the eight local New Bark banks. Mapping source hashes
and rows are recorded in ignored `interaction_provenance.json`. All message
includes used by these scripts are packed through the existing UTF-8 adapter.
The resource loader resolves message banks by their original numeric IDs,
bounded by the generated upstream bank extent; missing resources still fail.
There is no per-message-bank native allowlist.

Root standard IDs and `CallStd` children now share one checked bank-entry
resolver. Original table order determines the bank and message group; index
gaps cannot fall through into another bank, and entry offsets cannot target
the definition table. The former two-gift child allowlist is removed. Existing
original context-count, wait and restart handlers still run. A child ending
without returning rejects immediately; a root cannot run the child-only restart
instruction, whose original shift requires a positive context ID.

Executable scenarios cover all eight ordinary original scenery scripts against
two different current maps, their original text/message selection, and the
additional `std_give_item_verbose` routine with private bag mutation and atomic
root publication. The existing three-starter application route remains covered.
Original pocket-name formatting type 27 is now admitted with a bound buffer.

This prepares shared routines; it does not implement every opcode they request.
Town-map, healing, saving, battle and other unavailable services still reject.
Child scene ownership is still limited, and local-ID `CallStd` is unsupported.
The native field input path still lacks original metatile-interaction selection,
so the scenery scenarios establish script execution, not input-driven bookshelf
or trash-can interaction. That original selector, broader field scheduling and
their service dependencies are the next integration boundary. Older milestone
sections below describe their respective historical execution limits.

For this checkpoint, all 30 CTests pass in normal and ASan/UBSan builds
(`ctest -j 8`: 3.74 and 12.38 seconds elapsed on this host). The targeted
story/interaction pair took 0.71 seconds. A fresh native-window smoke check
could not be verified: the sandboxed launch reported unavailable macOS window
services, and the unrestricted attempt exited 1 without diagnostics. The
headless application route passes; this is not a native-window pass. Upstream
remains unmodified, and generated resources remain ignored.

Field exploration now retains one story owner across conversations and travel. Mom’s initial conversation and the New Bark woman’s conditional dialogue execute original flag checks, comparisons and branches. Original initialization controls actor visibility: the rival and lab actors appear, Cameron and the upstairs trophies are hidden, and the rival-room friend uses the original graphics variable. This remains a native inspection session, not complete original game startup.

## Executed original code

The complete unchanged `save_vars_flags.c` compiles inside `compat/story_records.c`. Original record initialization, flag addressing/check/set/clear and variable addressing execute. Native assertions check the 1,100-byte flag/variable record and 32-byte original player profile. The original static temporary-flag array is exchanged with the active owner at the native boundary, preserving independent sessions on the main thread.

`prepare_script_kernel.py` selects **77 unchanged original function definitions** and the original comparison table into ignored build-local includes. They come from `script_manager.c`, `scrcmd_c.c`, `sys_vars.c`, `scrcmd_lottery.c`, `player_data.c`, `scrcmd_17.c`, `scrcmd_strbuf.c`, `scrcmd_party.c`, `party.c` and the original inline variable helpers. Source/slice hashes and original line locations appear in `script_kernel_provenance.json`. These containing units are **not** claimed as whole-source compilation. The complete original `script.c` remains compiled and executed separately.

The admitted path executes original variable/flag services, command handlers for comparisons, forward/backward jumps, calls/returns, flag mutation, arithmetic/copy, frame waits, movement waits, name buffering, party counts, friend graphics, badges and lottery initialization. The entire original bank 149 `std_init` entry executes, including its lottery command; this does not execute the C `RunInitScript` wrapper or original boot sequence. Original profile initialization is followed by a native inspection name (Ethan) and an empty original-layout party record. The deterministic session RNG seed, profile and party are not loaded from a trainer/save file; original party initialization and Pokemon creation now execute through the connected starter task. The unpatched lottery high-half behavior is retained and tested.

All eight New Bark header banks and ten complete script banks are assembled from original macros into a partial build-local NARC with original member IDs. Empty slots are unsupported. The existing bounded NARC reader supplies both headers and bytecode, replacing the per-script native filename allowlist. Original map-header getters and `GetMapLoadScriptId` select transition hooks; original `GetMapSceneScriptId` is also executed in selector tests. Header extents, relative tables and variables are validated before original pointer-based parsing. Transition hooks now run for New Bark and the rival’s upstairs room, including original friend-graphics setup. The lab’s header-selected resume hook (map 61, entry 11) now executes during initial inspection entry and incoming portal travel; other resume/load/frame hooks are not automatically executed.

A typed scene-command adapter now executes unchanged `ScrCmd_GetPlayerCoords`, `ScrCmd_GetPersonCoords`, `ScrCmd_GetPlayerFacing` and `ScrCmd_MovePersonFacing`. The generated scene table uses original opcode indices; the native VM validates operand extents, variable IDs and scene ownership before dispatch. This replaces the host implementation of the facing query. Actor placement calls the original native `MapObject_SetPositionFromXYZAndDirection`, then executes original Thumb `sub_02060F78` and `sub_02061070` for previous-grid synchronization and static terrain settling. A host-only terrain-owner field is explicitly outside the DS layout. Missing actors, invalid directions and out-of-map placements fail admission; an absent follower retains the original coordinate-query sentinel behavior.

Tests execute these handlers both directly and through the same scene preflight/replay VM used by resource-backed scripts. They verify independent player/person coordinates, facing, exact Q12 placement and height, private scene snapshots, single publication of original variables, and complete preview discard after a later unsupported command. These are executable boundary/scenario tests, **not execution of Elm’s whole entrance sequence**. The subsequent lab-resume integration below connects starter-ball creation and that map’s resume hook. The later introduction integration connects map-61 coordinate scheduling and the original exclamation update; original effect graphics/tasks remain unconnected.

The original Thumb coordinate-event selector `sub_0203DE04` now executes against validated 16-byte records and the same native story-variable owner. Tests use the real Elm lab table to check initial/later scene selection, every rectangle boundary and invalid special-variable rejection. This is query functionality: automatic dispatch of coordinate scripts in the application is not connected yet. It does not establish execution of Elm’s complete introductory sequence.

## Native ownership and limits

`HGFieldScript` retains an original native-pointer `ScriptContext`, an immutable bank view, private candidate story records, special variables and native presentation effects. It uses a scoped native `FieldSystem` and an explicit save-owner handle; neither is the serialized DS ABI or a general `SaveData` owner. The full original `ScriptEnvironment`, `Task_RunScripts` and field-controller lifecycle remain unconnected.

Before showing an interaction, preflight executes its selected path privately through the same original handlers. It rejects unsupported opcodes, malformed operands, unimplemented formatting and exhausted budgets without publishing state. Untaken story branches are not treated as supported functionality. Actual execution then retains its context through every message acknowledgment and original frame wait. Native dispatch batches at most 32 commands per application tick, honors original yields and bounds total execution. Special variables belong to the script, not the persistent save record.

An original End makes a candidate ready. The owner publishes it only after dependent actor resources are ready, with a revision check that rejects stale/repeated commits. Canceling or rejecting a script discards its candidate. State, RNG and original temporary flags do not leak between owners. Map travel similarly stages original map-temporary resets, transition hooks, terrain, actors and local-field records before publication. Existing actors retain position and facing when flag changes reconcile the current population.

Message windows, pagination, interaction locking, sound requests and facing presentation are explicit native providers. The original `ScrCmd` drawing/locking/sound implementations are not executed. The known selection cue is captured but produces no audio. Native font pagination preserves original CR boundaries and UTF-8 characters. The legacy single-message API now delegates to this runtime with a private owner; its duplicate extraction interpreter was removed.

Scene services clone native-owned original avatars/managers/objects separately for preflight and replay. Native `ApplyMovement` parses bounded records and queues them; original `ScrCmd_WaitMovement` and `IsAllMovementFinished` use the scoped counter. Original Thumb command-table execution supplies faces, normal/slightly-fast walking, walking on the spot, direction locking and far jumps. Timings, grid/vector displacement, terrain following and jump-height tables run from the existing unchanged assembly units. This does not execute `EventObjectMovementMan` or its original task scheduler. `GetPlayerFacing` now executes the unchanged original handler through the typed scene boundary. Actor resources and original object poses are retained for allocation-free restoration if execution fails; story publication still waits for original End and dependent population preparation.

Original name-buffer and party-count commands run against scoped native owner lookups. Player names translate original profile character codes to the private UTF-8 message boundary; unknown controls and unbound tokens fail. Gender message selection is native, using the original profile accessor. These shared services enable the breeder, Elm’s default talk path and the rival’s complete two-message shove/jump interaction. Elm’s automatic introductory cutscene now runs; starter choice remains unsupported, and no story flags are fabricated to bypass them.

Live messages and the application now admit up to 4,095 UTF-8 bytes instead of 511, while the two-line page buffer stays bounded independently. A fixture runs the original name-buffer handler with Elm’s actual longer research message, selects the female-profile text, verifies the complete ending, paginates/renders every page using the original font functions and publishes only after acknowledgment and End. This removes a shared cutscene-text blocker without claiming the surrounding script or starter flow runs.

Original unsigned 32-bit branch addition needs an address boundary on 64-bit hosts. Each context owns a small execution arena: one copy of the bank, a 16-byte gap and a mirror target region. Immediately before a negative relative jump/call, the boundary temporarily relocates that operand to a positive offset into the mirror. The original handler chooses the branch and updates the original context/return stack. After exactly one command, the boundary restores the operand and canonicalizes the target into the bank. All pointer arithmetic stays within the owned allocation; the gap distinguishes EOF fallthrough from a taken branch to byte zero. Original source bytes and function bodies stay unchanged. Backward conditionals and calls now execute; infinite loops fail the same budget before publication. Raw DS-address opcodes, arbitrary standard-script calls, general formatting, menus, movement outside the admitted command set, concurrent dialogue/movement, script-driven Pokémon creation/party mutation and battles remain unsupported. Rejected interactions now show dismissible native feedback. No instruction/service is silently accepted to claim a story milestone.

## Behavioral evidence

`original_story_runtime` checks original initialization flags, the lottery vector, global RNG preservation, isolated temporary flags, integer wrap, comparisons, forward/backward calls and conditional loops, EOF/mirror disambiguation, empty returns, real frame waits, multiple message yields, private mutations, explicit publication, stale state and failure rollback. Header tests cover original selections, truncation, invalid offsets and unsupported special variables in frame tables.

Scenario tests use a reusable route finder over original collision, targeting and warp decisions, then replay each discovered step through actual application input. One scenario travels town → house → Mom → town, retains the same story owner and verifies that a persistent variable fixture changes the town woman’s original dialogue branch. Another reaches the upstairs boy. Actor tests expose Cameron through his real flag, execute the original header-selected transition script, and verify that publication removes him from the live population. The AppKit smoke test opens Mom’s dialogue and checks that story publication waits for acknowledgment.

Tests also cover validated UTF-8 player names, original name buffering/party count, all admitted directional movement commands and exact durations, original jump arcs, Elm/breeder/rival targeting and completion, rival branch fixtures, movement locking, stale-scene rollback and visible unsupported-interaction feedback. The AppKit smoke test sends keyboard input through those three conversations and the rival’s shove/jump. All **27 CTests** pass normally and with ASan/UBSan; graphical smoke passes. Ignored dialogue and jump snapshots were visually inspected. These establish connected original decisions under a native owner; complete story progression, original startup, disk saves and battles remain unfinished.

## Lab resume and script-created props

Map 61 now runs the original header-selected resume entry through the scene VM before map publication. Original `ScrCmd_MovePersonFacing` moves Elm from serialized (6,5) to scripted (4,5), and unchanged `ScrCmd_PlaceStarterBallsInElmsLab` selects 0–3 balls from original flags/party count. The native player now receives original `obj_player` ID 255 and its current map ID: leaving it at default ID 0 collided with Elm during original ID lookup. Tests verify Elm moves while the player remains at the incoming warp.

Script props own a separate original `MapPropManager` and immutable model resources, using existing NARC/model/material readers. Original `MapPropManager_LoadOne` creates the props and original `ov01_021F3C9C` draws them through the existing geometry callback. Private scene clones reconstruct their props through original accessors/load functions; failed preflight or incoming resume discards that owner. Terrain, incoming actor sprites, prop geometry and candidate story state are prepared before transition publication. Re-entry starts with a fresh prop manager, preventing accumulation. The native compositor admits fixed world-space geometry alongside camera-facing actors in one depth buffer.

Tests execute all four original ball-count branches (the two-ball branch uses an explicit count-only party fixture, without creating a usable Pokemon), verify original model 141 and its three Q12 positions, clone/free ownership, and render visible differences at three camera angles. Repeated real portal admission produces 3 → 1 → 0 → 3 balls as original flags change. A later lab-scene fixture requiring a hidden friend rejects the incoming resume and preserves current coordinates, actor resources and story revision. A headless application frame in ignored `build/evidence/elm-resume/lab.png` was visually inspected. This establishes map-entry initialization and rendered script props; the subsequent section connects the coordinate-triggered introduction. Starter selection, complete original effect/animation controllers and full startup remain incomplete. The new presentation has not been verified in a native GUI session.

## Automatic lab introduction and original party records

After a completed ordinary step in map 61, the native scheduler invokes original Thumb `sub_0203DE04` over the actual coordinate events and live story variables. It dispatches the selected original bank entry through the same private preflight/replay VM. The initial introduction runs all four original messages, movement into tile (4,7), Elm’s phone reaction and final escape-restriction flag. The assistant’s subsequent coordinate event now runs the original five-Potion gift and advances the scene; later unsupported coordinate paths still reject explicitly; other maps’ coordinate scheduling remains unconnected.

Movement command 75 now executes original command-table initialization/polling and unchanged assembly update slice `ov01_02200480`. Original update instructions supply its 6,10,12,12,10,6,0-unit arc, 30-tick hold and completion; the complete command takes 38 ticks. Native effect allocation initializes the scoped update environment. Typed handles supply position/visibility/completion/deallocation services. The host presents an original-font exclamation glyph; original effect-model loading, NNS graphics and effect task scheduling do not execute. `ScrCmd_609` now also resets an active follower through original Thumb code and its typed scene owner. Phone sound requests are captured, with no audio output.

`original_background_interaction` now starts in New Bark, enters the lab through actual portal admission, walks into the real trigger and acknowledges the complete introduction. It checks the resulting position and real story flag. The route driver drains actual cutscenes and replans after scripted repositioning, instead of writing flags to skip them. A separate scene scenario checks all four message yields and visible effect state; movement tests check every original arc sample and exact duration.

The session now uses original `Party_InitWithMaxSize`, backed by the [original Pokemon kernel](first-starter.md). All three level-5 starters can be created and added to a real party in executable tests. All three starters are now selectable in the native application, with original encounter/Pokedex updates, post-choice script and following. The [connected acceptance route](first-starter.md) passes with the original name.

The [first-starter milestone](first-starter.md) now connects original party/species and starter-choice handlers, bounded native menu input, follower control and the standard item child script. A story-owned original bag is initialized and receives the five-Potion lab gift through original bag functions. Root publication remains atomic across child completion, and child special variables/formatting are shared. Fixed operand widths for admitted original handlers now come from their original reader calls in the generator; semantic checks remain explicit. This is not the complete original `Task_RunScripts` scheduler or save manager.
