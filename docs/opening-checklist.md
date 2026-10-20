# Opening checklist: original start through catching tutorial

“Working” means connected script/gameplay execution in the current native port,
verified for all three starters. It does not imply finished original graphics,
audio, boot or saving. “To add” means required integration, not absent source/data.

| Step | Original content | Current status |
|---|---|---|
| 1. New-game introduction | Professor Oak introduction, player setup and naming | To add; native session supplies Ethan and starting records |
| 2. Bedroom start | Original upstairs position, facing, stairs; no Running Shoes | Working |
| 3. Mom's introduction | Dialogue and Bag/trainer-card/save/options unlocks | Working unlocks; their complete interfaces and saving remain to add |
| 4. First friend encounter | Friend and Marill scene outside | Working |
| 5. Elm's lab introduction | Professor, dialogue, movements and starter-machine event | Working |
| 6. Choose a starter | Chikorita/Cyndaquil/Totodile, party acquisition and following | Working for all three |
| 7. Nickname prompt | Keep the original name or enter a nickname | Original name works; naming application to add |
| 8. Lab assistant's gift | Five Potions and original item dialogue | Working |
| 9. Post-starter town scenes | Friend/Marill, return to Mom and Pokegear gift | Working |
| 10. Elm's farewell | Door animation, Elm approaches and registers his number | Working |
| 11. Route 29 outward trip | Terrain, travel, people/objects, grass encounters | Travel and admitted events work; encounters/NPC AI remain to add |
| 12. Cherrygrove tour | Guide, choreography, door event and Running Shoes | Working |
| 13. Leave Cherrygrove north | Guide returns and gives the Pokegear Map Card | Working acquisition; map application to add |
| 14. Route 30 | Apricorn man's scene and Apricorn Box | Working gift; harvesting remains to add |
| 15. Mr. Pokemon's house | Mystery Egg, rest/healing, Oak, Pokedex, phone registration and rival setup | Working; Pokedex browsing remains to add |
| 16. Elm's panic call | Incoming Pokegear call reporting the theft, then return to field | Working through the shared application lifecycle |
| 17. Return through Cherrygrove | Rival confrontation, starter-dependent trainer battle and departure | Next blocker: original preflight reaches TrainerBattle opcode 213; battle/field return to add |
| 18. Return to Elm's lab | Police questioning, friend intervenes, rival naming/confirmation | To add, including actual naming application |
| 19. Deliver the Mystery Egg | Elm takes the Egg, dialogue and next journey setup | To add connected post-battle sequence |
| 20. Talk to Mom (optional) | Offer to save part of the player's money | To add original savings interaction/ownership |
| 21. Route 29 catching lesson | Friend/Marill choreography, catching demonstration, five Poke Balls, departure | To add; the active opening target ends here |

The first rival battle is on the return trip, after Mr. Pokemon's visit. The
catching tutorial follows the return-to-lab sequence. Wild encounters are a
shared missing system throughout the route, including the earlier outward trip.

Required supporting work: original encounter/battle execution, party/bag/Pokedex/
Pokegear interfaces, naming, save/load, audible sound with original durations,
reaction-effect assets/palettes, original menu presentation and broader object
behavior. The current gold-font exclamation remains a presentation placeholder.
Mom's follow-up Route 30 call is another later phone consumer.

Application consumers should reuse the original TaskManager/OverlayManager,
resource readers and retained party/save owners. The starter already uses the
shared launcher/scheduler and original brightness controller. Elm's scripted call also uses that runtime. General Pokegear browsing and
battles remain unported; see [Elm's call](elm-phone.md).

Evidence and boundaries: [opening route](opening-route.md),
[Mr. Pokemon](mr-pokemon.md), [application lifecycle](application-lifecycle.md).
Original sequence checked against the local T20/T20R0101/T20R0201, T21, R29,
R30 and R30R0201 script banks. No original source or game assets are changed.
