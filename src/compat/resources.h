#pragma once
#include <stddef.h>
#include <stdint.h>
int native_resource_member(int archive,int group,int member,const uint8_t **bytes,size_t *size);

typedef struct NARC NARC;
/* Borrowed bounded sequential stream, used by original resource parsers. */
NARC *native_resource_stream(const void *,size_t);

int native_resource_file(const char *,const uint8_t **,size_t *);
/* Existing packed-file allowlist; bridge-local IDs. Caller closes returned fd. */
int native_resource_file_id(const char *);
int native_resource_file_open(unsigned);

/* Original filesystem range semantics, bounded by the archive image. */
int native_resource_range(int archive,int group,int member,size_t offset,size_t length,const uint8_t **bytes);
/* Native PNG container; deliberately distinct from the original DS archive. */
#define HG_RESOURCE_BATTLE_PNG (-2)

/* Original encrypted bank archive; never the preview UTF-8 member containers. */
#define HG_RESOURCE_ORIGINAL_MESSAGES (-3)
