# Original walk/run branch, input history and statistics

New Bark and its seven tested interiors support **WASD to walk** and **Shift + WASD to run**. Shift supplies the original B-button bit. The temporary inspection session initializes the original running-shoe ownership flag through `PlayerSaveData_SetRunningShoesFlag`; this is session equipment, not execution of the story event that awards shoes. No disk save is loaded or written.

Normal walking executes eight ticks at two world units per tick. Running executes four ticks at four units per tick. Both traverse a 16-unit tile through original command routines. Pressing or releasing Shift during a command affects the next command; it does not change an in-flight command's duration. A Shift-only tap cannot queue movement or attach itself to a later walking tap. Focus loss clears directional and modifier input.

## Original execution

`hg_motion_locomotion_begin` resolves direction through original `sub_0205DDD4`, then executes original `sub_0205CC4C` to record the horizontal/vertical input history and clear flag 6. Recording occurs after direction resolution, matching the inspected original entry point. This connects the original diagonal policy across successive inputs: the newly pressed axis takes priority, while unchanged diagonal input retains its selected axis. Idle updates reset the axis history after movement settles.

Original collision admission still restricts this path to state-0 movement between static New Bark tiles with a zero collision mask. Nonzero masks stop the ordinary step. The boundary refreshes the object's source-tile behavior through its original setter because the full original map-object update loop is not present.

For an admitted step, the **unchanged original `sub_0205D4B4` executes** its walk/run branch. It calls original native `PlayerSaveData_CheckRunningShoes`, original Thumb `sub_0205DE88`, original command mapping and `sub_0205DA1C` / `MapObject_SetHeldMovement`. That selects commands 12–15 for walking or 88–91 for running, sets the avatar's command and movement flags, and establishes original held-movement state. The original source branch, rather than a second native shoe/B rule, makes that selection.

Original dispatch and command routines advance each position, follow static terrain and complete the command. The native owner then executes original `MapObject_ClearHeldMovement` to release held-command ownership. The owner still controls when to dispatch and admit the next command; this is not the original complete map-object scheduler.

Original `sub_0205E048` increments the original step statistic through native-compiled `GameStats_Inc`. Original `ov01_02205990` writes the command, previous coordinates and notification state into a bounded, pointer-free, 32-byte follower-state mirror. The follower actor pointer remains zero. This executes the original notification function, not follower population, rendering or AI.

The guest preparation now assembles six complete original code units and three complete data-table units. `overlay_01_022053EC.s` supplies the follower notification routine; `unk_data_020FD978.s` supplies the remaining directional running tables. At this milestone the build-local image was 26,776 bytes, including 22,144 executable bytes, with 375 named unresolved imports; the [transition integration](original-transitions.md) records the expanded current image. Original message-constant headers required to assemble the complete overlay are generated from its original JSON inputs. Provenance hashes cover all nine assembly inputs and both message JSON inputs. These assembly counts are not execution coverage: only admitted routines and branches execute.

## Original statistics and math code

`hg_stats` compiles the complete unchanged `src/game_stats.c` and `src/math_util.c`. Native execution tests cover original statistics initialization, reads, increments, addition, capping, bounded updates and score functions, including the encrypted secure region. They also execute original `Math_CalcArraySum`, `_MonEncryptSegment`, `_MonDecryptSegment`, their internal encryption RNG, `LCRandom`, its seed accessors, `PRandom`, `SetMTRNGSeed` and `MTRandom`. Known ciphertext and MT19937 output vectors are checked independently of the application.

The 448-byte original statistics record contains fixed-width scalar fields and no pointers. The native player owns one such record. The guest's `Save_GameStats_Get` boundary resolves an opaque native session handle to that record; the original general `SaveArray_Get` and `Save_GameStats_Get` are not executed. `MI_CpuClear32` maps to bounded-owner memory clearing. `OS_GetVBlankCount` returns the native system-task frame counter; it does not emulate DS interrupts. The separate field-viewer clock does not advance that counter, so a field-only session normally initializes with a zero frame seed. This affects the record's encryption seed, not step timing.

Original trigonometry, matrix, CRC and Pokédex-dependent functions compile with explicit boundary declarations but do not link into this exercised path. No SDK providers or successful stand-ins are supplied for them. General save-file ownership, checksums, disk persistence and gameplay RNG integration beyond these tested functions remain pending.

Failed admission execution restores object/avatar/statistics state and preserves output. A successfully evaluated collision denial still records the new input history, as intended. If a later movement tick fails, the player owner restores its complete pre-step object, avatar, statistics record and follower notification mirror. This keeps a failed command from leaving an extra counted step or a stale follower notice.

## Presentation and verification

The original hero textures were decoded and visually inspected together: running frames are 17–32, grouped north/south/west/east like walking frames 1–16. Native pose selection uses running standing/stride textures during the original running command and returns to normal standing textures when it ends. Pose timing, foot alternation, immediate blocked turning and blocked retry remain native policies. Full `PlayerAvatar_MoveControl`, turn-in-place commands, running-shoe lock UI, footsteps, jumping, surfing and complete original transition tasks do not execute.

`original_locomotion_controller` checks all 3,968 in-chunk New Bark edges with shoes and B held: 1,590 runs complete and 2,378 are denied. Every run checks all four exact Q12 positions, one completion notification, original held-state cleanup, one original step increment and no cycling increment. Additional checks cover all four directions with every shoes/B combination, original follower notification transitions, idle input and a sequence of diagonal changes written by original input-history processing. The existing exhaustive eight-tick walking and terrain tests remain in place.

Application tests verify run displacement and poses, release of Shift halfway through a run, transition back to eight-tick walking, stopping, and a standalone Shift tap. The AppKit smoke test sends real modifier-bearing key events and a modifier-release event through the native canvas, then exercises the existing sign dialogue and browser paths. All 24 CTests pass normally and under ASan/UBSan; the native window smoke test passes.

The later [original transition integration](original-transitions.md) attaches avatar save state to an original local-field record and preserves the player, equipment and step statistics through New Bark doors and stairs. General save-file loading and full field-loop execution remain pending.
