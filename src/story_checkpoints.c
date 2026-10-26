#include "story_checkpoints.h"
#include "compat/pokemon.h"
#include "player_data.h"
#include <stdio.h>
#include <unistd.h>

typedef struct { unsigned species,level,moves[4]; } TeamMon;
typedef struct { const char *title,*source;int cartridge;TeamMon team[6]; } StoryCheckpoint;
#define M(species,level,a,b,c,d) {species,level,{MOVE_##a,MOVE_##b,MOVE_##c,MOVE_##d}}
static const StoryCheckpoint checkpoints[]={
 {"First steps into Johto · Lv. 5","155-1.hgcheckpoint",0,{
 M(155,5,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(492,5,MAGICAL_LEAF,LEECH_SEED,GROWTH,SYNTHESIS),
 M(447,5,QUICK_ATTACK,FORCE_PALM,ENDURE,COUNTER),
 M(116,5,WATER_GUN,SMOKE_SCREEN,DRAGON_BREATH,LEER),
 M(403,5,THUNDER_SHOCK,TACKLE,LEER,CHARGE),
 M(333,5,PECK,SING,ASTONISH,GROWL)}},
 {"The Mystery Egg & a rival · Lv. 6","155-2.hgcheckpoint",0,{
 M(155,6,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(491,6,OMINOUS_WIND,HYPNOSIS,DISABLE,QUICK_ATTACK),
 M(147,6,TWISTER,THUNDER_WAVE,DRAGON_BREATH,WRAP),
 M(123,6,WING_ATTACK,QUICK_ATTACK,FOCUS_ENERGY,FALSE_SWIPE),
 M(60,6,WATER_GUN,HYPNOSIS,BUBBLE,POUND),
 M(179,6,THUNDER_SHOCK,THUNDER_WAVE,TACKLE,GROWL)}},
 {"Rival defeated—return to Elm · Lv. 7","155-3.hgcheckpoint",0,{
 M(155,7,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(115,7,FAKE_OUT,BITE,LOW_KICK,TAIL_WHIP),
 M(133,7,QUICK_ATTACK,BITE,HELPING_HAND,SAND_ATTACK),
 M(280,7,CONFUSION,MAGICAL_LEAF,DOUBLE_TEAM,TELEPORT),
 M(194,7,WATER_GUN,MUD_SHOT,TAIL_WHIP,MUD_SPORT),
 M(163,7,PECK,HYPNOSIS,GROWL,FORESIGHT)}},
 {"Elm's mystery & the catching lesson · Lv. 7–8","155-4.hgcheckpoint",0,{
 M(155,8,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(215,7,ICE_SHARD,QUICK_ATTACK,FAINT_ATTACK,TAUNT),
 M(418,8,WATER_GUN,QUICK_ATTACK,SONIC_BOOM,WATER_SPORT),
 M(406,7,MEGA_DRAIN,GROWTH,STUN_SPORE,WATER_SPORT),
 M(200,7,CONFUSION,ASTONISH,SPITE,GROWL),
 M(172,7,THUNDER_SHOCK,CHARM,SWEET_KISS,TAIL_WHIP)}},
 {"Poké Balls packed—your adventure begins · Lv. 8–9","155-5.hgcheckpoint",0,{
 M(155,9,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(403,8,SPARK,TACKLE,LEER,CHARGE),
 M(258,8,WATER_GUN,MUD_SLAP,ROCK_TOMB,GROWL),
 M(175,8,EXTRASENSORY,CHARM,METRONOME,SWEET_KISS),
 M(246,8,BITE,ROCK_THROW,SANDSTORM,LEER),
 M(359,8,BITE,QUICK_ATTACK,LEER,PURSUIT)}},
 {"Violet City & the swaying Sprout Tower · Lv. 10–12","155-violet-arrival.sav",1,{
 M(155,12,EMBER,QUICK_ATTACK,SMOKE_SCREEN,LEER),
 M(492,11,MAGICAL_LEAF,LEECH_SEED,GROWTH,SYNTHESIS),
 M(92,10,LICK,HYPNOSIS,SPITE,MEAN_LOOK),
 M(214,11,AERIAL_ACE,BRICK_BREAK,HORN_ATTACK,ENDURE),
 M(179,11,THUNDER_SHOCK,THUNDER_WAVE,LIGHT_SCREEN,TACKLE),
 M(194,11,WATER_GUN,MUD_SHOT,TAIL_WHIP,MUD_SPORT)}}
};
#undef M
unsigned hg_story_checkpoint_count(void){return NELEMS(checkpoints);}
const char *hg_story_checkpoint_title(unsigned index){return index<NELEMS(checkpoints)?checkpoints[index].title:NULL;}

HGFieldApp *hg_story_checkpoint_load(unsigned index,const char *directory,char *error,size_t size){
 if(error&&size)error[0]=0;
 if(index>=NELEMS(checkpoints)||!directory){if(error&&size)snprintf(error,size,"Unknown story checkpoint");return NULL;}
 const StoryCheckpoint *entry=&checkpoints[index];char path[4096];
 if(snprintf(path,sizeof(path),"%s/%s",directory,entry->source)>=(int)sizeof(path))return NULL;
 HGFieldApp *app=entry->cartridge?hg_field_app_continue_copy(path,directory,"violet-team",error,size):hg_field_app_checkpoint_load(path,error,size);
 if(!app)return NULL;
 FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);
 Party party;Party_InitWithMaxSize(&party,6);PlayerProfile *profile=hg_story_save_profile(field.saveData);
 for(unsigned i=0;i<6;i++){
  const TeamMon *spec=&entry->team[i];Pokemon mon;
  CreateMon(&mon,spec->species,spec->level,24,TRUE,0x123400+i,OT_ID_PRESET,profile->id);
  SetMonData(&mon,MON_DATA_OT_NAME,profile->name);SetMonData(&mon,MON_DATA_OT_GENDER,&profile->gender);
  for(unsigned move=0;move<4;move++)MonSetMoveInSlot(&mon,spec->moves[move],move);
  if(!Party_AddMon(&party,&mon)){hg_field_app_free(app);return NULL;}
 }
 *hg_story_save_party(field.saveData)=party;
 return app;
}
