#pragma once
#include <stdint.h>
/* Main-thread-owned native host for the original four SysTask queues.
 * step() is deterministic; advance() accepts a monotonic host timestamp.
 */
int hg_scheduler_init(void);
int hg_scheduler_step(void);
unsigned hg_scheduler_advance(uint64_t now_ns);
uint64_t hg_scheduler_ticks(void);
/* Refuses shutdown during dispatch or while caller-owned tasks remain. */
int hg_scheduler_shutdown(void);
