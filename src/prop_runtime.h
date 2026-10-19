#pragma once
/* This internal boundary is C-only and requires compat/props.h. */
Field3dRenderObjManager *hg_prop_resources_new(void);
void hg_prop_resources_free(Field3dRenderObjManager *);
int hg_prop_resources_load(Field3dRenderObjManager *,unsigned,const uint8_t *,size_t);
unsigned hg_prop_resources_live(const Field3dRenderObjManager *);
int hg_props_validate(const uint8_t *,size_t);
void hg_prop_trace_begin(void);
void hg_prop_trace_mark(const char *,unsigned live);
int hg_prop_trace_write(FILE *);

#include "ds_transform.h"
typedef int (*HGPropDraw)(void *,unsigned,const uint8_t *,size_t,const DSTransform *,int material,int shape);
AreaDataManager *hg_prop_area_new(const uint8_t *,size_t);
void hg_prop_area_free(AreaDataManager *);
int hg_prop_area_has_model(const AreaDataManager *,unsigned);
void hg_prop_draw_begin(HGPropDraw,void *);
int hg_prop_draw_end(unsigned *submissions);

typedef struct HGFieldProps HGFieldProps;
MapPropManager *hg_field_props_manager(HGFieldProps *);
