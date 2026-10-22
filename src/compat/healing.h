#pragma once
#include "compat/script.h"
#include "compat/prop_animation.h"
#include "field_view.h"
typedef struct HGHealing HGHealing;
HGHealing *hg_healing_new(FieldSystem *,unsigned count);
HGHealing *hg_healing_clone(const HGHealing *);
void hg_healing_free(HGHealing *);
int hg_healing_tick(HGHealing *,FieldSystem *);
int hg_healing_busy(const HGHealing *);
const HGFieldView *hg_healing_view(HGHealing *,HGPropAnimations *);
int hg_healing_present(HGHealing *,HGPropAnimations *,HGFieldView *);

u16 hg_healing_party_lead(SaveData *);
