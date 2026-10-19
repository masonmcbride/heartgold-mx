# Free exploration

Run `./build/heartgold_field_viewer` and check **Explore freely**, then use the
map list or search to choose a location. `./build/heartgold_field_viewer --explore
76` opens Goldenrod in exploration mode. WASD walks; Shift runs; camera controls
are unchanged. Uncheck the box to restore the exact suspended story session,
including its position, party, dialogue and script continuation. The suspended
session is not ticked, and resuming resets its wall clock to avoid catching up.
This is an in-memory suspension, not disk saving. Closing the app loses it.

Exploration creates a separate player/story owner and suppresses original map
entry, frame, coordinate and interaction scripts and all event actors, including
NPCs and followers. Original buildings and other scenery remain. It uses the
existing original collision, locomotion, warp and outdoor-header crossing paths;
it does not walk through walls or bypass unavailable terrain services. Original
entrances anchor spawns where available; otherwise the original matrix supplies
a candidate chunk. Unsupported player initialization leaves a labeled view-only
scene. Maps whose renderer rejects their resources still cannot be displayed.
The existing 453/540 rendering inventory is not a walking-coverage claim.

No original source or game data was changed. The ordinary opening continues to
use original scripts and NPCs, including explicit rejection of unported features.
Exploration does not establish encounters, battles, story progression, NPC AI,
audio or general map support.

## Execution evidence

`free_field_exploration` checks walking and rendering on eleven maps: bedroom,
New Bark, Route 29, Cherrygrove, Route 30, Mr. Pokemon's house, Route 5, Route 31,
Violet, Goldenrod and Ecruteak. It checks actor/script suppression and drives a
connected round trip through Elm's lab, Routes 29/30, Cherrygrove and Mr. Pokemon's
house using actual application input and original collision/transition logic.
The scenario retains the same story owner across travel and exercises clock
reset after suspension.

The native AppKit smoke test switches modes, browses Mr. Pokemon's house, and
restores the identical story pointer, position and revision. Its build-local
`build/evidence/exploration-mr-pokemon.png` was visually inspected: the player
appears at the original entrance within the room. The first attempted center
spawn landed outside visible interior geometry despite collision acceptance;
using original entrances fixes that case.

All 33 CTests pass normally (9.01 seconds) and under ASan/UBSan (32.95 seconds).
The native AppKit smoke passes, including suspending and completing an active
conversation after restoration. Iteration can run only
`ctest --test-dir build -R '^free_field_exploration$' --output-on-failure`;
the full sanitizer route is a checkpoint check rather than a per-edit requirement.
Upstream remains unmodified and generated evidence remains ignored.
