#include "compat/native.h"
#include "compat/native_irq.h"
#include "compat/scheduler.h"
static DSIrq *current(void){DSIrq *irq=hg_scheduler_irq();GF_ASSERT(irq);return irq;}
uint32_t hg_native_irq_disable(void){return ds_irq_cpu_enable(current(),0)?0:0x80;}
uint32_t hg_native_irq_enable(void){return ds_irq_cpu_enable(current(),1)?0:0x80;}
uint32_t hg_native_irq_restore(uint32_t state){return ds_irq_cpu_enable(current(),!(state&0x80u))?0:0x80u;}

uint32_t hg_native_irq_mask(void){return current()->mask;}
uint32_t hg_native_irq_disable_mask(uint32_t mask){return ds_irq_disable_mask(current(),mask);}
uint32_t hg_native_irq_enable_mask(uint32_t mask){return ds_irq_enable_mask(current(),mask);}
void hg_native_irq_set_function(uint32_t mask,DSIrqHandler handler){
    GF_ASSERT(mask&&!(mask&~(DS_IRQ_HBLANK|DS_IRQ_VBLANK)));ds_irq_set_handler(current(),mask,handler);
}
int hg_native_irq_hblank_request(int enabled){return ds_irq_hblank_request(current(),enabled);}

void hg_native_irq_check_set(uint32_t mask){GF_ASSERT(mask==DS_IRQ_VBLANK);current()->check_flags|=mask;}
void hg_native_irq_wait_dma(uint32_t channel){GF_ASSERT(channel==3);/* Current native transfers complete synchronously before publication. */__atomic_thread_fence(__ATOMIC_ACQUIRE);}
