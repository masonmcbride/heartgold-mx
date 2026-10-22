#include "compat/battle.h"
#include "compat/motion.h"
#include "field_app.h"
#include "constants/vars.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,app?hg_field_app_error(app):"");exit(1);}}while(0)
#include "field_routes.h"
static void elm(HGFieldApp *app){
 hg_test_portal(app,33);hg_test_portal(app,60);hg_test_portal(app,61);unsigned done=0;
 for(unsigned i=0;i<8192;i++){
  hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);
  if(hg_field_app_name_pending(app))CHECK(hg_field_app_name_submit(app,"Silver",0));
  else if(hg_field_app_menu_pending(app)){hg_field_app_clear_input(app);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);hg_field_app_interact(app);}
  else if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);
  if(!hg_field_app_interaction_busy(app)){done=1;break;}
 }CHECK(done);
}
static void scenario(unsigned species,int resume){
 char error[256],path[256];snprintf(path,sizeof(path),"build/checkpoints/%u-%u.hgcheckpoint",species,resume?4:3);HGFieldApp *app=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!app)fprintf(stderr,"%s; generate checkpoints with heartgold_checkpoints first\n",error);CHECK(app);
 HGFieldApp *restored;
 if(!resume){elm(app);hg_test_portal(app,60);hg_test_portal(app,33);hg_test_settle(app);hg_test_walk_to(app,667,398);hg_test_settle(app);
  snprintf(path,sizeof(path),"build/checkpoints/%u-4.hgcheckpoint",species);CHECK(hg_field_app_checkpoint_write(app,path,4,error,sizeof(error)));restored=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(restored);hg_field_app_free(app);app=restored;
 }
 HGStoryState *story=hg_field_app_story(app);FieldSystem field={0};hg_story_bind_field(story,&field);Party before=*hg_story_save_party(field.saveData);PlayerProfile profile=*hg_story_save_profile(field.saveData);Pokedex dex=*hg_story_save_dex(field.saveData);GameStats stats=*hg_story_save_stats(field.saveData);Bag bag=*hg_story_save_bag(field.saveData);
 size_t size=hg_story_snapshot_size();void *initial=malloc(size),*current=malloc(size);CHECK(initial&&current&&hg_story_snapshot_export(story,initial,size));
 hg_field_app_input(app,0x20);hg_field_app_step(app);hg_field_app_clear_input(app);
 unsigned seen=0,done=0,phases=0,marill=0,caught=0,weakened=0,attack=0;
 for(unsigned tick=0;tick<8192;tick++){
  hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);
  if(hg_field_app_battle_active(app)){
   CHECK(hg_field_app_battle_tutorial(app));if(!seen){CHECK(hg_story_snapshot_export(story,initial,size));stats=*hg_story_save_stats(field.saveData);}seen=1;CHECK(hg_story_snapshot_export(story,current,size)&&!memcmp(initial,current,size));
   int state=hg_field_app_battle_state(app);CHECK(state!=HG_BATTLE_COMMAND&&state!=HG_BATTLE_MOVES);
   HGBattleMonView mon,enemy;if(hg_field_app_battle_mon(app,0,&mon)&&mon.species){CHECK(mon.species==SPECIES_MARILL&&mon.level==5);marill=1;}
   if(hg_field_app_battle_mon(app,1,&enemy)&&enemy.species){CHECK(enemy.species==SPECIES_RATTATA&&enemy.level==2);if(enemy.hp&&enemy.hp<enemy.max_hp)weakened=1;}
   if(strstr(hg_field_app_dialogue(app),hg_pokemon_species_name(SPECIES_MARILL))&&strstr(hg_field_app_dialogue(app),"used"))attack=1;
   unsigned phase=hg_field_app_capture_phase(app);if(phase<32)phases|=1u<<phase;
   if(strstr(hg_field_app_dialogue(app),"was caught"))caught=1;
  }
  if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);
  if(seen&&!hg_field_app_interaction_busy(app)){done=1;break;}
 }
 CHECK(seen&&done&&marill&&caught&&weakened&&attack);CHECK((phases&((1u<<1)|(1u<<5)|(1u<<7)))==((1u<<1)|(1u<<5)|(1u<<7)));
 CHECK(!memcmp(&before,hg_story_save_party(field.saveData),sizeof(before)));CHECK(!memcmp(&profile,hg_story_save_profile(field.saveData),sizeof(profile)));CHECK(!memcmp(&dex,hg_story_save_dex(field.saveData),sizeof(dex)));CHECK(!memcmp(&stats,hg_story_save_stats(field.saveData),sizeof(stats)));
 CHECK(Bag_AddItem(&bag,ITEM_POKE_BALL,5,HEAP_ID_FIELD1));CHECK(!memcmp(&bag,hg_story_save_bag(field.saveData),sizeof(bag)));
 int flag;CHECK(hg_story_flag_get(story,FLAG_UNK_09A,&flag)&&flag);CHECK(hg_story_flag_get(story,FLAG_HIDE_ROUTE_29_FRIEND,&flag)&&flag);CHECK(hg_story_flag_get(story,FLAG_HIDE_ROUTE_29_MARILL,&flag)&&flag);u16 value;CHECK(hg_story_var_get(story,VAR_UNK_408B,&value)&&value==0);
 snprintf(path,sizeof(path),"build/checkpoints/%u-5.hgcheckpoint",species);CHECK(hg_field_app_checkpoint_write(app,path,5,error,sizeof(error)));restored=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(restored);hg_field_app_free(app);app=restored;
 hg_test_walk_to(app,667,398);hg_test_walk_to(app,665,398);hg_test_settle(app);CHECK(!hg_field_app_battle_active(app));
 free(initial);free(current);hg_field_app_free(app);printf("Starter %u: original automatic Marill/Rattata capture, five Balls, unchanged player records, no retrigger passed\n",species);
}
int main(int argc,char **argv){int resume=argc==2&&!strcmp(argv[1],"--resume");if(argc>2||(argc==2&&!resume))return 2;for(unsigned i=0;i<3;i++)scenario((unsigned[]){152,155,158}[i],resume);
 char error[256];HGFieldApp *app=hg_field_app_checkpoint_load("build/checkpoints/152-4.hgcheckpoint",error,sizeof(error));CHECK(app);hg_field_app_input(app,0x20);hg_field_app_step(app);hg_field_app_clear_input(app);int pending=0;
 for(unsigned i=0;i<8192;i++){hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);if(hg_field_app_capture_phase(app)==5){pending=1;break;}if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);}
 CHECK(pending&&!hg_field_app_checkpoint_write(app,"build/checkpoints/pending-tutorial.hgcheckpoint",5,error,sizeof(error)));hg_field_app_free(app);puts("Pending capture cancellation and mid-demo snapshot rejection passed");return 0;}

