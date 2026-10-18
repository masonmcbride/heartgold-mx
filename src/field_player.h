#pragma once
#include <stdint.h>
#include <stddef.h>
#include "field_transition.h"
#include "field_view.h"
#include "field_script.h"
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
int hg_field_player_interaction_begin(HGFieldPlayer *);
/* Integration entry for an already-selected map script; actor_id=-1 for map
 * hooks. Uses the same preflight/live context as player interaction. */
int hg_field_player_start_script(HGFieldPlayer *,unsigned entry,int actor_id);
int hg_field_player_refresh_actors(HGFieldPlayer *);
int hg_field_player_interaction_tick(HGFieldPlayer *,char *,size_t);
int hg_field_player_interaction_ack(HGFieldPlayer *);
int hg_field_player_interaction_busy(const HGFieldPlayer *);
const char *hg_field_player_interaction_error(const HGFieldPlayer *);
HGStoryState *hg_field_player_story(HGFieldPlayer *);
int hg_field_player_probe_transition(HGFieldPlayer *,uint16_t,HGWarpDestination *);
int hg_field_player_transition(HGFieldPlayer *,const HGWarpDestination *);
typedef struct {
    unsigned id,sprite,movement,script,direction;int x,z;
    HGFieldBillboard billboard;
} HGFieldActor;
typedef struct { unsigned events,active,conditional,unsupported; } HGFieldActorCounts;
/* Original flag-selected event actors, idle except for admitted script moves.
 * Borrowed sprite lifetime ends at facing changes, travel or destruction. */
HGFieldActorCounts hg_field_player_actor_counts(const HGFieldPlayer *);
int hg_field_player_actor(const HGFieldPlayer *,unsigned index,HGFieldActor *);
