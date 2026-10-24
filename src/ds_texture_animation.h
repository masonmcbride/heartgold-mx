#pragma once
#include "ds_texture.h"
/* Borrowed NSBTP pattern and NSBTA/SRT0 material tracks. Unsupported records reject. */
typedef struct { const uint8_t *bytes;size_t size;unsigned frames,materials,textures,palettes,table,names,texture_names,palette_names,kind; } DSTextureAnimation;
int ds_texture_animation_open(const uint8_t *,size_t,DSTextureAnimation *);
int ds_texture_animation_frame(const DSTextureAnimation *,unsigned frame,const char *material,char texture[17],char palette[17]);

/* Borrowed SRT0 records share the texture-animation view and original resource lifetime.
 * Matrix maps pixel-space UVs; a missing material returns 0, invalid input -1. */
int ds_texture_animation_matrix(const DSTextureAnimation *,double frame,const char *material,unsigned width,unsigned height,double matrix[6]);

/* Allocation/initialization view: validates container/dictionaries and binding
 * metadata only. Track evaluation remains a separate, fallible operation. */
enum { DS_ANIMATION_JOINT=1,DS_ANIMATION_PATTERN,DS_ANIMATION_SRT,DS_ANIMATION_COLOR };
typedef struct {
    const uint8_t *bytes;size_t size;const uint8_t *record;size_t record_size;
    unsigned kind,frames,nodes,materials,names,table,stride;
} DSAnimationResource;
int ds_animation_resource_open(const uint8_t *,size_t,unsigned,DSAnimationResource *);
