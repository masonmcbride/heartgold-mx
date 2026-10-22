#include "compat/battle.h"
#include "compat/misc.h"
#include "field_app.h"
#include "game_name.h"
#include "compat/naming.h"
#include "constants/vars.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,app?hg_field_app_error(app):"");exit(1);}}while(0)
#include "field_routes.h"
static void scenario(unsigned species){
 char error[256],path[256];snprintf(path,sizeof(path),"build/checkpoints/%u-3.hgcheckpoint",species);HGFieldApp *app=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);
 hg_test_portal(app,33);hg_test_portal(app,60);hg_test_portal(app,61);
 unsigned named=0,done=0,confirmed=0;
 for(unsigned i=0;i<16000;i++){
  hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);
  if(strstr(hg_field_app_dialogue(app),"Silver"))confirmed=1;
  if(hg_field_app_name_pending(app)){
   FieldSystem pending={0};hg_story_bind_field(hg_field_app_story(app),&pending);CHECK(hg_story_save_misc(pending.saveData)->rivalName[0]==0xffff);
   CHECK(!hg_field_app_name_edit(app,"TooLongName"));CHECK(!hg_field_app_name_submit(app,"",0));CHECK(!hg_field_app_name_edit(app,"😀"));
   CHECK(hg_field_app_name_edit(app,"Silver"));CHECK(!strcmp(hg_field_app_name_text(app),"Silver"));
   CHECK(hg_field_app_name_submit(app,"Silver",named==0));named++;
  }
  else if(hg_field_app_menu_pending(app)){CHECK(hg_field_app_menu_choice_supported(app,0));hg_field_app_clear_input(app);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);hg_field_app_interact(app);}
  else if(hg_field_app_dialogue(app)[0]){hg_field_app_interact(app);}
  if(named>=2&&!hg_field_app_interaction_busy(app)){done=1;break;}
 }
 CHECK(named==2&&done&&confirmed);int flag;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_GAVE_RIVAL_NAME_TO_OFFICER,&flag)&&flag);
 uint16_t scene;CHECK(hg_story_var_get(hg_field_app_story(app),VAR_SCENE_ELMS_LAB,&scene)&&scene==4);
 FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);char name[64];CHECK(hg_name_decode(hg_story_save_misc(field.saveData)->rivalName,8,name,sizeof(name))&&!strcmp(name,"Silver"));
 CHECK(!Bag_HasItem(hg_story_save_bag(field.saveData),ITEM_MYSTERY_EGG,1,HEAP_ID_FIELD1));
 CHECK(hg_field_app_follower(app,NULL,NULL,NULL));
 hg_test_portal(app,60);hg_test_portal(app,61);hg_test_settle(app);CHECK(!hg_field_app_name_pending(app));
 hg_field_app_free(app);printf("Starter %u: original police, cancel/retry, rival name, Egg handoff, re-entry passed\n",species);
}

int main(void){
 for(unsigned i=0;i<3;i++)scenario((unsigned[]){152,155,158}[i]);
 HGFieldApp *app=NULL;HGStoryState *story=hg_story_new();CHECK(story);FieldSystem field={0};hg_story_bind_field(story,&field);uint16_t result=99;
 HGNaming *n=hg_naming_new(&field,3,0,7,0,NULL,&result,0);CHECK(n);for(unsigned i=0;i<32&&!hg_naming_pending(n);i++)CHECK(hg_naming_tick(n)>=0);CHECK(hg_naming_pending(n));hg_naming_free(n);CHECK(result==1&&hg_story_save_misc(field.saveData)->rivalName[0]==0xffff);hg_story_free(story);
 puts("Pending original naming task cancellation unwound safely");return 0;
}
