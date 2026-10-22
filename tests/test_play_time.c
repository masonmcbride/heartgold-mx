#include "compat/play_time.h"
#define CHECK(x) do { if(!(x))abort(); } while(0)
int main(void) {
    HGPlayTime a={0},b={0};IGT first={0},second={0},replacement;
    hg_play_time_update(&a,&first,10000000000ULL);
    hg_play_time_update(&a,&first,10999999999ULL);CHECK(first.seconds==0);
    hg_play_time_update(&a,&first,11000000000ULL);CHECK(first.seconds==1);
    replacement=first;
    hg_play_time_update(&a,&replacement,71500000000ULL);
    CHECK(first.seconds==1&&replacement.minutes==1&&replacement.seconds==1);
    hg_play_time_update(&a,&replacement,71999999999ULL);CHECK(replacement.seconds==1);
    hg_play_time_update(&a,&replacement,72000000000ULL);CHECK(replacement.seconds==2);
    hg_play_time_update(&b,&second,80000000000ULL);
    hg_play_time_update(&b,&second,81000000000ULL);CHECK(second.seconds==1);
    hg_play_time_update(&a,&replacement,73000000000ULL);CHECK(replacement.seconds==3);
    /* Continue starts a fresh elapsed baseline and retains saved play time. */
    HGPlayTime continued={0};hg_play_time_update(&continued,&replacement,900000000000ULL);
    CHECK(replacement.seconds==3);
    hg_play_time_update(&continued,&replacement,901000000000ULL);CHECK(replacement.seconds==4);
    replacement=(IGT){998,59,59};HGPlayTime cap={0};
    hg_play_time_update(&cap,&replacement,0);hg_play_time_update(&cap,&replacement,1000000000ULL);
    CHECK(replacement.hours==999&&replacement.minutes==59&&replacement.seconds==59);
    puts("Original elapsed play time: boundaries, owner replacement, sessions, Continue and cap pass");
}
