# Save implementation and acceptance

The original save registry is now connected: 42 normal records in the original
SaveData owner and six separately managed extra records. The persistence-kernel gate passes the registry, initialization, durability,
recovery and ABI checks below. Whole-game Continue acceptance still expands
with the admitted field/application systems. Record retention does
not establish that all gameplay which produces those records is ported.

## Executed behavior

- `test_save_manager` runs original directory/record initialization, normal
  save and Continue validation. It saves an empty party, then a full six-mon
  party and all 540 PC slots containing real encrypted Pokemon, plus nonzero
  coins and IGT. Every one of the 42 complete payloads is compared after reopen;
  opaque-record byte markers are persistence fixtures, not gameplay execution.
- All six original extra-record initializers run with surrounding canaries.
  Each extra writes/reads through the original mirrored APIs; the five journaled
  extras also write, persist their normal-save generation references, reopen and
  read through the original journal API. Hall of Fame retains its original lazy
  extra-record lifecycle.
- Injected normal and extra write failures preserve the live owner and allow
  retry/read recovery. Corrupting the latest general sector causes original
  prior-generation recovery. The flash test checks bounds, durable reopen,
  closed-device rejection and malformed-file preservation.
- `test_save_c_records` checks complete PLAYERDATA initialization, original
  SysInfo RTC initialization, independent Nitro epoch/leap/calendar expectations,
  18 original encoded PC box names/wallpapers and all 540 initially empty slots.
- `test_save_game` saves/continues the bedroom with an empty party and a real
  gameplay-produced post-rival state, preserving party, Bag, map, position and
  follower. A fresh executable process then loads that save and advances the
  field. The connected opening still passes for all three starters.
- The native viewer has Save and Continue controls. Save follows Mom's original
  unlock flag, rejects busy states and keeps one cartridge path per session so
  separately stored extra records cannot be silently lost through Save As.
  Continue retains the previous session through the existing Back mechanism.
  Native smoke saves after the rival-name/Egg scene, reopens and renders the
  continued field. File chooser dialogs are not automated by that smoke.

The live IGT now runs the original `unk_02018380.c` elapsed-time controller and
`AddIGTSeconds` against a monotonic host timer. Controller state belongs to each
field session, binds the current story owner on every update and keeps no stale
story pointer. Script publication preserves time accumulated while its candidate
was running. Save samples the clock before copying the owner, including when no
frame has rendered; Continue retains saved IGT and starts a fresh elapsed baseline.
Offline wall time is not added to play time. The original 999-hour cap behavior,
including its documented early saturation, is retained.

`test_play_time` checks subsecond boundaries, minute rollover, owner replacement,
independent sessions, fresh Continue baselines and the original cap. The connected
save test checks real elapsed time through save/reopen without an intervening
frame and verifies that publishing an older candidate cannot roll IGT back.

Offline time now executes the original day/minute dispatchers from
`unk_02055418.c` on Continue and on idle field updates. The complete original
consumer closure is connected: daily flags, badge polish, friend/roamer RNG,
Pokerus, lottery, WiFi history, Apricorn trees/drinks, persistent phone state,
Safari encounter aging and Pokeathlon reset. Original phone counters/Baoba
trigger production and Shaymin evening/new-console resets execute too. This
now feeds original automatic trigger-call selection and the live Bug Contest minute counter. Incoming-call and contest integration are described below.

`test_field_time` checks leap-day rollover; independent state assertions for
flags, badges, party, friend/roamer RNG, Apricorns and Pokeathlon; repeated and
subminute samples; backwards clocks; hardware/offset changes; the original
24-hour penalty and its expiry; evening Shaymin reset; Baoba's phone trigger;
and independent sessions. Safari aging uses all 12 original encounter members,
generated through the original assembly template. A late missing-resource
failure preserves every owner byte and RNG state and permits retry. An active
Bug-Catching Contest requires its actual volatile owner; a saved active flag without
that owner still rejects rather than fabricating contest state.
The connected Save/Continue test also ages a real cartridge's RTC by two days
and verifies native Continue clears a daily flag and ages Safari areas before
play resumes, without a false console-change penalty.

The latest full-suite checkpoint passes 74 CTests. Normal nurse healing now also survives durable
Save/Continue for every starter; see [battle runtime](battle-runtime.md). Seven focused healing/blackout and existing
prop-animation checks also pass with ASan/UBSan instrumentation; the final
private-avatar change was followed by six focused healing/blackout reruns.
Original signed-shift UBSan diagnostics remain; this is not a sanitizer-clean
claim. Native AppKit opening-route smoke passes with both home and Center recovery.
Door/transition/prop/healing checks also pass with sanitizer instrumentation;
the entrance scenario rejects truncated and invalid scale-curve resources. The
window smoke includes visible Center entrance opening before its fade. It does
not exercise the National Park route or the contest capture-choice presentation.

`test_save_recovery` exercises every observed write command in first, ordinary,
single-box and all-box saves, all six mirrored extras and all five journaled
extras including normal-generation publication. Each command gets zero-byte,
one-byte, halfway, last-byte-missing and full-write-reported-failed cases, plus
sync failure. It also checks original asynchronous cancellation, allocation
failures and corruption of individual/both copies. This covers command boundaries
and representative torn prefixes, not every possible byte or physical flash
failure. Original journal rejection of unmatched prior-generation extras remains.

The [independent ABI audit](save-abi.md) compares 35 native C payload rows and
104 recursive layouts with an ARM witness, classifies 13 opaque original ARM
payload rows, and executes all 48 linked registry size callbacks. This closes
that layout comparison, without claiming retail-cartridge interoperability.

Automatic trigger calls now use the original timed scheduler and standard script
2047, including original application/fade/audio sequencing. The connected Baoba
scenario validates trigger retention during private preparation, clearing on
publication, quest state and the area-report controller's original text slots.
Random generic contacts, outgoing calls and the full Pokegear menu remain open.

The original Bug Contest owner now supplies party backup, encounters, scoring,
keep/replace and restoration. It is volatile, as in the original game; saving is
inhibited while it exists. Scripts clone it with their private save candidate,
and minute updates bind the same live owner. Connected command scenarios cover
cancelled entry, real entry, timer, judging/text, exit and captured-mon insertion.
A component scenario carries the owner across an original bedroom stair warp
and map-resume script, then restores the complete party on exit. This verifies
map transaction ownership, not legal contest travel outside National Park.
Battle scenarios cover Sport Ball capture, deferred ball publication, stale-owner
rejection and the 20-minute battle-entry limit. Full entrance/travel/timeout and
judging narratives through National Park still need connected route evidence.

Ordinary wild captures now publish party/PC, Bag, Pokedex, profile and statistics
through one checked battle transaction. Scenario tests cover full-party insertion,
full active-box overflow, stale/duplicate publication, and original durable
save/reopen. Actual after-tutorial checkpoints complete a first catch with the
original five-Ball gift for all three starters. Met location, terrain, OT and ball
are checked. Original post-wild-battle Bill/Oak notifications run before the next
script/save boundary. Pokemon nickname entry and full battle item selection
remain open; current ordinary capture supports Poke Balls and the original name.
Home blackout recovery also reconstructs the field through Mom's original
standard script, preserves IGT and saves/reopens the healed party and money loss.
Center blackout recovery also completes its original standard script for all
three starters, including the healing-machine task, and saves/reopens the healed
party and money loss. Ordinary Nurse Joy entry/healing is connected; full character animation remains
open; machine texture/palette animation is connected.
See [battle runtime](battle-runtime.md).

## Ownership and compatibility boundaries

StoryData embeds the original pointer-free SaveData; original getters and the
native inspection getters now address its complete table-owned records. Private
script candidates copy the complete owner. The transition driver exports its
complete original local-field record at save preparation, and the original
saved-object exporter serializes live objects. Continue reconstructs original
serialized object fields through the existing native actor/resource boundary.
Temporary flags and runtime RNG belong to native continuation state rather than
the cartridge format; ordinary Continue clears/reseeds them.

The original manager retains slot/CRC/counter/PC-write and extra-record format
logic. Two original bounds predicates are reordered in generated build-local
code to avoid out-of-bounds native reads; upstream remains unmodified. Backup
I/O is a reusable bounded 512 KiB flash service. It verifies and syncs writes
before success, including directory sync for a new file. Scoped manager calls
use a heap-owned private candidate, contain original fatal error paths and leave
live state unchanged on failure. Original process-global Frontier/WiFi mirrors
are loaded from the selected owner before commit, avoiding cross-session BSS
contamination.

Original WiFi initialization calls missing SDK account services. The locally
written `ds_account` compatibility service supplies the 64-byte temporary local
identity and CRC/dirty lifecycle. Layout evidence comes from the public
[pret/pokediamond DWC account decompilation](https://github.com/pret/pokediamond/blob/master/arm9/asm/DWC_account.s);
reference source remains ignored under build/save-audit. Host entropy replaces
console entropy. The independent record fixture uses Python zlib's CRC32 result
and checks every byte for corruption. No network account registration or online
service is implemented or reported successful; no SDK source is committed.

## Remaining whole-game Continue integration

1. Finish application coverage around the connected Continue/time services.
   Native Continue now invokes original console-change checks, berry clock reset,
   Shaymin reset, button-mode selection and day/minute processing; IGT also runs.
   The full original Continue overlay/field startup is still not executing.
   Trigger-driven incoming calls and active Bug Contest owners now execute in
   connected scenarios; generic calls and full contest route integration remain. Button-mode selection updates the original System;
   the native keyboard/menu input flow is not the original DS input application.
   Firmware birthday setup is not ported; the host preserves saved birthday
   metadata rather than inventing one.
2. Expand recovery evidence when new save flows introduce command sequences.
   Current command-boundary, corruption, cancellation, partial-write, incremental
   PC and allocation checks pass; physical-device failure is not simulated.
3. The per-record native-versus-ARM layout comparison passes. Independently
   produced cartridge interoperability remains unverified; it is not established
   as a prerequisite for the native pre-credits target.
4. Broaden field Continue reconstruction and prove moved NPCs, puzzles,
   subsequent door/header crossings and every admitted save location. Current
   reconstructed-field evidence covers the tested bedroom/post-rival states.
   The complete original field startup/object-manager controller is not running.
5. Complete save application lifecycle coverage. Active scripts, battles,
   transitions and menus currently reject saving. Original pre-save event 4 is
   an optional active-overlay callback and must be dispatched or rejected when
   an admitted application requires it; arbitrary mid-script/battle snapshots
   are outside ordinary original save semantics.

The required reconstruction/callback services have one shared-root owner.
Generic phone, full contest travel and firmware UI are feature-specific integration
work, not missing save payloads. Independent chapter playthroughs still require
their own predecessor saves and connected acceptance; scenario preparation can
proceed without reimplementing persistence.

## Time ownership and host policy

The time service uses a private complete SaveData candidate and scoped binding
of the original manager singleton and one RTC sample. It restores the prior
owner on every exit; no temporary candidate remains bound after release. The
phone manager belongs to the field session, and its record pointers are rebound
for each transaction and cleared before returning. Failed updates restore phone
counters, RNG, allocations and System button mode without consuming the saved
RTC baseline. Existing field/prop programs retain weak explicit-failure fallbacks;
programs linked with the save service use the original Safari functions.

Idle field updates run before a new interaction and defer while scripts,
applications, battles or transitions own private state. This prevents later
script publication from overwriting daily state. All elapsed time is processed
when the field becomes idle; arbitrary mid-application saves remain rejected.
One complete transaction is considered per distinct wall-clock second, and
unchanged records do not advance the story revision.

A stable, locally administered six-byte identity derived from the macOS host UUID
replaces the DS MAC address; it is cached per process and serialized by the
original system-info function. The host timezone's UTC offset supplies the RTC
offset boundary. A different host/offset on Continue takes the original
24-hour-penalty branch. This is a native host policy, not a physical DS firmware
RTC-offset implementation; manual host clock adjustment is handled by the
original forward/backward calendar rules. Saved birthday metadata is retained.

The next shared save dependency is broader original application and field
reconstruction: generic phone calls, complete contest travel, normal field-object
restoration and required lifecycle callbacks. The command-boundary recovery and
per-record ARM/native ABI checks above now pass.
