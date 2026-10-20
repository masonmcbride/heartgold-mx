# Shared dependency plan

Measured 2026-10-20. Target: original bedroom start through the first rival battle
and catching tutorial. This is an implementation order, not a completion estimate.

Current checkpoint: live grass encounters now enter a visible wild battle and
return EXP/party changes to the same field owner. Original trainer AI and
blackout recovery are the next battle bottlenecks. See
[battle runtime](battle-runtime.md#live-grass-encounters-and-field-return).
Historical measurements below describe earlier admission stages.

## What the measurements mean

The existing `build/audit/WORKSTREAMS.md` surveys 706 source units and indexes
29,745 lexical C definitions/assembly markers. Its first compiler barriers are
not a count of missing runtime services. In particular, zero units assigned to
persistence does not mean saving works. That survey is background evidence;
rerunning the whole compiler survey after every small change is not useful.

The current generated battle closure contains 633 selected original functions
from five source files, with 109 generated rejecting boundary functions. These
are static, branch-dependent requirements, not 109 independent projects or 633
executed functions. The lexical import list has 200 entries but includes comment
words such as `the`, `which` and `section`; it must not be presented as an exact
API count. Linked symbols and runtime traces take precedence over lexical hints.

Reproduce those bounded measurements after configuring/building the probe:

```sh
python3 - <<'PY'
import json, pathlib, re
p = pathlib.Path('build/generated')
d = json.loads((p / 'battle_provenance.json').read_text())
print('Selected original functions:', len(d['functions']))
print('Generated rejecting boundaries:', len(re.findall(
    r'battle_boundary_failed\("(\w+)"\)',
    (p / 'battle_unsupported.inc').read_text())))
PY
```

## Service inventory and order

These nine work packages group observed dependencies. They are not a claim that
the entire game has exactly nine remaining dependencies. Order favors shared
prerequisites and connected gameplay; effort bands are relative, not time promises.

| Order | Shared service / integration boundary | Current evidence and missing contract | Unlocks | Relative effort |
| --- | --- | --- | --- | --- |
| 1 | Archive and generated data access | Existing NARC reader and preparation tools work for field resources. Battle initialization exposes whole-move-table versus individual-member read semantics; weight, trainer and battle presentation resources also need correct preparation. Preserve bounds and original record layouts. | Original battle setup, trainer parties, encounter tables, sprites | Small per format; medium combined |
| 2 | Owned scheduling, input and application lifecycle | Original queues and OverlayManager already support field/starter/phone. Battle must own its tasks, input and cancellation, including EXP tasks; do not introduce another scheduler or borrow mutable field globals indefinitely. | EXP/level-up, menus, battle waits, reliable field return | Medium |
| 3 | Encoded messages and formatting | Original Pokémon strings and native UTF-8 field strings differ. Share a typed conversion/formatting boundary, preserving original parameters; replace battle message-number placeholders. | Battle text, item/move/trainer names, later menus | Medium |
| 4 | Party, trainer and result ownership | Original Pokémon/party routines execute. Battle currently owns a private party with synthetic enemy setup. Connect original trainer creation, original enemy decisions, seen/caught state and complete result publication/rollback. | Correct rival fight, wild battles, rewards, capture | Medium–large |
| 5 | Two-screen presentation and resource lifetime | Field rendering works; battle presentation packets currently acknowledge several visual commands without displaying them. Translate packets into real native surfaces using existing decoders; retain original control/rules. | Visible battle, move/party/bag choices, level-up UI | Large |
| 6 | Field admission, encounters and return | Rival opcode 213 rejects safely; grass does not trigger encounters. Reuse original encounter selection and battle setup, suspend/resume the same field owner and commit results once. | Rival and wild encounters through one battle owner | Medium–large after 1–5 |
| 7 | Original save ownership and host persistence | Session state exists, disk saving is absent. Map original save-block ownership before serialization; preserve original checksums/layout and use atomic host writes with corruption recovery. | Resume play, persistent progress, durable party/bag state | Large |
| 8 | Remaining original assembly and ABI contracts | Bounded Thumb execution exists. Original trainer AI and menu assembly require measured call closures and explicit guest/native layout handling. This is a cross-cutting dependency: resolve the first required path during 4, not after the UI. | Original AI and additional overlays without handwritten duplicates | Path-dependent |
| 9 | Remaining audio and optional peripherals | Music/effects/fanfares execute; cries and broader sound policy remain. Wireless/accessories stay explicit unsupported capabilities until relevant to an accepted route. | Presentation fidelity; later optional features | Path-dependent |

## Execution gates

1. **Original battle, headless:** complete a deterministic battle through original
   controller/move scripts; verify PP, damage, fainting, EXP/level-up and owned
   result state. Failure/cancellation must leave the caller unchanged. No message
   placeholders or silently successful gameplay providers on the admitted path.
2. **Visible rival battle:** actual original rival team and enemy decisions for
   each starter; left battle/right controls; original outcome dialogue; party,
   inventory and flags retained on field return. Exercise both win and loss.
3. **Wild encounter and capture:** original encounter eligibility/table selection,
   shared battle runtime, flee/faint/capture, ball consumption and caught state.
   Exercise party-full behavior before admitting that branch.
4. **Opening route:** complete rival aftermath and catching tutorial through
   original scripts. Recheck all three starter routes and cancellation paths.
5. **Persistent session:** save/load round trip plus corruption/failure recovery;
   then broaden menus/story using the same owners and services.

Do not finish an entire graphics SDK, CPU emulator or save subsystem speculatively
before gate 1. Conversely, do not bypass a required branch to claim a gate passed.
If a shared API reveals another caller with the same semantics, move the provider
to the shared compatibility boundary and retire the duplicate.

## Feedback loop and method checks

- Use the battle probe and focused test for each resource/task/message change.
  Measure its execution time separately from configure/build time. Run the full
  relevant suite at integration checkpoints, not after each edit.
- Record the first reachable missing service, its owner, original provider,
  callers and completion semantics. Group siblings before implementation; do not
  mechanically replace the next generated rejecting function with a stub.
- Audit existing no-op providers and acknowledged visual packets before admitting
  the battle route. A successful headless turn is not a rendered battle.
- Use native UI and sanitizer checks at presentation/ownership checkpoints;
  confirm upstream stays unchanged and proprietary evidence stays ignored.
- Judge improvement by connected behavior and removed duplicate boundaries.
  Repeated ABI marshaling or handwritten original logic is a reason to compare a
  broader guest execution path against the existing approach using that concrete
  workload. Do not restart as a full emulator without that evidence.

## Resource checkpoint, 2026-10-20

Superseded execution status: the subsequent [battle runtime checkpoint](battle-runtime.md)
now completes a headless wild battle, including original EXP tasks. The resource
findings below remain valid; the overlay stop described below is historical.

The resource boundary now preserves original filesystem range semantics: explicit
reads may cross NARC members, bounded by the validated archive image. This is what
the unchanged `LoadMoveTbl` requires. The Pokémon test compares every bulk-loaded
move against its individual record and rejects overflow/invalid-member requests.
`NARC_New` now uses the same archive resolver rather than a second limited list.

The original Pokédex measurement loader, HP-bar calculations and seen/caught
updates are selected unchanged. Measurement archives are generated from the
original JSON/template, assembled through the existing cached ARM data assembler,
and packed from validated ELF objects for both Giratina variants. Battle input
acknowledgements feed the original touch polling boundary.

The headless probe now executes the opening battle sequence and stops explicitly
at `ov12_02237ED0` (step 249 in the current probe). Original assembly shows this
switches overlays 7 and 10 between presentation and AI. It is not implemented as
an empty function. Next: resolve that lifecycle together with the original AI
entry path and owned task/input services. Battle messages still contain numeric
placeholders; visual commands are acknowledged only in the headless probe. No
field admission or playable battle is claimed. All 39 existing tests pass after
the resource change (12.43 seconds total on this run).
The sanitized Pokémon test and battle probe report no AddressSanitizer errors;
UBSan still reports the previously observed original signed shifts in
`pokemon.c:195` and `pokedex.c:76–77`. The probe intentionally exits unsuccessfully
at the missing overlay service. Upstream remains unmodified.
