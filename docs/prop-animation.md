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
Door sound playback and the assembly `GetDoorSE` selector remain unconnected.

`ds_joint_animation.c` is a reusable bounded graphics decoder, separate from the
HeartGold controller. It currently supports NSBCA pivot rotations sampled every
frame, with explicit identity translation/scale. Packed basis rotations,
translation/scale curves and interpolated sampling fail explicitly. Format
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
