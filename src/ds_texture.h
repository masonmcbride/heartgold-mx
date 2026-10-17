#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct { const uint8_t *bytes;size_t size;unsigned textures,palettes; } DSTextureBank;
typedef struct {
    const uint8_t *texels,*palette;
    size_t texel_size,palette_size;
    unsigned width,height,format,transparent_zero;
} DSTexture;
int ds_texture_bank(const uint8_t *,size_t,DSTextureBank *);
int ds_texture_find(const DSTextureBank *,const char *,const char *,DSTexture *);
/* Nearest sample with DS clamp/repeat/mirror bits from material TEXIMAGE_PARAM.
 * RGBA packed low-to-high; coordinates in texels. Unsupported format 5 fails. */
int ds_texture_sample(const DSTexture *,double,double,uint32_t params,uint32_t *rgba);
