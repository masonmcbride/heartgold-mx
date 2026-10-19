#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct HGStoryState HGStoryState;
typedef struct HGFieldScript HGFieldScript;
struct FieldSystem;
void hg_story_bind_field(HGStoryState *,struct FieldSystem *);
int hg_story_enter_zone(HGStoryState *,unsigned map,char *error,size_t);
/* Muted native sound-request state. No audio synthesis/output is claimed. */
typedef struct { unsigned sequence,fade_ticks,requests; } HGStoryMusic;
HGStoryMusic hg_story_music(const HGStoryState *);
/* Original flags/variables/profile initialization plus the complete original
 * std_init script. Native inspection profile/seed; not complete game startup. */
HGStoryState *hg_story_new(void);
void hg_story_free(HGStoryState *);
HGStoryState *hg_story_clone(const HGStoryState *);
/* Revision-checked native transaction boundary, including original records. */
int hg_story_commit(HGStoryState *,const HGStoryState *,unsigned long long expected_revision);
int hg_story_flag_get(const HGStoryState *,unsigned flag,int *value);
int hg_story_flag_set(HGStoryState *,unsigned flag,int value);
int hg_story_var_get(const HGStoryState *,unsigned var,uint16_t *value);
int hg_story_var_set(HGStoryState *,unsigned var,uint16_t value);
unsigned long long hg_story_revision(const HGStoryState *);
/* Native inspection profile; validates original font characters and seven-
 * character name limit. This is not the original naming-screen workflow. */
int hg_story_profile_set(HGStoryState *,const char *name,unsigned gender);
enum { HG_SCRIPT_TEXT_CAPACITY=4096 };
typedef struct { char text[HG_SCRIPT_TEXT_CAPACITY];unsigned face_player,select_sound; } HGScriptMessage;
enum { HG_SCRIPT_ERROR=-1,HG_SCRIPT_DONE=0,HG_SCRIPT_RUNNING=1,HG_SCRIPT_MESSAGE=2 };
/* Preflight executes the selected path privately through original handlers.
 * Unknown commands fail before presentation or live state changes. */
HGFieldScript *hg_field_script_new(HGStoryState *,unsigned map,unsigned entry,char *error,size_t);
/* Bounded synthetic programs use the same validation/execution path; borrowed
 * bytes must outlive the context. No raw DS-address opcodes are admitted. */
HGFieldScript *hg_field_script_bytes(HGStoryState *,const uint8_t *,size_t,size_t start,unsigned message_bank,char *,size_t);
int hg_field_script_tick(HGFieldScript *);
/* Original End makes a candidate ready. Publish only after dependent native
 * resources are ready; stale or repeated publication fails. */
const HGStoryState *hg_field_script_candidate(const HGFieldScript *);
int hg_field_script_commit(HGFieldScript *);
int hg_story_population_changed(const HGStoryState *,const HGStoryState *);
int hg_field_script_ack(HGFieldScript *);
const HGScriptMessage *hg_field_script_message(const HGFieldScript *);
const char *hg_field_script_error(const HGFieldScript *);
void hg_field_script_free(HGFieldScript *);
/* Original map-temporary reset and admitted transition/resume scripts. */
int hg_story_enter_map(HGStoryState *,unsigned map,char *,size_t);
/* Original header selectors after bounds validation. 65535 means no hook;
 * querying a frame hook does not execute its cutscene. */
int hg_story_map_script(HGStoryState *,unsigned map,unsigned type,unsigned *entry);
int hg_story_header_script(HGStoryState *,const uint8_t *,size_t,unsigned type,unsigned *entry);

int hg_field_script_menu_pending(const HGFieldScript *);
int hg_field_script_menu_choice_supported(HGFieldScript *,unsigned choice);
int hg_field_script_menu_choose(HGFieldScript *,unsigned);
/* Read the retained original phonebook; no Pokegear application is launched. */
int hg_story_phone_registered(const HGStoryState *,unsigned contact);

int hg_story_apricorn_tree(const HGStoryState *,unsigned index,unsigned *kind,int *available);
