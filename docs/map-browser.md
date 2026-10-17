# Native map browser

Launch `./build/heartgold_field_viewer`. The AppKit window now has a scrollable list of all 540 original map IDs, a search field and Previous/Next buttons. It starts at New Bark Town.

Search by readable name, original `MAP_...` symbol or exact numeric ID. Previous/Next traverses the filtered list; selecting the table and using its arrow keys also navigates. Each successful selection resets the camera to fit the complete supported scene. The canvas keeps its drag/arrow orbit, scroll/+/− zoom, shift-drag pan (WASD walks in New Bark and pans elsewhere) and R reset controls. No web browser or server is involved.

A check mark indicates a map successfully loaded during this session; an exclamation mark indicates a failed attempt. Unvisited rows have neither mark. These are session results, not precomputed declarations of correctness. Hover a failed row or the bottom status text to read the complete reason. Unsupported selections clear the old image so it cannot be mistaken for the requested map. An empty search result clears the canvas and disables navigation; clearing the search restores the list.

## Renderer coverage

The renderer now assembles every matching matrix cell for a selected map, up to 64 cells, rather than requiring exactly one. It translates land and original prop submissions together, preserving relative matrix coordinates. The original `ov01_021F5FB8` assembly establishes 512 model units per x/z chunk and eight model units per matrix-altitude increment; those placement rules are applied at the native scene boundary. This assembly function is inspected, not executed here. Scene coordinates are rebased by the minimum matching x/z matrix cell, and the camera fits their combined bounds.

Both observed area types (0 and 1) use their original area-specific texture-bank/model-list references. Type 0 selects indoor prop archive `a/1/4/8` and `bm_room_matshp.dat`; type 1 selects outdoor `a/0/4/0` and `bm_field_matshp.dat`, following original `AreaDataManager_Alloc` and `AreaDataManager_Load`. The earlier browser incorrectly reused outdoor props for interiors; its 278-map result was raster completion, not verified resource selection. This enables additional interiors without claiming their scripts, original cameras or animations execute. Materials remain bounded at 4,096 entries and triangles at 100,000. A failed chunk rejects the complete scene rather than silently dropping part of the map. Some map headers have no matching cells, and the special `MAP_EVERYWHERE` entry exceeds the scene bound.

Original prop allocation/loading/traversal still executes for each chunk. Unsupported direct-shape graphics submissions now propagate failure through the native draw boundary rather than aborting the app. Original upstream code remains unchanged.

The complete audit currently renders **453 of 540 map entries** at 96×96 in both normal and ASan/UBSan builds. Remaining outcomes:

| Outcome | Maps |
| --- | ---: |
| Unsupported prop resources/materials/scene commands | 43 |
| No matching chunks | 21 |
| Unsupported material texture/UV binding | 21 |
| Special 598-chunk scene exceeds limit | 1 |
| Invalid/missing land/model resource | 1 |

“Rendered” means scene construction and rasterization completed. It does not establish visual accuracy against hardware, correct interpretation of special/unused map IDs or a complete original field lifecycle. Route 29's three-chunk result and a further scene were visually inspected; all maps have not been inspected individually. New Bark now has limited [player movement](player-movement.md); other maps remain inspection views.

## Reproduce and export

```sh
cmake --build build -j
./build/heartgold_field_viewer --smoke
./build/heartgold_field_viewer --render-map 33 build/route-29.png
./build/heartgold_field_catalog 60
python3 scripts/audit_maps.py --build build
ctest --test-dir build --output-on-failure
```

`--render-only path.png` continues to export New Bark. `--render-map ID path.png` exports a selected map or reports its failure. The catalog executable outputs one JSON record after actually rendering the map. The audit runs all 540 probes in isolated processes with time limits, writing `build/evidence/map-catalog/maps.json` and `REPORT.md`. Failures and process diagnostics remain reviewable; a crashing map cannot terminate the audit. The audit counts raster completion, not screenshot comparison or function-entry coverage.

The native UI smoke test exercises camera changes, Previous/Next, filtered supported and unsupported selections, empty results, recovery and AppKit display. Scene tests cover unchanged New Bark geometry, the three-chunk Route 29, another area-type scene, failure diagnostics and successful reload after failures. All 18 CTest checks pass normally and under ASan/UBSan; the complete map audit also runs under sanitizers with no process failures. Exports and audit artifacts stay in ignored build directories.


## Untextured materials and scene lifecycle

Valid untextured surfaces now reach the existing vertex-color raster path. Material polygon attributes, including opacity, are initialized regardless of texture presence. This removed the broad apparent land-geometry failures without adding geometry instructions. The measured count briefly reached 474 before visual inspection exposed the indoor prop-selection error; correcting that dependency gives the current 453 result. The lower corrected count is deliberate: unsupported actual indoor models must not be replaced with unrelated outdoor models.

The player-house render now shows its room and furniture; this image was visually inspected. Normals/lighting, diffuse-register semantics and hardware precision remain incomplete. A rendered image is not proof of full SDK material fidelity.

The viewer now owns `HGFieldApp`, whose native callbacks are driven by the unchanged original `OverlayManager` init/exec/exit state machine. Replacing or closing a map executes original lifecycle transitions. These scene callbacks remain native; original task/application dispatch now advances on a native timer while redraws remain on demand; they are not the original `FieldMap_Main`, avatar update or a continuous game loop. See [playable integration](playable-integration.md).
