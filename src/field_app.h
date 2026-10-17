#pragma once
#include "field_view.h"
/* Native scene application, driven by the original OverlayManager. Its
 * callbacks are host scene callbacks, not the original FieldMap overlay. */
typedef struct HGFieldApp HGFieldApp;
HGFieldApp *hg_field_app_new(unsigned map,char *error,size_t error_size);
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

/* Wall-clock-driven simulation, bounded catch-up; first call primes the clock. */
unsigned hg_field_app_advance(HGFieldApp *,uint64_t monotonic_ns);

/* Cancel held and latched input on focus loss/map ownership changes. */
void hg_field_app_clear_input(HGFieldApp *);
/* Original in-memory step statistic; no disk save is loaded or written. */
unsigned hg_field_app_steps(const HGFieldApp *);
unsigned hg_field_app_map(const HGFieldApp *);
