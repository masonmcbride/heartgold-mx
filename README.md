# HeartGold MX

An experimental Apple Silicon build that compiles and executes original Pokémon HeartGold gameplay C with a small compatibility runtime. MX means M-series chips, with X as the variable. The current applications are an interactive inventory and a native searchable map browser; 453 of 540 map entries currently render. Original gameplay C runs natively; original ARM static terrain routines now execute through a bounded Thumb interpreter. New Bark and its seven interiors now support walking, running, ledge hopping and door/stair travel through admitted original input, collision, command and warp logic, with native scene presentation and the original fade/sound travel animation. A persistent native story owner now runs original initialization and admitted conditional script handlers. Elm’s lab now runs its original resume script and coordinate-triggered introduction, including the professor’s movement and reaction. The complete first-starter route now runs for all three starters: choice, original post-choice dialogue, five-Potion gift and leaving/re-entering with the chosen Pokemon following. The native nickname prompt currently supports keeping the original name. Full original field-controller execution is still incomplete.

The default field viewer now starts at the original bedroom position, without
running shoes. The connected original-script route covers Mom's introduction,
the friend/Marill scenes, all three starters, the five-Potion gift and Mom's
Pokegear gift and Elm's farewell, including the original rendered door animation
and phone registration. Route 29 travel, Cherrygrove's tour, Running Shoes and
Map Card, and Route 30's Apricorn Box gift now execute. Mr. Pokemon's introduction
now completes with the Mystery Egg, healing, Pokedex and Oak's phone number.
Elm's original incoming call now completes through a native phone view and
returns to the field. The return trip now triggers the visible first-rival battle
and completes its original post-battle dialogue, shove and departure for all
three starters. The original allowed-loss path heals and resumes too.
Returning to Elm now completes the police scene, native rival-name entry and
Mystery Egg handoff for all three starters, including cancellation/retry and
lab re-entry. See [iteration measurements](docs/iteration-deltas.md).
Walking in grass now starts visible wild battles with Fight, Run and Poké Ball
when available, original damage and EXP, level-up and field return. See [Mr. Pokemon's checkpoint](docs/mr-pokemon.md).
The starter now uses the [shared original application lifecycle](docs/application-lifecycle.md)
and the same original brightness controller as field scripts. Elm's scripted call now uses that lifecycle too; general Pokegear browsing,
general trainer encounters and broader Continue/time handling still need integration. See [Elm's phone call](docs/elm-phone.md).

Use **Jump to checkpoint…** at the top right to start at **Route 29**, **Before
rival**, **After rival**, **Before tutorial** or **After tutorial**, with any
starter. These fifteen prepared states come
from executing the original opening route; they include the required party,
items, flags and calls. **Back** restores the session you left. Checkpoints are
versioned native development snapshots, not original DS saves. See
[checkpoint validation and regeneration](docs/checkpoints.md).

Field menus now inspect your live session: click **Bag (B)** or **Pokémon (P)**,
or press B/P. The panel opens on the right and pauses walking. Use A/D for Bag
pockets, W/S for items or Pokémon, and Esc to close. Menu directions move once
per press; release before moving again. Item quantities/descriptions
and Pokémon HP, status, EXP, stats and move PP reflect story gifts and battles.
Press Enter on a Bag medicine, choose a Pokémon, and choose a move when needed.
HP/status/PP medicines, revives, vitamins and applicable berries use the original
item-effect functions and consume an item only when it takes effect. Rare Candy,
evolution, Sacred Ash and party rearrangement remain unavailable.

Cherrygrove’s Poké Mart clerk now offers Buy/Sell/Quit through the original
script. The native shop panel uses W/S to select, Enter to confirm, A/D to
adjust quantities by ten, and Esc to return. Original stock progression,
prices, purchases, sales and the ten-Poké-Ball Premier Ball bonus are connected
to the same saved Bag and money as battles and gifts. Poké Balls appear after
the original catching-tutorial unlock. See [item transactions](docs/item-transactions.md).

Mom’s first gifts now show acknowledgment boxes for the Bag, Trainer Card,
Save and Options unlocks, alongside their original fanfares.

The [battle runtime](docs/battle-runtime.md) now connects live land encounters
to a visible battle on the left and controls on the right. Choose with W/S and
confirm with Enter/Space. Original Pokémon artwork, HP, moves and PP are shown;
finished battles publish party/EXP changes back to the suspended field session.
The native battle command menu also offers **Bag** and **Pokémon**. Bag lists
supported HP/status/PP medicines; choose the party target and, for Ether/Max
Ether, a move. Esc backs out without spending an item or turn. Pokémon supports
voluntary switching and required replacement after fainting. Original item effects
update the active battler and party together; the original controller runs the
enemy response. Trainer medicine use, X items, other balls and move-learning/
evolution applications remain incomplete.
Normal/shiny sprite palettes now follow the original palette sources, and the
two-screen battle view scales to the available width.
The original trainer AI executes through the existing Thumb interpreter. All
three first-rival matchups now pass through the live field/cutscene interface,
including original messages, EXP, result publication and scripted departure.
The original catching tutorial now executes its automated Marill/Rattata battle,
capture and five-Ball gift for all starters. Capture presentation uses native
animation adapters; ordinary Poke Ball capture now publishes into the party or PC and survives
Save/Continue. Keeping the original name is supported; nickname entry, other
ball/item selection and Bag/party battle menus remain unported. Wild defeat can
now return home or to the registered Pokémon Center through the original
recovery scripts, with the healed result retained by Save/Continue. Center
recovery includes the original healing-machine task and texture animation.
Supported entrance doors now open using original prop animation and sound
before the travel fade, including Cherrygrove Pokémon Center. Exiting supported
buildings now opens the destination door, steps the player outside and closes it
before returning control. Normal Nurse Joy interaction now runs the original
greeting, confirmation, follower handoff and healing script; declining, fainted
lead recovery, Pokérus and Save/Continue after healing have connected checks.
Full character action frames/effects and contest defeat recovery remain unfinished.

The field viewer now plays original map music, script effects, item fanfares and
Elm’s phone sounds through native audio output. See [audio implementation and checks](docs/audio.md)
for ownership, playback evidence and remaining fidelity limits.

For scenery browsing, check **Explore freely** in the viewer or launch
`./build/heartgold_field_viewer --explore 76` (Goldenrod). This separate session
suppresses story events and NPCs, uses existing walking/travel, and restores your
suspended story session when unchecked. Unsupported scenes remain view-only or
report their resource limitation. See [free exploration](docs/free-exploration.md).

The active target is now a continuous new game through Johto's eight badges,
Lance, Hall of Fame and first credits, before postgame Kanto exploration.
Complete saving remains the first priority. The native viewer now offers
**Save** and **Continue…** using the original save manager and its complete
42-record owner plus six extra-record APIs. Original elapsed play time now advances
and survives Save/Continue. Original daily/minute updates now run on Continue
and in the idle field. Original automatic trigger-call selection/ringing and
Baoba call presentation now execute. The original Bug Contest owner, timer,
script commands and Sport Ball battle capture also execute in connected
component scenarios; the full National Park route and generic calls remain
incomplete. Save unlocks after Mom's introduction;
wait for field objects to stop and close menus/dialogue first. Save and Continue
are separate from the development checkpoint picker. See
[save evidence and remaining acceptance](docs/save-acceptance.md). See the
[save inventory](docs/save-inventory.md) and
[Johto implementation boundaries](docs/johto-playthrough.md).
The [opening route](docs/opening-route.md) records the narrower executed checkpoint
and its remaining presentation and runtime limits.


See [the full-game workstreams](docs/full-game-workstreams.md) for the generated whole-source/function inventory, shared solution contracts and current batch fixes.

See [platform architecture and field integration gates](docs/platform-architecture.md) for reusable DS boundaries, source-verified field relationships and current execution limits. See the [reuse audit](docs/reuse-audit.md) for measured function/import inventory, library and license findings, and the next continuous-gameplay integration sequence.

## Build and run

Requires Apple Silicon macOS, Xcode command-line tools (Apple Clang and macOS SDK), CMake 3.24+, and Python 3. No Nintendo SDK, ROM, Wine, original compiler, or external CPU emulator is required; the project includes a bounded Thumb execution engine for the terrain slice.

Keep a locally obtained, unmodified decompilation at `upstream/pokeheartgold`, or supply `-DHG_UPSTREAM=/absolute/path`. Tested revision: `708163e10169173d2bbab252538231721d983155` from pret/pokeheartgold.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/heartgold_native
./build/heartgold_metal
```

`heartgold_native` runs original bag allocation, item add/remove, quantity, price lookup, and TM lookup. Expected output includes `pointer=64 bits; bag=1948 bytes; Potion quantity=3; price=300; TM01 move=264`.

`heartgold_metal` opens the inventory window. Its seeded inventory is temporary and resets on launch; it does not load or write a save file. All eight pockets contain sample items. Select a row to see its original icon, name, description, and current quantity. The default **Classic** view shows two 256x192 screens side by side, using original tilemaps, sprites, animation data, item icons, and font glyphs. Click item rows on the left or pocket graphics on the right. **List** switches back to the macOS table layout. This is our composition of original resources, not the full original bag overlay; that overlay is ARM assembly and is not ported.

| Input | Action |
| --- | --- |
| Up / Down | Select previous / next item through the original menu engine |
| Left / Right | Switch pocket, remembering each pocket's selection |
| Page Up / Page Down | Page through the original menu engine |
| + or = / − (keyboard hyphen) | Add / remove one selected item |
| Return | Confirm selection and show its name; does not use the item |
| Mouse | Click pocket tabs, item rows, and Add/Remove buttons |
| Connected extended gamepad | D-pad selects/switches pockets; A/B adds/removes; shoulders switch pockets |
| Command-Q or close window | Exit |

Adding to an empty pocket inserts a representative item for that pocket. Removing the last item shows an empty state. Stack caps and sorting are enforced by original bag functions. Gamepad input currently responds to discrete presses, without held-button repeat. Physical gamepad behavior has not been verified on hardware.

Additional commands:

```sh
./build/heartgold_metal --decode-only  # decode the original Ultra Ball icon, no window
./build/heartgold_metal --smoke        # exercise UI/input and complete a GPU draw, then exit
```

The window requires a graphical login session and Metal device. Resources are read from an absolute build-directory path; this is not yet an application bundle or relocatable installation.

## What actually runs natively

CMake compiles the entire original `src/item.c`, `src/bag.c`, `src/list_menu.c`, `src/font_data.c`, `src/text.c`, `src/string_control_code.c`, `src/sys_task.c`, `src/sys_task_api.c`, and `src/systask_environment.c` directly from upstream into `hg_logic`. A separate `hg_field` library compiles original `src/map_header.c`, `src/map_matrix.c` and `src/metatile_behavior.c`. Original item, bag, bag cursor, list-menu, and list-menu-item declarations remain in use. No original game algorithm is copied, translated, or rewritten.

The inventory uses original `Save_Bag_New`, `Bag_AddItem`, `Bag_TakeItem`, `Bag_GetQuantity`, `Bag_GetPocketSlotN`, `GetItemNameIntoString`, `GetItemDescIntoString`, and `GetItemIndexMapping`. Original `ListMenuInit`, `ListMenu_ProcessInput`, selection/scroll getters, and `DestroyListMenu` drive row selection, scrolling, and confirmation. Original `BagCursor_*` functions store each pocket's cursor and scroll position. Mouse row selection is translated into steps through that same original menu engine.

`src/inventory.c` is the native application adapter: it seeds data, builds menu entries, switches pockets, and exposes a C API to Cocoa. It does not port the original full bag overlay or item-use state machine. AppKit draws the surrounding controls and the optional List layout. The Classic scene is rasterized into bounded host pixel buffers and displayed by Metal; it uses original font-loading, glyph-expansion, and glyph-width functions.

## Compatibility boundary

Our locally authored `src/compat/include/nitro/types.h` supplies fixed-width DS scalar types, volatile aliases and a four-byte BOOL; its 64-bit integer aliases use original four-byte data alignment. It contains no hardware access APIs. It is shared by the runtime and direct upstream includes. `test_sdk_types` checks it independently against original personal/evolution record layouts.

`src/compat/native.h` suppresses SDK-dependent header bodies using their existing include guards and supplies fixed-width scalar types, opaque unused types, and boundary declarations. It is force-included for C only. The Objective-C UI calls a C API because Objective-C BOOL and the game's four-byte BOOL have different ABIs.

Implemented services:

- malloc/free-backed heaps (heap IDs do not yet describe distinct arenas), memory clear and copy;
- item parameter reads from generated typed data, interpreted by original attribute functions;
- bounded NARC member reads for icons and private UTF-8 message containers;
- message and string adapters used by original item name/description entry points;
- input bits for the original list menu, plus a window/cursor adapter that records invalidation, scrolling requests, and cursor positions;
- four native-owned original task queues, with original queue APIs and a bounded monotonic 60 Hz host driver. The original `System` declaration is retained with native pointers, and key updates preserve its queue fields.

The menu's DS drawing calls notify the native view boundary; they do not rasterize original DS window tiles or fonts. Its menu entries deliberately have null DS String pointers, since the native table obtains text through the item APIs. The native String/MsgData adapters are not DS binary layouts. The original font code receives the original font archive bytes and validated glyph codes, independently of these string adapters. Only the glyph lookup-table and expansion functions from `text.c` are enabled. Task queues now run original scheduler code, but full text-printer rendering, `FontID_RenderText`, and focus-indicator graphics remain explicitly unsupported. The original save manager now supplies save-array access for the field story owner; full bag-view APIs still abort explicitly. Compiling their callers does not mean those features work.

`ListMenuGetTemplateField` contains original pointer-to-s32 casts. Clang reports these during compilation. The native adapter does not call pointer-valued cases; it uses original typed structures/accessors. Those casts must be addressed at a boundary before introducing any such callers.

## Resource preparation

The original build uses Meson, a Metrowerks-compatible ARM946E compiler, ARM assembly, Nitro/NNS/DWC includes, and asset tools such as csv2bin and nitroarc. We inspected its actual source dependencies, manifests, build configuration, and archive tooling; that ROM build is not invoked.

Our configure step runs two Python scripts, writing only into ignored `build/generated`:

1. `prepare.py` generates needed archive-index headers and typed ItemData initializers from the original CSV/manifest. It verifies CSV archive order against the original item mapping table: **item IDs differ from archive member indices**. Original `GetItemAttr` interprets these records. Host-compiled initializers avoid casting disk bytes to host bitfield structs; padding is zero initialized rather than preserving archive padding bytes.
2. `pack_resources.py` places original NCGR/NCLR/NCER/NANR bytes, unchanged, in a local NARC container. It also copies the original `files/a/0/1/5` bag archive and `files/graphic/font.narc` byte-for-byte into the ignored build resource directory, and derives a UTF-8-to-glyph-code table from the upstream charmap. Original English message JSON is normalized to UTF-8, with each string in a private per-group NARC container. Unused garbage description slots become empty strings. These message archives are an explicit compatibility format, **not the original encrypted DS message binaries**. Original message decryption, DS character codes, control codes, and original String algorithms are not ported.

`ds_resources.c` validates NARC headers, block sizes, member tables, and member bounds before returning views into cached file data. Archive files are capped at 16 MiB by the local loader, except the original land archive which has a specific 32 MiB cap. A bounded LZ10 decoder supports overlapping backreferences and rejects truncation, invalid distances, and output overflow. Current item icons are uncompressed, so LZ10 is tested independently and is not needed by this inventory path. LZ11 and automatic compressed-resource dispatch remain unimplemented.

The cache is process-lifetime and main-thread-only. The font boundary validates header lengths, tile dimensions, glyph data size, width table bounds, and glyph widths before invoking original font code. NARC member and absolute-position reads are bounded by the owning archive buffer. Resource changes require regeneration/rebuild. `HG_UPSTREAM` selects the source checkout, not a ROM or SDK installation.

## Memory and graphics architecture

Native pointers are 64-bit; DS addresses are 32-bit values. `ds_memory.h` independently demonstrates bounded translation of DS main RAM starting at 0x02000000 into a host 4 MiB buffer. Gameplay currently uses ordinary native pointers and is not routed through that buffer. Native C gameplay uses ordinary host pointers. Admitted original Thumb routines execute in isolated guest RAM with typed native handles; no low-address host mapping or general CPU/hardware emulation is performed.

Do not truncate native pointers to u32 or cast DS addresses directly to pointers. Fixed low mappings would not solve pointer-containing struct layouts, serialized pointers, assembly, function pointers, or MMIO. Native runtime objects should use native pointers; serialized/resource addresses need explicit 32-bit representations and boundary decoders. Future translated VRAM, palette, and OAM regions can track dirty writes. The current arena does not emulate mirrors, MMIO, DMA, caches, interrupts, or timing.

`ds_graphics.c` validates little-endian NCGR/NCLR headers and decodes the supported 32x32 uncompressed tiled 4bpp slice, converting BGR555 palettes to RGBA8 with index zero transparent. Metal uses a runtime-compiled shader and nearest-neighbor texture sampling. Before replacing the texture, the prototype waits for any preceding draw to finish. This is a small correctness-first renderer, not a complete DS GPU.

`ds_scene_graphics.c` adds bounded resource decoders for normal 4bpp NCER cells (signed placement, shape/size, flips, palette banks and 1D tile mapping), NANR forward/loop sequences with frame durations and index/SRT/translation records, and text NSCR maps with tile flips and palette banks. Classic pocket changes use the original bag SRT animation at 60 ticks per second. Rendering uses nearest sampling and an integer pixel viewport when the window permits it; mouse coordinates pass through the same viewport transform.

`game_font.c` calls original `FontData_New`, `TryLoadGlyph`, `GetGlyphWidth`, and `FontData_Delete`, with original `GenerateFontHalfRowLookupTable` and `DecompressGlyphTile` doing glyph expansion. It adds a UTF-8/glyph-code boundary and native placement into the pixel buffer. Tests also execute original direct/lazy resource access, access-mode switching and string-width calculation. The full original text-layout/printer engine is not enabled.

The scene uses the bag overlay's actual resource references: archive `a/0/1/5`, top tiles/palette/map 7/8/94, bottom 46/38/39, and pocket characters/palette/cells/animation 2/3/1/0. Item cell/animation indices come from original item functions. The native layout adds list panels and pocket labels; it is not an exact DS screenshot reproduction. The original bag's 3D model is not rendered.

Current limits include affine OAM matrices, 8bpp, OBJ window/semitransparent modes, cell VRAM-transfer banks, reverse animation playback, and complete BG/OBJ priority/blending semantics. SRT resampling currently uses a 128x128 intermediate cell canvas, suitable for these bag assets. There is no complete GX register/MMIO translation path; the later model renderer interprets its admitted model display-list commands. All decoded addresses are file offsets or buffer indices; none are interpreted as host virtual addresses.

## Tests and verified behavior

Twenty-seven CTest checks cover:

- original item/bag attributes, healing amount, limits, removal, compaction, sorting, registration, copy, moves, and item ID mappings;
- DS scalar widths/qualifications and original personal/evolution data layouts, plus bitfield packing;
- DS RAM translation boundaries and synthetic tiled graphics;
- decoding the original Ultra Ball graphic;
- NARC malformed lengths/tables, mutation bounds checks, and LZ10 overlapping copies and failure cases;
- original list-menu header skipping, scrolling, callback, confirmation and cancellation; inventory mutation, cap rejection, pocket memory, empty/refilled pockets, UTF-8 names, and names/descriptions/icons for all 537 item IDs;
- original map-header/matrix loading across all 540 map IDs (280 referenced matrices), validation of all 288 original matrix resources, cell metadata, matrix mutation functions, metatile predicates and invalid field CLI input;
- textured rendering semantics: signed delta/full vertex equivalence, named material/texture/palette binding, addressing, depth order, cutout, alpha blending, shared-edge coverage, camera changes/reset and malformed texture resources;
- original walk/run command selection, running-shoe/B gating, stateful diagonal input, step statistics, encrypted statistics records and RNG/encryption vectors;
- all 14 New Bark portal anchors, staged local-field commits, interior movement, town/house and upstairs/downstairs travel with retained player statistics, and the original fade-out/scene-swap/fade-in presentation with the door/stair sound and busy-during-fade input gating;
- the original directional ledge hop selected from the player-input collision mask (command 56+direction over the original sixteen-tick, two-tile arc);
- original event-object initialization, fourteen visible event actors, source metadata, real occupancy and staged population replacement;
- original new-game flag script, conditional conversations, persistent variables/flags, original forward/backward control flow/frame and movement waits, player-name formatting, empty party query, isolated temporary state/RNG and publication/scene rollback;
- original task priority/FIFO order, queue capacity/reuse, insertion/removal during dispatch, environment lifetime, four-queue routing, input-state preservation and bounded 60 Hz host timing;
- original font direct/lazy loading and mode switching, glyph expansion and width measurement, all-item NCER rendering compared with the previous icon decoder, original pocket SRT animations and tilemaps, two-screen rendering and hit testing, and malformed/truncated sprite/animation resources.

Tests keep their checks active in Release. Original ItemSlot and ItemData sizes are checked as 4 and 36 bytes. Those checks do not establish general binary save/SDK ABI compatibility.

The separate graphical smoke test exercises actual buttons, a synthesized keyboard event, table selection, Classic/List switching, screen-coordinate mouse mapping, original item text, and a successfully completed Metal scene draw. The CPU scene preview has also been visually inspected; no original-game screenshot comparison has been performed, and physical controller input remains unverified. All 27 checks pass normally and with AddressSanitizer + UndefinedBehaviorSanitizer on this host.

Sanitizer checks for C code:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize -j
ctest --test-dir build-sanitize --output-on-failure
```

## Limits and next milestone

Original gameplay preconditions and bugs remain: invalid pocket IDs may leave locals uninitialized, and the original empty-slot add helper does not validate an initial oversized quantity. The UI passes valid IDs/pockets and adds one at a time. Item use, persistence, full field gameplay, battles, and original bag overlay behavior are absent.

The native map browser now provides search, a scrollable map selector and Previous/Next controls: `./build/heartgold_field_viewer`. See [docs/textured-field-viewer.md](docs/textured-field-viewer.md) for controls, validation and limitations. Original prop placement/traversal now supplies houses, doors, signs and the windmill. Next: persistent field/object lifecycle and collision-height/movement integration. Bounded model meshes, the supported multi-node SBC transform slice, CPU wireframe/solid diagnostics and collision-table validation now work. Original message character-code/decryption paths and DS window/font drawing remain separate work. Rendering the original bag 3D model will require a separate audited geometry/texture path. The resource scene and original menu loop now give those additions a working graphics/input target. A playable application still needs full main-loop/overlay integration, save/filesystem compatibility, map scripts/movement, complete audio integration, battles, and further pointer/ABI auditing.

## Repository hygiene

Only our implementation, tests, scripts, CMake configuration, and documentation belong in Git. `upstream/` and `build*/` are ignored, as are common ROM/archive/resource formats. Never commit original source/assets, generated item/message tables, ROMs, SDK components, binaries, or game screenshots. Review staged files as well as ignore rules. Upstream must remain unmodified.

## Process and address isolation

The executable is a normal user process and requires no administrator privileges. No original hardware-register/MMIO path is enabled by the current native slice. DS addresses are not mapped onto low host addresses. The new font sources were inspected for fixed DS hardware addresses; the original pointer-to-s32 menu getters noted above remain uncalled. A mistaken unmapped access would ordinarily terminate the app under macOS memory protection, rather than grant access to another process or kernel memory. This does not replace code review or make native C memory-safe; bounds validation and sanitizer tests remain part of each new subsystem milestone. The local runtime opens resource archives read-only and does not write save data.

## Whole-source compilation audit

Run `cmake --build build --target audit_native` for a compile-only survey of the original game and selected support manifests. It never runs the generated objects. On the tested revision after field-data integration, 32 of 399 game C units emitted ARM64 objects with the existing compatibility layer; none of the 286 selected game ARM/Thumb assembly files assembled for ARM64. That is compilation coverage, not a playable-port percentage. Additional successful files are not claimed as ported.

The initial measured blockers, exact scope, SDK/resource dependencies, and scheduler recommendation are documented in [docs/native-audit.md](docs/native-audit.md). Raw compiler commands, diagnostics, include graph and object files stay under ignored `build/audit/`. An audit target can succeed while game compilation fails, because recording those failures is its purpose.

The first shared SDK scalar interface removes the `nitro/types.h` blocker from 175 game C files, exposing deeper dependencies without increasing the 27-file compilation count. See [docs/sdk-types.md](docs/sdk-types.md) for measured before/after categories and reproduction commands.

## Original scheduler milestone

The original task queue, queue API, and environment implementations now execute natively and are behavior-tested. A task dispatched by the original scheduler drives the app's animation ticks. The terminal demo also dispatches 60 native frames. See [docs/scheduler.md](docs/scheduler.md) for lifecycle, timing, tests, original-code boundaries and remaining main-loop work. This is a working scheduler integration, not a port of DS interrupts or the complete game loop.

## Original field-data milestone

`./build/heartgold_field` inspects New Bark Town using original map-header and map-matrix functions. Try `--map 63` for the player's house or `--tile 2` to execute original tall-grass behavior predicates. Use `--terrain 60` to inspect actual New Bark attributes through original terrain predicates. All 676 land chunks are bounded-decoded, and world-tile queries select resources through original matrix functions. `--wireframe 60` writes a transformed land PPM diagnostic to stdout; `--solid 60` adds opaque triangle shading and a host depth buffer. See [docs/model-geometry.md](docs/model-geometry.md) for geometry, collision validation and limitations. The separate `heartgold_field_viewer` now provides a textured interactive land view; props now draw through original traversal; New Bark supports the limited walking integration described below. See [docs/field-data.md](docs/field-data.md) for source boundaries, complete resource/test coverage, the original byte-sized matrix-ID limitation, and the next map-chunk milestone.

## Original prop lifecycle and execution evidence

`cmake --build build --target field_evidence` runs a reproducible New Bark prop scenario and generates a compiler-derived dependency graph, symbol-resolved original execution trace, provenance hashes and ranked unsupported boundaries. The unchanged original `src/field/map_prop_manager.c` now executes allocation, loading all 17 original placements, getters/mutation, slot reuse, capacity, reset, fallback, reload and cleanup. Twenty-two of its 24 original functions are observed, including original drawing traversal. Safari placement and the separate view-culling/lighting draw path remain unexecuted. This adds original execution rather than another visual reconstruction. See [docs/field-lifecycle.md](docs/field-lifecycle.md) for contracts, evidence scope and the next integration frontier. Props are now visible in the textured viewer; see [docs/prop-rendering.md](docs/prop-rendering.md) for the shared graphics boundary and its verified limits.

Original ARM terrain execution and its limits are documented in [docs/thumb-terrain.md](docs/thumb-terrain.md). `heartgold_field --terrain 60` now executes original static height queries as well as original metatile predicates.

See [docs/map-browser.md](docs/map-browser.md) for the 453/540 rendering audit, multi-chunk scenes, map exports and browser controls.

The native viewer now executes the original overlay manager for its scene lifecycle. [Playable integration](docs/playable-integration.md) distinguishes this application shell from the original field/game loop.

The native viewer also has a **Characters · sprite meshes** category: 192 supported overworld character sprites, including all 16 gym leaders, with search, orbit and zoom. These are original billboard sprite meshes, not full volumetric human models. See [character inspection](docs/character-browser.md).

Original `task.c` now executes child/parent field tasks and application waits in the running scene app, on a native timer independent of camera redraws. See [field task runtime](docs/field-task-runtime.md) for execution evidence and the remaining avatar/field-loop boundary.


Run `./build/heartgold_field_viewer` to start at the original bedroom position. Click the scene and hold **WASD** to walk; **Shift + WASD** runs once running shoes are available. Selecting a map creates a separate inspection session; ordinary travel retains the same game state. Original collision checks gate each step; release stops movement. The camera follows the player; arrows/drag orbit, scroll or +/− zoom, shift-drag pans, and R recenters. Door and stair travel connects New Bark with its seven interiors, retaining the player and step count, and now plays the original fade-to-black/scene-swap/fade-in with the door/stair sound. Ledges hop down with the original sixteen-tick, two-tile arc. Other maps remain inspection views. Original walking commands now advance the player’s position and static terrain height each tick through their original command tables. Original hero textures animate direction and stepping in a native billboard; pose selection remains native. See [original walking execution](docs/original-walking.md) and [walk/run control and statistics](docs/original-locomotion.md). Face the town or lab sign and press Enter/Space to read its original text. Professor Elm, the red-haired rival at the window, the breeder, Mom, the town woman, the boy in Elm’s upstairs apartment and the southwest-house woman now use the [persistent script owner](docs/story-runtime.md). The rival’s two messages and original shove/jump movement complete before input resumes; the breeder and player-house sign format the inspection name Ethan. Admitted conditions execute original handlers and state survives travel. Unsupported interactions display a dismissible explanation; the original introduction, starter acquisition and Pokegear route now run as described in [the opening route](docs/opening-route.md). Enter/Space advances pages and closes the final page. See [original transition execution](docs/original-transitions.md) for the eight-map network and source/host boundaries. Twelve actors selected by original flags now render and block movement through original occupancy; see [event actor integration](docs/event-actors.md). Original animation-controller execution, NPC AI, general story interactions, outdoor header crossings and full game startup remain pending. See [background interaction](docs/field-interaction.md) for original event/script execution and native UI boundaries. See [player movement](docs/player-movement.md) for execution boundaries and tests.
