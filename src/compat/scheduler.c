#include "compat/scheduler.h"
#include "sys_task_api.h"
#include "system.h"
#include "ds_frame_clock.h"

struct System gSystem;
static void *arenas[4];
static SysTaskQueue *queues[4];
static SysTask *frame_task;
static uint64_t ticks;
static DSFrameClock frame_clock;
static int initialized,dispatching;

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
    ticks=0;frame_clock=(DSFrameClock){0};dispatching=0;
    frame_task=SysTask_CreateOnMainQueue(count_frame,NULL,UINT32_MAX);
    if(!frame_task)abort();initialized=1;return 1;
}

int hg_scheduler_step(void) {
    if(!initialized || dispatching)return 0;
    dispatching=1;
    /* Host phases, not emulated DS interrupts or a port of the full main loop. */
    for(unsigned i=0;i<4;i++)SysTaskQueue_RunTasks(queues[i]);
    dispatching=0;return 1;
}

unsigned hg_scheduler_advance(uint64_t now_ns) {
    if(!initialized || dispatching)return 0;
    unsigned due=ds_frame_clock_advance(&frame_clock,now_ns),count=0;
    while(count<due){if(!hg_scheduler_step())break;count++;}
    return count;
}

uint64_t hg_scheduler_ticks(void) { return ticks; }

int hg_scheduler_shutdown(void) {
    if(!initialized)return 1;
    if(dispatching || queues[0]->activeCount!=1)return 0;
    for(unsigned i=1;i<4;i++)if(queues[i]->activeCount)return 0;
    SysTask_Destroy(frame_task);frame_task=NULL;
    gSystem.mainTaskQueue=gSystem.printTaskQueue=gSystem.vblankTaskQueue=gSystem.vwaitTaskQueue=NULL;
    for(unsigned i=0;i<4;i++) { free(arenas[i]);arenas[i]=NULL;queues[i]=NULL; }
    initialized=0;ticks=0;frame_clock=(DSFrameClock){0};return 1;
}
