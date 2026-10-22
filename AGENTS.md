# HeartGold MX working instructions

## Required correction to working method

- Maximize useful completed work per unit of usage. Reuse recorded findings,
  batch discovery, keep tool output and updates concise, and avoid repeated
  planning or broad rereads. Run focused checks during implementation and broader
  checks at coherent checkpoints; repeat passing checks only for relevant changes
  or unresolved risks. Passing evidence should support confident progress, while
  unexecuted behavior must remain explicitly unproven.

The user has repeatedly said the agent is missing the mark on professional
judgment, productivity and clever problem solving. Repeated promises followed
by another narrow fix have not met their expectations. Treat this feedback as
an unresolved requirement to improve the method, not as something satisfied by
acknowledging it or adding this section.

- Before substantial implementation, spend focused time identifying the real
  dependency chain, repeated patterns and highest-leverage shared change. State
  the concrete approach briefly, then execute it without waiting for approval.
- Before implementing any boundary, compute its complete transitive dependency
  closure. Follow every dependency recursively until all leaves are reached;
  listing immediate callers or stopping at the first unsupported command is
  insufficient. Include every branch, nested/standard script, indirect dispatch,
  callback, data/resource requirement, initialization, state owner, publication,
  failure recovery and resume dependency relevant to that boundary. Resolve
  indirect targets from original tables/data and runtime traces where necessary.
  Missing implementations and unresolved targets are open dependencies, not
  terminal leaves that make a closure complete. Handle cycles as strongly
  connected components and traverse all their outgoing dependencies.
- Apply closure analysis wherever dependencies can be computed: events, chapters,
  functions, services, applications, assets, build generation and verification.
  Extend existing inventories/graph tools to compute and cache closures in
  batches, share common subgraphs, and invalidate affected closures when inputs
  change. Reuse these results instead of rediscovering dependencies per caller.
  Distinguish a complete dependency inventory from implemented or executed
  behavior; none proves the others.
- Use the completed closures to choose shared dependencies and implement coherent
  groups from their leaves toward the connected outcome. Do not resume a cycle
  of fixing one rejection, rebuilding and discovering the next dependency. If
  closure extraction exceeds the initial investigation bound, record the concrete
  unresolved edges and switch to a batched graph/table query or execution trace;
  the bound does not waive the closure requirement. Present the closure's shared
  bottleneck briefly, rather than narrating every edge.
- Test shared behavior with one representative gameplay-produced starter state.
  Cover other starters only for demonstrated starter-dependent branches, rival
  teams or specific regressions. Exercise independent choices/failure paths
  directly; do not multiply an otherwise symmetric route by starter count.
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

## Operating bounds for faster delivery

These are default guardrails, not correctness deadlines. Exceeding a bound
requires a brief evidence-based reason and a changed method in the current work
record, not user permission. Never bypass original behavior, discard user work,
weaken acceptance or stop an authorized task merely to satisfy a budget.

- Maintain one active connected gameplay outcome in `docs/current-work.md`.
  Keep that record to at most 20 nonblank lines: entry state, visible exit state,
  current blocking boundary, shared callers, decisive next check, evidence and
  deviations. Do not create another planning system for an ordinary iteration.
- Before substantial edits, state the original execution path and ownership
  boundary. Each change must either advance that outcome, fix a reported
  regression, or remove demonstrated repeated work. One blocking boundary plus
  one necessary adjacent repair may be active; queue unrelated improvements.
- Initial investigation is bounded by 10 minutes or 8 focused read/search calls,
  whichever comes first. By then write a falsifiable hypothesis and its cheapest
  decisive check. If evidence is insufficient, change to a batched dependency
  query, original call closure or execution trace; do not continue broad browsing.
  Reuse recorded findings and read only new or changed code afterward.
- After two failed experiments against the same hypothesis, stop patching that
  hypothesis. Record what each ruled out and change the model or collect a trace.
  Before a third caller-specific fix in one service family, inspect sibling
  callers and compare reuse of the original subsystem with a shared adapter.
  A one-line user-reported bug does not require an elaborate redesign.
- Build the smallest affected target while iterating. Configure only when inputs
  require it. If identical generated output repeatedly causes rebuilding, address
  content-stable generation before another equivalent rebuild cycle. Never run
  builds concurrently in the same directory or start UI verification before its
  executable finishes building. Sanitized and ordinary builds may run separately.
- Use focused scenario checks during edits; run the relevant broad suite once at
  the connected checkpoint. Repeat passing checks only after a relevant change,
  failure or unresolved concern. Keep required sanitizer/native-window checks;
  speed comes from targeting and batching, not omitting observable verification.
- Before calling a new boundary complete, cover the applicable success,
  cancel/no-effect, failure/retry and resume/publication paths. For presentation,
  inspect the recovered scene as well as the successful animation. Extend an
  existing scenario driver instead of duplicating route or input machinery.
- A boundary pass is a checkpoint, not the default stopping point. Continue along
  the active route to the next actual blocker. If the full outcome is unfinished,
  record its exact last reachable state, next rejecting service and evidence;
  never relabel a headless engine pass as a completed map/event/chapter.
- At a checkpoint report only the observable gain, relevant checks, material
  limits and next blocker. Judge method improvement by connected route distance,
  concrete callers unlocked and avoided discovery/build cycles. Passing-test
  totals, code volume and updated plans are evidence inputs, not productivity.

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

The expanded target is a continuous original new game through eight Johto badges,
Lance, Hall of Fame and first credits, before postgame Kanto exploration. The
first priority is complete original save-state ownership and durable save/load:
all 42 normal and 6 extra registered records, original initialization, field-object
synchronization, time-dependent state, original save integrity/recovery and real
application resume. Do not describe the selective development checkpoints as
complete saves or omit records because their gameplay is not yet ported. Saving
is not complete until every inventory row has executed evidence; no zero-filled
placeholder initializers or silently successful services. See
`docs/save-inventory.md`, `docs/save-runtime-boundary.md` and
`docs/johto-playthrough.md`. Gym/narrative work should have disjoint event ownership
and explicit shared-service dependencies; parallelism must not duplicate subsystem
implementations or replace original progression conditions.

The narrower connected opening checkpoint below remains regression evidence,
not the endpoint of the expanded objective.

The active target is now the original bedroom starting position through the first
rival battle and Pokemon-catching tutorial, with the required original events,
objects, menus, items and battles present. Do not bypass required services with
story flags. Read `docs/opening-route.md` for the connected checkpoint and the
current battle/encounter dependencies. The connected route now
passes Route 29, Cherrygrove's tour/Running Shoes/Map Card and Route 30's Apricorn
Box gift and Mr. Pokemon's complete introduction for all starters, including
original fades, healing, Mystery Egg/Pokedex acquisition and Oak's phone number.
Elm's incoming call now completes through the shared application lifecycle. The
return route now completes the visible first-rival battle and original post-battle
dialogue, shove and departure for all three starters. The original allowed-loss
path heals and resumes too. Validated Route 29 / before-rival / after-rival
checkpoints are available for every starter; read `docs/checkpoints.md`. Wild
field battles, the catching tutorial and its five-Ball gift, ordinary Poke Ball
capture into party/PC, native Bag/party inspection and original Save/Continue now
have connected evidence. Home and registered Center blackout recovery complete
their original standard scripts; Center recovery runs the original healing task
with native texture/palette animation. Supported north-facing entrance doors
now open before the travel fade using original prop playback and door sounds.
South-facing door arrivals open during fade-in, run the original threshold step,
reposition the follower and close before returning control. Host phases schedule
this choreography; the full original transition task chain is not running.
Normal Nurse Joy interaction now executes the original greeting/confirmation,
follower handoff and healing script, including refusal, fainted lead restoration,
Pokérus, Save/Continue and interaction immediately after Center recovery. Dialogue
waits keep scene movement running; menu confirmation retains queued selection.
Battle HP/status/PP medicine targeting, voluntary switching and forced replacement
now have six-Pokemon scenario, rendered-menu, durable-publication and native UI
evidence. Original Falkner team/AI/replacement/rewards execute at the battle
engine boundary; this does not establish the Violet gym event/badge route.
Trainer medicines, broader battle items, full character action animation,
contest defeat and later narrative services remain open. Read `docs/battle-runtime.md` and
`docs/save-acceptance.md` for current boundaries and evidence.

The earlier New Bark → Elm introduction → starter → following checkpoint
passes for all three
starters with the original name, including the five-Potion gift, reverse walking
and lab re-entry. Read `docs/first-starter.md` for evidence and remaining limits.
Keep No selected at the nickname prompt; the naming application is not ported.
Continue toward broader original field/script integration, menus, saving and
battles. Do not substitute compile counts or fixture flags for gameplay.

## Chapter testing teams

For every chapter, provide a full six-Pokemon test team scaled to that point in
the game. Make it a fun, varied team with diverse types, battle roles and useful,
stage-appropriate moves rather than six copies or an overleveled sweep fixture.
Keep curated-team feature/coverage scenarios separate from the continuous
original-progression acceptance route. Never replace gameplay-produced chapter
entry saves, unlocks or acquisitions with injected teams or story flags. Include
team composition, levels and moves in each chapter's test contract.
