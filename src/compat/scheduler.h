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

struct DSIrq;
/* Explicit display-event input; it does not run or advance task queues. */
struct DSIrq *hg_scheduler_irq(void);
int hg_scheduler_hblank(void);

/* Optional original VBlank IRQ owner. Real monotonic display deadlines only;
 * excludes the host step's VBlank pump, never runs main/print/VWait work. */
int hg_scheduler_start_vblank(void (*handler)(void));
int hg_scheduler_stop_vblank(void);
unsigned hg_scheduler_service_irqs(void);
int hg_scheduler_wait_vblank(void);
int hg_scheduler_vblank_active(void);
