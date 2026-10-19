# Whole-source native compilation audit

For the current whole-source classification and batch fixes, see [full-game workstreams](full-game-workstreams.md).

This document records the initial baseline before the scalar SDK header was added. For current before/after results and exposed dependencies, see [the scalar compatibility milestone](sdk-types.md). Current raw evidence is regenerated under `build/audit/`. The later [field-data milestone](field-data.md) now brings compilation to 32 game C units with source-specific field compatibility. The subsequently completed [scheduler milestone](scheduler.md) brings game C compilation to 28 units and executes the original queue/API/environment code.

Measured on Apple Silicon macOS with Apple Clang 17, against upstream revision `708163e10169173d2bbab252538231721d983155`. This audit compiles original files directly and never runs their output. It does not create a full game executable or establish a port-completion percentage.

## Results

| Source group | Listed files | ARM64 objects emitted with existing compatibility layer |
| --- | ---: | ---: |
| Game C (`src/meson.build`) | 399 | 27 |
| Game ARM/Thumb assembly (`asm/meson.build`) | 286 | 0 |
| Particle-library C | 8 | 0 |
| Support-library assembly | 5 | 0 |
| Available DSProt C, inspected separately | 8 | 0 |

Without our compatibility header, all 415 inspected C files failed. With it, 27 game files compiled and 388 C files failed. All 291 selected game/support assembly files failed for ARM64. No invocation hit its time/diagnostic budget in this run.

The scope follows actual build manifests, rather than globbing every file: `asm/overlay_13_thumb_2.s` exists but is not selected by the game manifest. Support assembly covers the explicit DWC/crypto/lobby, IRC and HeartGold syscall sources. DSProt C is shown separately because the original build uses a prebuilt archive, not those sources. External SDK implementations, host asset tools, ARM7 firmware and the prebuilt archive itself are outside this compilation survey.

The game manifests select 205,145 lines of C text and 1,067,614 lines of assembly text, with 19,049 `arm_func_start`/`thumb_func_start` markers. These include comments/data and are not effort estimates, unique algorithm counts, or proof that every assembly routine needs a separate hand port. They do establish that a large portion of the selected build is ARM/Thumb source rather than portable C.

## What blocks the current C build

Compatibility-pass classifications across all 415 C attempts:

| Visible failure category | Units |
| --- | ---: |
| Missing SDK header | 307 |
| Other missing header, including generated resources | 34 |
| Type/header boundary errors without a fatal missing include | 31 |
| Missing declarations/constants without the above | 15 |
| Other compiler error | 1 |
| Compiled | 27 |

The first visible missing SDK headers include `nitro/types.h` in 179 files, `nnsys.h` in 72, `nitro.h` in 24, RTC API headers in 9, graphics `g2.h` in 6, and fixed-point `fx.h` in 5. These counts are per first fatal missing-header dependency; they do not count every missing include, and some units report semantic errors before the fatal include. Fixing one shared include can expose many further failures.

Representative dependencies found by the actual compiler:

| Subsystem | Current visible blocker | Required boundary work |
| --- | --- | --- |
| Pokémon data and battle system | `nitro/types.h` through personal-data structures | Minimal real scalar interfaces, then audit struct layouts, casts and further dependencies |
| Field control | `nnsys.h` | Identify the actual graphics/math types used; separate native data from DS representations |
| Save/system code | `fx32` and other unavailable declarations | Establish fixed-point interfaces and inspect downstream dependencies; this alone does not implement saving |
| Audio | `NNSSndHandle` | Native audio runtime and lifecycle, not merely a placeholder type |
| Overlays and main entry | `FSOverlayID` | Native module/scheduler lifecycle rather than loading DS executable overlays |
| Map matrix | Generated `map_matrix.naix` | Reproduce resource indexing from the actual manifest and provide bounded reads |
| Task API | Our `NativeInput` lacks the four original task queues | Extend the runtime deliberately and use the original queue implementation |
| Timer 3 | Volatile DS types, IRQ APIs and a direct timer-register read | A host clock implementation at the boundary; do not execute the original register path |

The static include graph covers 1,015 local source/header nodes. It is an index of literal includes, not a preprocessor-aware proof: inactive branches and ordinary macOS system includes must not be treated as missing dependencies just because this graph does not resolve them.

No SDK headers were downloaded or copied into our implementation, and the audit adds no fake success stubs to increase compile counts. It reuses the existing compatibility header. That header intentionally supports the inventory slice and suppresses several original header bodies, so compilation against it does not prove the rest of the game sees complete or correct interfaces.

## Why assembly still blocks a full game

Among the 291 assembly attempts, 212 reached target/assembler failures and 79 stopped at missing generated message headers. A generated header can unblock preprocessing, but ARM/Thumb instructions, calling conventions, assembler directives, absolute data layouts and SDK assembly remain incompatible with native ARM64 Mach-O.

The original bag overlay, for example, is still ARM assembly; our inventory is a native composition around compiled original bag/menu logic. Other major overlay and field paths have similar source constraints. Simply supplying SDK declarations cannot solve them. A native port needs original C where available, carefully audited native implementations of platform services, and an explicit strategy for remaining assembly. CPU emulation is not introduced by this audit.

## The 27 files that compile

Existing executed slice:

- `bag.c`, `item.c`, `list_menu.c`
- `font_data.c`, `text.c`, `string_control_code.c`

Additional object-producing candidates:

- `application/pokegear/phone/extra_ethan_lyra_data.c`
- `coins.c`, `credits/data2.c`, `igt.c`, `menu_input_state.c`
- `metatile_behavior.c`, `pm_version.c`
- `save_special_ribbons.c`, `save_trainer_card.c`, `save_vars_flags.c`
- `string_util.c`, `sys_flags.c`, `sys_task.c`, `systask_environment.c`
- `unk_02018380.c`, `unk_02020A0C.c`, `unk_0202F370.c`
- `unk_02091054.c`, `unk_02091278.c`, `unk_020912AC.c`, `unk_02092BB8.c`

These additional candidates have **not** been integrated, executed or behavior-tested. A C file can compile while retaining DS pointer assumptions, hardware dependencies, unsupported runtime calls, or incorrect ABI assumptions.

After subtracting definitions in other successful objects and our existing runtime, undefined references consist of five host/library symbols plus three game symbols: `SysTask_CreateOnMainQueue`, `sub_020254FC` and `sub_02025504`. Their indexed source providers are `sys_task_api.c` and `timer3.c`. This is only the small successful subset; it says nothing about unresolved calls in the other 388 C files or assembly. Runtime symbols may also resolve to explicit aborting stubs. No full link is attempted while required objects are missing.

## Recommended next implementation

The most concrete low-dependency engine milestone is the original task scheduler:

1. Audit `sys_task.c` and run meaningful tests for ordering, insertion/removal during execution, and lifecycle.
2. Add native main/print/VBlank/VWait queue ownership and connect original `sys_task_api.c`, replacing current abort boundaries only after those paths work.
3. Provide a host-backed time boundary for the original in-game timer, leaving DS timer/IRQ code unexecuted.
4. Use that scheduler to drive a small field/map slice, starting with original metatile behavior and bounded map resources.

In parallel with that sequence of milestones, a shared compatibility interface for SDK scalar/fixed-point types and proper generated resource indices will reveal deeper compiler failures. Types must match actual game data semantics; declaring every unknown type as a pointer or making every API return success would produce misleading progress.

## Reproduce and inspect

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target audit_native
```

The target depends on the existing native library and invokes `scripts/audit_native.py`. It performs original-header C, current-compatibility C and ARM64 assembly passes. Each invocation has a 40-second limit and bounded diagnostics; C errors are capped at 12 per unit. Audit compilation failures are expected findings, so a successful audit target does **not** mean successful game compilation.

Generated evidence is confined to ignored `build/audit/`:

- `REPORT.md`: full generated summary and missing-header ranking;
- `summary.json`: source lists, compiler commands, exit codes, limits, logs, hashes and symbol-provider hints;
- `include_graph.json`: static include edges and unresolved literal includes;
- per-pass `.log` diagnostics and successful `.o` files.

The audit executes compiler and inspection tools only. It neither links nor runs an upstream game executable. Source/manifest hashes matched before and after, and the upstream Git checkout remained clean. Logs can include upstream excerpts and must remain ignored with the objects. The existing six executable tests continue to define which original functions actually run natively.
