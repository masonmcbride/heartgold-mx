#pragma once
#include <stdint.h>
/* Native-owned original objects. These calls do not perform collision checks. */
typedef struct PlayerAvatar PlayerAvatar;
typedef struct LocalMapObject LocalMapObject;
int hg_motion_direction(PlayerAvatar *, uint16_t held_keys, int *direction);
int hg_motion_grid_step(LocalMapObject *, unsigned direction);

/* Original normal-field attribute dispatch, wall-bit and directional barrier
 * rules. This does not include heights, occupancy, surf or player mode rules. */
typedef struct HGFieldResources HGFieldResources;
typedef struct { int wall; uint8_t behavior; } HGMotionTile;
int hg_motion_tile(HGFieldResources *,int x,int z,HGMotionTile *);
int hg_motion_terrain_barrier(HGFieldResources *,LocalMapObject *,unsigned direction,int *blocked);

/* Original object occupancy, including previous tiles and vertical separation. */
int hg_motion_occupied(LocalMapObject *,int x,int y,int z,int *occupied);

/* Original player collision mask, currently admitted only within New Bark's
 * normal static field, with no active gym gimmick/dynamic platforms. Nonzero
 * masks include special movement requirements, not just solid obstacles.
 * Query preserves object state; unsupported environments leave output intact. */
int hg_motion_player_collision(HGFieldResources *,PlayerAvatar *,unsigned direction,unsigned *mask);

typedef struct { int direction,moved; unsigned collision_mask; } HGMotionMove;
/* Discrete walking integration (state 0): only a zero original collision mask
 * commits the original grid step and coordinate conversion. No animation,
 * jumping, surfing, warp/script transitions or controller timing is supplied. */
int hg_motion_walk_step(HGFieldResources *,PlayerAvatar *,uint16_t held_keys,HGMotionMove *);
