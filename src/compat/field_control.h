#pragma once
#include "compat/motion.h"
/* Original metatile selector. 1 queried (65535 means no script), 0 invalid or
 * unsupported service. Output is unchanged on failure; no live state mutates. */
int hg_field_metatile_script(FieldSystem *,unsigned map,unsigned *script);
