#include "compat/save_c_records.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while(0)
/* These are the newly connected unchanged upstream registry callbacks. */
u32 Save_PlayerData_sizeof(void);
void Save_PlayerData_Init(PLAYERDATA *);
u32 Save_SysInfo_sizeof(void);
void Save_SysInfo_Init(SysInfo *);
int main(void) {
    /* Independent epoch dates expose leap and month boundary errors in the host
     * clock service; these constants are not calculated by the implementation. */
    RTCDate epoch={0,1,1,6},leap={0,2,29,2},march={0,3,1,3};
    RTCDate nonleap={1,3,1,4},century_end={99,12,31,4};
    RTCTime midnight={0,0,0},last_second={23,59,59};
    CHECK(RTC_ConvertDateToDay(&epoch)==0);
    CHECK(RTC_ConvertDateToDay(&leap)==59);
    CHECK(RTC_ConvertDateToDay(&march)==60);
    CHECK(RTC_ConvertDateToDay(&nonleap)==425);
    CHECK(RTC_ConvertDateToDay(&century_end)==36524);
    CHECK(RTC_ConvertDateTimeToSecond(&epoch,&midnight)==0);
    CHECK(RTC_ConvertDateTimeToSecond(&century_end,&last_second)==3155759999LL);

    PLAYERDATA player;memset(&player,0xa5,sizeof(player));
    CHECK(Save_PlayerData_sizeof()==sizeof(player));Save_PlayerData_Init(&player);
    CHECK(player.coins==0&&player.igt.hours==0&&player.igt.minutes==0&&player.igt.seconds==0);
    CHECK(player.profile.language==GAME_LANGUAGE&&player.profile.version==GAME_VERSION);
    CHECK(player.profile.money==0);

    SysInfo info;memset(&info,0xa5,sizeof(info));
    CHECK(Save_SysInfo_sizeof()==sizeof(info));Save_SysInfo_Init(&info);
    CHECK(info.rtc_info.initialized);
    CHECK(info.rtc_info.date.year<100&&info.rtc_info.date.month>=1&&info.rtc_info.date.month<=12);
    CHECK(info.rtc_info.days_since_nitro_epoch==RTC_ConvertDateToDay(&info.rtc_info.date));
    CHECK(info.rtc_info.seconds_since_nitro_epoch==RTC_ConvertDateTimeToSecond(&info.rtc_info.date,&info.rtc_info.time));
    CHECK(info.rtc_info.seconds_at_game_clear==0&&info.rtc_info.penaltyInMinutes==0);
    CHECK(info.dwcProfileId==0&&info.mysteryGiftActive==0);

    PCStorage *storage=malloc(sizeof(*storage));CHECK(storage);
    memset(storage,0xa5,sizeof(*storage));PCStorage_Init(storage);
    CHECK(PCStorage_sizeof()==sizeof(*storage)&&storage->curBox==0&&storage->unlockedWallpapers==0);
    for(unsigned box=0;box<NUM_BOXES;box++) {
        CHECK(storage->wallpapers[box]==box%DEFAULT_WALLPAPER_MAX);
        CHECK(storage->box_names[box][0]!=0&&storage->box_names[box][0]!=EOS);
        int terminated=0;for(unsigned i=0;i<20;i++)terminated|=storage->box_names[box][i]==EOS;
        CHECK(terminated);
        if(box)CHECK(memcmp(storage->box_names[box],storage->box_names[box-1],sizeof(storage->box_names[box])));
        for(unsigned slot=0;slot<MONS_PER_BOX;slot++)
            CHECK(GetBoxMonData(&storage->boxes[box].mons[slot],MON_DATA_SPECIES,NULL)==SPECIES_NONE);
    }
    free(storage);
    puts("Original player/RTC and all 540 PC slots initialize; Nitro calendar boundary dates pass");
    return 0;
}
