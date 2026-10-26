#pragma once
#include <stdint.h>
/* Host-owned DS interrupt routing. Event sources raise edges explicitly; this
 * controller owns no clock, task queue, CPU exception frame or scanline timer. */
enum { DS_IRQ_VBLANK=1u, DS_IRQ_HBLANK=2u };
typedef void (*DSIrqHandler)(void);
typedef struct DSIrq {
    uint32_t mask,pending,check_flags;
    DSIrqHandler handlers[32];
    unsigned cpu_enabled,hblank_request,dispatching;
} DSIrq;
void ds_irq_init(DSIrq *);
int ds_irq_cpu_enable(DSIrq *,int);
uint32_t ds_irq_enable_mask(DSIrq *,uint32_t);
uint32_t ds_irq_disable_mask(DSIrq *,uint32_t);
void ds_irq_set_handler(DSIrq *,uint32_t,DSIrqHandler);
void ds_irq_raise(DSIrq *,uint32_t);
int ds_irq_hblank_request(DSIrq *,int);
void ds_irq_hblank_edge(DSIrq *);
