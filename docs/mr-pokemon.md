# Mr. Pokemon checkpoint

Current continuation: the first rival now runs as a visible field battle and
finishes its original post-battle departure for all three starters. Both win and
allowed loss resume. See [checkpoints](checkpoints.md) and the latest
[battle checkpoint](battle-runtime.md#rival-fieldscript-application-connection).
Earlier limits below record the preceding checkpoint.

This checkpoint is extended by [Elm's phone call](elm-phone.md): the panic call
now completes, and the next rejection is the rival's TrainerBattle command.

The original bedroom route now completes Mr. Pokemon's introduction for all
three starters. The original script gives the Mystery Egg, fades out, heals the
party, fades in, gives and enables the Pokedex, enables Mystery Gift, registers
Oak's number, walks Oak out and publishes the original rival/Elm/Route 30 setup.
The blackout destination is retained as Cherrygrove; blackout execution remains
unported. No progression flags are supplied to bypass the script.

On leaving the house, Route 30's original Elm panic-call event rejects opcode
430 before publishing. The Egg, Pokedex, party, bag and phonebook remain intact;
the panic-call flag remains unset. The next shared dependency is the original
Pokegear phone-call application and its return to field scripts, followed by
encounters and battles. Free exploration remains available independently.

## Reused controller and native services

The existing bounded Thumb engine runs unchanged `FadeFunc_00`, `FadeFunc_01`
and their exact brightness-controller dependency closure from
`asm/unk_0201010C.s`. Original signed interpolation, frame cadence, final value
and allocation-release phases execute. Native initialization supplies the
original FadeWork fields. The original global fade manager and SDK video
register access do not execute. Original script FadeScreen/WaitFade handlers and
their polling callback run unchanged through a typed private scene boundary.

Guest allocations use one bounded private environment; division and master
brightness cross explicit host boundaries. Work/environment records are
copyable, with guest-local addresses, so preflight, continuation forks and
independent sessions cannot share a live fade. Unsupported wipe types, colored
fades, overlapping fades and invalid durations reject rather than succeeding.
Currently admitted brightness fades use black or white. Original hardware-window
reset requests reach an already unmasked native display.

`ds_brightness` is a game-independent RGBA compositor using DS brightness levels
-16 through 16. It applies after scene, objects and text, preserves alpha and
retains the final level. The real scene ticks advance the original controller;
WaitFade does not use a guessed delay. Unjoined scene work rejects End.

The original HealParty dependency closure restores HP, status and move PP in
the existing encrypted Pokemon/party owner. GivePokedex and CheckPokedex use the
retained original Pokedex. Opcode 573 executes the original Mystery Gift setter
against a retained original SysInfo record. RTC, console identity and full
SysInfo startup remain unconnected; only the admitted Mystery Gift field is
used. SetSpawn executes the original local-field setter, with its result copied
into the private story candidate and retained across travel. This is not full
save-array, blackout or disk-save integration.

Scene-command routing now derives membership and operand widths together from
the selected original handler table, avoiding another manually maintained
opcode list when this boundary grows.

## Verification

The connected scenario starts in the original bedroom and runs through the
entire house event for each starter. It checks actual restored HP/status/PP,
Egg/Pokedex acquisition, Oak's contact, Mystery Gift and spawn state, both fade
directions, a completely black rendered frame, and rejection/rollback of the
following phone event. Test damage is applied to exercise healing; no story
progression is seeded. Build-local fade-out, black, fade-in and completed-house
frames were generated and visually inspected.

Focused checks exercise original fade rounding/cadence, white/black endpoints,
private continuations, invalid inputs, native alpha preservation, save-owner
preflight isolation, cloning, publication and stale rejection. These take about
0.2 seconds together. All 35 normal and ASan/UBSan CTests pass; the native AppKit
smoke check also passes. The connected house scenario is driven through the
actual C application/renderer; the native-window smoke is not a complete manual
bedroom-to-house run.

Original music, scripted effects and fanfares now play through the shared
[native audio renderer](audio.md); hardware timing/fidelity is not established. Pokedex
menu browsing, phone UI, encounters, battles and original startup remain absent.
Upstream remains unmodified; all generated assets and evidence stay ignored.
