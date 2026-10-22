# Iteration measurements

Record dated 2026-10-20. This compares the uncommitted working state captured at
the start of this iteration with the resulting working state, not Git HEAD.
The build-local baseline hashes are in `build/audit/iteration-current.json`.

| Measure | Previous iteration | This iteration |
| --- | --- | --- |
| Connected endpoint | First rival, original aftermath, all three starters | Return to Elm, police, rival naming, Egg handoff, original quest progression and lab re-entry, all three starters |
| Development resume | Nine validated checkpoints | Same checkpoints reused to test the next original scene; schema updated to version 2 |
| Main new boundary | Original trainer AI via existing Thumb runtime and field/battle continuation | Original naming task/application and misc record, with native keyboard frontend |
| Regression suite | 47 passing, recorded 20.09 seconds | 48 passing, 21.95 seconds with `ctest --test-dir build --output-on-failure -j8` |
| Focused new scenario | Not present | All three starters, cancel/retry, name validation/publication, Egg removal and re-entry: 3.18 seconds |
| Native UI | Rival battle and checkpoint/Back | Adds real typed name entry, panel rendering, Elm handoff and Back restoration |
| Sanitizer | Existing opening/checkpoint evidence | New connected Elm and pending naming-task cancellation: no ASan memory errors; existing original signed-shift UBSan reports remain |

The measured implementation/verification window was 20:47:14–21:02:23 UTC
(15 minutes 9 seconds), including implementation, builds, checks and documentation. Initial
investigation happened before that timer. The prior iteration's engineering time
was not recorded, so a before/after engineering speed multiplier cannot be
computed. The previous suite command/concurrency was not captured either; the
suite times are recorded feedback costs, not a controlled performance benchmark.
A serial run with concurrent sanitizer/UI work took 37.04 seconds, illustrating
why concurrency must be recorded with future samples.

The concrete reuse is that the next scene uses the existing original-script VM,
private preflight/publication, task queues, OverlayManager, scene/follower
movement, audio, font, Bag and checkpoint restore. No Elm-specific scene
implementation or forced completion flags were added. Faster original walk
commands 80–83 now use the already-assembled movement table. A shared name codec
also replaces the profile setter's duplicated encoding loop.

The bounded naming extractor selects ten original functions. This is a source
selection count, not ten newly exercised behaviors. The rival scenario executes
the original naming factory, task, application call, name application,
argument destruction and rival-name setter, plus three newly admitted original
script handlers. The original player-name and Pokémon-name workflows are not
claimed working. Other fields in the full misc record do not imply those
subsystems run.

This is one reuse sample, insufficient to establish acceleration. For subsequent
iterations record the connected endpoint, implementation window, new shared
boundary, reused services, focused feedback cost and remaining dependency. Use
those observations across several iterations; do not use line count, selected
functions or an invented completion percentage as the productivity score.

Next shared dependency: the original tutorial battle setup/task and automated
tutorial command/capture path. Capture and battle item presentation remain
unsupported; the tutorial and its five-Poké-Ball gift have not been marked done.
