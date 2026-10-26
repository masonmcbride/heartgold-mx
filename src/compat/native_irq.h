#pragma once
#include "ds_irq.h"
/* Typed native scalar/callback interfaces used by original system.c. */
uint32_t hg_native_irq_disable(void);
uint32_t hg_native_irq_enable(void);
uint32_t hg_native_irq_restore(uint32_t state);
uint32_t hg_native_irq_mask(void);
uint32_t hg_native_irq_disable_mask(uint32_t);
uint32_t hg_native_irq_enable_mask(uint32_t);
void hg_native_irq_set_function(uint32_t,DSIrqHandler);
int hg_native_irq_hblank_request(int);

void hg_native_irq_check_set(uint32_t);
void hg_native_irq_wait_dma(uint32_t);
