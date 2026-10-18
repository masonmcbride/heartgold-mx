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
/* Original type-0/type-7 table resolves event IDs to model members. Other
 * templates and unresolved variables fail. direction is original N/S/W/E. */
HGFieldView *hg_character_sprite_load(unsigned sprite,unsigned direction,char *,size_t);

/* Native billboard presentation using original hero texture and shared depth. */
HGFieldView *hg_player_sprite_load(char *,size_t);
int hg_field_view_render_player(const HGFieldView *,const HGFieldView *,const double world[3],const DSCamera *,uint8_t *,unsigned,unsigned);
typedef struct { const HGFieldView *sprite;double world[3]; } HGFieldBillboard;
/* Shared depth for terrain, props and up to 64 native actor billboards. */
int hg_field_view_render_actors(const HGFieldView *,const HGFieldBillboard *,size_t,const DSCamera *,uint8_t *,unsigned,unsigned);
void hg_field_view_focus(const HGFieldView *,const double world[3],DSCamera *,unsigned,unsigned);

int hg_player_sprite_frame(HGFieldView *,unsigned frame);
