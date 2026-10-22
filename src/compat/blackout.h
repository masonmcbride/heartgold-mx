#pragma once
#include "compat/save_c_records.h"
typedef struct HGBlackout HGBlackout;
struct DSAudio;
enum {HG_BLACKOUT_FAILED=-1,HG_BLACKOUT_RUNNING,HG_BLACKOUT_MESSAGE,HG_BLACKOUT_DONE};
/* Original task against a private complete save. Field reconstruction and the
 * requested original standard script must succeed before publication. */
HGBlackout *hg_blackout_new(const SaveData *,struct DSAudio *,char *error,size_t);
void hg_blackout_free(HGBlackout *);
int hg_blackout_tick(HGBlackout *);
int hg_blackout_ack(HGBlackout *);
const SaveData *hg_blackout_save(const HGBlackout *);
unsigned hg_blackout_script(const HGBlackout *);
const char *hg_blackout_message(const HGBlackout *);
const char *hg_blackout_error(const HGBlackout *);
int hg_blackout_brightness(const HGBlackout *);

int hg_blackout_waiting(const HGBlackout *);
