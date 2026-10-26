#include "ds_texture.h"
#include "ds_model.h"
#pragma once
#include "compat/resources.h"
/* Typed sidecar for the original ARM loader, never an original DS struct cast. */
typedef struct HGOriginalLoader HGOriginalLoader;
HGOriginalLoader *hg_original_loader_new(MapMatrix *,unsigned);
int hg_original_loader_initial_load(HGOriginalLoader *,int,int);
int hg_original_loader_prepare_area(HGOriginalLoader *,unsigned);
const char *hg_original_loader_error(const HGOriginalLoader *);
unsigned hg_original_loader_texture_count(const HGOriginalLoader *);
const DSTextureBank *hg_original_loader_texture(const HGOriginalLoader *,unsigned);
unsigned hg_original_loader_texture_shrinks(const HGOriginalLoader *);
void hg_original_loader_delete(HGOriginalLoader *);
unsigned hg_original_loader_bound_models(const HGOriginalLoader *);
int hg_original_loader_model_material(HGOriginalLoader *,unsigned,unsigned,DSModelMaterial *,DSTexture *);

int hg_original_loader_animation_sample(HGOriginalLoader *,double [6]);
void hg_original_loader_tick_animations(HGOriginalLoader *);

unsigned hg_original_loader_active_props(HGOriginalLoader *);

int hg_original_loader_tick(HGOriginalLoader *);
/* Borrowed byte view; valid only while the selected original land slot lives. */
void *hg_original_loader_soundplate(HGOriginalLoader *,int,int);
unsigned hg_original_loader_land_triangles(HGOriginalLoader *);

/* End requests teardown once. Tick remains legal until owner destruction. */
int hg_original_loader_end(HGOriginalLoader *);
int hg_original_loader_has_ended(HGOriginalLoader *,int *);

int hg_original_loader_render_probe(HGOriginalLoader *,unsigned *);

void hg_original_terrain_install(void **,int);
int hg_original_loader_terrain(HGOriginalLoader *,const void *,int,int,int,int32_t,int32_t *);

/* Transport success is separate from the original no-surface kind zero. */
int hg_original_loader_height(HGOriginalLoader *,const void *,int,int,int32_t,int32_t *,unsigned *);
int hg_original_loader_behavior(HGOriginalLoader *,const void *,int,int,unsigned *);

/* Same guest transport, without allocating a gameplay map-loader owner. */
HGOriginalLoader *hg_original_resource_context_new(void);
int hg_original_billboards_create(HGOriginalLoader *,unsigned,unsigned);
int hg_original_billboards_delete(HGOriginalLoader *);
int hg_original_billboards_draw(HGOriginalLoader *);

int hg_original_loader_attach_map(HGOriginalLoader *,MapMatrix *,unsigned);

void hg_original_oam_module_init(HGOriginalLoader *);
int hg_original_oam_create(HGOriginalLoader *,const uint32_t [9]);
int hg_original_oam_apply(HGOriginalLoader *);
int hg_original_oam_free(HGOriginalLoader *);

void *hg_original_aux_new(HGOriginalLoader *,unsigned);
int hg_original_aux_free(HGOriginalLoader *,void *);

void *hg_original_area_new(HGOriginalLoader *,unsigned);
void *hg_original_area_headers(HGOriginalLoader *,void *);
NARC *hg_original_area_animation_list(HGOriginalLoader *,void *);
void hg_original_bind_pools(HGOriginalLoader *,void *);
int hg_original_area_load(HGOriginalLoader *,void *,void *,void *,void *,void *);
void hg_original_release_pools(HGOriginalLoader *);
void *hg_original_map_owner(HGOriginalLoader *);
void *hg_original_height_new(HGOriginalLoader *,unsigned,unsigned);

void hg_original_loader_bind_save(HGOriginalLoader *,void *);

/* Opaque mapped ARM effect owner; callback descriptors remain original ARM32. */
void *hg_original_effects_new(HGOriginalLoader *,void *,unsigned,unsigned);
int hg_original_effects_anim_init(HGOriginalLoader *,unsigned);
int hg_original_effects_resources_init(HGOriginalLoader *,const uint32_t [8]);
int hg_original_effects_register(HGOriginalLoader *);
int hg_original_effects_free(HGOriginalLoader *);

unsigned hg_original_effect_member_size(HGOriginalLoader *,unsigned);
int hg_original_effect_member_read(HGOriginalLoader *,unsigned,void *);
int hg_original_effect_native_model(HGOriginalLoader *,void *,void *,void **);
int hg_original_effect_native_free(HGOriginalLoader *,void *);

void hg_original_effects_cancel_pending(HGOriginalLoader *);
unsigned hg_original_effects_registered(HGOriginalLoader *);
int hg_original_effects_animation_probe(HGOriginalLoader *);
unsigned hg_original_effects_deferred(HGOriginalLoader *);

void *hg_original_effects_owner(HGOriginalLoader *);
unsigned hg_original_effects_uploads(HGOriginalLoader *);

int hg_original_effects_reject_alias_free(HGOriginalLoader *);

/* Original raw keys/prefix ownership, before or after actual VBlank/VWait work. */
unsigned hg_original_effects_raw_probe(HGOriginalLoader *,int);

unsigned hg_original_effects_pending_vwait(HGOriginalLoader *);

int hg_original_objects_graphics_new(HGOriginalLoader *,unsigned,unsigned,const int *,unsigned);
int hg_original_objects_graphics_free(HGOriginalLoader *);

int hg_original_objects_graphics_prepare(HGOriginalLoader *,unsigned);

int hg_original_avatar_features_start(HGOriginalLoader *);

void *hg_original_object_graphics_info(HGOriginalLoader *,uint32_t);
int hg_original_object_callback(HGOriginalLoader *,unsigned,uint32_t);
int hg_original_object_graphics_mode(HGOriginalLoader *,unsigned,unsigned);

int hg_original_comm_force_position(HGOriginalLoader *);
/* Existing RAM identity or a registered native heap allocation. Borrowed
 * views observe that owner's free/shrink; untracked stack/malloc lifetimes
 * reject. Numeric guest addresses carry no lifetime generation. */
int hg_original_bottom_screen_select(HGOriginalLoader *,void *,unsigned,unsigned *);
uint32_t hg_original_map_native(HGOriginalLoader *,const void *,size_t);
int hg_original_map_track_target(HGOriginalLoader *,void *,size_t);
uint32_t hg_original_object_work(HGOriginalLoader *,unsigned,void *,size_t);
int hg_original_followmon_pending(HGOriginalLoader *,void *,uint8_t *);

#ifdef HG_LOADER_TEST_HOOKS
void hg_original_graphics_fail_import(HGOriginalLoader *,unsigned);
unsigned hg_original_graphics_imports(HGOriginalLoader *);
unsigned hg_original_test_graphics_raw_objects(HGOriginalLoader *);
size_t hg_original_test_region_count(HGOriginalLoader *);
uint32_t hg_original_test_next(HGOriginalLoader *);
int hg_original_test_mapped(HGOriginalLoader *,uint32_t);
#endif

void *hg_original_model_attributes_new(HGOriginalLoader *);
int hg_original_model_attributes_configure(HGOriginalLoader *,void *);
int hg_original_model_attributes_free(HGOriginalLoader *,void **);
void *hg_original_fog_new(HGOriginalLoader *);
int hg_original_fog_free(HGOriginalLoader *,void **);

#include "ds_raster.h"

int hg_original_camera_new(HGOriginalLoader *,void *,unsigned,unsigned);
int hg_original_camera_delete(HGOriginalLoader *);
int hg_original_camera_update(HGOriginalLoader *);
int hg_original_camera_push(HGOriginalLoader *);
int hg_original_camera_retarget(HGOriginalLoader *,void *);
void *hg_original_camera_owner(HGOriginalLoader *);
void *hg_original_camera_transition(HGOriginalLoader *);
const DSCamera *hg_original_camera_render_state(HGOriginalLoader *);
unsigned hg_original_camera_depth(HGOriginalLoader *);

#ifdef HG_LOADER_TEST_HOOKS
void *hg_original_test_bytes(HGOriginalLoader *,uint32_t,size_t);
int hg_original_test_camera_angles(HGOriginalLoader *,int32_t,const uint16_t [4],unsigned);
#endif

unsigned hg_original_area_light_archive(HGOriginalLoader *,void *);
void *hg_original_light_new(HGOriginalLoader *,void *,unsigned);
int hg_original_light_free(HGOriginalLoader *,void **);
int hg_original_light_update(HGOriginalLoader *,void *);

#ifdef HG_LOADER_TEST_HOOKS
const void *hg_original_test_camera_template(HGOriginalLoader *,unsigned);
#endif

void *hg_original_weather_new(HGOriginalLoader *);
void *hg_original_weather_owner(HGOriginalLoader *);
int hg_original_weather_free(HGOriginalLoader *,void **);
int hg_original_weather_set(HGOriginalLoader *,unsigned);
#ifdef HG_LOADER_TEST_HOOKS
void hg_original_owner_fail_import(HGOriginalLoader *,unsigned);
unsigned hg_original_owner_imports(HGOriginalLoader *);
unsigned hg_original_test_main_tasks(HGOriginalLoader *);
#endif

#ifdef HG_LOADER_TEST_HOOKS
unsigned hg_original_test_persistent_graphics_vblank(HGOriginalLoader *);
#endif

#ifdef HG_LOADER_TEST_HOOKS
const void *hg_original_test_camera_preset(HGOriginalLoader *);
int hg_original_test_camera_preset_start(HGOriginalLoader *);
#endif

#ifdef HG_LOADER_TEST_HOOKS
int hg_original_test_file_import(HGOriginalLoader *,const char *,const uint32_t *,unsigned,uint32_t *);
int hg_original_test_sound_call(HGOriginalLoader *,const char *,const uint32_t *,unsigned,uint32_t *);
unsigned hg_original_test_arm7_irq(HGOriginalLoader *);
unsigned hg_original_test_sound_submission(HGOriginalLoader *,uint32_t out[6]);
size_t hg_original_test_image_size(void);
uint32_t hg_original_test_symbol(const char *);
uint32_t hg_original_test_scratch_address(void);
int hg_original_test_call(HGOriginalLoader *,const char *,const uint32_t *,unsigned,uint32_t *);
int hg_original_test_weather_task(HGOriginalLoader *);
#endif

void *hg_original_area_texture(HGOriginalLoader *,void *);
void *hg_original_field_texture_owner(HGOriginalLoader *);
void *hg_original_field_texture_new(HGOriginalLoader *);
int hg_original_field_texture_load(HGOriginalLoader *,void *,void *,unsigned *);
int hg_original_field_texture_clear(HGOriginalLoader *,void *);
int hg_original_field_texture_destroy(HGOriginalLoader *,void *);

/* Staged original sound software owners, not InitSoundData or device admission.
 * Only these scalar original entrypoints may cross this adapter. Unsupported
 * scene effects restore original static state and the bound audio preimage for retry.
 * Audio allocation/destruction remains with the caller. */
enum HGOriginalSoundPolicy {
    HG_SOUND_FIELD_BGM, HG_SOUND_FIELD_GUARD, HG_SOUND_FIELD_SCENE,
    HG_SOUND_STOP_SE, HG_SOUND_MOVE_VOLUME, HG_SOUND_PLAY_SE, HG_SOUND_ENVIRONMENT
};
int hg_original_sound_policy(HGOriginalLoader *,enum HGOriginalSoundPolicy,unsigned,uint32_t *);
int hg_original_sound_request(HGOriginalLoader *,enum HGOriginalSoundPolicy,const uint32_t *,unsigned,uint32_t *);

/* Original graphics-pool views expire with the object-graphics owner. */
void *hg_original_objects_pool(HGOriginalLoader *,const void *);
int hg_original_objects_pool_publish(HGOriginalLoader *,void *);

struct DSAudio;
/* Borrow the existing sound owner; it must outlive this binding. No audio owner
 * is allocated/destroyed by the loader. Destruction releases the lease. */
int hg_original_sound_bind(HGOriginalLoader *,struct DSAudio *);

void *hg_original_hblank_new(HGOriginalLoader *,unsigned);
int hg_original_hblank_start(HGOriginalLoader *,void *);
int hg_original_hblank_free(HGOriginalLoader *,void *);
int hg_original_hblank_dispatch(HGOriginalLoader *);
void *hg_original_poison_new(HGOriginalLoader *,unsigned,void *);
int hg_original_poison_free(HGOriginalLoader *,void *);
