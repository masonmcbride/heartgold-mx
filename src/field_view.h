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
typedef struct { const HGFieldView *sprite;double world[3];int fixed; } HGFieldBillboard;
/* Shared depth for terrain, props and up to 64 native actor billboards plus one fixed prop scene. */
int hg_field_view_render_actors(const HGFieldView *,const HGFieldBillboard *,size_t,const DSCamera *,uint8_t *,unsigned,unsigned);
void hg_field_view_focus(const HGFieldView *,const double world[3],DSCamera *,unsigned,unsigned);

int hg_player_sprite_frame(HGFieldView *,unsigned frame);

/* Native owner of script-created original MapProps (separate from land props).
 * Snapshots contain world-space geometry for fixed shared-depth composition. */
typedef struct HGFieldProps HGFieldProps;
HGFieldProps *hg_field_props_new(unsigned map);
HGFieldProps *hg_field_props_clone(const HGFieldProps *);
void hg_field_props_free(HGFieldProps *);
int hg_field_props_prepare(HGFieldProps *,unsigned model,unsigned slots);
unsigned hg_field_props_count(const HGFieldProps *);
HGFieldView *hg_field_props_view(HGFieldProps *);

int hg_field_view_project(const HGFieldView *,const double world[3],const DSCamera *,unsigned,unsigned,double screen[3]);

#include "ds_joint_animation.h"
int hg_field_view_prop_poses(HGFieldView *,const HGPropPose *,unsigned);
