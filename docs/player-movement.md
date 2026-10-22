# New Bark walking

Run `./build/heartgold_field_viewer`, select New Bark (60), click the scene and hold WASD, or Shift + WASD to run. W/S move north/south and A/D west/east in world coordinates, independently of camera orbit. Release stops input. The camera follows each committed step. Drag/arrows orbit, scroll or +/− zoom, shift-drag pans, and R recenters on the player. Moving keyboard focus away from the scene or deactivating the window clears held input. Switching maps creates a fresh inspection session; other maps retain camera inspection controls.

The native session owns original `LocalMapObject`, `MapObjectManager`, `PlayerSaveData` and `PlayerAvatar` state. A deterministic spawn search near tile (688,398) admits a non-wall tile with an ordinary walking exit. This is a native session spawn, not original new-game/script execution. A nominal 60 Hz timer samples elapsed monotonic time to drive original field-task/application dispatch, with bounded catch-up when callbacks are delayed. Its native scene callback admits a walk/run command while input is held. Original command-table dispatch now owns per-tick displacement and completion: two model units per tick for eight ticks. At the host clock’s 60 Hz this is 7.5 unobstructed tiles per second. The subsequent [original locomotion integration](original-locomotion.md) executes the original walk/run selection branch: running takes four ticks per tile when the original shoe flag and B input are present. The inspection session supplies shoes; Shift maps to B. The full original input/turning controller remains unconnected; blocked-attempt retry is still a native eight-tick policy.

Each step uses original directional input resolution, original player collision-mask execution, original direction-to-command selection, original grid-step bookkeeping and original per-tick Q12 displacement. When the facing tile carries the original directional jump-ledge bit, the same selection branch chooses the original hop command (`56 + direction`) instead of an ordinary step: the sixteen-tick arc crosses the ledge and settles two tiles away. `test_locomotion` executes this on Ilex Forest's real ledge. Original height-following code queries static BDHC at every intermediate position. Height, directional terrain, occupancy and cycling restrictions execute through the boundaries documented in [playable integration](playable-integration.md). Nonzero masks prevent ordinary walking, including special movement requirements. Missing resources/unsupported execution do not commit an object update. The scope now includes static New Bark and its seven tested interiors, with no dynamic platforms or active gym gimmick. See [original transitions](original-transitions.md) for door/stair travel and local-field state.

The hero uses original mmodel resource 69's textures and its type-7 mesh, with directional stepping presentation described below. It shares the terrain/prop raster and depth buffer, so buildings and trees can occlude it. This does not execute original sprite animation controllers, lighting or object-render tasks. The default camera starts near the player rather than fitting the entire town.

The application test verifies that input progresses through simulation ticks, changes both original coordinates and rendered pixels, stops on key release, and eventually stops at a collision boundary under sustained input. Rendering does not itself advance simulation. The AppKit smoke test sends WASD key-down and key-up events and verifies movement and stopping alongside map/character browser behavior. A rendered New Bark frame was visually inspected to confirm visible spawning on the path. The full motion corpus separately covers 3,968 neighbor edges, height thresholds, water/cycling checks and synthetic object occupancy. Twelve flag-selected event actors now render and block movement through original occupancy; see [event actors](event-actors.md). NPC AI, full map-init/cutscene behavior, original animation-controller execution, general interactions, surfing, full original animation graphics, saves and full original game startup remain pending.

## Directional stepping presentation

The player selects the original hero type-7 mesh (member 272), following `SPRITE_HERO` in `overlay_01_sprite_data.s`, rather than generic member 266. Its 34-unit height replaces the previous native 24-unit normalization. At the native billboard boundary, horizontal geometry and UV range expand together to cover the full 32-pixel texture canvas while preserving the original horizontal texel scale, vertical placement and centered pivot. The former center-16 crop cut off hat/arm pixels, particularly during sideways running; frames 26 and 32 each lost 42 opaque source texels. Full-frame padding now stays available across all poses, without changing map scale, collision, movement or the size of the already-visible body pixels. The same boundary corrects other admitted type-7 characters.

The native presentation caches all 32 original `hero.N` textures. Inspected standing frames are north/south/west/east 1/5/9/13; paired step poses are 2 & 4/6 & 8/10 & 12/14 & 16. Native pose selection alternates stepping feet and standing textures using distance traveled by the original command. Blocked attempts turn the actor without playing a walking step. The renderer reads the original object’s position vector each tick; there is no native position interpolation. Releasing input lets the active original movement command complete, then stands still. Camera following uses the original position vector, and a presentation revision triggers redraws for pose changes as well as motion. Rendering itself never advances animation.

Position and command duration execute original code and tables; texture-pose selection still does not execute the original sprite animation controller or its timing tables. See [original walking commands](original-walking.md) for exact execution boundaries. The character gallery remains a static inspection view. Tests check intermediate displacement, stride-to-idle transition, settling on key release and no animation advance on redraw/idle ticks. A regression compares every opaque texel in all 32 original hero frames against both direct rendering and the rotated, shared-depth billboard path at independently projected positions. This checks complete silhouettes, texel scale, height and pivot stability; it fails on the old crop. The added native keyboard smoke cases compile and cover both east/west running strides, but their GUI execution has not been verified for this fix. All 27 headless CTests pass normally and with ASan/UBSan. Before/after frame strips were visually inspected; the original source and resource bytes remain unchanged.

The subsequent [background interaction milestone](field-interaction.md) now adds original event selection and bounded New Bark sign-script execution. Enter/Space reads the town and lab signs; dialogue freezes walking until closed. General interactions, NPC AI, story scripts and complete original transition tasks remain pending.

The [movement performance fix](movement-performance.md) separates simulation speed from CPU render throughput and preserves short key presses between polls. Focus loss clears both held input and pending taps.

The [transition integration](original-transitions.md) now connects all 14 portals across eight maps, using original warp selection and anchor initialization. The session, equipment and original step count survive travel; native scene replacement, camera/list synchronization and input clearing remain host responsibilities.

The [story runtime](story-runtime.md) now retains original flags, variables and a live script context across admitted conversations and travel. Script activity locks movement through completion; original transition hooks select the current visible population.

Admitted script movement now shares the original Thumb command table: faces, normal/slightly-fast steps, walking on the spot, direction locks and far jumps. The rival’s two-message window interaction completes its original shove/jump and return sequence. Native scene snapshots validate the full selected sequence before presentation and retain poses/sprites for rollback. Jump presentation includes the original facing-vector height offset; ordinary WASD surfing and original animation controllers remain unsupported. Player-input ledge hops run through the same selection branch (see above). See [field interaction](field-interaction.md) for the native scheduler boundary and behavioral evidence.

The [first-starter route](first-starter.md) now adds an original follower in the same actor manager. Player commands retain their completion flags until the original follower controller observes the frame, then clear held movement. The original walking/running follower modes are tested in four directions, through lab exit and re-entry; the host still owns rendering and frame scheduling.

## Ledge controller and follower correction

Player-input ledges now use the existing unchanged `sub_0205D4B4` controller,
including its avatar state and original follower queue notification. The separate
host ledge selector and its duplicate rollback/allocation path were removed.
The follower sprite now reads original `ov01_022054E0`, which derives its arc
from the original follower controller counters and lookup tables. Its jump height
is separate from the ordinary facing vector; ignoring it rendered the follower
walking across the ledge even when the movement controller progressed.

`test_locomotion` checks avatar and follower queue publication for the real Ilex
ledge. `test_follow` drives the player jump, follower approach, subsequent player
step and follower crossing, checks a nonzero original sprite arc and the landing.
These are motion/controller checks, not a complete native-window ledge scenario.

Verification of this correction: all 49 CTests pass; focused locomotion/follower
checks pass under ASan/UBSan. The native AppKit smoke passes after its house-exit
check was updated to wait for the existing asynchronous original fade rather
than assuming travel completes in one tick. This UI check covers the broader
field presentation and route, not a recorded native-window Ilex ledge crossing.
