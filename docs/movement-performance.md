# Movement follows elapsed time

The original viewer advanced one simulation tick per timer callback and synchronously rasterized every animated frame on the main thread. A nominal 60 Hz timer does not guarantee 60 callbacks per second when rendering occupies the thread. On the measured Debug build, an 800-by-600 field frame took about 98 ms; 240 simulation ticks without rendering took about 1.4 ms. Missing timer firings therefore slowed the eight-tick walking cadence to roughly one-sixth of its intended rate.

The viewer now passes `CLOCK_MONOTONIC` timestamps into `hg_field_app_advance`. A shared `DSFrameClock` computes rational 60 Hz ticks, so delayed presentation callbacks advance the necessary simulation steps before one redraw. The existing system-task scheduler uses the same accumulator rather than maintaining a duplicate timing implementation. Catch-up remains capped at eight ticks per poll; long stalls drop backlog, and backward timestamps do not rewind simulation. The simulation/presentation split and original collision checks remain intact. This is a host clock policy, not DS cycle accuracy.

New WASD press edges are latched until a simulation step can consume them. A press and release between timer callbacks produces one attempted step. Normal release stops repetition; window/focus loss explicitly clears both held and pending movement. Dialogue interaction also clears movement input. There is at most one pending direction-mask sample, not an unbounded key-repeat queue.

The CPU renderer now skips alpha-pass work for materials whose texture formats can only produce fully opaque/transparent samples and whose polygon alpha is opaque. Translucent polygon materials skip the unnecessary opaque pass. A3I5/A5I3 textures retain both passes because texture alpha can be partial even with opaque polygons. Sorting is omitted only when no submitted material needs a translucent pass. Depth, cutout, blending and polygon/texture alpha tests cover these decisions.

The per-pixel raster and texture translation units compile with `-O2` even in Debug, retaining debug symbols, assertions and sanitizer instrumentation, without fast-math. For instruction-by-instruction debugging of those two files, configure `-DHG_OPTIMIZE_RASTER=OFF`.

A repeated 30-frame measurement after these changes took about 22.3 ms per 800-by-600 frame and 1.4 ms for 240 simulation ticks on the same machine/build: about a 4.4-fold reduction in CPU frame cost. These measurements exclude AppKit presentation overhead and are not a guaranteed displayed frame rate. Movement no longer depends on render callback count within the bounded catch-up window.

Reproduce a fresh diagnostic measurement with:

```sh
cmake --build build --target heartgold_field_benchmark
./build/heartgold_field_benchmark
```

This prints JSON; timing is diagnostic and is not a test pass/fail threshold. The measured output is saved under ignored `build/evidence/movement-performance.json`.

Regression tests advance the same application for one real second using synthetic timestamps at 10, 30 and 60 polls per second. All produce exactly 60 ticks and identical presentation position/pose. Additional cases cover backward timestamps, bounded long-stall catch-up, quick taps, no unintended repetition and focus/input cancellation. The native UI smoke test sends a short key-down/key-up pair followed by a delayed six-tick update, alongside the existing walking, collision and sign-dialogue checks. No game speed multiplier or collision bypass was added.

The subsequent [original walking command integration](original-walking.md) removes native position interpolation. Source execution confirms that normal walking commands themselves specify eight ticks at two model units per tick. The host clock and native sprite-pose policy remain distinct from the full original field controller.
