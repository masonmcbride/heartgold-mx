#pragma once
#include "ds_raster.h"
typedef struct HGFieldView HGFieldView;
HGFieldView *hg_field_view_new(unsigned map);
/* Failure text is bounded and suitable for a catalog row or status label. */
HGFieldView *hg_field_view_load(unsigned map,char *error,size_t error_size);
unsigned hg_field_map_count(void);
const char *hg_field_map_name(unsigned map);
void hg_field_view_free(HGFieldView *);
void hg_field_view_camera(const HGFieldView *,DSCamera *,unsigned width,unsigned height);
int hg_field_view_render(const HGFieldView *,const DSCamera *,uint8_t *,unsigned,unsigned);
size_t hg_field_view_triangles(const HGFieldView *);

unsigned hg_character_count(void);
const char *hg_character_name(unsigned);
HGFieldView *hg_character_view_load(unsigned,char *,size_t);

/* Native billboard presentation using original hero texture and shared depth. */
HGFieldView *hg_player_sprite_load(char *,size_t);
int hg_field_view_render_player(const HGFieldView *,const HGFieldView *,const double world[3],const DSCamera *,uint8_t *,unsigned,unsigned);
void hg_field_view_focus(const HGFieldView *,const double world[3],DSCamera *,unsigned,unsigned);

int hg_player_sprite_frame(HGFieldView *,unsigned frame);
