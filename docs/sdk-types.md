# First SDK compatibility milestone: scalar types

Our locally authored `src/compat/include/nitro/types.h` now supplies fixed-width signed/unsigned integers, volatile aliases, floating scalar types, four-byte game BOOL, and TRUE/FALSE. This is a small native compatibility interface, not a copied SDK header. It contains no hardware addresses, register access, pointer conversion, or function implementations.

`native.h` imports these shared definitions instead of maintaining a second set. CMake exposes the include directory through `hg_sdk_types`. The audit adds it only to its compatibility pass; the unadapted pass still uses original include paths, preserving the baseline meaning.

## Executed ABI verification

`test_sdk_types` intentionally does not force-include `native.h`. It includes the compatibility scalar header and the original `struct_defs/personal_narc_structs.h` directly, verifying:

- scalar widths, signedness, unsigned ranges, floating widths, BOOL and volatile qualifications;
- a 64-bit native process with little-endian data;
- the original personal-data record's 44-byte size and item/TM field offsets;
- the original evolution record's 6-byte size;
- actual packing of the personal-data EV-yield bitfields.

These checks establish the tested records' layout on this compiler/target, not general SDK, save-file or pointer-bearing-structure compatibility. Volatile aliases preserve C qualification; they do not make DS memory addresses usable on macOS. No fixed-point or graphics structures were guessed or aliased to float/pointers.

The later [scheduler milestone](scheduler.md) is now implemented and tested; its audit emits 28 game C objects. The comparison below records only the scalar-header change.

## Measured audit change

Compared with the [initial audit](native-audit.md), on the same upstream revision and compiler:

| Game C outcome (399 files) | Before | After |
| --- | ---: | ---: |
| Compiled | 27 | 27 |
| Missing SDK header | 292 | 168 |
| Other missing header | 33 | 39 |
| Type/header boundary errors | 31 | 146 |
| Missing declarations/constants | 15 | 18 |
| Other compiler error | 1 | 1 |

The 175 files that previously stopped at `nitro/types.h` now pass that include. None still report that missing header. Their next visible blockers are:

| Next blocker among those 175 files | Files |
| --- | ---: |
| Type/header boundary errors | 116 |
| RTC API header | 24 |
| `nnsys.h` | 22 |
| Fixed-point `fx.h` | 5 |
| Other generated resource headers | 6 |
| Missing declarations/constants | 2 |

Thus compilation coverage remains unchanged, with no regressions in the previous 27 successes. Resolving a common include exposed additional errors; it did not port those files.

Across all game C attempts, the largest remaining missing headers are now `nnsys.h` (94 files), the RTC API header (33), and `nitro.h` (16). Among visible unknown types, `NNSG3dTexKey`, `NNSG3dResTex` and `NNSG3dPlttKey` recur heavily through the Pokémon → sprite → 3D-resource header chain. Other next dependencies include `FSOverlayID` and `fx32`. Our narrow forced global-header replacement also leaves original game constants such as name lengths/party size unavailable in these broader includes; that needs a deliberate boundary correction, not replacement game algorithms.

## Next boundary work

First separate and preserve original game constants while providing minimal, semantically correct SDK interfaces. Audit fixed-point and graphics key/data types against actual resource layouts and callers before defining them. Opaque pointer targets can remain opaque only where callers never access their members. Do not use arbitrary placeholder structs or float aliases to inflate compile counts.

Original Timer 3 now gets past its volatile scalar declarations and exposes real DS timer/IRQ calls. Those require a host clock boundary; the audit output is not executed. The original task scheduler remains a separate low-dependency candidate for integration and behavior tests.

## Reproduce

```sh
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --build build --target audit_native
python3 scripts/compare_audits.py \
  build/audit-baselines/before-sdk-types.json build/audit/summary.json
```

The baseline JSON is preserved locally under ignored `build/audit-baselines/`; it is not part of the repository. Save `build/audit/summary.json` before future compatibility changes to make another comparison. The comparison script requires the same upstream revision and source set. Raw objects and diagnostics remain in ignored build directories.
