#include "compat/save_c_records.h"
#include <time.h>
#include "save_box_names.inc"
/* PC initialization consumes original encoded box names, rather than the
 * inventory UTF-8 String ABI. These handles belong only to this boundary. */
static MsgData *save_box_messages(int mode,int archive,int bank,enum HeapID heap) {
    (void)heap;
    GF_ASSERT(mode==MSGDATA_LOAD_LAZY&&archive==NARC_msgdata_msg&&bank==24);
    return (MsgData *)save_box_names;
}
static void save_box_message_read(MsgData *messages,u32 index,u16 *dest) {
    GF_ASSERT(messages==(MsgData *)save_box_names&&index>=6&&index<24);
    const u16 *name=save_box_names[index-6];
    do { *dest++=*name; } while(*name++!=EOS);
}
static void save_box_messages_destroy(MsgData *messages) {
    GF_ASSERT(messages==(MsgData *)save_box_names);
}
#define NewMsgDataFromNarc save_box_messages
#define ReadMsgDataIntoU16Array save_box_message_read
#define DestroyMsgData save_box_messages_destroy
#include "save_c_records_decls.inc"
#include "save_c_records_kernel.inc"
#undef NewMsgDataFromNarc
#undef ReadMsgDataIntoU16Array
#undef DestroyMsgData

/* Nitro calendar uses local wall time and an epoch of 2000-01-01. The
 * serialized date stores year 0..99 and weekday Sunday=0. */
static const HGSaveRTC *rtc_sample;
const HGSaveRTC *hg_save_rtc_exchange(const HGSaveRTC *sample) {
    const HGSaveRTC *previous=rtc_sample;rtc_sample=sample;return previous;
}
void GF_RTC_CopyDateTime(RTCDate *date,RTCTime *clock) {
    if(rtc_sample){if(date)*date=rtc_sample->date;if(clock)*clock=rtc_sample->time;return;}
    time_t now=time(NULL);struct tm local;
    GF_ASSERT(now!=(time_t)-1&&localtime_r(&now,&local));
    GF_ASSERT(local.tm_year>=100&&local.tm_year<200);
    if(date)*date=(RTCDate){local.tm_year-100,local.tm_mon+1,local.tm_mday,local.tm_wday};
    if(clock)*clock=(RTCTime){local.tm_hour,local.tm_min,local.tm_sec};
}
s32 RTC_ConvertDateToDay(const RTCDate *date) {
    static const unsigned before_month[]={0,31,59,90,120,151,181,212,243,273,304,334};
    static const unsigned month_days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    GF_ASSERT(date&&date->year<100&&date->month>=1&&date->month<=12);
    unsigned leap=date->year%4==0;
    GF_ASSERT(date->day>=1&&date->day<=month_days[date->month-1]+(date->month==2&&leap));
    return 365*date->year+(date->year+3)/4+before_month[date->month-1]+(date->month>2&&leap)+date->day-1;
}
s64 RTC_ConvertDateTimeToSecond(const RTCDate *date,const RTCTime *clock) {
    GF_ASSERT(clock&&clock->hour<24&&clock->minute<60&&clock->second<60);
    return (s64)RTC_ConvertDateToDay(date)*86400+clock->hour*3600+clock->minute*60+clock->second;
}

#include "ds_account.h"
/* The original accessor is a single bx lr: DWC bytes begin at WiFiList+0. */
DWCUserData *sub_0202C08C(WiFiList *list) { return (DWCUserData *)list; }
BOOL DWC_CheckUserData(const DWCUserData *data) {
    return ds_account_valid((const DSAccountData *)data);
}
void DWC_CreateUserData(DWCUserData *data,u32 game) {
    u64 identity;u32 player;
    arc4random_buf(&identity,sizeof(identity));arc4random_buf(&player,sizeof(player));
    ds_account_create((DSAccountData *)data,game,identity,player);
}
void DWC_ClearDirtyFlag(DWCUserData *data) {
    ds_account_clear_dirty((DSAccountData *)data);
}
