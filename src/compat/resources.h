#pragma once
#include <stddef.h>
#include <stdint.h>
int native_resource_member(int archive,int group,int member,const uint8_t **bytes,size_t *size);

typedef struct NARC NARC;
/* Borrowed bounded sequential stream, used by original resource parsers. */
NARC *native_resource_stream(const void *,size_t);

int native_resource_file(const char *,const uint8_t **,size_t *);
