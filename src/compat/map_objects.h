#pragma once
#include "compat/motion.h"
/* Native static-actor initialization. Runs the original event initializer and
 * facing/flag setup, without launching the DS sprite/movement callbacks. The
 * caller supplies an inactive slot. Conditional flags and dynamic graphics
 * require the bound story owner; otherwise they fail. Failure preserves slot. */
int hg_map_object_init_event(LocalMapObject *,MapObjectManager *,unsigned map,const ObjectEvent *);

void hg_map_object_set_map(LocalMapObject *,unsigned);

void hg_map_object_set_movement(LocalMapObject *,unsigned);
