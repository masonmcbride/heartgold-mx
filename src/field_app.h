#pragma once
#include "field_view.h"
#include "battle_runtime.h"
#include "field_script.h"
/* Native scene application, driven by the original OverlayManager. Its
 * callbacks are host scene callbacks, not the original FieldMap overlay. */
typedef struct HGFieldApp HGFieldApp;
HGFieldApp *hg_field_app_new(unsigned map,char *error,size_t error_size);
HGFieldApp *hg_field_app_new_game(char *error,size_t error_size);
void hg_field_app_free(HGFieldApp *);
const HGFieldView *hg_field_app_view(const HGFieldApp *);
int hg_field_app_render(HGFieldApp *,const DSCamera *,uint8_t *,unsigned,unsigned);

/* Simulation dispatch is separate from camera/presentation. */
void hg_field_app_step(HGFieldApp *);
unsigned long long hg_field_app_ticks(const HGFieldApp *);

void hg_field_app_input(HGFieldApp *,uint16_t held_keys);
int hg_field_app_player(const HGFieldApp *,int *x,int *z);
void hg_field_app_camera(const HGFieldApp *,DSCamera *,unsigned,unsigned);

unsigned long long hg_field_app_revision(const HGFieldApp *);
int hg_field_app_presentation(const HGFieldApp *,double world[3],unsigned *frame);

void hg_field_app_interact(HGFieldApp *);
const char *hg_field_app_dialogue(const HGFieldApp *);
/* Current native page; dialogue() retains the complete original UTF-8 text. */
const char *hg_field_app_dialogue_page(const HGFieldApp *);
/* Includes message pages and scripted movement between messages. */
int hg_field_app_interaction_busy(const HGFieldApp *);

/* Wall-clock-driven simulation, bounded catch-up; first call primes the clock. */
unsigned hg_field_app_advance(HGFieldApp *,uint64_t monotonic_ns);
/* After an external suspension, discard elapsed wall time and held input. */
void hg_field_app_resume(HGFieldApp *);

/* Cancel held and latched input on focus loss/map ownership changes. */
void hg_field_app_clear_input(HGFieldApp *);
/* Original in-memory step statistic; no disk save is loaded or written. */
unsigned hg_field_app_steps(const HGFieldApp *);
unsigned hg_field_app_map(const HGFieldApp *);
/* Door/stair travel is a multi-frame fade; the destination map is installed
 * before the fade-in completes, so callers that must not act mid-animation wait
 * on this rather than on the map identity alone. */
int hg_field_app_transition_active(const HGFieldApp *);
/* Borrowed session owner, retained through native door/stair travel. */
HGStoryState *hg_field_app_story(HGFieldApp *);

int hg_field_app_starter_pending(const HGFieldApp *);
int hg_field_app_menu_pending(const HGFieldApp *);
int hg_field_app_menu_choice_supported(HGFieldApp *,unsigned);

int hg_field_app_follower(const HGFieldApp *,unsigned *,int *,int *);
const char *hg_field_app_error(const HGFieldApp *);

int hg_field_app_running_shoes(const HGFieldApp *);

HGFieldApp *hg_field_app_new_explore(unsigned map,char *error,size_t error_size);
int hg_field_app_is_exploring(const HGFieldApp *);

/* Current original master-brightness level, including the live script scene. */
int hg_field_app_brightness(const HGFieldApp *);

int hg_field_app_phone_active(const HGFieldApp *);

/* Output is opt-in; headless simulation and script previews remain silent. */
int hg_field_app_audio_enable(HGFieldApp *,int enabled,char *error,size_t size);

uint64_t hg_field_app_audio_frames(HGFieldApp *);

int hg_field_app_battle_active(const HGFieldApp *);
int hg_field_app_battle_state(const HGFieldApp *);

int hg_field_app_battle_mon(const HGFieldApp *,unsigned,HGBattleMonView *);
int hg_field_app_battle_choose(HGFieldApp *,unsigned);

/* Read-only live field panels: 0 close, 1 Bag, 2 Pokemon. Repeating toggles. */
int hg_field_app_inspect(HGFieldApp *,int kind);
int hg_field_app_inspect_kind(const HGFieldApp *);
void hg_field_app_inspect_text(HGFieldApp *,char *,size_t);

int hg_field_app_battle_trainer(const HGFieldApp *);

/* Curated original-route checkpoints; writes require an idle, stable session. */
int hg_field_app_checkpoint_write(HGFieldApp *,const char *path,unsigned stage,char *,size_t);
HGFieldApp *hg_field_app_checkpoint_load(const char *path,char *,size_t);

int hg_field_app_name_pending(const HGFieldApp *);
const char *hg_field_app_name_text(const HGFieldApp *);
int hg_field_app_name_edit(HGFieldApp *,const char *);
int hg_field_app_name_submit(HGFieldApp *,const char *,int cancel);

int hg_field_app_battle_tutorial(const HGFieldApp *);
unsigned hg_field_app_capture_phase(const HGFieldApp *);

/* Original cartridge save manager, distinct from development checkpoints. */
int hg_field_app_save(HGFieldApp *,const char *path,char *,size_t);
HGFieldApp *hg_field_app_continue(const char *path,char *,size_t);
const char *hg_field_app_save_path(const HGFieldApp *);

int hg_field_app_save_unlocked(HGFieldApp *);

unsigned hg_field_app_battle_command_count(const HGFieldApp *);

struct HGShop *hg_field_app_shop(HGFieldApp *);
void hg_field_app_shop_cancel(HGFieldApp *);
unsigned hg_field_app_menu_count(const HGFieldApp *);

int hg_field_app_battle_cancel(HGFieldApp *);
