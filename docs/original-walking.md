# Original normal-walk command execution

The viewer now takes each intermediate player position from original movement code. The later [original locomotion integration](original-locomotion.md) adds the original walk/run selection branch, input history, step statistics and follower notification, and supersedes the native normal-walk-only admission described below. Its previous native interpolation happened to match the normal-walk command's eight ticks, but that duration was previously a host choice. At the current 60 Hz host simulation clock, two model units per tick across a 16-unit tile gives 7.5 tiles per second on an unobstructed path. This verifies command duration from source execution, not timing against a running DS or full-controller parity.

## Code and data actually executed

`prepare_field_guest.py` assembles the unchanged original five movement/field code units plus complete `unk_data_020FCBD8.s` and `unk_data_020FDB44.s`. At this milestone the relocated image was 22,484 bytes with 339 named import traps; the locomotion document records the expanded current image. The extra data supplies original direction-to-command mappings, `gMovementCmdTable`, and its step-function tables. Other routines referenced by those complete tables remain unsupported unless their explicit boundary services are implemented. Assembly and relocation are not execution coverage.

The connected normal-walking path executes:

- Original `sub_0205DDD4` direction resolution and scoped `sub_0205DA34` collision admission, as before.
- Original `sub_0206234C` command selection using the normal-walk base `0x0c`. The base is selected by native admission following the inspected normal-walk branch of `sub_0205D4B4`; that full controller does not execute.
- Original `sub_02062428`, `sub_02062400`, and `MapObject_RunMovementCommand` dispatch through the original command and step tables.
- Original `MapObjectMovementCmd012_Step0` through `015_Step0` and `sub_020624CC`, which initialize the original scratch state, advance logical grid coordinates and set direction, flags and movement step through original native C accessors.
- Original shared `MapObjectMovementCmd090_Step1`, which applies Q12 displacement through `sub_0206101C`, follows terrain through `sub_02061070`/`sub_02061248`, decrements the original remaining-tick counter, and performs completion bookkeeping.
- Original `sub_02060F78`, callback invocation through native-compiled original `sub_0205F484`, terminal `MapObjectMovementCmd098_Step2`, and dispatcher reset to command 255 / step 0.

The first dispatch initializes and advances the command within the same tick. Eight dispatch ticks move exactly one tile. Logical coordinates identify the destination during the command; the position vector advances incrementally. Previous coordinates retain the source until original completion bookkeeping synchronizes them with the destination. A new command is admitted on the next simulation tick after completion.

## Compatibility boundaries

Native pointers never enter guest registers. Original object accessors operate on native original `LocalMapObject` state behind an opaque handle. Its 16-byte original movement scratch buffer is mirrored into bounded guest RAM; original assembly reads and writes those bytes, and successful dispatch synchronizes them back. The image and scratch have disjoint fixed regions. RAM access and execution retain interpreter bounds and instruction budgets.

Static height lookup now accepts arbitrary matrix-global Q12 positions, selecting the validated chunk through the original matrix getters and subtracting the chunk center before executing the existing original Thumb BDHC solver. The tile-center API delegates to the same provider. This supports terrain following between tile centers without introducing a second height algorithm. Dynamic terrain selection and gym gimmicks remain outside the admitted New Bark environment.

The renderer supplies an explicit native completion notification: its resources are native-owned and require no DS VRAM work. The original sprite resource callback is not executed. Missing callback ownership is rejected. Failed command dispatch restores the object for that tick; the player owner rolls back the complete in-flight step and stops it if an unsupported service is reached. There is no successful fallback to a guessed position.

Native pose selection still chooses original standing/stride textures, alternating feet by walk serial and switching pose halfway across the tile. Blocked attempts turn immediately and retry every eight ticks. At this walking-only milestone, original turn-in-place commands, running-shoe input, held-movement ownership, sprite animation control, footsteps, followers, NPC scheduling, map transitions and `PlayerAvatar_MoveControl` were not integrated. The later locomotion and transition documents identify the subsequently connected parts. The monotonic host clock is not DS cycle emulation.

## Verification

`original_motion_state` exercises all 3,968 in-chunk directed New Bark edges through timed admission: 1,590 walks complete and 2,378 are denied by original collision masks. Every admitted walk verifies all eight exact horizontal Q12 positions, intermediate terrain height, destination grid coordinates, command reset, previous-coordinate completion bookkeeping and exactly one native completion notification. Denied walks and invalid/duplicate/ownerless dispatch preserve state or output as appropriate.

`original_thumb_terrain` compares 4,096 intermediate/fractional world-position queries with direct chunk-local original solver execution, in addition to existing independent plane-equation and tile-center checks. Application tests retain the same movement, stopping, short-tap, clock-rate independence, collision, rendering and sign-interaction assertions. Full normal and ASan/UBSan CTest runs and the AppKit smoke test validate the connected viewer path.
