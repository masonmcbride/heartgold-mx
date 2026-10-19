# HeartGold MX working instructions

## Required correction to working method

The user has repeatedly said the agent is missing the mark on professional
judgment, productivity and clever problem solving. Repeated promises followed
by another narrow fix have not met their expectations. Treat this feedback as
an unresolved requirement to improve the method, not as something satisfied by
acknowledging it or adding this section.

- Before substantial implementation, spend focused time identifying the real
  dependency chain, repeated patterns and highest-leverage shared change. State
  the concrete approach briefly, then execute it without waiting for approval.
- Use targeted `rg` searches, regex extraction, compiler/linker output, existing
  inventories and small analysis programs to answer questions across the codebase
  in batches. Avoid repeatedly reading large files or discovering each related
  function manually. Use parsing/compiler tools when regex cannot safely express
  the structure; validate generated output and reject ambiguous matches.
- When work repeats, change the method: write a bounded generator, reusable
  adapter, dependency query or scenario driver when it removes more effort and
  maintenance than it adds. Prefer extending existing project tools. Do not
  automate a poor approach or build tooling merely to produce more tooling.
- Prefer a coherent original subsystem and its shared host services over a long
  series of special cases. Evaluate how the change connects gameplay and reduces
  future integration work before implementing it.
- Check whether the chosen method actually helped: original behavior connected,
  repeated steps eliminated, custom surface reduced, or faster reliable feedback.
  Do not substitute code volume, passing tests, new documents, elaborate plans or
  activity counts for evidence of improved engineering judgment and productivity.
- At checkpoints, assess the remaining bottleneck and carry an improved approach
  into the next useful step. Keep reflection brief and actionable; do not turn it
  into repeated self-criticism, apologies or another reason to stop working.

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

## Active acceptance target

The active target is now the original bedroom starting position through the first
rival battle and Pokemon-catching tutorial, with the required original events,
objects, menus, items and battles present. Do not bypass required services with
story flags. Read `docs/opening-route.md` for the connected checkpoint and the
current phone-application/battle dependencies. The connected route now
passes Route 29, Cherrygrove's tour/Running Shoes/Map Card and Route 30's Apricorn
Box gift and Mr. Pokemon's complete introduction for all starters, including
original fades, healing, Mystery Egg/Pokedex acquisition and Oak's phone number.
Leaving his house rejects the unported panic-call application (opcode 430); see
`docs/mr-pokemon.md`. Original field encounters/battles remain absent. The full target is incomplete.

The earlier New Bark → Elm introduction → starter → following checkpoint
passes for all three
starters with the original name, including the five-Potion gift, reverse walking
and lab re-entry. Read `docs/first-starter.md` for evidence and remaining limits.
Keep No selected at the nickname prompt; the naming application is not ported.
Continue toward broader original field/script integration, menus, saving and
battles. Do not substitute compile counts or fixture flags for gameplay.
