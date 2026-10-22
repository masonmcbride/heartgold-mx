# Original one-shot prop animation

Elm's New Bark farewell now completes with visible opening and closing lab-door
cycles, his original movement/dialogue, phone registration and gate-condition
update. The connected bedroom-to-farewell scenario passes for every starter.

The existing prop/model renderer now accepts per-instance joint poses. It records
original model, instance transform, material binding and triangle ranges while
loading the land; animation updates replace those same triangles. It does not
add a second door over the closed one. Changed meshes are staged together before
publication. Removing an animation restores the original model geometry.

`prepare_props.py` follows the call closure of the original one-shot load, play,
unload, completion-query and frame-update entry points across
`field/map_prop_animation.c` and `field/overlay_01_02204004.c`. Their unchanged
functions run with typed native arenas and graphics handles; the original DS
pointer-sized arena allocations do not run. Build-local provenance records every
selected function. Other automatic/time-of-day/bicycle prop controllers remain
unconnected.

Original script handlers 307–311 execute. Their native field boundary locates
an animated original prop in the original coordinate rectangle, validates every
animation frame against its actual model, and binds its original animation-list
members to original one-shot playback. Unsupported curves, ambiguous selection,
missing resources, duplicate tags and invalid wait targets fail admission.
Native wait scheduling resumes scripts only after original frame/loop control
reports completion. The original assembly field-controller lookup and TaskManager
wait wrapper are not executed; native selection/scheduling are explicit boundaries.
The original assembly `GetDoorSE` selector now executes through the movement
guest, reading the selected prop's original animation-list record.

`ds_joint_animation.c` is a reusable bounded graphics decoder, separate from the
HeartGold controller. It supports NSBCA pivot rotations and constant/every-frame signed Q12
translation/scale curves. Scale poses are admitted only on single-node models;
child scale-compensation semantics remain unsupported. Packed basis rotations,
model fallback and reduced-rate/interpolated sampling fail explicitly. Format
research was checked against [Apicula's joint reader](https://github.com/scurest/apicula/blob/master/src/nitro/animation.rs)
and [pivot representation](https://github.com/scurest/apicula/blob/master/src/nitro/rotation.rs).
The existing SBC evaluator applies these transforms to the actual model nodes.

Animation state belongs to the private script scene. Continuation forks rebuild
the original pointer graph with independent resources and copy frame/loop state;
probing cannot advance the live animation. Root failure discards its animation
owner, and the next render restores the retained land instance.

`original_prop_animation` verifies original seven-update completion of the
eight-pose door resource, midpoint cloning, rendered open/closed differences,
identical closed/closed-again/restored images, rejected-pose atomicity, truncated
resources and invalid curve offsets. The original opening route additionally
executes two opening/closing cycles per farewell for all starters. Build-local
closed/middle/open frames were generated and visually inspected. These checks
also pass under ASan/UBSan; the native AppKit smoke check passes.

The next connected dependency is Route 29's tree and Apricorn actor state and
presentation. It still rejects entry; the successful farewell does not establish
encounters, battles, phone calls, saving or the catching tutorial.

## Automatic building doors

The native travel staging now opens an entrance prop before starting its fade.
It searches the original inclusive door rectangle `[x-1,x+2] × [z-1,z]` across
its intersecting land chunks, using each prop's original animation-list door
classification. Ambiguous or unsupported animated doors reject travel explicitly.
There is no hardcoded Pokémon Center model mapping in the runtime.

The selected original opening animation runs through the same original one-shot
owner as script doors and the healing machine; original `GetDoorSE` selects its
sound. The outdoor scene remains visible at neutral brightness until playback
reports completion. The open pose is retained through fade-out and released on
map commit or cancellation. Saving is inhibited while travel owns this animation.
The transition probe's source snapshot remains untouched before commit.

Cherrygrove Center's actual door is model 15, with ten-frame opening/closing
resources using X translation and X scale. The connected test checks its original
sound, nine-update completion, endpoint transform, changed rendered geometry,
closing/restoration and visible opening before travel to map 69. Existing house,
stair and script-door checks remain regressions. Native UI smoke approaches the
Center and enters/exits with keyboard input. Exit evidence is captured in
`build/evidence/center-door-exit-ui.png`.

Destination door props are prepared against incoming resources before map commit,
using the original exit rectangle `[x-1,x+2] × [z,z+1]`. For south-facing door
arrivals the host schedules the phases of `ov01_021E9374`: hide the player while
opening during fade-in, show the player, execute original movement command 13,
place the follower at the player through the original position setter, then play
closing animation 1 and its original sound selection. Control and saving remain
inhibited until both fade and closing finish. The threshold step does not increment
ordinary input step statistics; current location is synchronized afterward.

The connected Center round trip verifies changing exit geometry, the real threshold
step, unchanged statistics and exact restoration to the static closed field. The
house round trip now expects the same original step out of the doorway. All 72
CTest scenarios and native AppKit smoke pass; door/transition checks also pass
with ASan/UBSan instrumentation (existing upstream UB warnings remain).

The host schedules these phases; it does not execute the complete original
entrance/exit TaskManager chain. Special camera paths remain incomplete.
