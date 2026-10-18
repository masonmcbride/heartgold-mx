# Original event actors in field exploration

The eight-map New Bark inspection session now instantiates **twelve flag-selected event actors**. Their original sprites render in the same depth buffer as the player, buildings and terrain, and their original map objects participate in player collision. The population changes with door/stair travel. Their visibility uses the [persistent story owner](story-runtime.md), including original initialization and transition hooks. Actors remain stationary between admitted script movements; original movement IDs and ranges are retained for later AI integration.

| Map | Active | Hidden by original flags | Unsupported |
| --- | ---: | ---: | ---: |
| 60 New Bark | 4 | 5 | 1 |
| 61 Elm lab 1F | 2 | 2 | 0 |
| 62 Elm lab 2F | 2 | 0 | 0 |
| 63 Player house 1F | 1 | 0 | 0 |
| 64 Player house 2F | 0 | 3 | 0 |
| 65 Southwest house | 1 | 0 | 0 |
| 66 Rival house 1F | 1 | 0 | 0 |
| 384 Rival house 2F | 1 | 0 | 1 |

Ten events are hidden by real original flags. The two unsupported events are static Marill sprites, whose specialized template is not implemented; they create no invisible colliders. The original new-game flag script and admitted transition hooks run, but automatic resume/frame cutscenes and full startup do not. Elm, the rival, the breeder, Mom, the town woman, the upstairs boy and southwest-house woman have admitted conversations. Player-name formatting and the rival’s movement sequence now work; general formatting and other commands still fail explicitly. NPC AI and trainer detection remain unimplemented.

## Original execution and host ownership

`src/compat/map_objects.c` includes the complete unchanged original `src/map_object.c`. This replaces the former standalone compilation of that file; it does not compile a second copy. The boundary can call its private `MapObject_InitFromObjectEvent` and `sub_0205EC90` directly. Original accessors, sprite-ID resolution for fixed IDs, parameter/range copying, initial/current/previous coordinates, Q12 placement, facing and activation execute. Original `MapObject_ClearHeldMovement` runs through the existing Thumb bridge. `motion_provenance.json` records whole-source and declaration hashes; the reuse audit reports included originals separately from standalone translation units.

The native owner validates the serialized 32-byte pointer-free event before initialization, resolves the static BDHC surface and owns the resulting sprite resources. It retains the original movement metadata but does not call full `MapObject_CreateFromObjectEvent`, DS sprite resource callbacks, object system tasks, movement-type AI or original reconciliation. Original flag services select visible events. Original graphics-variable lookup resolves dynamic event IDs through the bound native story owner; there is no fabricated flag service. A caller without a bound owner cannot create a conditional/dynamic actor.

The manager owns a stable contiguous array with the player first, followed by event actors. Existing original occupancy code sees these objects through the existing typed guest handles. There is no separate native list of collision rectangles. Native presentation obtains coordinates and metadata through original accessors.

Event sprite IDs and texture archive member IDs differ: for example `SPRITE_GSWOMAN1` is 325, while its model resource is 123. Preparation derives the standard-template mapping from the original sprite table and constants. The renderer selects the directional standing texture. Type-0 and type-7 templates share the existing geometry/texture path. Type-7 uses the original mesh's height and texel scale, with native horizontal geometry/UV expansion to preserve its complete texture canvas; the former center-strip crop clipped wider hats and arms. It supports the profile-selected friend. Other specialized templates remain unsupported. The gallery renders 192 of 266 slots, with original sprite-to-model identities retained.

Destination actors and sprite resources are prepared before committing local-field state. Failure frees the candidate and retains source objects/resources. On success, the native owner replaces the population while retaining the existing avatar pointer, player slot, statistics and session. Destination actors are reconstructed on reentry; NPC AI state is not persisted. Flag changes during a completed script stage a replacement population before story publication; actors that remain retain current position and facing. Native conversation-facing effects can replace a borrowed sprite before the next travel; callers must reacquire actor presentation after interactions.

## Evidence

`original_event_actors` verifies original metadata and exact Q12 initialization independently, original occupancy and height separation over an initialized actor, invalid-input preservation, every admitted actor against its source event, explicit conditional counts, render contribution, invalid billboard rejection, a real blocked player step and population replacement through a house round trip. A rejected destination retains the same borrowed source sprites. The existing fourteen-portal and movement corpora still pass.

All **27 CTests** pass normally and with ASan/UBSan. The AppKit smoke test passes. Ignored `build/evidence/event-actors-town.png` and `event-actors-house.png` were visually inspected for town characters, Mom, original sprite proportions and terrain/prop occlusion. These checks establish static event population and collision, not full field startup or story progression. Original initialization, conditional targeting and script publication have separate [story evidence](story-runtime.md).
