#pragma once
#include "ds_joint_animation.h"
#include "compat/script.h"
typedef struct HGPropAnimations HGPropAnimations;
HGPropAnimations *hg_prop_animations_new(unsigned map);
HGPropAnimations *hg_prop_animations_clone(const HGPropAnimations *);
void hg_prop_animations_free(HGPropAnimations *);
int hg_prop_animations_tick(HGPropAnimations *);
int hg_prop_animations_busy(const HGPropAnimations *);
int hg_prop_animations_validate(HGPropAnimations *,HGFieldResources *,const ScriptContext *,unsigned op);
unsigned hg_prop_animations_poses(const HGPropAnimations *,HGPropPose out[16]);
void ov01_021E9AE8(FieldSystem *,int x,int z,u8 tag);
void ov01_021E9BB8(FieldSystem *,u8 tag);
void ov01_021E9BDC(FieldSystem *,u8 tag);
void ov01_021E9C00(FieldSystem *,u8 tag);
void ov01_021E9C20(FieldSystem *,u8 tag);

/* Task-facing operations share the same original animation owner as scripts. */
int hg_prop_animation_load(HGPropAnimations *,unsigned tag,unsigned model,const double world[3]);
int hg_prop_animation_play(HGPropAnimations *,unsigned tag,unsigned index);
int hg_prop_animation_finished(HGPropAnimations *,unsigned tag);
int hg_prop_animation_unload(HGPropAnimations *,unsigned tag);

/* Original door rectangle/list metadata; 1 opened, 0 no door, -1 unsupported. */
int hg_prop_animation_open_door(HGPropAnimations *,HGFieldResources *,int x,int z,unsigned tag,unsigned *sound);

int hg_prop_animation_door_begin(HGPropAnimations *,HGFieldResources *,int x,int z,unsigned tag,int exiting,unsigned *sound);
int hg_prop_animation_close_door(HGPropAnimations *,unsigned tag,unsigned *sound);
