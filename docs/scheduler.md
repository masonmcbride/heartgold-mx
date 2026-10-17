# Native execution of the original task scheduler

The native library now compiles original `src/sys_task.c`, `src/sys_task_api.c`, and `src/systask_environment.c` directly from upstream. Their queue algorithms, API routing, and environment lifecycle functions are executed by tests without changes to upstream. The application uses the original queue implementation and API to dispatch a frame-counting task that drives Classic-view animation.

## Runtime ownership and timing

`src/compat/scheduler.c` owns four zero-initialized, normally allocated host arenas. Original `SysTaskQueue_GetArenaSize` and `SysTaskQueue_PlacementNew` calculate and place queues using native structure and pointer sizes. Capacities match the original system initialization: main 160, print 4, VBlank 32, VWait 32. One main-queue slot belongs to the native frame-counter task.

`hg_scheduler_init()` must run before using the original global queue API. It imports the **original `struct System` declaration** rather than extending the previous two-field substitute. That structure has native pointers and is not a DS memory/save ABI. Input changes update only the two key fields, so navigation cannot erase queue pointers or other system state.

`hg_scheduler_step()` runs the original queue dispatchers in the native host order main → print → VBlank → VWait. It is deterministic and useful for tests or headless callers. `hg_scheduler_advance(now_ns)` accumulates a monotonic host timestamp at rational 60 Hz and dispatches at most eight frames per call. It clamps a single elapsed interval to 250 ms and drops excess backlog after eight frames. Backwards timestamps are ignored. This prevents a stalled window from triggering unlimited catch-up work.

The macOS event-loop timer supplies monotonic uptime to `advance`. Classic sprite animation reads the tick count incremented by an actual callback dispatched through original `SysTaskQueue_RunTasks`. The original queues therefore perform work in the running app, rather than merely linking into it. No worker thread, DS interrupt, hardware timer, low-address mapping, or emulated CPU is involved.

This host phase order is not the complete original frame/interrupt schedule. The original main loop conditionally processes overlays, sometimes runs print tasks twice, waits for hardware VBlank, handles audio/DMA, and performs other work. None of that is claimed as ported. The global frame/VBlank counters are not yet driven as in the original hardware loop.

## Lifecycle boundaries

The runtime is main-thread-only. Reentrant stepping and shutdown during a task callback are rejected. `hg_scheduler_shutdown()` refuses to free arenas while caller-owned tasks remain. Callers must destroy their own tasks and environments before shutdown; the runtime removes its frame-counter task and frees the four arenas afterward.

The prior abort stubs for `SysTask_CreateOnPrintQueue`, `SysTask_Destroy`, and `SysTask_GetData` have been removed. Their original implementations now run. Full text-printer rendering remains unsupported at `String_cstr`/`FontID_RenderText`; enabling task queues does not port that rendering path.

The original environment helper allocates its environment before trying to enqueue. If the queue is full, that original path can leak the allocation. It has not been rewritten. Native callers should establish available capacity before requesting an allocating environment; tests exercise successful ownership and zero-size environments. Double-destruction, stale task pointers, invalid callback behavior and arbitrary manipulation of queue internals remain outside supported game preconditions.

## Executable verification

`test_scheduler` executes original functions to verify:

- priority ordering and FIFO behavior among equal priorities;
- full queue rejection, slot reuse and arena placement/alignment, with a trailing canary;
- self-removal and removal of the next task during dispatch;
- insertion at earlier and later priorities while dispatching, preserving the original deferral behavior;
- callback replacement, task data and priority accessors;
- routing to all four original global queue APIs and native phase ordering;
- zero-initialized environments, zero-size environments and self-destruction/freeing;
- cross-phase creation, where a VBlank-created main task waits for the next main phase;
- input/menu operations preserving queue ownership;
- initialization, pending-task shutdown rejection, reentry rejection and clean reinitialization;
- exact 60-tick accumulation over one second, backwards timestamps and bounded catch-up after a stall.

The normal inventory and graphics tests still run. The graphical smoke test also requires scheduled ticks to advance before exercising input and rendering.

```sh
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/heartgold_native
./build/heartgold_metal --smoke
```

The terminal demo now reports `Original SysTask queues: 60 native frames dispatched` after its original inventory demonstration. The whole-source audit now emits 28 game C objects rather than 27, with `sys_task_api.c` newly compiling. More significantly, the queue, queue API, and environment implementation have moved from compile candidates to tested original native execution.

## Next integration

A practical next step is an original field/metatile slice driven by these queues, together with a host time interface where needed. The full DS Timer 3/IRQ implementation remains unexecuted; it contains a direct register access and needs a host boundary. Original overlay loading and the main game loop remain separate work.
