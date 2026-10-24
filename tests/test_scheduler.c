#include "compat/scheduler.h"
#include "systask_environment.h"
#include "inventory.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static unsigned log_values[128],log_count;
static void record(SysTask *task,void *data) { GF_ASSERT(SysTask_GetData(task)==data);log_values[log_count++]=*(unsigned *)data; }
static void self_remove(SysTask *task,void *data) { record(task,data);SysTask_Destroy(task); }
static SysTask *victim;
static void remove_next(SysTask *task,void *data) { record(task,data);SysTask_Destroy(victim);SysTask_SetFunc(task,record); }
static unsigned early=2,later=10;
static void insert_children(SysTask *task,void *data) {
    record(task,data);GF_ASSERT(SysTaskQueue_InsertTask(task->queue,record,&early,2));
    GF_ASSERT(SysTaskQueue_InsertTask(task->queue,record,&later,10));SysTask_SetFunc(task,record);
}
static void increment(SysTask *task,void *data) { (void)task;(*(unsigned *)data)++; }
static unsigned environment_calls;
static void environment_callback(SysTask *task,void *data) {
    GF_ASSERT(data && *(unsigned *)data==0);*(unsigned *)data=123;environment_calls++;DestroySysTaskAndEnvironment(task);
}
static void zero_environment_callback(SysTask *task,void *data) { GF_ASSERT(!data);environment_calls++;DestroySysTaskAndEnvironment(task); }
static unsigned rejected_reentry;
static void reentry(SysTask *task,void *data) { (void)data;if(!hg_scheduler_step() && !hg_scheduler_shutdown())rejected_reentry++;SysTask_Destroy(task); }
static SysTask *from_main;
static void spawn_main(SysTask *task,void *data) { from_main=SysTask_CreateOnMainQueue(increment,data,1);GF_ASSERT(from_main);SysTask_Destroy(task); }
int main(void) {
    log_count=environment_calls=rejected_reentry=0;victim=from_main=NULL;
    /* Queue placement must use native sizeof, not original DS structure sizes. */
    size_t arena_size=SysTaskQueue_GetArenaSize(8);u8 *arena=calloc(1,arena_size+16);CHECK(arena);memset(arena+arena_size,0xa5,16);
    SysTaskQueue *q=SysTaskQueue_PlacementNew(8,arena);
    CHECK((uintptr_t)q->taskList%_Alignof(SysTask)==0);
    unsigned values[]={30,10,20,21};SysTask *tasks[4];
    tasks[0]=SysTaskQueue_InsertTask(q,record,&values[0],30);tasks[1]=SysTaskQueue_InsertTask(q,record,&values[1],10);
    tasks[2]=SysTaskQueue_InsertTask(q,record,&values[2],20);tasks[3]=SysTaskQueue_InsertTask(q,record,&values[3],20);
    CHECK(SysTask_GetPriority(tasks[0])==30);SysTaskQueue_RunTasks(q);
    CHECK(log_count==4 && log_values[0]==10 && log_values[1]==20 && log_values[2]==21 && log_values[3]==30);
    for(unsigned i=0;i<4;i++)SysTask_Destroy(tasks[i]);CHECK(q->activeCount==0);
    for(unsigned i=0;i<8;i++)CHECK(tasks[0]=SysTaskQueue_InsertTask(q,increment,&values[0],0));
    CHECK(!SysTaskQueue_InsertTask(q,record,&values[0],0));
    while(q->activeCount)SysTask_Destroy(q->headSentinel.next);
    log_count=0;CHECK(SysTaskQueue_InsertTask(q,self_remove,&values[0],0));SysTaskQueue_RunTasks(q);CHECK(log_count==1 && q->activeCount==0);
    log_count=0;CHECK(SysTaskQueue_InsertTask(q,remove_next,&values[1],10));victim=SysTaskQueue_InsertTask(q,record,&values[2],20);CHECK(victim);CHECK(SysTaskQueue_InsertTask(q,record,&values[0],30));
    SysTaskQueue_RunTasks(q);CHECK(log_count==2 && log_values[0]==10 && log_values[1]==30);
    while(q->activeCount)SysTask_Destroy(q->headSentinel.next);
    unsigned parent=5,tail=20;log_count=0;
    CHECK(SysTaskQueue_InsertTask(q,insert_children,&parent,5));CHECK(SysTaskQueue_InsertTask(q,record,&tail,20));
    SysTaskQueue_RunTasks(q);CHECK(log_count==2 && log_values[0]==5 && log_values[1]==20);
    log_count=0;SysTaskQueue_RunTasks(q);CHECK(log_count==4 && log_values[0]==2 && log_values[1]==5 && log_values[2]==10 && log_values[3]==20);
    while(q->activeCount)SysTask_Destroy(q->headSentinel.next);
    for(unsigned i=0;i<16;i++)CHECK(arena[arena_size+i]==0xa5);free(arena);
    /* Original queue API routes to four native-owned queues. */
    CHECK(hg_scheduler_init());CHECK(hg_scheduler_init());CHECK(gSystem.mainTaskQueue->limit==160 && gSystem.printTaskQueue->limit==4);
    unsigned counts[4]={0};
    tasks[0]=SysTask_CreateOnMainQueue(increment,&counts[0],1);tasks[1]=SysTask_CreateOnPrintQueue(increment,&counts[1],1);
    tasks[2]=SysTask_CreateOnVBlankQueue(increment,&counts[2],1);tasks[3]=SysTask_CreateOnVWaitQueue(increment,&counts[3],1);
    CHECK(!hg_scheduler_shutdown());
    for(unsigned i=0;i<60;i++)CHECK(hg_scheduler_step());
    CHECK(hg_scheduler_ticks()==60);for(unsigned i=0;i<4;i++) { CHECK(counts[i]==60);SysTask_Destroy(tasks[i]); }
    /* Native frame phases dispatch all four original queues in a fixed order. */
    unsigned phase_ids[]={1,2,3,4};log_count=0;
    tasks[0]=SysTask_CreateOnMainQueue(record,&phase_ids[0],1);tasks[1]=SysTask_CreateOnPrintQueue(record,&phase_ids[1],1);
    tasks[2]=SysTask_CreateOnVBlankQueue(record,&phase_ids[2],1);tasks[3]=SysTask_CreateOnVWaitQueue(record,&phase_ids[3],1);
    CHECK(hg_scheduler_step());CHECK(log_count==4);
    for(unsigned i=0;i<4;i++) { CHECK(log_values[i]==i+1);SysTask_Destroy(tasks[i]); }
    CHECK(CreateSysTaskAndEnvironment(environment_callback,sizeof(unsigned),2,HEAP_ID_DEFAULT));CHECK(CreateSysTaskAndEnvironment(zero_environment_callback,0,3,HEAP_ID_DEFAULT));
    CHECK(hg_scheduler_step());CHECK(environment_calls==2);
    CHECK(SysTask_CreateOnMainQueue(reentry,NULL,1));CHECK(hg_scheduler_step());CHECK(rejected_reentry==1);
    /* Input and menu interaction must not erase queue ownership. */
    HGInventory *inventory=hg_inventory_new();hg_inventory_action(inventory,HG_DOWN);hg_inventory_action(inventory,HG_NEXT_POCKET);hg_inventory_action(inventory,HG_ADD);hg_inventory_free(inventory);
    CHECK(gSystem.mainTaskQueue && gSystem.printTaskQueue);CHECK(hg_scheduler_step());
    unsigned cross_phase=0;CHECK(SysTask_CreateOnVBlankQueue(spawn_main,&cross_phase,1));CHECK(hg_scheduler_step());CHECK(cross_phase==0);CHECK(hg_scheduler_step());CHECK(cross_phase==1);SysTask_Destroy(from_main);
    CHECK(hg_scheduler_shutdown());CHECK(!gSystem.mainTaskQueue && !hg_scheduler_step());
    /* Rational 60 Hz timing, backwards clock rejection and bounded catch-up. */
    CHECK(hg_scheduler_init());CHECK(hg_scheduler_advance(1000000000)==0);
    for(unsigned i=1;i<=60;i++)hg_scheduler_advance(1000000000+(uint64_t)i*1000000000/60);
    CHECK(hg_scheduler_ticks()==60);CHECK(hg_scheduler_advance(1999999999)==0);CHECK(hg_scheduler_ticks()==60);
    CHECK(hg_scheduler_advance(UINT64_C(20000000000))==8);CHECK(hg_scheduler_ticks()==68);
    CHECK(hg_scheduler_shutdown());puts("Original task ordering, mutation, queue APIs, environments and native 60 Hz dispatch passed");return 0;
}
