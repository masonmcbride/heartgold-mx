#pragma once
#include "field_app.h"
/* Optional sandbox parties over gameplay-produced event states. */
unsigned hg_story_checkpoint_count(void);
const char *hg_story_checkpoint_title(unsigned);
HGFieldApp *hg_story_checkpoint_load(unsigned,const char *directory,char *,size_t);
