#pragma once
#include <stdint.h>
/* Native-owned original objects. These calls do not perform collision checks. */
typedef struct PlayerAvatar PlayerAvatar;
typedef struct LocalMapObject LocalMapObject;
int hg_motion_direction(PlayerAvatar *, uint16_t held_keys, int *direction);
int hg_motion_grid_step(LocalMapObject *, unsigned direction);
/* Original held-command reset through the existing bounded Thumb bridge. */
int hg_motion_clear_held(LocalMapObject *);

/* Original normal-field attribute dispatch, wall-bit and directional barrier
 * rules. This does not include heights, occupancy, surf or player mode rules. */
typedef struct HGFieldResources HGFieldResources;
typedef struct { int wall; uint8_t behavior; } HGMotionTile;
int hg_motion_tile(HGFieldResources *,int x,int z,HGMotionTile *);
int hg_motion_terrain_barrier(HGFieldResources *,LocalMapObject *,unsigned direction,int *blocked);

/* Original object occupancy, including previous tiles and vertical separation. */
int hg_motion_occupied(LocalMapObject *,int x,int y,int z,int *occupied);

/* Original player collision mask, admitted in New Bark and its seven tested
 * static interiors, with no active gym gimmick/dynamic platforms. Nonzero
 * masks include special movement requirements, not just solid obstacles.
 * Query preserves object state; unsupported environments leave output intact. */
int hg_motion_player_collision(HGFieldResources *,PlayerAvatar *,unsigned direction,unsigned *mask);

typedef struct { int direction,moved; unsigned collision_mask; } HGMotionMove;
/* Discrete walking integration (state 0): only a zero original collision mask
 * commits the original grid step and coordinate conversion. No animation,
 * jumping, surfing, warp/script transitions or controller timing is supplied. */
int hg_motion_walk_step(HGFieldResources *,PlayerAvatar *,uint16_t held_keys,HGMotionMove *);

/* Normal walking only: native admission selects command 12..15 after original
 * collision checks. Tick executes original command-table dispatch, displacement,
 * terrain following and completion. Object callback must be a native provider.
 * Begin leaves the position vector at the source; tick reports completion.
 * The shared tick also admits running commands 88..91 from locomotion_begin. */
int hg_motion_walk_begin(HGFieldResources *,PlayerAvatar *,uint16_t,HGMotionMove *);
int hg_motion_walk_tick(HGFieldResources *,LocalMapObject *,int *complete);
/* Bounded original command-table execution for script-owned actors. Includes
 * faces, walking on the spot, normal/slightly-fast steps, far jumps and
 * direction locking. Does not run the original movement-script scheduler. */
int hg_motion_script_command(unsigned command);
int hg_motion_script_tick(HGFieldResources *,LocalMapObject *,int *complete);

typedef struct GameStats GameStats;
/* Native-owned statistics plus a pointer-free serialized follower notification.
 * This is not an original SaveData layout or a populated follower actor. */
typedef struct { GameStats *stats; uint8_t follower[32]; } HGMotionSession;
/* Scoped original sub_0205D4B4 walk/run branch, plus original input-history
 * update. B is bit 1; original running-shoe ownership gates running. Only zero
 * collision masks are admitted; other movement types remain unsupported. */
int hg_motion_locomotion_begin(HGFieldResources *,PlayerAvatar *,uint16_t,HGMotionSession *,HGMotionMove *);

/* Original facing-tile/direction selector over validated serialized BgEvents.
 * Hidden items are unsupported without their original saved flag state. */
int hg_motion_background_event(PlayerAvatar *,const uint8_t *,unsigned count,unsigned *script);
/* Original field interaction target: counter tiles, active objects, flag 19
 * and exact elevation. Success with no target writes NULL; error preserves it. */
int hg_motion_facing_object(HGFieldResources *,PlayerAvatar *,LocalMapObject **);
typedef struct FieldSystem FieldSystem;
/* Original coordinate-event rectangle/variable selection. Caller supplies a
 * native field with a bound story owner and validated 16-byte event records.
 * Success writes script ID or 65535; failure preserves output. */
int hg_motion_coordinate_event(FieldSystem *,PlayerAvatar *,const uint8_t *,unsigned count,unsigned *script);
/* Original spawn-table helper for local-field record initialization. */
unsigned hg_field_guest_mom_spawn(void);
