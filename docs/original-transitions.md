# Original warp decisions and inspection travel

The field viewer now supports walking, running and door/stair travel through **New Bark and its seven interiors**. Approach a door and move into it; use the original direction indicated by a stair tile. Selecting one of these eight maps directly also creates an inspection player at its first warp, except New Bark which retains its existing town-center spawn. Other maps remain inspection-only scenes.

The player, running-shoe state, original step-statistics record and native application owner survive travel. The camera resets around the destination, the map list follows the player, and map-list synchronization does not recreate the session. The native host clears held/pending input after travel to prevent an immediate repeat. Scene replacement is immediate; original door animations, fades, sound, NPC reconciliation and story/map-init scripts are not executed.

## Original code executed

The entire unchanged original `src/map_events.c` is included and compiled in the native transition boundary's translation unit. The running path executes original `Field_AllocateMapEvents`, `MapEvents_ComputeRamHeader`, `Field_GetWarpEventAtXYPos`, `Field_GetWarpEventI` and `Field_FreeMapEvents`. The existing bounded decoder validates all four serialized arrays before bytes enter the original aligned event buffer. Native static assertions require original record widths of 20/32/12/16 bytes. Original event header parsing establishes native pointers into that buffer; serialized DS pointers are not reinterpreted as host pointers.

The complete unchanged original `src/save_local_field_data.c` also compiles. Executed functions include its size, initialization, current/previous/entrance/dynamic-position accessors and player-record accessor. The avatar's native save pointer attaches to the original local-field player record, preserving its existing equipment state. Freeing the transition owner copies that player state back and restores the prior pointer before releasing the record. Settled movement synchronizes the current location at the native ownership boundary. General saved-map-object serialization/restoration and the original `SaveArray_Get` owner remain unexecuted.

Original `GetMomSpawnId` executes through the bounded Thumb runtime during local-field initialization. The complete original `unk_0203BA5C.s` spawn unit is assembled; other spawn/blackout routines are not exercised. The expanded guest image is 27,660 bytes, including 22,488 executable bytes, with 377 named unresolved imports. Assembly coverage is separate from executed functionality.

The full `field_control.c` and `field_warp_tasks.c` units require services outside the admitted runtime. To execute their relevant decisions without rewriting them, `prepare_transitions.py` selects **seven unchanged original function definitions** into an ignored build-local include, preserving original line locations and recording source/slice SHA-256 hashes:

- `PlayerAvatar_GetStandingTileCoords`, `ShiftFieldCoordsByCompassDirection` and `PlayerAvatar_GetFacingTileCoords`;
- `SetLocation`, `FieldSystem_MapConnection` and `FieldSystem_CheckMapTransition`;
- `sub_02052F94`, the destination event/anchor initializer.

Clang compiles those original definitions with native pointer layouts. Neither complete source unit is compiled by this integration, and full original transition-task execution is not claimed. The generator rejects missing/ambiguous definitions; build-local `transition_kernel.inc` and `transition_provenance.json` make the exact selected text reviewable. Original algorithm bodies are not stored as project source or edited upstream.

The original selector checks terrain predicates, facing/direction, event coordinates, target map and anchor. Original destination initialization loads the admitted event context, updates previous/current location and resolves coordinates from the destination warp, rather than retaining the source portal's coordinates. Its dynamic-warp branch is compiled but not exercised by this static network.

## Compatibility boundary and commit

`FieldSystem` is the project's scoped native owner, not the DS binary layout. The selected original functions access native fields by name. Their general save lookup resolves to the owned original local-field record. Their terrain calls use the existing original Thumb attribute/wall queries over validated resources. The original transition-environment calls capture a bounded map/warp/direction/type request; original transition tasks are not launched.

The selected destination helper's `Field_InitMapEvents` call is bound to an explicit **zone-event-only adapter**. It runs the original event parser after validation. Original encounter loading, script-header loading, NPC creation and map-init scripts do not run; their data must not be treated as initialized. The unused original full `Field_InitMapEvents` body compiles but does not link into this path. Its unimplemented archive readers receive no successful substitute.

The host retains the viewer's immediate-facing input policy. Transition probing temporarily presents the intended facing to the original selector, then restores the object. Probing occurs before ordinary movement admission: an indoor exit can face beyond the room's collision surface and still have a valid original warp. This avoids interpreting a missing floor height as permission to walk outside.

A probe stages local-field changes privately and restores the live record and source event context before returning. The host loads the complete destination scene, validates destination terrain height, and only then commits the staged location/events and moves the existing actor. A modified, stale or already-consumed destination cannot commit. Failed selection leaves output unchanged. Failed scene/terrain loading preserves the source scene and local-field state. No duplicate native destination-selection algorithm is used.

The static movement admission policy expands to these eight maps and still requires current and target tiles to belong to the same header. Walking across an outdoor map-header boundary remains unsupported. Terrain/height support remains the existing static provider; dynamic platforms and gym mechanics are excluded. NPC occupancy uses only the inspection player because actual map-event actors are not populated.

## Tested network

| Map | Location | Outgoing portals |
| --- | --- | ---: |
| 60 | New Bark | 5 |
| 61 | Elm's Lab 1F | 1 |
| 62 | Elm's Lab 2F | 1 |
| 63 | Player house 1F | 2 |
| 64 | Player house 2F | 1 |
| 65 | Southwest house | 1 |
| 66 | Rival house 1F | 2 |
| 384 | Rival house 2F | 1 |

`original_field_transitions` resolves all **14 directed portals** using actual events and original selection. Destination coordinates are checked independently against the serialized target anchor. It checks read-only probes, rejection of modified/repeated commits, previous/current/entrance records, avatar save-pointer lifetime and all seven interior movement corpora. Across interior surface-backed neighbor edges, 11,322 running commands complete and 363 are denied by original collision. This corpus excludes unresolved collision surfaces and does not measure story/NPC completion.

The same test discovers collision-checked routes and drives the actual application through a town → player house → town round trip, plus player-house stairs in both directions. It renders the destinations, checks retained step counts and verifies running after return. The AppKit smoke test sends keyboard events through the native canvas, verifies the house round trip without replacing the application, checks camera/map-list synchronization and saves ignored `build/evidence/new-bark-house-travel.png`. That image was visually inspected for the original room, props and player placement.

All 25 CTests pass normally and with ASan/UBSan; the native window smoke test passes. These establish original warp decisions connected to native inspection travel. Full field/game startup, original transition choreography, event-object reconciliation, map-init/story scripts, outdoor header crossings, battles and disk saves remain pending.
