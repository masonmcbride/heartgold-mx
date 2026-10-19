#pragma once
#include "compat/motion.h"
#include "follow_public.inc"
LocalMapObject *hg_follow_create(MapObjectManager *,unsigned,unsigned,unsigned,unsigned,int,int,int);
BOOL MapObject_IsMovementPaused(LocalMapObject *);
