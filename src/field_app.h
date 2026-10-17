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
