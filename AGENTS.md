# HeartGold MX — engineering instructions

## Mission and authority

Port the original Pokémon HeartGold engine to Apple Silicon with the smallest correct compatibility runtime. Original Nintendo DS **observable semantics** are the specification: ARM9/ARM7-visible addresses, instruction behavior, struct layout, memory aliasing, heaps and overlays, device/MMIO behavior, interrupts, timing where gameplay observes it, and graphics output. Preserve original game logic and assets. Host conveniences may not silently change DS-observable behavior.

Read the relevant existing project specifications and recent evidence before changing their subsystem. The active gameplay target and validated progress live in `docs/current-work.md`, `docs/johto-playthrough.md`, and relevant subsystem documents; this file is not a progress log. Follow the most recent user request when it changes the active focus.

## Compatibility architecture

- The **guest DS address space is authoritative**. Original code sees DS addresses, not arbitrary host addresses. Do not remap an existing guest allocation merely to simplify a native bridge.
- Shared byte-compatible live regions may use a partial guest↔host correspondence, preserving bounded offsets, contents, identity, and coherent aliases. Define canonical addresses where aliases exist. Noncompatible C/ARM32 layouts require explicit ABI adapters.
- Allocation, borrowing, ownership, retirement, reuse, and alias invalidation must be explicit. Hardware/MMIO accesses may have side effects and must not be modeled as ordinary RAM merely because they have addresses.
- ARM instruction execution must preserve guest-observable state transitions; Metal must refine DS graphics operations to their specified observable effects. Neither requires a literal whole-machine isomorphism.
- Use the project's existing runtime primitives and mathematical specifications (see `docs/memory-contract.md` and `docs/compatibility-semantics.md` when present). Improve a shared primitive when repeated adapters expose a missing capability. Do not build a second emulator, duplicate Nintendo algorithms, or add speculative frameworks.
- Keep per-function bridges minimal, but never at the expense of correct ABI, device, ownership, or lifetime semantics. Reject unsupported behavior explicitly rather than reporting false success.

## Method

1. Inspect the current implementation, callers, relevant upstream code, state owners, and tests **before editing**. Search efficiently and reuse recorded findings. Trace indirect dependencies far enough to justify the proposed change; record unresolved edges rather than claiming complete closure without evidence.
2. State briefly the behavioral invariant, current failure, smallest shared correction, and cheapest decisive test. Distinguish direct source evidence from hypotheses.
3. Make a coherent, minimal, reviewable change. Preserve working behavior and user edits. Avoid unnecessary generators, layers, duplicate state, and broad unrelated refactors. If a narrow repair repeats, investigate a shared cause.
4. Add tests for the actual contract: success and failure, bounds, aliases, ownership/lifetime, resume paths, and relevant DS-address observations. Prefer differential checks against original execution when feasible. Tests are evidence, **not** proof of equivalence.
5. Run targeted checks while editing and the relevant regression suite at a coherent checkpoint. Use sanitizers, native rendering validation, or machine-checked proofs where appropriate. Distinguish compiled code, linked code, executed code, verified properties, and unproved assumptions.
6. Report actual observable behavior gained, changed files, tests and proof status, remaining risks, and the next blocker. Do not invent completion percentages or call an unexecuted path complete.

Investigation and execution budgets are heuristics, not correctness constraints. Do not insist on exhaustive analysis when it is undecidable or disproportionate. Do not rush a foundational invariant to advance a single gameplay boundary. Autonomous routine investigation, reversible editing, and local testing are authorized; ask only for a genuinely blocking decision or a separately permissioned action.

## Non-negotiable repository boundaries

- Never edit `upstream/pokeheartgold` or commit proprietary original assets, ROMs, Nintendo SDK material, generated game resources, or game screenshots. Keep generated resources/evidence ignored.
- Do not overwrite, discard, or revert user work. Confirm the actual working tree before touching files. Do not push, publish, deploy, or delete user work without authorization.
- Avoid npm, npx, and Node-based tooling. Prefer existing C/C++, CMake, Python/uv, compiler tools, and standalone utilities.
- Preserve the complete save-record and connected-gameplay acceptance requirements in their dedicated docs. Do not substitute synthetic story flags, zero-filled placeholder initializers, or test fixtures for original progression; curated test teams are separate from the original gameplay route.
- Preserve regression coverage. Do not weaken assertions or bypass genuine dependencies to achieve a green build.
