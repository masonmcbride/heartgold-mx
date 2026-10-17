#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Supported slice: uncompressed 32x32, tiled 4bpp item NCGR + BGR555 NCLR. */
bool ds_decode_icon(const uint8_t *ncgr, size_t ncgr_size, const uint8_t *nclr, size_t nclr_size, uint8_t rgba[32*32*4]);
