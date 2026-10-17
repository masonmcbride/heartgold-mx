#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { const uint8_t *bytes; size_t size; const uint8_t *fat; size_t count; const uint8_t *image; size_t image_size; } DSNarc;
bool ds_narc_open(DSNarc *archive, const uint8_t *bytes, size_t size);
bool ds_narc_member(const DSNarc *archive, size_t index, const uint8_t **bytes, size_t *size);
/* LZ10 only; caller supplies a bounded destination. Overlapping copies supported. */
bool ds_lz10(const uint8_t *src, size_t size, uint8_t *dst, size_t capacity, size_t *written);
