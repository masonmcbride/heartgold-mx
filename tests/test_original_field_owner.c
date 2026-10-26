/* Isolated process-mechanism harness. Parent/child callbacks and control are
 * explicit test doubles, NOT FieldMap or FieldSystem_Control acceptance. */
#include "original_field_owner.h"
#include "task.h"

/* Harness static module binding only; production registration is still open. */
#define FS_OVERLAY_ID(name) HG_TEST_OVERLAY_##name
enum { HG_TEST_OVERLAY_field = 1 };
void FieldSystem_LoadFieldOverlayInternal(FieldSystem *field) {
    (void)field;
    GF_ASSERT(!"real FieldMap initialization is not implemented in this harness");
    abort();
}

static unsigned control_frames, parent_steps, child_steps;
void FieldSystem_Control(FieldSystem *field) { GF_ASSERT(field); ++control_frames; }
void ov01_021F6830(FieldSystem *field, int a, int b) {
    (void)field; (void)a; (void)b;
    GF_ASSERT(!"presentation callback requires a real field-map owner");
}
#include "original_field_process.inc"

static BOOL init(OverlayManager *man, int *state) {
    GF_ASSERT(*state == 0 && OverlayManager_GetArgs(man));
    return TRUE;
}
static BOOL parent_exec(OverlayManager *man, int *state) {
    (void)man;
    ++parent_steps;
    return ++*state == 2;
}
static BOOL child_exec(OverlayManager *man, int *state) {
    (void)man;
    ++child_steps;
    return ++*state == 2;
}
static BOOL exit_app(OverlayManager *man, int *state) {
    GF_ASSERT(*state == 0 && OverlayManager_GetArgs(man));
    return TRUE;
}
static BOOL wait_child(TaskManager *task) {
    return !sub_0203DFA4(TaskManager_GetFieldSystem(task));
}

int main(void) {
    /* The entire original declaration, using native pointers. No guest offsets. */
    FieldProcessManager processes = {0};
    FieldSystem field = {0};
    field.processManager = &processes;
    field.runningFieldMap = TRUE;
    OverlayManagerTemplate parent = {init, parent_exec, exit_app, FS_OVERLAY_ID_NONE};
    OverlayManagerTemplate child = {init, child_exec, exit_app, FS_OVERLAY_ID_NONE};
    GF_ASSERT(hg_overlay_register(1));
    /* Original Main unloads the original field overlay ID after parent exit. */
    GF_ASSERT(HandleLoadOverlay(1, OVY_LOAD_ASYNC));
    processes.parent = OverlayManager_New(&parent, &field, HEAP_ID_FIELD2);
    GF_ASSERT(sub_0203DF7C(&field) && sub_0203DF8C(&field));
    FieldSystem_LaunchApplication(&field, &child, &field);
    GF_ASSERT(!field.runningFieldMap && sub_0203DFA4(&field));
    FieldSystem_CreateTask(&field, wait_child, NULL);
    processes.isDone = TRUE;
    /* Parent has priority; child must not advance while it exists. */
    for (int i = 0; i < 4; ++i) {
        GF_ASSERT(!FieldSystem_Main(&field));
        GF_ASSERT(child_steps == 0);
    }
    GF_ASSERT(!processes.parent && parent_steps == 2 && hg_overlay_live() == 0);
    GF_ASSERT(!sub_0203DF7C(&field) && !sub_0203DF8C(&field));
    for (int i = 0; i < 4; ++i) GF_ASSERT(!FieldSystem_Main(&field));
    GF_ASSERT(!processes.child && child_steps == 2 && field.taskman);
    /* Original task resumes on the following frame, then isDone can publish. */
    GF_ASSERT(FieldSystem_Main(&field));
    GF_ASSERT(!field.taskman && control_frames == 9);
    GF_ASSERT(hg_overlay_unregister(1));
    puts("original native field declaration, process arbitration and task return pass; synthetic applications only");
    return 0;
}
