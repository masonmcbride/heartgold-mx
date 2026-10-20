#pragma once
#include "compat/field_tasks.h"
typedef struct HGPhone HGPhone;
typedef struct { void *owner;int (*message)(void *,unsigned bank,unsigned message);int (*waiting)(void *);int (*acknowledged)(void *); } HGPhoneServices;
int hg_phone_admit(unsigned caller,unsigned kind,unsigned script);
HGPhone *hg_phone_new(FieldSystem *,HGPhoneServices);
void hg_phone_set(HGPhone *,unsigned caller,unsigned kind,unsigned script);
void *hg_phone_launch(HGPhone *);
void hg_phone_free(HGPhone *);
unsigned hg_phone_bank(const HGPhone *);
int hg_phone_failed(const HGPhone *);
const char *hg_phone_contact(const HGPhone *);
int hg_phone_ready(const HGPhone *);
