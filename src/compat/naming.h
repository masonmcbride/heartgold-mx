#pragma once
#include "compat/field_tasks.h"
typedef struct HGNaming HGNaming;
HGNaming *hg_naming_new(FieldSystem *,unsigned kind,unsigned species,unsigned max,unsigned party,const uint16_t *initial,uint16_t *result,int preview);
void hg_naming_free(HGNaming *);
int hg_naming_tick(HGNaming *);
int hg_naming_pending(const HGNaming *);
unsigned hg_naming_kind(const HGNaming *);
const char *hg_naming_text(const HGNaming *);
int hg_naming_submit(HGNaming *,const char *,int cancel);
int hg_naming_edit(HGNaming *,const char *);
