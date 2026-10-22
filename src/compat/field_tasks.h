#pragma once
#include "compat/overlays.h"
/* Scoped native owner for admitted task, motion and transition code. This is NOT the original FieldSystem
 * layout and must not be passed to unported field units or guest ARM code. */
#define POKEHEARTGOLD_FIELD_SYSTEM_H
#define POKEHEARTGOLD_SCRIPT_MANAGER_H
typedef struct TaskManager TaskManager;
typedef struct MapEvents MapEvents;
typedef struct MapObjectManager MapObjectManager;
typedef struct Location Location;
typedef struct HGFieldResources HGFieldResources;
typedef struct MapPropManager MapPropManager;
typedef struct MapPropAnimationManager MapPropAnimationManager;
struct FieldSystem {
    struct GearPhoneRingManager *phoneRingManager;
    struct BugContest *bugContest;
    TaskManager *taskman;
    struct MenuInputStateMgr *native_menu_input;
    struct BagCursor *bagCursor;
    struct DSAudio *native_audio;
    BOOL native_field_suspended;
    BOOL unkD2_7; /* Original script phone-ring latch, host-owned. */
    PlayerAvatar *playerAvatar;
    OverlayManager *application;
    unsigned long long ticks;
    u16 reverseTurnFrameSteps;u8 lastFacingDirection;
    MapEvents *mapEvents;
    MapObjectManager *mapObjectManager;
    Location *location;
    SaveData *saveData;
    struct HGStarter *native_starter;
    struct LocalFieldData *native_local_field;
    struct HGMotionFade *native_fade;
    struct HGPropAnimations *native_prop_animations;
    struct HGHealing *native_healing;
    struct HGMotionTask *native_motion_task;
    struct HGMotionSession *native_follow_session;
    MapPropManager *mapPropManager;
    MapPropAnimationManager *mapPropAnimationManager;
    HGFieldResources *native_scene_resources; /* Host-only static terrain owner. */
};
#include "task.h"
void *Heap_AllocAtEnd(enum HeapID,u32);BOOL sub_0203DF7C(FieldSystem *);
BOOL sub_0203DFA4(FieldSystem *);
BOOL sub_0203DF8C(FieldSystem *);
void FieldSystem_LoadFieldOverlayInternal(FieldSystem *);
void FieldSystem_LaunchApplication(FieldSystem *,const OverlayManagerTemplate *,void *);
/* Host frame boundary: original task dispatch, then original application
 * dispatch. Completed applications are deleted; waiting tasks resume next tick. */
void hg_field_tasks_tick(FieldSystem *);
int hg_field_tasks_idle(const FieldSystem *);
/* One original contest owner per field system. The accessor is inline so the
 * field-time, battle, script and naming layers share the same owner without a
 * link-order dependency between them. */
static inline struct BugContest *FieldSystem_BugContest_Get(FieldSystem *field) { return field->bugContest; }
