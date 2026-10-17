# Shared graphics boundary and original prop drawing

New Bark's houses, doors, signs and windmill now appear in the interactive viewer:

```sh
./build/heartgold_field_viewer
./build/heartgold_field_viewer --render-only build/new-bark-props.png
cmake --build build --target field_evidence
```

The original prop manager supplies placement and traversal. The host graphics boundary decodes geometry/materials and produces the cached scene used by the existing camera renderer. The New Bark image was generated and visually inspected; this is not a pixel comparison against original hardware.

## Original functions and measured dependency change

The unchanged `ov01_021F3C9C` now traverses all 17 loaded New Bark props. Original `DrawModelShapewise` executes for eight placements, using the actual `bm_field_matshp.dat` material/shape lists. The traversal emits nine whole-model submissions and 26 per-shape submissions, for 35 commands and 1,294 prop triangles. Together with 654 land triangles the viewer caches 1,948 triangles.

The scenario independently matches each command's world translation to the original placement records and requires coverage of all 17 placements. It also validates command counts, multi-node model decoding, cleanup and the existing lifecycle assertions. Compiler instrumentation now observes **22 of the unit's 24 functions**, with these two still unexecuted:

- `MapPropManager_LoadFromSafariZone`: Safari configuration remains unsupported.
- `ov01_021F3A3C`: its original view-culling and lighting-attribute boundaries remain unsupported.

The generated unsupported-boundary list shrank from 14 to three for this translation unit. This is a scoped dependency measurement, not a percentage of the game ported. Boundary availability does not imply complete SDK semantics or support for every resource.

`GF3dRender_DrawModel`, the NNS-style submission operations, area/shape accessors and rotation conversion in this slice are **native graphics boundaries**. The original `gf_3d_render.c`, original area-manager assembly and proprietary SDK implementations are not compiled/executed. Original gameplay algorithms were not rewritten to draw the scene. The native submission API supplies explicit host transform/material state and dispatches the decoded model to a callback; it does not pretend to write DS GPU registers.

## Shared capabilities

The changes extend the common model path rather than add separate house/sign decoders:

- BMD0 containers can carry MDL0 plus embedded TEX0. BTX0 and BMD0 texture-bank reads validate their block tables and bounded sections.
- Host affine composition handles model, node and world transforms using one 3×4 matrix implementation.
- Supported SBC scenes handle multiple nodes, full and pivot rotations, matrix save/restore and position scaling. Non-unit node scale/compensation, skinning, billboards and in-mesh matrix commands still fail explicitly.
- Indexed textures permit compact palettes. Validation checks the indices actually referenced by the texture, then sampling remains bounded by the available palette block. It no longer requires every possible color index to have a palette entry.
- The Maya-mode translation-only material UV slice supports New Bark's windmill. Other nonidentity material transforms still fail; identity transforms remain supported.
- World rotation accepts indexed-turn angles with 4096-step quantization. Native trigonometry and Q12 rounding are a host graphics adaptation, not bit-exact SDK sine-table/matrix arithmetic.

The model rotation formats were cross-checked against the independent [apicula rotation decoder](https://github.com/scurest/apicula/blob/master/src/nitro/rotation.rs). Tests require all nine pivot placements to preserve basis orthogonality and compare a known quarter-turn in pivot and full-matrix encodings. They also check noncommuting affine composition and matrix restoration. The simple texture-translation slice was checked against [EveryFileExplorer's material matrix implementation](https://github.com/Gericom/EveryFileExplorer/blob/master/NDS/NitroSystem/G3D/MDL0.cs); original DS arithmetic precision is not claimed.

## Area metadata and supported submission contract

`bm_field_matshp.dat` is copied unchanged into ignored build resources. Its two counts bound a model table and a material/shape-pair table. The native boundary validates exact total length and every nonempty range, then exposes native-owned decoded halfword arrays. Original C chooses the whole-model/per-shape branch and its material-switch behavior. The material-send flag is checked against retained native material state.

The per-shape submission slice requires an identity single root. That matches all per-shape New Bark models; multi-node signs use the whole-model branch. This restriction avoids silently treating a general SBC node traversal as the SDK's direct shape operation. New Bark's standard placements have no rotation override; explicit rotation tests cover cardinal indexed turns and quantization, not every mixed-axis SDK rounding case.

## Lifetime, tests and limitations

`hg_field_view` now links through `hg_props` so the viewer can execute original allocation, loading, traversal, reset and free. The base field-data library remains independent. The scene cache owns its resulting triangles; texture bytes remain borrowed from the process-lifetime resource cache. Prop-manager/render handles are freed after geometry capture. This is a load-time original traversal, not a persistent original field main loop. The windmill is static and prop mutations are not yet reflected live in the viewer.

All 15 CTest checks pass normally and under ASan/UBSan. The native viewer smoke test also passes resource loading, a camera key event, framebuffer change and AppKit display. New checks cover the original drawing traversal, all-placement transform coverage, exact scene triangle totals, matrix encodings/composition, embedded-resource truncation, compact palette validation, windmill UV translation and malformed area tables. The existing opaque/cutout/translucent renderer tests and all 676 original land-scene tests still pass.

Remaining work includes persistent original field/object lifecycle, original collision-height/movement rules, interior transitions, saves and scripts. Rendering still lacks original lighting, camera, animations, hardware precision, culling and complete translucent ordering. The single-chunk field/area-resource restriction remains. Unsupported services continue to fail explicitly.
