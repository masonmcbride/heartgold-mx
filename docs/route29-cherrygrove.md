# Route 29 and Cherrygrove checkpoint

This earlier checkpoint is extended by [Mr. Pokemon](mr-pokemon.md): the house
introduction now completes; the next rejection is Elm's incoming phone call.

The original bedroom-to-starter route now continues through Elm's farewell,
Route 29, Cherrygrove's complete tour and north-exit Map Card gift, and Route 30's
Apricorn Box gift. All three starters retain their party, bag, follower and phone
contacts. Running Shoes are acquired through the original tour, not seeded.
Mr. Pokemon's house is reachable; its introduction rejects the missing screen
fade before Mystery Egg/Pokedex gifts publish. Encounters/battles remain absent.

## State and graphics

The entire unchanged `apricorn_tree_sys.c` executes. Its pointer-free original
records belong to each story candidate, so clones, map travel and rollback keep
independent fruit availability. Original `ApricornTrees_Init` followed by the
new-game initializer `sub_0202AE0C` matches `overlay_36.c`; no RTC refill is
invented. The original `sub_020640A4` Thumb selector and original sprite LUT run
in the existing guest engine. Route 29's tree selects its original green fruit
variant. Harvesting animation and daily updates are still unconnected.

`prepare_characters.py` now resolves template-to-mesh relationships from the
original template metadata and mesh LUT. It admits nondirectional tree and item
objects and Apricorn templates through the existing renderer. Cuttable trees use
mesh 266, item balls mesh 267 and Apricorn trees mesh 278. These remain native
billboard presentations of original geometry/textures; idle animation controllers
and NPC AI are not established. Green/empty Apricorn, cuttable-tree and item-ball
frames were generated build-locally; the green Apricorn frame was inspected.

The original HasItem handler enables the actual no-Apricorn-Box script branch.
Once the Box is owned, unsupported harvesting still rejects visibly.

## Shared ownership fixes

An admitted outdoor crossing updates its step statistics against the staged
destination owner. Previously the input updated the live source while publishing
an older destination snapshot, losing the crossing step. Tests now require the
step to persist and exercise forward and reverse crossings. Cancellation returns
the session's statistics pointer to the live owner before freeing the candidate.

Original GiveRunningShoes executes using a privately owned original local-field
record, allocated only when needed. Its PlayerSaveData result synchronizes with
the scene avatar and publishes at successful End. Continuation clones own the
record, and rejection cannot grant equipment. This does not connect the full
original save array or disk persistence.

Original visibility movement commands 69/70 use the existing command table and
object flag boundary. The Apricorn man now returns inside and becomes invisible
through the actual movement script before the original HidePerson handler runs.

## Verification

The connected scenario covers all three starters through the tour, Map Card,
Apricorn Box and rejected Mr. Pokemon introduction, then returns to New Bark.
The original Egg/Pokedex gifts remain unpublished after rejection. Component
checks cover tree ownership, species/color selection, empty-state travel retention
and all seven fruit variants. The shared route planner now searches the bounded
original map-header footprint, including multiple chunks, and uses actual script
IDs for standard NPC interactions as well as local scripts.

These are script/movement scenarios. The missing original encounter loop means
that walking through grass here does not demonstrate authentic field gameplay.
Phone calls, party healing, Pokedex presentation and battle/field return remain
required for the complete opening target.

All 32 checks pass normally and under ASan/UBSan. The native AppKit smoke check
also passes. Upstream remains unmodified and generated evidence stays ignored.
