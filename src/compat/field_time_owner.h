#pragma once
/* Borrowed canonical owners; this interface imports no scoped FieldSystem. */
#include "compat/native.h"
typedef struct GearPhoneRingManager GearPhoneRingManager;
typedef struct BugContest BugContest;
int hg_field_time_update_live(SaveData *,GearPhoneRingManager *,BugContest *,char *,size_t);
