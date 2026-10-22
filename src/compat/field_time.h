#pragma once
#include "compat/save_c_records.h"
#include "compat/field_tasks.h"
typedef struct HGFieldTime HGFieldTime;
typedef struct HGTimeSample {
    HGSaveRTC rtc;
    u8 identity[6];
    s64 rtc_offset;
} HGTimeSample;
/* The host supplies one atomic wall-clock/identity sample. State is session-local. */
enum { HG_FIELD_TIME_FAILED=0,HG_FIELD_TIME_UNCHANGED=1,HG_FIELD_TIME_CHANGED=2 };
HGFieldTime *hg_field_time_new(void);
void hg_field_time_free(HGFieldTime *);
int hg_field_time_sample(HGTimeSample *,char *error,size_t size);
void hg_field_time_claim(SaveData *,const HGTimeSample *);
int hg_field_time_continue(HGFieldTime *,SaveData *,const HGTimeSample *,char *error,size_t size);
int hg_field_time_update(HGFieldTime *,SaveData *,const HGTimeSample *,char *error,size_t size);

/* A live contest is volatile; time mutates its owner in the same transaction. */
void hg_field_time_attach_contest(HGFieldTime *,struct BugContest *);

typedef struct HGIncomingCall { unsigned caller,kind,script,trigger,forced; } HGIncomingCall;
int hg_field_time_poll(HGFieldTime *,SaveData *,const HGTimeSample *,unsigned map,char *,size_t);
int hg_field_time_pending(const HGFieldTime *,HGIncomingCall *);
void hg_field_time_answered(HGFieldTime *);
struct DSAudio;
int hg_field_time_ring_tick(HGFieldTime *,struct DSAudio *);

int hg_field_time_pc_full(HGFieldTime *,SaveData *,const HGTimeSample *,char *,size_t);

/* Original ordinary encounter completion notifications (Bill/Oak). */
int hg_field_time_after_battle(HGFieldTime *,SaveData *,const HGTimeSample *,char *,size_t);

unsigned hg_field_time_of_day(FieldSystem *);

void hg_field_time_set_call_trigger(SaveData *,unsigned trigger);
