#pragma once
#include <stdint.h>
typedef struct HGFieldPlayer HGFieldPlayer;
HGFieldPlayer *hg_field_player_new(void);
void hg_field_player_free(HGFieldPlayer *);
/* One discrete input attempt; returns whether presentation changed. */
int hg_field_player_move(HGFieldPlayer *,uint16_t keys);
int hg_field_player_position(const HGFieldPlayer *,int *x,int *z,double world[3]);

int hg_field_player_facing(const HGFieldPlayer *);
