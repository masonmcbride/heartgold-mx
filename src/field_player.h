#pragma once
#include <stdint.h>
#include <stddef.h>
#include "field_transition.h"
typedef struct HGFieldPlayer HGFieldPlayer;
HGFieldPlayer *hg_field_player_new(void);
HGFieldPlayer *hg_field_player_new_map(unsigned map);
void hg_field_player_free(HGFieldPlayer *);
/* Admit one ordinary walk/run. Original commands advance it on tick. */
int hg_field_player_move(HGFieldPlayer *,uint16_t keys);
int hg_field_player_walking(const HGFieldPlayer *);
int hg_field_player_running(const HGFieldPlayer *);
unsigned hg_field_player_steps(const HGFieldPlayer *);
/* 1 advanced, 0 idle, -1 unsupported execution (rolls back the whole step). */
int hg_field_player_tick(HGFieldPlayer *);
int hg_field_player_position(const HGFieldPlayer *,int *x,int *z,double world[3]);

int hg_field_player_facing(const HGFieldPlayer *);

int hg_field_player_interact(HGFieldPlayer *,char *,size_t);
int hg_field_player_probe_transition(HGFieldPlayer *,uint16_t,HGWarpDestination *);
int hg_field_player_transition(HGFieldPlayer *,const HGWarpDestination *);
