# First-starter milestone

The active target and current expanded route are in [the opening route](opening-route.md). The connected scenario now begins at the original bedroom position, includes Mom's introduction and both friend/Marill scenes, and returns to Mom for the Pokegear. It currently stops at the required west-exit door-animation service. The sections below retain the starter subsystem evidence.

The connected acceptance route now passes for **Chikorita, Cyndaquil and Totodile**: start at the original bedroom location, complete the initial house/town scenes, enter Elm’s lab, complete the original introduction, select a starter, keep its original name, complete the post-choice conversation, receive five Potions, leave with it following, walk back and re-enter with the same party and bag.

This is the native inspection session, not original boot/new-game execution. State is retained during the session; disk saving and battles are not connected. The nickname prompt currently admits **No**; the native UI marks Yes unavailable and does not submit it to the unported naming application.

## Connected original behavior

`original_background_interaction` drives actual application input from the bedroom through the house and town, the lab portal, the introduction trigger, the ball-machine event, the picker and the original scripts. It repeats the complete route for all three choices. It checks story flags/revisions, actual party species and Pokedex registration, the lab’s scene reaching 2, five Potions in the story-owned original bag, follower coordinates outside, reverse walking and lab re-entry without duplicating the party or gift. No story flags are planted to bypass the introduction or acquisition.

The original `LaunchStarterChoiceScene` / `CreateStarter` task runs through the original task manager. Original code creates the candidates, assigns trainer/encounter metadata, inserts the selected Pokemon, and updates the Pokedex and score. The native boundary provides a visible input-driven picker, fade, host calendar and field-scene ownership. Original map-section conversion runs through the existing Thumb bridge. Original script handlers store the starter choice, query and buffer the actual party, position the follower and switch its movement modes. Original `ReleaseAll` resumes the object manager.

The lab exit exposes the original assistant’s gift script. Original `CallStd`, its native wait callback and `RestartCurrentScript` now execute with bounded child contexts. Contexts share original special-variable and formatting semantics while each child keeps a private candidate until it returns. The standard item routine from original bank 3 runs unchanged: item pocket/fanfare selection, original bag insertion, singular/plural item and pocket buffering, both messages and return. The original assistant script then advances the lab scene. UTF-8 presentation reads original noun/plural/pocket message banks and admits the specific original formatting/color controls used here; it does not invent English inflection.

`original_story_runtime` separately checks that a child item routine cannot leak a gift during preflight, after a later unsupported opcode or on cancellation after its first message. A successful root publishes the complete gift once, with shared special-variable results and RNG isolation. Representative singular gifts in all eight bag pockets also execute the original standard routine, including its pocket/fanfare branches; rendered text contains no leaked formatting controls.

## Follower lifecycle

`prepare_follow.py` computes an 18-function closure from unchanged `follow_mon.c`, retains its original model/form/female lookup tables, validates the four-byte parameter schema, and records source/slice hashes. The original follower factory sets ID, script, species/form/shiny parameters and flags. Allocation is adapted to a native-owned inactive slot initialized by the original event initializer; metadata is supplied through the existing bounded NARC reader. The selected starter is read from the actual party, rather than a separate native selection variable.

The complete original `unk_020658D4.s` is assembled into the existing guest image. Both lab movement mode 55 and normal following mode 48 execute their original state machines and movement-command dispatch. The bridge now marshals the independent 16-byte AI scratch record, distinguishes the player handle from the follower handle, and retains the original 32-byte movement-notification state. Original reset and relative-position routines execute through typed native imports. Unsupported imports still fail explicitly.

The runtime initially consumed the player’s end-of-step flags before the original follower could observe them. Player frame advancement now uses original `sub_02062400`, lets the follower observe the completed command, then clears the player’s held movement. Script-owned movement still uses original `sub_02062428` completion consumption. `original_follower_controller` independently checks scripted following, normal following, overlap-at-entry, four directions, walking and running.

Followers use the same actor pool, occupancy, resource publication and map transactions as NPCs. The native renderer binds original type-19 textures and provides a two-frame directional presentation for the admitted resources. Original NNS sprite/animation controllers and emergence graphics are not executed. The original ten-frame script wait remains; native presentation makes the real follower visible. Sound requests remain silent.

Supporting type-19 resources also admits two previously unsupported event Pokemon: the eight-map initial population now has 14 active and 10 conditional actors, with no unsupported initial event actors. This is not a claim that every follower resource, movement mode or other map works.

## Original Pokemon runtime and record ABI

`prepare_pokemon.py` selects 136 unchanged function definitions for creation, encrypted access, stats, moves, party operations, encounter/trainer metadata, Pokedex updates, profile/options, mail, encoded strings and shininess. It retains original declarations, tables and source/resource hashes. Original `pm_version.c` compiles whole; RNG/encryption come from the existing whole `math_util.c`. Closure membership establishes compilation/linkage, not execution of every branch.

Personal data uses upstream jsonproc, its original template and 44-byte record; growth data comes from the original CSV, and move/learnset NARCs remain unchanged. Original encoded String functions have prefixed symbols and remain distinct from the inventory’s UTF-8 owners. The shared scalar boundary preserves original four-byte alignment for 64-bit DS integer fields. Independent checks enforce BoxPokemon 136 bytes, Pokemon 236 bytes, party offset 136 and Party 1456 bytes. Native-pointer owners intentionally are not DS binary layouts.

## Evidence and limits

All 30 CTests pass in normal and ASan/UBSan builds for this milestone. Native application frames of the lab, picker and three followers outside were generated under ignored `build/evidence/first-starter/` and visually inspected. The connected application is exercised headlessly; native-window verification is recorded with the expanded opening route.

Reproduce the input-driven route with `./build/test_interaction`, or set `HG_ACCEPT_EVIDENCE=build/evidence/first-starter` from the repository root to write PPM frames into an existing ignored directory. Original source/assets, generated kernels, parameter archives, screenshots and logs remain outside version control. Upstream remains unmodified.

The next integration gates are broader original field lifecycle and script services, naming, party/bag menus, saving and battle orchestration. This milestone does not imply those paths work or that the whole game compiles.
