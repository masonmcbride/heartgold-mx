#pragma once
#include "compat/field_tasks.h"
typedef struct HGPhone HGPhone;
typedef struct { void *owner;int (*message)(void *,unsigned bank,unsigned message);int (*waiting)(void *);int (*acknowledged)(void *);int (*buffer_safari_area)(void *,unsigned slot,unsigned area);int (*buffer_message)(void *,unsigned slot,unsigned bank,unsigned message);void (*menu_begin)(void *,u32 *);int (*menu_waiting)(void *);int (*buffer_integer)(void *,unsigned slot,unsigned value); } HGPhoneServices;
int hg_phone_admit(unsigned caller,unsigned kind,unsigned script);
HGPhone *hg_phone_new(FieldSystem *,HGPhoneServices);
void hg_phone_set(HGPhone *,unsigned caller,unsigned kind,unsigned script);
void *hg_phone_launch(HGPhone *);
void hg_phone_free(HGPhone *);
unsigned hg_phone_bank(const HGPhone *);
int hg_phone_failed(const HGPhone *);
const char *hg_phone_contact(const HGPhone *);
int hg_phone_ready(const HGPhone *);

struct PhoneBookEntry;
const struct PhoneBookEntry *hg_phone_entries(unsigned *count);
int hg_phone_set_incoming(HGPhone *,unsigned caller,unsigned kind,unsigned script,unsigned trigger);

int hg_phone_menu_active(const HGPhone *);
