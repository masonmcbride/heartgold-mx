#include "compat/stats.h"
#include "compat/scheduler.h"
void MI_CpuClear32(void *p,u32 n) { GF_ASSERT(!(n&3));memset(p,0,n); }
/* Native system-task frames, not a DS display interrupt counter. */
u32 OS_GetVBlankCount(void) { return (u32)hg_scheduler_ticks(); }
