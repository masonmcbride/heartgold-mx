#pragma once
#include "compat/pokemon.h"
#include "unk_02018380.h"
/* State belongs to a session. No persistent pointer to a replaceable story. */
typedef struct HGPlayTime { UnkStruct_021D10E8 controller; } HGPlayTime;
void hg_play_time_update(HGPlayTime *state, IGT *igt, uint64_t monotonic_ns);
