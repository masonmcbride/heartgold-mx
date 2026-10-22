#include "compat/play_time.h"
/* Original controller executes against a selected session. Timer units at this
 * boundary are nanoseconds; its conversion still receives elapsed ticks. */
static u64 play_time_now;
static u64 sub_020254FC(void) { return play_time_now; }
static u64 sub_02025504(u64 ticks) { return ticks / 1000000000ULL; }
#include "save_play_time.inc"
void hg_play_time_update(HGPlayTime *state, IGT *igt, uint64_t now) {
    play_time_now=now;
    s_021D10E8=state->controller;
    if (!s_021D10E8.unk0 || now<s_021D10E8.unk18) sub_0201838C(igt);
    s_021D10E8.igt=igt;
    sub_020183B0();
    state->controller=s_021D10E8;
    state->controller.igt=NULL;
    s_021D10E8.igt=NULL;
}
