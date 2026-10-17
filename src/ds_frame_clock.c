#include "ds_frame_clock.h"
unsigned ds_frame_clock_advance(DSFrameClock *clock,uint64_t now) {
    if(!clock)return 0;
    if(!clock->primed){clock->previous_ns=now;clock->primed=1;return 0;}
    if(now<clock->previous_ns)return 0;
    uint64_t delta=now-clock->previous_ns;clock->previous_ns=now;
    if(delta>250000000)delta=250000000;
    clock->phase+=delta*60;
    unsigned ticks=(unsigned)(clock->phase/1000000000);
    clock->phase%=1000000000;
    return ticks>8?8:ticks;
}
