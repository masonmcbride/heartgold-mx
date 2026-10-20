#pragma once
#include "battle_runtime.h"
typedef struct HGBattleView HGBattleView;
HGBattleView *hg_battle_view_new(void);
void hg_battle_view_free(HGBattleView *);
/* Original sprites/font; game on left, command/move choices on right. */
int hg_battle_view_render(HGBattleView *,HGBattle *,int state,unsigned choice,uint8_t *,unsigned,unsigned);
