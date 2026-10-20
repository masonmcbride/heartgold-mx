#pragma once
#include <stddef.h>
typedef struct FieldSystem FieldSystem;
typedef struct HGStarter HGStarter;
typedef struct { unsigned species,level,hp;const char *name; } HGStarterOption;
HGStarter *hg_starter_begin(FieldSystem *,int preview);
void hg_starter_free(HGStarter *);
int hg_starter_tick(HGStarter *);
int hg_starter_busy(const HGStarter *);
int hg_starter_pending(const HGStarter *);
int hg_starter_option(HGStarter *,unsigned,HGStarterOption *);
int hg_starter_choose(HGStarter *,unsigned);
