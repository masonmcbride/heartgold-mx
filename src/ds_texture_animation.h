#pragma once
#include "ds_texture.h"
/* Borrowed NSBTP texture/palette keyframes. Unsupported records fail admission. */
typedef struct { const uint8_t *bytes;size_t size;unsigned frames,materials,textures,palettes,table,names,texture_names,palette_names; } DSTextureAnimation;
int ds_texture_animation_open(const uint8_t *,size_t,DSTextureAnimation *);
int ds_texture_animation_frame(const DSTextureAnimation *,unsigned frame,const char *material,char texture[17],char palette[17]);
