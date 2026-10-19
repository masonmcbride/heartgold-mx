#pragma once
#include <stddef.h>
#include <stdint.h>
/* DS master brightness [-16,16], native RGBA8 surface. Alpha is preserved. */
int ds_brightness_apply(uint8_t *,size_t pixel_count,int level);
