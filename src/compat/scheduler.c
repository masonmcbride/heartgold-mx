#include "compat/scheduler.h"
#include "sys_task_api.h"
#include "system.h"
#include "ds_frame_clock.h"
#include "ds_irq.h"
#include <time.h>
#include <errno.h>

struct System gSystem;
static void *arenas[4];
static SysTaskQueue *queues[4];
static SysTask *frame_task;
static uint64_t ticks;
static DSFrameClock frame_clock;
static int initialized,dispatching;
static DSIrq interrupts;
static uint64_t display_deadline;
static int display_irq;
static uint64_t clock_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}

static void count_frame(SysTask *task,void *data) { (void)task;(void)data;ticks++; }

int hg_scheduler_init(void) {
    if(initialized)return 1;
    if(gSystem.mainTaskQueue || gSystem.printTaskQueue || gSystem.vblankTaskQueue || gSystem.vwaitTaskQueue)return 0;
    /* Capacities used by the original system initialization; native arena sizes. */
    static const u32 capacities[]={160,4,32,32};
    for(unsigned i=0;i<4;i++) {
        arenas[i]=calloc(1,SysTaskQueue_GetArenaSize(capacities[i]));
        if(!arenas[i]) {
            for(unsigned j=0;j<=i;j++) { free(arenas[j]);arenas[j]=NULL;queues[j]=NULL; }
            return 0;
        }
        queues[i]=SysTaskQueue_PlacementNew(capacities[i],arenas[i]);
    }
    gSystem.mainTaskQueue=queues[0];gSystem.printTaskQueue=queues[1];
    gSystem.vblankTaskQueue=queues[2];gSystem.vwaitTaskQueue=queues[3];
    ticks=0;frame_clock=(DSFrameClock){0};dispatching=0;ds_irq_init(&interrupts);
    frame_task=SysTask_CreateOnMainQueue(count_frame,NULL,UINT32_MAX);
    if(!frame_task)abort();initialized=1;return 1;
}

int hg_scheduler_step(void) {
    if(!initialized || dispatching || interrupts.dispatching)return 0;
    dispatching=1;
    /* Host phases, not emulated DS interrupts or a port of the full main loop. */
    for(unsigned i=0;i<4;i++){if(i==2&&display_irq)hg_scheduler_service_irqs();else SysTaskQueue_RunTasks(queues[i]);}
    dispatching=0;return 1;
}

unsigned hg_scheduler_advance(uint64_t now_ns) {
    if(!initialized || dispatching || interrupts.dispatching)return 0;
    unsigned due=ds_frame_clock_advance(&frame_clock,now_ns),count=0;
    while(count<due){if(!hg_scheduler_step())break;count++;}
    return count;
}

uint64_t hg_scheduler_ticks(void) { return ticks; }

int hg_scheduler_shutdown(void) {
    if(!initialized)return 1;
    if(display_irq || dispatching || interrupts.dispatching || gSystem.vBlankIntr || gSystem.hBlankIntr || queues[0]->activeCount!=1)return 0;
    for(unsigned i=1;i<4;i++)if(queues[i]->activeCount)return 0;
    SysTask_Destroy(frame_task);frame_task=NULL;
    gSystem.mainTaskQueue=gSystem.printTaskQueue=gSystem.vblankTaskQueue=gSystem.vwaitTaskQueue=NULL;
    for(unsigned i=0;i<4;i++) { free(arenas[i]);arenas[i]=NULL;queues[i]=NULL; }
    initialized=0;ticks=0;frame_clock=(DSFrameClock){0};return 1;
}

DSIrq *hg_scheduler_irq(void){return initialized?&interrupts:NULL;}
int hg_scheduler_hblank(void){if(!initialized)return 0;ds_irq_hblank_edge(&interrupts);return 1;}

int hg_scheduler_start_vblank(void (*handler)(void)){
    if(!initialized||display_irq||!handler||interrupts.dispatching||interrupts.handlers[0])return 0;
    display_deadline=clock_ns()+1000000000/60;display_irq=1;
    ds_irq_set_handler(&interrupts,DS_IRQ_VBLANK,handler);ds_irq_enable_mask(&interrupts,DS_IRQ_VBLANK);return 1;
}
int hg_scheduler_stop_vblank(void){
    if(!display_irq||interrupts.dispatching||queues[2]->activeCount)return 0;
    ds_irq_disable_mask(&interrupts,DS_IRQ_VBLANK);interrupts.pending&=~DS_IRQ_VBLANK;
    ds_irq_set_handler(&interrupts,DS_IRQ_VBLANK,NULL);display_irq=0;return 1;
}
int hg_scheduler_vblank_active(void){return display_irq;}
unsigned hg_scheduler_service_irqs(void){
    if(!display_irq||interrupts.dispatching)return 0;uint64_t now=clock_ns();
    if(now<display_deadline)return 0;
    /* Hardware requests latch while masked; elapsed display edges coalesce.
     * No main-loop frame, VWait queue or host tick counter advances here. */
    display_deadline+=((now-display_deadline)/(1000000000/60)+1)*(1000000000/60);
    ds_irq_raise(&interrupts,DS_IRQ_VBLANK);return 1;
}
int hg_scheduler_wait_vblank(void){
    if(!display_irq||interrupts.dispatching||!interrupts.cpu_enabled||!(interrupts.mask&DS_IRQ_VBLANK)||!interrupts.handlers[0])return 0;
    for(;;){
        uint64_t now=clock_ns();if(now>=display_deadline)return hg_scheduler_service_irqs()!=0;
        uint64_t left=display_deadline-now;struct timespec delay={(time_t)(left/1000000000),(long)(left%1000000000)};
        if(nanosleep(&delay,NULL)&&errno!=EINTR)return 0;
    }
}
