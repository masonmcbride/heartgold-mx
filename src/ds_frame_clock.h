#pragma once
#include <stdint.h>
/* Host 60 Hz scheduling, independent of presentation; not DS hardware timing.
 * Zero-initialize to reset. Catch-up is bounded to eight ticks per poll. */
typedef struct { uint64_t previous_ns,phase;int primed; } DSFrameClock;
unsigned ds_frame_clock_advance(DSFrameClock *,uint64_t monotonic_ns);
