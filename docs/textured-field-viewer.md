# Interactive New Bark land viewer

Run `./build/heartgold_field_viewer`. The application displays original New Bark land geometry and textures with a host orthographic camera. Drag or use arrow keys to orbit, scroll or press +/− to zoom, use WASD or shift-drag to pan, and R to reset. Command-Q closes the app. The viewer renders on input changes and owns a cached triangle/material scene. It uses a CPU renderer displayed by AppKit; this is separate from the Metal inventory app.

```sh
./build/heartgold_field_viewer --render-only build/new-bark-textured.png
./build/heartgold_field_viewer --smoke
```

The first command generates a PNG without starting the application's window loop. The second starts the real window, synthesizes a camera key event, checks that it changes the framebuffer, and executes AppKit display before exiting. Graphical execution requires a login session. The graphical smoke test passed, including resource loading, a synthesized camera key event, changed framebuffer and AppKit display. Headless output was generated and visually inspected. No comparison with an original-game screenshot has been performed.

## Resource chain and executable boundaries

Original `MapHeader_GetAreaDataBank(60)` returns area bank 2. The native boundary reads its eight bytes from original NARC `a/0/4/2`. Inspection of `AreaDataManager_Alloc`/`AreaDataManager_Load` in upstream `asm/overlay_01_021FB878.s` identifies the second halfword as the map texture-bank reference into `a/0/4/4`. New Bark selects texture member 2, with 67 texture entries and 66 palette entries. Both archives are copied unchanged to ignored build resources.

Original map-header and map-matrix functions select New Bark's single land chunk. The model decoder follows its supported SBC stream, preserving UVs, vertex colors and material selections. Material dictionaries associate texture and palette names with lists of material indices. The boundary resolves those names in the selected texture bank; it does not assume texture, palette and material archive indices agree. The resulting scene has 654 triangles. Unused unbound material slots are allowed, but drawing an unbound material fails.

This adds resource/graphics boundary code, not new original gameplay execution coverage. Original SDK rendering and assembly overlays remain unexecuted. No Nintendo SDK definitions or implementation were added. Original source files, assets, generated data and images remain outside version control as described in the README.

## Correctness work

The audit found an error in the preceding mesh milestone: `VTX_DIFF` deltas were multiplied by eight. They actually increment the Q12 coordinate by the signed ten-bit value and wrap within the signed 16-bit vertex register. This behavior was cross-checked against [melonDS's geometry implementation](https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D.cpp). Our old test repeated the same incorrect scale and therefore passed. The corrected test now submits identical known triangles using absolute coordinates and delta coordinates and requires identical results. The older diagnostic PNGs should be regenerated with the corrected decoder.

The new raster tests use small scenes with known pixel colors. They check nearest sampling, negative-coordinate clamp/repeat/mirror, transparent palette index zero, A3I5/A5I3 alpha, opaque depth independent of submission order, cutout revealing geometry behind it, and translucent compositing. A square split into two translucent triangles verifies that the shared diagonal is covered once. That test guards against double blending, rather than merely checking for a nonempty output.

Thirteen CTest checks pass normally and under ASan/UBSan. The field integration test checks the original texture bank, material binding, visible image content, camera-induced frame changes, exact reset, missing resources, all original texture-member truncations and deterministic mutations. Existing tests continue to run all 676 land scenes and collision-table validators. These establish the supported host slice, not pixel-exact DS rendering.

## Supported slice and remaining work

Texture formats A3I5, 2/4/8-bit indexed, A5I3 and direct BGR555 are supported with bounded reads. Compressed 4×4 textures fail explicitly. The viewer accepts identity and the Maya-mode translation-only material UV slice; other material texture transforms fail explicitly. Vertex COLOR values modulate textures, but original NORMAL/light/material equations are not evaluated.

The host renderer has orthographic interpolation, nearest texture sampling, an opaque depth pass and a back-to-front translucent pass. It uses a shared-edge coverage rule. Original culling, polygon modes, hardware depth/lighting precision, intersecting translucent ordering, texture animation, fog and DS blending are not reproduced. Camera controls are host inspection controls, not the original camera engine.

The scene contract currently requires exactly one matching chunk; a multi-chunk map fails instead of silently showing only part of a route. The UI loads New Bark. The subsequent [prop-rendering integration](prop-rendering.md) now draws buildings, signs and other props using their separate placements and original traversal. Player objects, collision-height queries, scripts and movement are absent.

The next vertical slice should add the original map-prop resource chain and placement lifecycle, auditing `src/field/map_prop_manager.c` for direct original execution through a graphics compatibility boundary. Acceptance should include correctly placed houses/signs in this same interactive view and behavioral tests for the original prop manager. Separately, enable movement only after auditing the original height query and its player/object dependencies. Valid terrain attributes and collision tables alone do not establish passability.

The subsequent [original field lifecycle harness](field-lifecycle.md) now executes the original prop manager and measures its dependencies. The [shared graphics boundary](prop-rendering.md) now connects original prop drawing traversal to this viewer.
