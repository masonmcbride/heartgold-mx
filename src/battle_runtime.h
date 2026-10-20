#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct HGBattle HGBattle;
struct Party;
struct DSAudio;
enum { HG_BATTLE_FAILED=-1, HG_BATTLE_RUNNING, HG_BATTLE_MESSAGE, HG_BATTLE_COMMAND, HG_BATTLE_MOVES, HG_BATTLE_FINISHED };
typedef struct {unsigned species,level,hp,max_hp,status,moves[4],pp[4],gender,form,shiny;} HGBattleMonView;
/* Private battle owner: original rules run against an owned copy of the party. */
HGBattle *hg_battle_new(const struct Party *,unsigned species,unsigned level,int trainer,struct DSAudio *,char *,size_t);
void hg_battle_free(HGBattle *);
int hg_battle_tick(HGBattle *);
int hg_battle_input(HGBattle *,unsigned choice);
const char *hg_battle_message(const HGBattle *);
const char *hg_battle_error(const HGBattle *);
unsigned hg_battle_result(const HGBattle *);
int hg_battle_mon(const HGBattle *,unsigned side,HGBattleMonView *);
const struct Party *hg_battle_party(const HGBattle *);

struct PlayerProfile;
struct Bag;
struct Options;
struct Pokedex;
struct GameStats;
typedef struct {
    const struct Party *party;
    const struct Party *opponent; /* Original encounter-generated party, for wild battles. */
    const struct PlayerProfile *profile;
    const struct Bag *bag;
    const struct Options *options;
    const struct Pokedex *pokedex;
    const struct GameStats *stats;
    unsigned trainer_id,enemy_species,enemy_level,seed;
    struct DSAudio *audio;
} HGBattleSetup;
/* Live-session setup: required snapshots are copied before original execution. */
HGBattle *hg_battle_new_setup(const HGBattleSetup *,char *,size_t);
const struct Party *hg_battle_opponent(const HGBattle *);
/* Atomic, one-time result publication; rejects unfinished or stale state. */
int hg_battle_commit(HGBattle *,struct Party *,struct PlayerProfile *,struct Bag *,struct Pokedex *,struct GameStats *);

struct FieldSystem;
typedef struct {uint32_t rng;uint16_t reverse_steps,inhibit_steps;uint8_t last_facing;} HGEncounterState;
/* Original ordinary land rolls/generation. -1 error, 0 none, 1 owned opponent. */
int hg_encounter_step(HGEncounterState *,struct FieldSystem *,unsigned map,unsigned behavior,unsigned time_of_day,struct Party *,char *,size_t);

int hg_battle_move_allowed(HGBattle *,unsigned slot);
