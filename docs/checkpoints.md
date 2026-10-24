# Story checkpoints

The **Jump to checkpoint…** picker starts with six story moments, each with a
fresh, distinct six-Pokémon party. Levels follow the opening's pace (5–12), while
species and moves prioritize fun over normal availability. Shaymin, Darkrai and
Kangaskhan appear across different teams; they are not a fixed core. Every member
starts healthy with four moves and full PP. Open **Pokémon (P)** to see all six
members. **W/S or arrow keys** select a Pokémon and show its details; **Enter**
picks it up, then choose another slot and press **Enter** to swap. Choose the same
slot to cancel, or close the panel with **Esc/P**. Moving a healthy Pokémon into
the first slot updates its overworld follower too. The new order and follower
survive Save/Continue. Menu navigation uses key presses/OS repeat instead of
cycling through the party every rendered frame.

The story entries are First steps into Johto; The Mystery Egg & a rival;
Rival defeated—return to Elm; Elm's mystery & the catching lesson;
Poké Balls packed—your adventure begins; and Violet City & the swaying Sprout Tower.

The **Original party** entries below them retain the fifteen existing opening
checkpoints for all three starters. Curated teams use separate loaded sessions;
their Pokémon are never injected into continuous-progression acceptance saves.
No story flags, gifts, positions or unlocks are manufactured for the team entries.
They load the original gameplay-produced source and replace only the party.
Violet arrival remains before the first badge: Nico's Tower event currently
rejects actor movement query 574. The team does not bypass that event.

**Back** restores the exact prior in-memory session, including a suspended
conversation or battle. A failed load keeps that session. Selecting a story
moment starts its preset again; it does not resume your last curated run.
**Save/Continue** retains the curated party normally. Violet uses a uniquely
named `build/checkpoints/violet-team-*.sav` copy, preserving all original cartridge
bytes/extra records and preventing Save from overwriting its source. Opening
snapshot entries ask for a new save destination as usual.

Generate every source and launch the viewer:

```sh
cmake --build build --target heartgold_checkpoints heartgold_field_viewer -j8
./build/heartgold_field_viewer
```

Generated saves, checkpoints and screenshots remain ignored. The picker reads
sources from the `checkpoints` directory beside its executable. Native development
snapshots remain distinct from original-manager cartridge Save/Continue.

## Team contracts

These are sandbox teams, not legal acquisitions or continuous-progression evidence.
The shared catalog is `src/story_checkpoints.c`; names below reflect that catalog.

### First steps into Johto · Lv. 5

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 5 | Ember, Quick Attack, Smoke Screen, Leer |
| Shaymin | 5 | Magical Leaf, Leech Seed, Growth, Synthesis |
| Riolu | 5 | Quick Attack, Force Palm, Endure, Counter |
| Horsea | 5 | Water Gun, Smoke Screen, Dragon Breath, Leer |
| Shinx | 5 | Thunder Shock, Tackle, Leer, Charge |
| Swablu | 5 | Peck, Sing, Astonish, Growl |

### The Mystery Egg & a rival · Lv. 6

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 6 | Ember, Quick Attack, Smoke Screen, Leer |
| Darkrai | 6 | Ominous Wind, Hypnosis, Disable, Quick Attack |
| Dratini | 6 | Twister, Thunder Wave, Dragon Breath, Wrap |
| Scyther | 6 | Wing Attack, Quick Attack, Focus Energy, False Swipe |
| Poliwag | 6 | Water Gun, Hypnosis, Bubble, Pound |
| Mareep | 6 | Thunder Shock, Thunder Wave, Tackle, Growl |

### Rival defeated—return to Elm · Lv. 7

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 7 | Ember, Quick Attack, Smoke Screen, Leer |
| Kangaskhan | 7 | Fake Out, Bite, Low Kick, Tail Whip |
| Eevee | 7 | Quick Attack, Bite, Helping Hand, Sand Attack |
| Ralts | 7 | Confusion, Magical Leaf, Double Team, Teleport |
| Wooper | 7 | Water Gun, Mud Shot, Tail Whip, Mud Sport |
| Hoothoot | 7 | Peck, Hypnosis, Growl, Foresight |

### Elm's mystery & the catching lesson · Lv. 7–8

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 8 | Ember, Quick Attack, Smoke Screen, Leer |
| Sneasel | 7 | Ice Shard, Quick Attack, Faint Attack, Taunt |
| Buizel | 8 | Water Gun, Quick Attack, Sonic Boom, Water Sport |
| Budew | 7 | Mega Drain, Growth, Stun Spore, Water Sport |
| Misdreavus | 7 | Confusion, Astonish, Spite, Growl |
| Pichu | 7 | Thunder Shock, Charm, Sweet Kiss, Tail Whip |

### Poké Balls packed—your adventure begins · Lv. 8–9

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 9 | Ember, Quick Attack, Smoke Screen, Leer |
| Shinx | 8 | Spark, Tackle, Leer, Charge |
| Mudkip | 8 | Water Gun, Mud Slap, Rock Tomb, Growl |
| Togepi | 8 | Extrasensory, Charm, Metronome, Sweet Kiss |
| Larvitar | 8 | Bite, Rock Throw, Sandstorm, Leer |
| Absol | 8 | Bite, Quick Attack, Leer, Pursuit |

### Violet City & the swaying Sprout Tower · Lv. 10–12

| Pokémon | Level | Moves |
| --- | ---: | --- |
| Cyndaquil | 12 | Ember, Quick Attack, Smoke Screen, Leer |
| Shaymin | 11 | Magical Leaf, Leech Seed, Growth, Synthesis |
| Gastly | 10 | Lick, Hypnosis, Spite, Mean Look |
| Heracross | 11 | Aerial Ace, Brick Break, Horn Attack, Endure |
| Mareep | 11 | Thunder Shock, Thunder Wave, Light Screen, Tackle |
| Wooper | 11 | Water Gun, Mud Shot, Tail Whip, Mud Sport |

## Validation and original source evidence

Snapshot validation rejects unsupported stages/maps, incompatible versions/sizes,
damaged checksums, invalid party records and missing original story prerequisites.
Writes require an idle field and replace files through a temporary file/rename.
`connected_checkpoint_resume` checks all six teams, levels, HP/PP, moves, distinct
rosters, favorite species coverage, missing/invalid entries, original-party
isolation, cancel/no-effect, failed follower preparation/retry, original full-party
swaps and saving/reopening every reordered team. It also retains the original rival,
allowed-loss, no-retrigger and damaged-snapshot scenarios. The native smoke checks
the story picker, party panel, recovered field and Back's exact owner/revision.

`original_violet_arrival` uses one representative Cyndaquil route plus the other
Mother savings response independently. Its source is
`build/checkpoints/155-violet-arrival.sav`; earlier 152/158 evidence is retained,
but the current generator does not repeat symmetric routes for those starters.
The original nickname application still requires choosing No. General blackout,
Nurse Joy, the catching tutorial and original-manager saving now execute; see
[save acceptance](save-acceptance.md) for their exact boundaries.

Historical version 2 added the original misc record and rival name. Both versions
1 and 2 are now rejected; regenerate all checkpoints using the command above. The new
`original_elm_return` scenario continues every post-rival starter through naming,
Egg handoff and lab re-entry, without injected story flags.

Version 3 replaces the selective story snapshot with the complete original
normal-record owner and exports full field/object state into checkpoint data.
Versions 1 and 2 are rejected. Regenerate using `heartgold_checkpoints`.
These development snapshots remain distinct from the original-manager cartridge
Save/Continue controls; see [save acceptance](save-acceptance.md).
