# HeartGold MX working instructions

## Objective and engineering judgment

- Continue toward a playable native HeartGold on Apple Silicon. Recover current
  state from README and relevant docs before changing implementation.
- Think about the original game's whole system: field control, objects, scripts,
  party/save ownership, rendering, menus and battles. Choose shared bottlenecks
  whose resolution connects several systems or unlocks multiple gameplay paths.
- Ground the strategy in the material relationships between original code, game
  data, hardware assumptions, compiler/SDK dependencies and host resources. Use
  that system knowledge to avoid treating the port as unrelated compiler fixes.
- Use programmatic inventories and dependency evidence to find coherent paths
  through the original system, then verify those paths in execution. Automate
  repeated discovery and validation instead of manually expanding allowlists.
- Favor elegant reuse and reduced custom surface. Judge a solution by what it
  enables, its correctness, complexity and effort, not by code volume, compiler
  errors fixed or isolated tasks completed. Reconsider the approach when work
  becomes repetitive instead of mechanically extending adapters.
- Keep reusable DS runtime services separate from HeartGold integration so the
  compatibility work can support other DS games on Apple Silicon. Do not build
  speculative general infrastructure without a concrete execution path.

## Original code and assets

- Compile and execute unchanged original upstream game functions wherever
  possible. Keep native adaptations at explicit compatibility boundaries.
- Never modify `upstream/pokeheartgold`. Never commit original game assets,
  generated proprietary data, ROMs, Nintendo SDK components or game screenshots.
  Build-local generated resources and evidence stay ignored.
- Reuse existing readers, renderers, compiler tools and execution engines. Do not
  introduce a second implementation merely to make a milestone appear complete.
- Distinguish source compilation, linked code and behavior actually executed and
  tested. Unsupported commands and services must not silently report success.

## Autonomous execution

- Existing authorization covers routine implementation, reversible fixes,
  local builds, tests, investigation and documentation. Make routine engineering
  decisions without asking whether to continue.
- A passing small milestone is a checkpoint: update accurate documentation and
  proceed to the next useful step toward the active objective. Do not repeatedly
  end work after a narrow fix when broader authorized work remains.
- Ask only for a genuinely blocking requirement or an action requiring explicit
  permission. Do not infer authorization to publish, push, deploy, delete user
  work or bypass enforced approval controls.
- Treat feedback and bug reports as steering within the continuing objective,
  unless the user explicitly changes or cancels it. Preserve their changes.
- Keep progress updates brief. Explain decisions, meaningful discoveries and
  actual blockers; avoid repeated promises and lengthy routine-work narration.

## Verification and handoff

- Finish milestones with relevant passing checks and accurate documentation.
  Use scenario tests for connected gameplay, failure recovery and state ownership;
  do not equate a successful build or rendered map with a playable game.
- Run appropriate sanitizer and native UI checks for runtime/presentation changes.
  Confirm upstream remains unmodified and review diffs for accidental assets.
- Report concrete behavior gained and material remaining limits. Do not invent
  completion percentages or imply unexecuted startup, SDK, save or battle paths
  work. Carry forward the next shared dependency instead of asking to continue.
