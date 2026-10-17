#include "compat/field_tasks.h"
BOOL sub_0203DF7C(FieldSystem *field) { return field->application!=NULL; }
BOOL sub_0203DFA4(FieldSystem *field) { (void)field;return FALSE; /* No secondary application in this scoped host. */ }
static void unsupported(const char *name) { fprintf(stderr,"Unsupported field task boundary: %s\n",name);abort(); }
BOOL sub_0203DF8C(FieldSystem *field) { (void)field;unsupported("field overlay status");return FALSE; }
void FieldSystem_LoadFieldOverlayInternal(FieldSystem *field) { (void)field;unsupported("original field overlay load"); }
void FieldSystem_LaunchApplication(FieldSystem *field,const OverlayManagerTemplate *template,void *args) {
    GF_ASSERT(field && template && !field->application);
    field->application=OverlayManager_New(template,args,HEAP_ID_DEFAULT);
}
void hg_field_tasks_tick(FieldSystem *field) {
    GF_ASSERT(field);FieldSystem_RunTaskFrame(field);
    if(field->application && OverlayManager_Run(field->application)) {
        OverlayManager_Delete(field->application);field->application=NULL;
    }
    field->ticks++;
}
int hg_field_tasks_idle(const FieldSystem *field) { return field && !field->taskman && !field->application; }
