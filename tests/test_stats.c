#include "compat/stats.h"
#include "compat/scheduler.h"
#include "math_util.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1;} } while(0)
int main(void) {
    CHECK(hg_scheduler_init());for(unsigned i=0;i<7;i++)CHECK(hg_scheduler_step());CHECK(OS_GetVBlankCount()==7);
    unsigned size=GameStats_sizeof();CHECK(size==448);GameStats *stats=malloc(size);uint8_t *saved=malloc(size);CHECK(stats&&saved);
    GameStats_Init(stats);memcpy(saved,stats,size);
    for(int i=0;i<NUM_GAME_STATS;i++)CHECK(GameStats_GetCapped(stats,i)==0);
    CHECK(!memcmp(saved,stats,size));
    CHECK(GameStats_Inc(stats,0)==1 && GameStats_Add(stats,0,9)==10);
    CHECK(GameStats_SetCapped(stats,0,UINT32_MAX)==999999999 && GameStats_Inc(stats,0)==999999999);
    CHECK(GameStats_SetCapped(stats,4,UINT32_MAX)==999999 && GameStats_Inc(stats,4)==999999);
    CHECK(GameStats_SetCapped(stats,NUM_GAME_STATS_WORD,UINT32_MAX)==65535);
    CHECK(GameStats_SetCapped(stats,NUM_GAME_STATS-1,UINT32_MAX)==9999);
    CHECK(GameStats_SetCapped(stats,GAME_STAT_SCORE,12345)==12345);
    CHECK(GameStats_UpdateBounded(stats,GAME_STAT_SCORE,12)==12345);
    CHECK(GameStats_UpdateBounded(stats,GAME_STAT_SCORE,23456)==23456);
    CHECK(GameStats_GetScore(stats)==23456 && GameStats_AddScore(stats,0)==23457);
    memcpy(saved,stats,size);CHECK(GameStats_GetScore(stats)==23457);CHECK(!memcmp(saved,stats,size));
    uint16_t clear[]={0,0x1234,0xffff,0xabcd},cipher[4];memcpy(cipher,clear,sizeof(cipher));
    _MonEncryptSegment(cipher,sizeof(cipher),0x12345678);
    const uint16_t expected[]={0x0b71,0x96de,0x2675,0x5f2d};CHECK(!memcmp(cipher,expected,sizeof(cipher)));
    _MonDecryptSegment(cipher,sizeof(cipher),0x12345678);CHECK(!memcmp(clear,cipher,sizeof(clear)));
    const uint8_t bytes[]={0,1,2,255};CHECK(Math_CalcArraySum(bytes,sizeof(bytes))==258);
    SetMTRNGSeed(5489);const uint32_t mt[]={3499211612u,581869302u,3890346734u,3586334585u,545404204u};
    for(unsigned i=0;i<sizeof(mt)/sizeof(*mt);i++)CHECK(MTRandom()==mt[i]);
    SetLCRNGSeed(0);CHECK(LCRandom()==0 && GetLCRNGSeed()==24691);
    CHECK(PRandom(0)==1 && PRandom(1)==1812433254u);
    CHECK(hg_scheduler_shutdown());free(saved);free(stats);
    puts("Original game stats: initialization, encrypted secure region, caps/updates/score, native frame seed; original encryption and RNG vectors passed");return 0;
}
