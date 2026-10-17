#include "compat/field_tasks.h"
#include "field_app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1); } } while(0)
typedef struct { FieldSystem *field;unsigned children,inits,execs,exits,completed; } Test;
static int app_init(OverlayManager *man,int *state) {
    Test *t=OverlayManager_GetArgs(man);CHECK(FieldSystem_ApplicationIsRunning(t->field));
    CHECK((unsigned)*state==t->inits);t->inits++;return ++*state==2;
}
static int app_exec(OverlayManager *man,int *state) {
    Test *t=OverlayManager_GetArgs(man);CHECK((unsigned)*state==t->execs);t->execs++;return ++*state==3;
}
static int app_exit(OverlayManager *man,int *state) {
    Test *t=OverlayManager_GetArgs(man);CHECK((unsigned)*state==t->exits);t->exits++;return ++*state==2;
}
static BOOL child(TaskManager *task) {
    Test *t=TaskManager_GetEnvironment(task);CHECK(TaskManager_GetFieldSystem(task)==t->field);
    u32 *state=TaskManager_GetStatePtr(task);CHECK(*state==t->children);t->children++;return ++*state==2;
}
static BOOL jumped(TaskManager *task) {
    Test *t=TaskManager_GetEnvironment(task);CHECK(*TaskManager_GetStatePtr(task)==0);
    CHECK(!task->unk14 && task->unk10==0);t->completed++;return TRUE;
}
static BOOL root(TaskManager *task) {
    Test *t=TaskManager_GetEnvironment(task);u32 *state=TaskManager_GetStatePtr(task);
    CHECK(TaskManager_GetFieldSystem(task)==t->field);
    switch(*state) {
    case 0:
        task->unk1C->unk0=42;CHECK(sub_02050658(task)==42);*state=1;
        TaskManager_Call(task,child,t);return FALSE;
    case 1:{
        CHECK(t->children==2);CHECK(!FieldSystem_ApplicationIsRunning(t->field));*state=2;
        static const OverlayManagerTemplate template={app_init,app_exec,app_exit,FS_OVERLAY_ID_NONE};
        CallApplicationAsTask(task,&template,t);return FALSE;
    }
    case 2:
        CHECK(t->inits==2 && t->execs==3 && t->exits==2);CHECK(!FieldSystem_ApplicationIsRunning(t->field));
        task->unk14=Heap_Alloc(HEAP_ID_DEFAULT,64);task->unk10=64;
        TaskManager_Jump(task,jumped,t);return FALSE;
    }
    CHECK(0);return FALSE;
}
int main(void) {
    for(unsigned pass=0;pass<4;pass++) {
        FieldSystem field={0};Test test={.field=&field};
        CHECK(hg_field_tasks_idle(&field));CHECK(!FieldSystem_RunTaskFrame(&field));
        CHECK(!FieldSystem_TaskIsRunning(&field));CHECK(!FieldSystem_ApplicationIsRunning(&field));
        TaskManager *task=FieldSystem_CreateTask(&field,root,&test);CHECK(task && FieldSystem_TaskIsRunning(&field));
        for(unsigned i=0;i<30 && !hg_field_tasks_idle(&field);i++)hg_field_tasks_tick(&field);
        CHECK(hg_field_tasks_idle(&field));CHECK(test.completed==1);CHECK(field.ticks>7 && field.ticks<30);
    }
    char error[256];HGFieldApp *app=hg_field_app_new(60,error,sizeof(error));CHECK(app);
    CHECK(hg_field_app_ticks(app)==2);
    for(unsigned i=0;i<120;i++)hg_field_app_step(app);
    CHECK(hg_field_app_ticks(app)==122);
    unsigned char pixels[64*64*4];DSCamera camera;hg_field_view_camera(hg_field_app_view(app),&camera,64,64);
    CHECK(hg_field_app_render(app,&camera,pixels,64,64));CHECK(hg_field_app_ticks(app)==122);
    double visual_before[3],visual_step[3],visual_end[3];unsigned pose;
    CHECK(hg_field_app_presentation(app,visual_before,&pose)&&pose==1);
    hg_field_app_input(app,0x40);hg_field_app_step(app);
    CHECK(hg_field_app_presentation(app,visual_step,&pose));CHECK(pose==2||pose==4);
    CHECK(visual_step[0]==visual_before[0] && visual_step[2]==visual_before[2]-2);
    hg_field_app_input(app,0);
    for(unsigned i=0;i<7;i++)hg_field_app_step(app);
    CHECK(hg_field_app_presentation(app,visual_end,&pose)&&pose==1);
    CHECK(visual_end[2]==visual_before[2]-16);
    unsigned long long revision=hg_field_app_revision(app);
    CHECK(hg_field_app_render(app,&camera,pixels,64,64));CHECK(hg_field_app_revision(app)==revision);
    for(unsigned i=0;i<8;i++)hg_field_app_step(app);
    CHECK(hg_field_app_revision(app)==revision);
    int startx,startz,x,z;CHECK(hg_field_app_player(app,&startx,&startz));
    hg_field_app_camera(app,&camera,64,64);
    unsigned char before_pixels[64*64*4];
    CHECK(hg_field_app_render(app,&camera,before_pixels,64,64));
    const unsigned keys[]={0x40,0x80,0x20,0x10};int moved=0;
    for(unsigned d=0;d<4&&!moved;d++) {
        hg_field_app_input(app,keys[d]);
        for(unsigned tick=0;tick<8;tick++)hg_field_app_step(app);
        CHECK(hg_field_app_player(app,&x,&z));moved=x!=startx||z!=startz;
        hg_field_app_input(app,0);
    }
    CHECK(moved);CHECK(hg_field_app_render(app,&camera,pixels,64,64));
    CHECK(memcmp(before_pixels,pixels,sizeof(pixels)));
    for(unsigned i=0;i<20;i++)hg_field_app_step(app);
    int stoppedx,stoppedz;CHECK(hg_field_app_player(app,&stoppedx,&stoppedz));CHECK(stoppedx==x&&stoppedz==z);
    /* Holding a direction eventually reaches a real collision boundary. */
    hg_field_app_input(app,0x40);
    for(unsigned i=0;i<600;i++)hg_field_app_step(app);
    CHECK(hg_field_app_player(app,&x,&z));CHECK(x>=672&&x<704&&z>=384&&z<416);
    for(unsigned i=0;i<32;i++)hg_field_app_step(app);
    CHECK(hg_field_app_player(app,&stoppedx,&stoppedz));CHECK(stoppedx==x&&stoppedz==z);
    hg_field_app_input(app,0);
    hg_field_app_free(app);hg_field_app_step(NULL);CHECK(hg_field_app_ticks(NULL)==0);
    CHECK(!hg_field_app_new(540,error,sizeof(error)) && error[0]);
    puts("Original field tasks: child yield/parent resume, jump/scratch release, application wait/completion, repeated cleanup and independent frame clock passed");return 0;
}
