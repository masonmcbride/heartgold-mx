# Native land geometry and collision resources

Current extension: [original Thumb terrain execution](thumb-terrain.md) now supplies static height queries. Earlier milestone limits below describe their original scope; movement remains pending.

`ds_model.c` reads original land BMD0 resources through bounded byte views. It validates model dictionaries, indexed names, model offsets and sizes, shape dictionaries, mesh records and display-list ranges. No resource offset is cast to a host address or SDK structure. Format research was checked against [the independent NSBMD viewer documentation](https://github.com/scurest/nsbmd_docs/blob/master/nsbmd_docs.txt); our implementation is locally authored and contains no proprietary SDK code.

The mesh interpreter handles packed GX command words, signed full/partial/delta vertex coordinates, and triangles, quads, triangle strips and quad strips. It emits mesh-local triangles in signed 20.12 coordinates. Color, normal, texture-coordinate, matrix-restore and matrix-scale parameters are bounded and consumed but their effects are **not applied**. Unsupported opcodes fail. A failed decode can have emitted callbacks; callers discard partial output. No game gameplay function is replaced by this hardware-resource decoder. The raw mesh API retains these limits; the land scene API described below applies the supported SBC/root-transform slice.

All 676 original land models successfully open and their mesh command streams decode. New Bark's model `map21_12c` has 18 shapes, 1,252 vertex submissions and 654 triangles after quad triangulation. Counts match the model's declared metadata for 672 resources. Members 468, 473, 475 and 476 have differing declared vertex/triangle totals; tests record those exceptions rather than treating header counts as authoritative. The same discrepancies persist when following SBC draw order. Their source has not been established; this does not prove complete scene execution or visual correctness.

## Diagnostic renderer

```sh
./build/heartgold_field --wireframe 60 > build/new-bark-wireframe.ppm
sips -s format png build/new-bark-wireframe.ppm --out build/new-bark-wireframe.png
```

The CLI selects the first matrix cell with the requested map header using original getters, decodes that land member, and writes a 640×640 PPM to stdout. Diagnostics go to stderr. The native CPU renderer now projects triangles emitted by the supported land scene interpreter into a centered isometric wireframe. A solid diagnostic is also available:

```sh
./build/heartgold_field --solid 60 > build/new-bark-solid.ppm
sips -s format png build/new-bark-solid.ppm --out build/new-bark-solid.png
```

The solid path uses opaque, double-sided triangles, a host depth buffer, and a fixed diagnostic color shaded by triangle normals. Both New Bark outputs were generated and visually inspected. Materials, textures, props and the original camera remain absent. This is not an original-game screenshot or a completed map renderer. Preview images stay under ignored `build/`.

## BDHC collision tables

`hg_collision_decode` checks all six collision tables and their index references. Original `ov01_021FB04C`/`ov01_021FB0B0` establish six count fields and record strides 8,12,4,8,8,2 in `asm/overlay_01_021FB04C.s`. Original `ov01_021FAD6C`, `ov01_021FAD9C` and `ov01_021FADBC` establish the plane's two point indices, normal index and constant index. `ov01_021FAE50` consumes each strip's index count/start. These original assembly routines were inspected, not compiled or executed.

All 676 collision sections pass exact size, plane-reference, strip-range and plane-index validation. New Bark contains 37 points, 2 normals, 3 constants, 20 planes, 7 strips and 47 plane indices. This is structural resource decoding; signed coordinate/plane evaluation, height selection and passability are not implemented. No original movement function is enabled.

## Verification and next steps

Thirteen CTest checks pass in Debug and with ASan/UBSan. The geometry test checks signed coordinates, packed commands, all four primitive modes, strip winding, missing parameters and invalid opcodes. Field tests open every land model, decode its meshes and collision tables, reject every New Bark model/collision truncation, mutate nested model records, reject invalid collision references, and execute the PPM renderer against a temporary file. Existing original gameplay tests still execute their linked upstream functions. This milestone adds no original C compilation/execution coverage; the new work is at the resource/graphics boundary.

The next [textured field viewer milestone](textured-field-viewer.md) now binds original materials/textures and provides interactive camera controls. It also corrects the VTX_DIFF scale error in the initial mesh decoder. Next: original map-prop placement/lifecycle. Audit the original collision height-query and movement dependencies separately before enabling movement. Do not infer walkability solely from terrain behavior or a valid collision table.

## Land scene transform milestone

`ds_model_scene` follows the bounded SBC stream, executes its shape draw order, validates material and shape indices, honors root visibility, and applies the supported root translation, position scale-up/down and matrix storage/restoration. Corpus inspection found one root in all 676 resources, with identity rotation and node scale; five roots carry a translation. Every original land scene now executes this supported slice. The later [prop-rendering integration](prop-rendering.md) extends this path with multiple nodes, full/pivot rotations and embedded textures. Unsupported node scale/compensation, billboards, skinning, other SBC opcodes and in-mesh GX matrix changes fail explicitly.

The scene output uses host doubles in model units. It is a graphics-boundary interpretation, not bit-exact DS fixed-point arithmetic or an SDK implementation. Materials are index-validated but not bound. The original SDK/SBC engine and field overlay assembly remain unexecuted; original map-header/matrix functions still select the resources. No additional upstream C compilation or execution coverage is claimed.

Synthetic tests verify signed root translation, scale-up/down, preservation of translation while scaling, matrix restoration, visibility, repeated mesh draws, and rejection of invalid slots, shapes, materials, unsupported transforms/opcodes and missing termination. Every land resource exercises the scene interpreter; malformed New Bark model mutations exercise it under sanitizers. Both wireframe and solid PPM paths execute in field tests; the solid test checks output size and substantial non-background coverage. Tests do not establish pixel-exact DS rendering or independently verify every depth-buffer case.

The higher-level [textured field viewer](textured-field-viewer.md) now binds materials and preserves UVs/vertex colors. The raw mesh and scene APIs described above remain separate from texture sampling and rasterization.
