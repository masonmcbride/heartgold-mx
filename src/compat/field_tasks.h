#pragma once
#include "compat/overlays.h"
/* Scoped native owner for task.c only. This is NOT the original FieldSystem
 * layout and must not be passed to unported field units or guest ARM code. */
#define POKEHEARTGOLD_FIELD_SYSTEM_H
#define POKEHEARTGOLD_SCRIPT_MANAGER_H
typedef struct TaskManager TaskManager;
struct FieldSystem {
    TaskManager *taskman;
    PlayerAvatar *playerAvatar;
    OverlayManager *application;
    unsigned long long ticks;
};
#include "task.h"
void *Heap_AllocAtEnd(enum HeapID,u32);
BOOL sub_0203DF7C(FieldSystem *);
BOOL sub_0203DFA4(FieldSystem *);
BOOL sub_0203DF8C(FieldSystem *);
void FieldSystem_LoadFieldOverlayInternal(FieldSystem *);
void FieldSystem_LaunchApplication(FieldSystem *,const OverlayManagerTemplate *,void *);
/* Host frame boundary: original task dispatch, then original application
 * dispatch. Completed applications are deleted; waiting tasks resume next tick. */
void hg_field_tasks_tick(FieldSystem *);
int hg_field_tasks_idle(const FieldSystem *);
