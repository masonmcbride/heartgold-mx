# Elm's incoming call checkpoint

The connected original opening now completes Elm's panic call after leaving
Mr. Pokemon's house for all three starters. The native phone view displays the
original contact name and gender-selected message, including the player's name.
The original standard script rings, fades out, launches the application, waits
for it to close, restores the retained field and fades in. The same player
position, party, Egg, Pokedex, bag and contacts survive. No completion flags are
planted; the original event publishes only after its child script returns.

The return trip then reaches Cherrygrove's rival trigger. Its private preflight
now reaches TrainerBattle opcode 213, which rejects. The player step and story
revision are retained, so no partial rival event is published. Battles and the
catching tutorial remain unported.

## Shared runtime and original execution

The phone runs through FieldSystem_LaunchApplication, the original OverlayManager
init/main/exit lifecycle and the same field task/application frame driver as the
starter. The original SetPhoneCall, RunPhoneCall and application-wait/argument
cleanup handlers execute. Native ringing parameters belong to the private
script owner; original request arguments are freed after actual application
completion. Cancellation closes the private application without publishing.

The unchanged Prof. Elm call selector, GearPhoneCall_Simple, PhoneCall_Main and
original ring/hangup controller functions execute against typed native records.
The original script-definition catalog and contact/message-bank tables are
prepared build-locally with hashes and schema checks. Current admission is
limited to Elm's five simple scripted calls; the panic call is the connected
acceptance case. Ordinary outgoing calls, other callers, contact menus and
non-simple phone scripts still reject. Compiling the complete 455-entry catalog
does not establish that those calls execute.

The original Pokegear graphics overlay, touchscreen animation and DS text printer
do not run. A native phone view uses the original font, contact and messages,
with the existing input-driven pager. The shared [native audio renderer](audio.md) now plays the original ring and
hangup sequences with real completion waits. The original
hangup frame counters run; DS tone glyph/window drawing is not reproduced.
The native restore boundary validates the actual retained scene owners instead
of loading a DS graphics overlay that was never unloaded.

## Shared ownership fixes

Standard child scripts now clone the parent's real scene, including objects,
resources and FadeWork. During the child, presentation reads its live scene;
a successful return transfers ownership back and rebinds the parent's candidate.
Failure/cancellation destroys private work. This connects standard-script fades
and application return using the same services as ordinary map scripts.
Active application continuations cannot be shallow-copied into menu probes.

The rival initially stands in the neighboring Route 29 header of the same
outdoor matrix. Scripted NPC positioning/movement now accepts supported terrain
in that loaded matrix rather than requiring the current map header. Player
script movement retains its map-header restriction; ordinary player crossings
still use the transition owner. The rival route proves this correction through
private original movement execution before the unsupported battle command.
Original GetStarterChoice now supplies the battle branch from retained state.

## Verification

All 36 normal and ASan/UBSan checks pass, plus native AppKit smoke. Focused phone
checks cover both player genders/names, waiting for input, cancellation, failed
preflight after the call, stale-owner rejection and publication only on commit.
The three-starter connected scenario covers fades, actual phone rendering,
return position/brightness, retained records, return to Cherrygrove and rejected
battle/step rollback. Build-local phone and returned-field frames were visually
inspected. The connected scenario uses the actual C application and renderer;
the AppKit smoke is not a complete manual opening playthrough.

Upstream is unchanged. Generated data and evidence remain ignored. Native
presentation is functional; full original Pokegear appearance and hardware audio fidelity remain
separate acceptance work.
