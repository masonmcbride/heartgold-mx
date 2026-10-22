# Original save manager: native reuse boundary

The feasibility probe below is historical. The manager, registry, durable
transport and field Save/Continue now execute; see [current acceptance and
remaining gates](save-acceptance.md). The original full Continue lifecycle and
complete-save acceptance are still unfinished.

The complete original save owner is `SaveData` in upstream `include/save.h`,
managed by `src/save.c`. Reusing that manager is a stronger implementation path
than extending a host snapshot field by field: the original manager owns the
42 normal records, slot layout, CRCs, counters, alternating sectors, modified PC
boxes, extra records and save/load status. The six extra records have a separate
lifecycle; Hall of Fame is not initialized by the ordinary extra-chunk wipe.

## Probe and evidence

A build-local copy of unchanged `src/save.c` compiled to a native object using
minimal SDK/service declarations in `build/save-audit/include`. This probe did
**not** link or execute the manager. Implicit function declarations were allowed
to enumerate imports; they must be replaced with exact declarations before any
execution. Assertions were disabled in this temporary probe. This is feasibility
evidence, not a save implementation or correctness test.

The original SaveData declaration contains no host pointers. Both native and
`clang -target arm-none-eabi` LLVM constant-layout probes produced the identical
array `[144140,16,143380,144052,144076,16,16,12]` for SaveData size,
dynamic/header/slot/async offsets, and chunk-footer/array-header/slot-spec sizes.
Probe artifacts are `build/save-audit/layout_{arm,native}.ll`. The equivalent
hexadecimal values are:

| Member | Native result | Original declaration/comment |
| --- | --- | --- |
| sizeof(SaveData) | 0x2330c | 0x2330c |
| dynamic_region | 0x10 | fixed fields precede it |
| arrayHeaders | 0x23014 | 0x23014 |
| saveSlotSpecs | 0x232b4 | 0x232b4 |
| asyncWriteMan | 0x232cc | follows two 0xc slot specs |
| sizeof(SaveChunkFooter) | 0x10 | four fixed-width fields |

This does not establish the layouts of every individual save record. Their
original size functions and type layouts still need compilation/execution or
original guest execution, including records whose initializers remain assembly.

## Concrete compatibility boundary

The whole manager object imports:

- Backup transport: `CARD_CancelBackupAsync`, `CARD_GetResultCode`,
  `CARD_IdentifyBackup`, `CARD_LockBackup`, `CARD_ReadBackup`,
  `CARD_ReadBackupAsync`, `CARD_TryWaitBackupAsync`, `CARD_UnlockBackup`,
  `CARD_WaitBackupAsync`, `CARD_WriteAndVerifyBackupAsync`.
- Lock lifecycle: `OS_GetLockID`, `OS_ReleaseLockID`.
- Host services: `Heap_Alloc`, `Heap_AllocAtEnd`, `Heap_Free`, `GF_CalcCRC16`,
  `PRandom`, `Sys_SetSleepDisableFlag`, `Sys_ClearSleepDisableFlag`,
  `ShowSaveDataReadError`, `ShowSaveDataWriteError`.
- Original registry: both `g*SaveChunkHeaders` arrays and their counts.
- Original ownership helpers: `PCStorage_GetNumBoxes`, `PCStorage_GetSizeOfBox`,
  `Save_GetPCBoxModifiedFlags`, `Save_ResetPCBoxModifiedFlags`,
  `Save_SetAllPCBoxesModified`, `sub_020271A0`, `Save_Misc_Get`,
  `SaveMisc_CheckExtraChunksExist`, `SaveMisc_SetExtraChunksExist`,
  `sub_0202AC38`, `sub_0202AC60`, `Save_Frontier_Load`,
  `Save_Frontier_Commit`, `sub_0202C6FC`, `sub_0202C714`.

The backup transport is a reusable DS runtime service: a bounded 512 KiB flash
image, explicit read/write failures, completion callback, result state and lock
ownership. The original manager should retain the file-format decisions. A host
transport must make completed writes durable and preserve the previous valid
sector under interrupted writes. A successful async enqueue is not a durable
save. Read/write error handlers currently free the global save owner, so native
error presentation must terminate or deliberately transfer control; returning to
ordinary field execution would risk use after free.

## First connected implementation

1. Compile the whole original manager with exact boundary declarations and
   assertions enabled. Bind original registry size/init functions, including
   assembly-backed entries; no guessed sizes or blanket-zero substitutes.
2. Supply the flash service and execute `SaveData_New`, original initialization,
   `SaveGameNormal`, process teardown, and `SaveData_TryLoadOnContinue` against
   all original records. Exercise extra chunks through their original APIs.
3. Replace detached StoryData/snapshot ownership with this owner. Adapt original
   getters at one explicit boundary; live writes must reach the same bytes the
   manager saves. Capture field/map objects through original save preparation,
   and restore them through original continue reconstruction.
4. Verify every record, both normal slots, all extra chunks, modified PC-box
   writes, corrupt/newer-sector fallback, interrupted writes, error paths,
   restart, and the actual save/continue UI. Test RNG/clock/platform metadata
   under their original lifecycle rather than treating all runtime state as save
   bytes.

Native sanitizers may expose original compiler assumptions. In particular,
`SaveData_InitSubstructs` and `SaveData_InitSlotSpecs` read an indexed header
before their trailing bounds predicate. The exact end-of-array behavior must be
checked with the real registry under ASan/UBSan before accepting unchanged native
execution. Never modify upstream; any required adaptation belongs at the explicit
port boundary or generated build-local compatibility transformation.

The remaining bottleneck is complete registry execution and shared ownership,
not the host filesystem write itself. A durable host snapshot of currently
ported fields cannot establish that all original Pokémon state is persisted.

## Field synchronization and continue reconstruction

`Field_SaveGameNormal` in original `asm/overlay_01_021F3F50.s` first calls
`ov01_021F4404`, then `SaveGameNormal`. That preparation synchronizes all map
objects through `FieldSystem_SyncMapObjectsToSave`, calls `ov01_021F6830` with
arguments `(fieldSystem,4,0)`, copies avatar X/Z/facing into the current Location,
and sets the warp ID to -1. Bypassing it loses live field state even if every
record is written. The unknown helper must be traced before replacing it.

Continue is also broader than reading bytes. `overlay_36.c` initializes RNG,
loads through `SaveData_TryLoadOnContinue`, reapplies button mode, handles changed
console MAC/RTC metadata, resets relevant timed state and Shaymin form when
required, restarts the in-game timer, then launches `gApplication_ContinueFieldsys`.
The existing original `sub_0205323C` in `field_warp_tasks.c` creates the map object
manager, restores saved objects, creates the avatar from the active saved object,
rebinds the follower and resumes object processing. These are the connected
pre-save and post-load execution paths to validate; copying location alone
cannot substitute for them.
