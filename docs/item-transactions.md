# Poké Mart and field item use

The connected `original_pokemart_transactions` scenario loads the executed
Cherrygrove after-rival checkpoint, walks through the real Mart entrance and
approaches its original clerk. Buy/Sell/Quit, standard message-bank selection,
external messages and the return-to-clerk loop execute original script handlers.
Original shop-entry scripts inspect the real Mom gift queue and saved Mystery
Gift records before controlling delivery actors. Gift receipt is not implemented;
unsupported Mystery Gift modes reject explicitly.

Menu and quantity directions advance once per physical press; holding W/S
does not repeat until release.

The native modal panel replaces the original DS shop application’s graphics and
UI task. Unchanged original functions select badge-dependent stock, apply the
Poké Ball unlock, read prices, adjust quantities, buy items, charge money, record
statistics and award the Premier Ball bonus. The original sale transaction
`ov15_021FD24C` executes through the bounded Thumb interpreter with explicit
Bag/profile/window/audio boundaries. Native selection lists eligible owned items.
Normal shops and the original special-stock lists share this adapter; decoration,
seal and Pokeathlon shops are rejected. Only Cherrygrove’s clerk has connected
route evidence. The native general script-menu adapter currently admits up to
three standard-message items; this is not full original menu coverage.

Transactions update the script’s private save candidate. The live story receives
them when the original clerk script completes; saving is blocked while that
interaction is active. Purchases recheck affordability and pocket capacity at
confirmation. Failed transactions restore Bag, money and statistics. Canceling
before confirmation consumes nothing. Receipt acknowledgment also accepts Esc,
matching the original A/B completion and preserving its bonus award.

Field Bag use shares unchanged `CanUseItemOnPokemon` and `UseItemOnPokemon` and
the original mood adjustment. A private Pokémon candidate is published and one
item consumed only after a successful effect. HP/status/PP restoration, revives,
vitamins and applicable berries are available; PP items expose a move selector.
Rare Candy, evolution, Sacred Ash, held-item actions, field tools and party
reordering remain unavailable. Battle medicine selection is the next integration
boundary; this change does not make it available in battle.

Evidence covers real clerk buy/cancel/sell, original pre-tutorial ball filtering,
private/live ownership, field Potion targeting and consumption, and exact
Bag/party/money restoration through durable Save/Continue. Additional isolated
cases cover the real after-tutorial unlock, ten-ball bonus, insufficient funds
at confirmation, full item stacks, no-effect preservation, poison cure, revive,
PP restoration, unsupported items and positive/empty saved gift queries.

The Mart room also exercises a previously rejected original root-node scale.
The shared renderer now admits explicit scale plus validated reciprocal scale
for single-node models. Multi-node scale/parent compensation remains unsupported.
No upstream source or original assets are modified or added to Git.

Verification: all 74 CTest scenarios pass. Focused ASan/UBSan model, door,
Nurse Joy and Mart scenarios complete without AddressSanitizer errors. UBSan
still reports existing original signed shifts in `pokedex.c:76–77` and
`sys_vars.c:171`; this is not a clean UBSan result. The native AppKit smoke
includes walking from the healed Center party to the Mart, opening the real
clerk menu, displaying its shop panel and returning through Quit. Local image
and log evidence stays under ignored `build/evidence` and build directories.
