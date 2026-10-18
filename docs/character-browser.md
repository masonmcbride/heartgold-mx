# Original overworld character inspection

Launch `./build/heartgold_field_viewer`, select **Characters · sprite meshes** above the sidebar, and search or scroll the list. Drag or use arrow keys to rotate; scroll or +/− to zoom. Previous/Next follows the filtered list. Gym leaders can be searched by English names, such as Falkner, Whitney, Clair and Brock, or by original `GSLEADER...` resource symbols. The Maps category retains the existing map browser.

HeartGold's overworld trainers and gym leaders are **sprite textures on shared billboard geometry**, not volumetric human models. This viewer displays the original assets and allows orbiting the plane; turning it sideways reveals its flat geometry. It does not invent missing backs/sides or provide full 3D battle character bodies. It currently shows one static texture frame, not original character animation or direction switching.

## Source relationships and boundaries

`asm/overlay_01_sprite_data.s` contains the original sprite/model relationship table. Standard entries have the `0x000 | (0 << 10)` descriptor. The type-0 geometry mapping at `ov01_02207294` selects original model member 266. That model refers to a frame ending `.5`; the inspection boundary explicitly binds the selected character's `.5` texture and first palette by bounded dictionary enumeration, falling back to its first frame when `.5` is absent. The original animation/frame-selection routines are not executed.

`scripts/prepare_characters.py` packs unchanged NSBMD/NSBTX files from `files/data/mmodel/mmodel/` into an ignored build-local archive, preserving their original numeric IDs and leaving unused IDs empty. It derives names from original `constants/mmodel.h` and admits the verified type-0 (`0x000`) and type-7 (`0x060`) descriptors from the original sprite table. Other specialized types remain unsupported rather than using a guessed mesh. The browser lists the first 266 resource slots: **192 render, 74 report unsupported**. All 16 Johto/Kanto gym leaders render. Follower Pokémon and later resource slots are outside this initial catalog.

The same bounded model, texture, raster and scene-cache code used for maps supplies character inspection. The shared texture API now supports bounded dictionary-name enumeration. An explicit `ds_model_inspection_scene` path freezes simple SBC billboard orientation in model space, allowing it to be orbited. This is an inspection convention; ordinary `ds_model_scene` still rejects unsupported billboards. It does not claim original camera-facing billboard math, SDK execution, skinning or character AI. Map application lifecycle still uses the original overlay manager; character inspection does not execute original avatar loading/update routines.

## Verification and exports

```sh
./build/heartgold_field_viewer --render-character 146 build/falkner.png
./build/heartgold_field_catalog --character 148
./build/test_characters
./build/heartgold_field_viewer --smoke
ctest --test-dir build --output-on-failure
```

The character corpus test attempts all 266 slots, checks all 16 leaders, verifies different characters produce different frames and orbit changes the image, and checks that inspection billboard support does not silently broaden normal scene support. It also tests dictionary bounds. The native smoke test switches Maps -> Characters -> Maps, searches Whitney, verifies character orbit changes the framebuffer and recovers the map application. Falkner's exported frame was visually inspected.

Enlarging the sprites exposed a floating-point shared-edge coverage gap. The rasterizer now applies the same epsilon to edge rejection and top-left inclusion; a fractional rotated quad test requires identical coverage with either triangulation, allowing one color level for floating interpolation rounding. This fix is shared with map rendering.

The character resource/catalog path is useful groundwork for actual field-object rendering. It is not a substitute for original avatar/object state, movement or task integration; those remain on the playable-integration critical path. Original assets remain in ignored build directories and upstream is unchanged.

The story integration derives type-7 identities from the original sprite table and uses original mesh 272. The native presentation now expands its horizontal geometry and UV span together to cover the complete texture canvas, retaining texel scale, height and centered pivot. The previous center-strip binding clipped hats and arms in wider poses. This also renders the profile-selected friend in the rival’s upstairs room; type-7 poses remain native static presentation rather than original animation-controller execution. All 32 hero frames have opaque-texel regression coverage through direct and shared-depth billboard rendering.
