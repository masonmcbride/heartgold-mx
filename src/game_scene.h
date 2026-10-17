#pragma once
#include "inventory.h"
/* Two independent 256x192 DS-size screens, side by side with a 16-pixel gap. */
#define HG_SCENE_WIDTH 528
#define HG_SCENE_HEIGHT 192
#define HG_SCENE_BYTES (HG_SCENE_WIDTH*HG_SCENE_HEIGHT*4)
typedef struct HGScene HGScene;
HGScene *hg_scene_new(void);
void hg_scene_free(HGScene *);
int hg_scene_render(HGScene *,const HGInventory *,uint64_t tick,uint8_t rgba[HG_SCENE_BYTES]);
int hg_scene_click(HGInventory *,unsigned x,unsigned y);
