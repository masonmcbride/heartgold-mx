#pragma once
#include <stdint.h>
#include <stddef.h>
#include "field_transition.h"
#include "field_view.h"
#include "field_script.h"
typedef struct HGFieldPlayer HGFieldPlayer;
HGFieldPlayer *hg_field_player_new(void);
HGFieldPlayer *hg_field_player_new_map(unsigned map);
HGFieldPlayer *hg_field_player_new_game(void);
void hg_field_player_free(HGFieldPlayer *);
/* Admit one ordinary walk/run. Original commands advance it on tick. */
int hg_field_player_move(HGFieldPlayer *,uint16_t keys);
int hg_field_player_pending_map(const HGFieldPlayer *);
int hg_field_player_commit_zone(HGFieldPlayer *);
void hg_field_player_cancel_step(HGFieldPlayer *);
int hg_field_player_walking(const HGFieldPlayer *);
int hg_field_player_running(const HGFieldPlayer *);
unsigned hg_field_player_steps(const HGFieldPlayer *);
/* 1 advanced, 0 idle, -1 unsupported execution (rolls back the whole step). */
int hg_field_player_tick(HGFieldPlayer *);
int hg_field_player_position(const HGFieldPlayer *,int *x,int *z,double world[3]);

int hg_field_player_facing(const HGFieldPlayer *);
unsigned hg_field_player_sprite(const HGFieldPlayer *);

int hg_field_player_interact(HGFieldPlayer *,char *,size_t);
int hg_field_player_interaction_begin(HGFieldPlayer *);
/* Integration entry for an already-selected map script; actor_id=-1 for map
 * hooks. Uses the same preflight/live context as player interaction. */
int hg_field_player_start_script(HGFieldPlayer *,unsigned entry,int actor_id);
int hg_field_player_refresh_actors(HGFieldPlayer *);
/* Original party swap plus privately prepared follower; failure changes neither. */
int hg_field_player_swap_party(HGFieldPlayer *,unsigned,unsigned);
int hg_field_player_interaction_tick(HGFieldPlayer *,char *,size_t);
int hg_field_player_interaction_ack(HGFieldPlayer *);
int hg_field_player_interaction_busy(const HGFieldPlayer *);
const char *hg_field_player_interaction_error(const HGFieldPlayer *);
HGStoryState *hg_field_player_story(HGFieldPlayer *);
int hg_field_player_probe_transition(HGFieldPlayer *,uint16_t,HGWarpDestination *);
int hg_field_player_probe_step_transition(HGFieldPlayer *,HGWarpDestination *);
int hg_field_player_transition(HGFieldPlayer *,const HGWarpDestination *);
typedef struct {
    unsigned id,sprite,movement,script,direction;int x,z;
    int hidden;int emote;double emote_offset;
    HGFieldBillboard billboard;
} HGFieldActor;
typedef struct { unsigned events,active,conditional,unsupported; } HGFieldActorCounts;
/* Original flag-selected event actors, idle except for admitted script moves.
 * Borrowed sprite lifetime ends at facing changes, travel or destruction. */
HGFieldActorCounts hg_field_player_actor_counts(const HGFieldPlayer *);
int hg_field_player_actor(const HGFieldPlayer *,unsigned index,HGFieldActor *);

const HGFieldView *hg_field_player_props(const HGFieldPlayer *);
unsigned hg_field_player_prop_count(const HGFieldPlayer *);

/* Admitted coordinate scheduling: 1 started, 0 no event, -1 rejected. */
int hg_field_player_coordinate_begin(HGFieldPlayer *);
/* Check original header frame conditions before accepting field input.
 * Rejected conditions retry only after story state changes or map travel. */
int hg_field_player_frame_begin(HGFieldPlayer *);

#include "compat/starter.h"
int hg_field_player_starter_pending(const HGFieldPlayer *);
int hg_field_player_starter_option(HGFieldPlayer *,unsigned,HGStarterOption *);
int hg_field_player_choose_starter(HGFieldPlayer *,unsigned);

int hg_field_player_menu_pending(const HGFieldPlayer *);
int hg_field_player_menu_choose(HGFieldPlayer *,unsigned);
int hg_field_player_menu_choice_supported(HGFieldPlayer *,unsigned);

int hg_field_player_follower(const HGFieldPlayer *,unsigned *,int *,int *);

int hg_field_player_present_props(const HGFieldPlayer *,HGFieldView *);

int hg_field_player_running_shoes(const HGFieldPlayer *);

/* Scenery/player-only session; original terrain motion and portals, no story/NPCs. */
HGFieldPlayer *hg_field_player_new_explore(unsigned map);

int hg_field_player_brightness(const HGFieldPlayer *);

/* Field-owned original FadeWork used for door/stair travel. Different fade
 * phases cannot overlap; callers wait for phase one to finish before phase two. */
int hg_field_player_fade_active(const HGFieldPlayer *);
int hg_field_player_fade_begin(HGFieldPlayer *,unsigned type,unsigned color,unsigned steps,unsigned speed);
int hg_field_player_fade_tick(HGFieldPlayer *);

int hg_field_player_phone_active(const HGFieldPlayer *);
const char *hg_field_player_phone_contact(const HGFieldPlayer *);

struct DSAudio *hg_field_player_audio(HGFieldPlayer *);
struct FieldSystem;
void hg_field_player_bind_runtime(HGFieldPlayer *,struct FieldSystem *);

int hg_field_player_battle_request(HGFieldPlayer *,unsigned *,struct FieldSystem *);
int hg_field_player_battle_complete(HGFieldPlayer *,unsigned result);

HGFieldPlayer *hg_field_player_resume(const HGStoryState *,const HGFieldLocation *,int shoes);

int hg_field_player_name_pending(const HGFieldPlayer *);
const char *hg_field_player_name_text(const HGFieldPlayer *);
int hg_field_player_name_edit(HGFieldPlayer *,const char *);
int hg_field_player_name_submit(HGFieldPlayer *,const char *,int cancel);

int hg_field_player_prepare_save(HGFieldPlayer *,HGStoryState *candidate);

int hg_field_player_restore_save_objects(HGFieldPlayer *);

int hg_field_player_start_script_args(HGFieldPlayer *,unsigned entry,int actor_id,const uint16_t *args,unsigned count);

struct BugContest;
struct BugContest *hg_field_player_contest(HGFieldPlayer *);
int hg_field_player_pc_full_event(const HGFieldPlayer *);
void hg_field_player_pc_full_ack(HGFieldPlayer *);

/* Scenario input uses the same original command/preflight/scene publication. */
int hg_field_player_start_bytes(HGFieldPlayer *,const uint8_t *,size_t,unsigned bank);

/* Export settled player/warp records before discarding the current map. */
int hg_field_player_prepare_relocation(HGFieldPlayer *,HGStoryState *);

HGFieldPlayer *hg_field_player_resume_checked(const HGStoryState *,const HGFieldLocation *,int shoes,char *error,size_t);

/* Staged native travel uses the shared original one-shot prop owner. */
int hg_field_player_door_begin(HGFieldPlayer *,unsigned direction);
int hg_field_player_door_tick(HGFieldPlayer *);
void hg_field_player_door_end(HGFieldPlayer *);

int hg_field_player_hidden(const HGFieldPlayer *);
/* 0 idle, 1/2 opening, 3 threshold walk, 4 closing. */
int hg_field_player_exit_active(const HGFieldPlayer *);
int hg_field_player_exit_tick(HGFieldPlayer *);

unsigned hg_field_player_menu_count(const HGFieldPlayer *);
const char *hg_field_player_menu_label(const HGFieldPlayer *,unsigned);
struct HGShop *hg_field_player_shop(HGFieldPlayer *);
