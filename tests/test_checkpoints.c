#include "compat/battle.h"
#include "field_app.h"
#include "field_player.h"
#include "story_checkpoints.h"
#include "compat/save_manager.h"
#include "constants/vars.h"
#include <stdio.h>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
static uint32_t file_hash(const char *path){
 FILE *file=fopen(path,"rb");CHECK(file);uint32_t hash=2166136261u;int byte;while((byte=fgetc(file))!=EOF)hash=(hash^(unsigned)byte)*16777619u;CHECK(!ferror(file)&&!fclose(file));return hash;
}
static void menu_press(HGFieldApp *app,unsigned keys,int confirm){
 if(confirm)hg_field_app_interact(app);else hg_field_app_input(app,keys);hg_field_app_step(app);hg_field_app_clear_input(app);
}
static void story_teams(void){
 char error[256];CHECK(hg_story_checkpoint_count()==6);unsigned favorites=0,rosters[6][6];
 uint32_t original_hash=file_hash("build/checkpoints/155-violet-arrival.sav");
 CHECK(!hg_story_checkpoint_load(99,"build/checkpoints",error,sizeof(error))&&error[0]);
 CHECK(!hg_story_checkpoint_load(0,"build/checkpoints/missing",error,sizeof(error))&&error[0]);
 for(unsigned index=0;index<hg_story_checkpoint_count();index++){
  HGFieldApp *app=hg_story_checkpoint_load(index,"build/checkpoints",error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);
  FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);Party *party=hg_story_save_party(field.saveData);CHECK(Party_GetCount(party)==6&&hg_save_manager_validate_ram(field.saveData));
  unsigned species[6],levels[6],moves[6][4];
  for(unsigned i=0;i<6;i++){
   Pokemon *mon=Party_GetMonByIndex(party,i);species[i]=GetMonData(mon,MON_DATA_SPECIES,NULL);levels[i]=GetMonData(mon,MON_DATA_LEVEL,NULL);CHECK(levels[i]>=5&&levels[i]<=12&&GetMonData(mon,MON_DATA_HP,NULL)==GetMonData(mon,MON_DATA_MAX_HP,NULL));
   for(unsigned j=0;j<i;j++)CHECK(species[i]!=species[j]);
   for(unsigned j=0;j<4;j++){moves[i][j]=GetMonData(mon,MON_DATA_MOVE1+j,NULL);CHECK(moves[i][j]&&GetMonData(mon,MON_DATA_MOVE1_PP+j,NULL));}
  }
  for(unsigned i=0;i<6;i++){if(species[i]==492)favorites|=1;if(species[i]==491)favorites|=2;if(species[i]==115)favorites|=4;}
  for(unsigned i=0;i<index;i++)CHECK(memcmp(rosters[i],species,sizeof(species)));memcpy(rosters[index],species,sizeof(species));
  CHECK(hg_field_app_inspect(app,2));char text[4096];
  for(unsigned i=0;i<6;i++){
   hg_field_app_inspect_text(app,text,sizeof(text));CHECK(strstr(text,"6 in party")&&strstr(text,hg_pokemon_species_name(species[i])));
   unsigned pp=0;for(char *at=text;(at=strstr(at," PP "));at+=4)pp++;CHECK(pp==4);
   if(i<5){hg_field_app_input(app,0x80);hg_field_app_step(app);hg_field_app_clear_input(app);}
  }
  Party before=*party;unsigned long long revision=hg_story_revision(hg_field_app_story(app));
  if(index==0){
   hg_field_app_inspect(app,0);CHECK(hg_field_app_inspect(app,2));
   u32 egg=1;for(unsigned i=0;i<6;i++)SetMonData(Party_GetMonByIndex(party,i),MON_DATA_IS_EGG,&egg);Party eggs=*party;
   menu_press(app,0,1);menu_press(app,0x80,0);menu_press(app,0,1);hg_field_app_inspect_text(app,text,sizeof(text));
   CHECK(strstr(text,"Couldn't switch")&&!memcmp(party,&eggs,sizeof(eggs))&&hg_story_revision(hg_field_app_story(app))==revision);
   *party=before;hg_field_app_inspect(app,0);CHECK(hg_field_app_inspect(app,2));
  }
  menu_press(app,0,1);CHECK(!memcmp(party,&before,sizeof(before)));
  hg_field_app_inspect(app,0);CHECK(!memcmp(party,&before,sizeof(before))&&hg_story_revision(hg_field_app_story(app))==revision);
  CHECK(hg_field_app_inspect(app,2));menu_press(app,0,1);menu_press(app,0,1);CHECK(!memcmp(party,&before,sizeof(before))&&hg_story_revision(hg_field_app_story(app))==revision);
  menu_press(app,0,1);menu_press(app,0x80,0);menu_press(app,0,1);
  Party expected=before;Party_SwapSlots(&expected,0,1);CHECK(!memcmp(party,&expected,sizeof(expected)));
  unsigned follower;CHECK(hg_field_app_follower(app,&follower,NULL,NULL)&&follower==species[1]);
  unsigned temporary=species[0];species[0]=species[1];species[1]=temporary;temporary=levels[0];levels[0]=levels[1];levels[1]=temporary;
  for(unsigned j=0;j<4;j++){temporary=moves[0][j];moves[0][j]=moves[1][j];moves[1][j]=temporary;}
  hg_field_app_inspect(app,0);
  if(index==0){menu_press(app,0x20,0);for(unsigned i=0;i<16;i++)hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);}
  char path[256];
  if(index==5){snprintf(path,sizeof(path),"%s",hg_field_app_save_path(app));CHECK(strcmp(path,"build/checkpoints/155-violet-arrival.sav"));CHECK(file_hash(path)==original_hash);}
  else{snprintf(path,sizeof(path),"/tmp/hg-story-team-XXXXXX");int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));}
  CHECK(hg_field_app_save(app,path,error,sizeof(error)));hg_field_app_free(app);app=hg_field_app_continue(path,error,sizeof(error));CHECK(app);hg_story_bind_field(hg_field_app_story(app),&field);party=hg_story_save_party(field.saveData);CHECK(Party_GetCount(party)==6);
  for(unsigned i=0;i<6;i++){Pokemon *mon=Party_GetMonByIndex(party,i);CHECK(GetMonData(mon,MON_DATA_SPECIES,NULL)==species[i]&&GetMonData(mon,MON_DATA_LEVEL,NULL)==levels[i]);for(unsigned j=0;j<4;j++)CHECK(GetMonData(mon,MON_DATA_MOVE1+j,NULL)==moves[i][j]);}
  CHECK(hg_field_app_follower(app,&follower,NULL,NULL)&&follower==species[0]);
  hg_field_app_free(app);CHECK(!unlink(path));
 }
 CHECK(favorites==7);
 CHECK(file_hash("build/checkpoints/155-violet-arrival.sav")==original_hash);
 HGFieldApp *original=hg_field_app_checkpoint_load("build/checkpoints/155-5.hgcheckpoint",error,sizeof(error));CHECK(original);FieldSystem field={0};hg_story_bind_field(hg_field_app_story(original),&field);CHECK(Party_GetCount(hg_story_save_party(field.saveData))==1);hg_field_app_free(original);
 puts("Six story teams: full parties/moves, original-party isolation, panel inspection and durable Save/Continue pass");
}
static void rival(HGFieldApp *app,unsigned expected){
    hg_field_app_input(app,0x10);hg_field_app_step(app);hg_field_app_clear_input(app);unsigned seen=0,done=0;
    for(unsigned tick=0;tick<16000;tick++){
        hg_field_app_step(app);if(hg_field_app_error(app)[0])fprintf(stderr,"Checkpoint rival: %s\n",hg_field_app_error(app));CHECK(!hg_field_app_error(app)[0]);
        if(hg_field_app_battle_active(app)){seen++;CHECK(hg_field_app_battle_trainer(app));hg_test_battle_input(app);}
        else if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);
        if(seen&&!hg_field_app_interaction_busy(app)&&!hg_field_app_dialogue(app)[0]){done=1;break;}
    }
    CHECK(seen&&done);uint16_t outcome;CHECK(hg_story_var_get(hg_field_app_story(app),VAR_BATTLE_RESULT,&outcome));if(outcome!=expected)fprintf(stderr,"Rival outcome %u, expected %u\n",outcome,expected);CHECK(outcome==expected);
    int hidden;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_HIDE_CHERRYGROVE_RIVAL,&hidden)&&hidden);CHECK(hg_field_app_follower(app,NULL,NULL,NULL));
}
int main(void){
    story_teams();
    char path[256],error[256];const unsigned starters[]={152,155,158};
    for(unsigned i=0;i<3;i++){
        snprintf(path,sizeof(path),"build/checkpoints/%u-1.hgcheckpoint",starters[i]);HGFieldApp *route=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(route);hg_test_portal(route,67);hg_test_settle(route);hg_test_walk_to(route,565,398);hg_test_settle(route);CHECK(hg_field_app_running_shoes(route));uint16_t tour;CHECK(hg_story_var_get(hg_field_app_story(route),VAR_SCENE_CHERRYGROVE_CITY_OW,&tour)&&tour==1);hg_field_app_free(route);

        snprintf(path,sizeof(path),"build/checkpoints/%u-2.hgcheckpoint",starters[i]);HGFieldApp *app=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);rival(app,1);hg_field_app_free(app);
        snprintf(path,sizeof(path),"build/checkpoints/%u-3.hgcheckpoint",starters[i]);app=hg_field_app_checkpoint_load(path,error,sizeof(error));CHECK(app);hg_test_walk_to(app,574,398);hg_test_settle(app);hg_field_app_input(app,0x10);for(unsigned tick=0;tick<60;tick++)hg_field_app_step(app);hg_field_app_clear_input(app);CHECK(!hg_field_app_battle_active(app)&&!hg_field_app_interaction_busy(app));hg_field_app_free(app);
    }
    HGFieldApp *loss=hg_field_app_checkpoint_load("build/checkpoints/152-2.hgcheckpoint",error,sizeof(error));CHECK(loss);FieldSystem field={0};hg_story_bind_field(hg_field_app_story(loss),&field);Party *party=hg_story_save_party(field.saveData);
    Pokemon *mon=Party_GetMonByIndex(party,0);u32 hp=1;SetMonData(mon,MON_DATA_HP,&hp);MonSetMoveInSlot(mon,MOVE_GROWL,0);rival(loss,2);CHECK(GetMonData(mon,MON_DATA_HP,NULL)==GetMonData(mon,MON_DATA_MAX_HP,NULL));CHECK(GetMonData(mon,MON_DATA_MOVE1_PP,NULL)>0);hg_field_app_free(loss);
    FILE *file=fopen("build/checkpoints/152-2.hgcheckpoint","rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long size=ftell(file);CHECK(size>40);rewind(file);uint8_t *bytes=malloc((size_t)size);CHECK(bytes&&fread(bytes,(size_t)size,1,file)==1);fclose(file);
    bytes[size-1]^=1;file=fopen("build/checkpoints/damaged.hgcheckpoint","wb");CHECK(file&&fwrite(bytes,(size_t)size,1,file)==1);fclose(file);free(bytes);
    CHECK(!hg_field_app_checkpoint_load("build/checkpoints/damaged.hgcheckpoint",error,sizeof(error)));CHECK(error[0]);
    puts("Loaded checkpoints: all starters battle rival, post-rival does not retrigger, loss heals/resumes, damaged state rejected");return 0;
}
