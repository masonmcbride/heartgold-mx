#include "ds_irq.h"
#include <string.h>
static void dispatch(DSIrq *irq) {
    if(irq->dispatching)return;
    irq->dispatching=1;
    while(irq->cpu_enabled) {
        uint32_t ready=irq->mask&irq->pending;
        if(!ready)break;
        unsigned index=0;while(!(ready&(1u<<index)))index++;
        /* Acknowledge before callback delivery, as the OS IRQ dispatcher does.
         * A source raised by the callback remains a new pending request. */
        if(!irq->handlers[index])break;
        irq->pending&=~(1u<<index);
        irq->handlers[index]();
    }
    irq->dispatching=0;
}
void ds_irq_init(DSIrq *irq){memset(irq,0,sizeof(*irq));irq->cpu_enabled=1;}
int ds_irq_cpu_enable(DSIrq *irq,int enabled){int previous=irq->cpu_enabled;irq->cpu_enabled=!!enabled;dispatch(irq);return previous;}
uint32_t ds_irq_enable_mask(DSIrq *irq,uint32_t mask){uint32_t previous=irq->mask;irq->mask|=mask;dispatch(irq);return previous;}
uint32_t ds_irq_disable_mask(DSIrq *irq,uint32_t mask){uint32_t previous=irq->mask;irq->mask&=~mask;return previous;}
void ds_irq_set_handler(DSIrq *irq,uint32_t mask,DSIrqHandler handler){for(unsigned i=0;i<32;i++)if(mask&(1u<<i))irq->handlers[i]=handler;dispatch(irq);}
void ds_irq_raise(DSIrq *irq,uint32_t mask){irq->pending|=mask;dispatch(irq);}
int ds_irq_hblank_request(DSIrq *irq,int enabled){int previous=irq->hblank_request;irq->hblank_request=!!enabled;return previous;}
void ds_irq_hblank_edge(DSIrq *irq){if(irq->hblank_request)ds_irq_raise(irq,DS_IRQ_HBLANK);}
