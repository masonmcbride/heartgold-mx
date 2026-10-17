#pragma once
#include "ds_raster.h"
typedef struct HGFieldView HGFieldView;
HGFieldView *hg_field_view_new(unsigned map);
void hg_field_view_free(HGFieldView *);
void hg_field_view_camera(const HGFieldView *,DSCamera *,unsigned width,unsigned height);
int hg_field_view_render(const HGFieldView *,const DSCamera *,uint8_t *,unsigned,unsigned);
size_t hg_field_view_triangles(const HGFieldView *);
