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
