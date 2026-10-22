#pragma once
#include <stddef.h>
#include <stdint.h>
/* Original glyph IDs, terminated with EOS. Reject unsupported/control text. */
int hg_name_encode(const char *,uint16_t *,unsigned max_chars);
int hg_name_decode(const uint16_t *,unsigned,char *,size_t);
