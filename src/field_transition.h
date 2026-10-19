#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct HGTransition HGTransition;
typedef struct HGFieldResources HGFieldResources;
typedef struct PlayerAvatar PlayerAvatar;
typedef struct { int map,warp,x,z,direction;unsigned kind; } HGWarpDestination;
typedef struct { int map,warp,x,z,direction; } HGFieldLocation;
/* Original Location_SetToPlayerRoom, including its original coordinates. */
HGFieldLocation hg_field_new_game_location(void);
typedef struct { HGFieldLocation current,previous,entrance,dynamic; } HGTransitionState;
/* Borrows the native avatar until free; attaches its save pointer to the
 * original local-field player record and restores the prior owner on free. */
HGTransition *hg_transition_new(unsigned map,PlayerAvatar *);
void hg_transition_free(HGTransition *);
/* Native staging owner executes original warp selection/anchor resolution.
 * 1 selected, 0 no admitted transition. Output is unchanged on failure.
 * Queries never commit local-field state; commit follows successful scene load. */
int hg_transition_probe(HGTransition *,HGFieldResources *,unsigned direction,HGWarpDestination *);
int hg_transition_commit(HGTransition *,const HGWarpDestination *);
/* Settled outdoor header change: current position changes, warp history stays. */
int hg_transition_commit_zone(HGTransition *,unsigned map);
int hg_transition_supported(unsigned map);
int hg_transition_inspect(HGTransition *,HGTransitionState *);
/* Native owner synchronization at a settled movement boundary. */
void hg_transition_sync_player(HGTransition *);
