#pragma once
typedef struct HGOriginalPropAnimations HGOriginalPropAnimations;
HGOriginalPropAnimations *hg_original_prop_animations_new(NARC *);
unsigned hg_original_prop_animations_count(HGOriginalPropAnimations *);
void hg_original_prop_animations_tick(HGOriginalPropAnimations *);
void hg_original_prop_animations_free(HGOriginalPropAnimations *);
int hg_original_prop_animations_preload(HGOriginalPropAnimations *,unsigned,NNSG3dRenderObj *,const u8 *,const DSTextureBank *);
unsigned hg_original_prop_animations_active(HGOriginalPropAnimations *);
const char *hg_original_prop_animations_error(HGOriginalPropAnimations *);

unsigned hg_original_prop_land_animation_count(HGOriginalPropAnimations *);

HGOriginalPropAnimations *hg_original_prop_animations_attach(void *,void *,void *);
void hg_original_prop_animations_release(HGOriginalPropAnimations *);

/* Shared decoder and native animation object; the ARM SDK prefix stays separate. */
void *hg_native_field_animation_new(const u8 *,size_t,NNSG3dResMdl *,unsigned);
int hg_native_field_animation_bind(NNSG3dRenderObj *,void *);
void hg_native_field_animation_detach(void *);
void hg_native_field_animation_free(void *);
int hg_native_field_animation_frame(void *,s32);

void hg_native_field_animation_observe_frame(void *,const u8 *);

int hg_native_field_animation_is_bound(void *,NNSG3dRenderObj *);
void hg_native_field_animation_detach_object(NNSG3dRenderObj *);
