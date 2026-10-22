#include "compat/battle.h"
#include "field_app.h"
#include "sys_flags.h"
#include "save_pokegear.h"
#include "constants/vars.h"
#include <stdio.h>
#include <unistd.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,app?hg_field_app_error(app):"");exit(1);} } while(0)
#include "field_routes.h"
static void durable(HGFieldApp **owner) {
 HGFieldApp *app=*owner;char path[]="/tmp/hg-violet-XXXXXX",error[256];if(hg_field_app_save_path(app)[0])snprintf(path,sizeof(path),"%s",hg_field_app_save_path(app));
 else {int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));}
 int saved=hg_field_app_save(app,path,error,sizeof(error));if(!saved)fprintf(stderr,"Save: %s\n",error);CHECK(saved);HGFieldApp *continued=hg_field_app_continue(path,error,sizeof(error));if(!continued)fprintf(stderr,"Continue: %s\n",error);CHECK(continued);
 CHECK(hg_field_app_map(continued)==hg_field_app_map(app));
 DSCamera camera;u8 pixels[256*192*4];hg_field_app_camera(continued,&camera,256,192);CHECK(hg_field_app_render(continued,&camera,pixels,256,192));hg_field_app_free(app);*owner=continued;
}
static void scenario(unsigned savings,int phone_only) {
 const unsigned species=155;
 char path[256],error[256];snprintf(path,sizeof(path),"build/checkpoints/%u-5.hgcheckpoint",species);
 HGFieldApp *app=hg_field_app_checkpoint_load(path,error,sizeof(error));if(!app)fprintf(stderr,"%s\n",error);CHECK(app);
 const unsigned route[]={67,34,35,97};
 for(unsigned i=0;i<sizeof(route)/sizeof(*route);i++) {
  hg_test_portal(app,route[i]);hg_test_settle(app);
  if(route[i]==34){
   hg_test_walk_to(app,553,293);hg_test_settle(app);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);for(unsigned tick=1;tick<8;tick++)hg_field_app_step(app);
   unsigned menu=0;
   for(unsigned tick=0;tick<2048;tick++){
    hg_field_app_step(app);CHECK(!hg_field_app_error(app)[0]);
    if(hg_field_app_menu_pending(app)){menu=1;break;}
    if(hg_field_app_dialogue(app)[0])hg_field_app_interact(app);
   }
   CHECK(menu);int published;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_TALKED_TO_MOM_AFTER_NAMING_RIVAL,&published)&&!published);
   CHECK(!hg_field_app_save(app,"/tmp/hg-violet-pending",error,sizeof(error))&&error[0]);
   DSCamera camera;u8 pixels[256*192*4];hg_field_app_camera(app,&camera,256,192);CHECK(hg_field_app_render(app,&camera,pixels,256,192));
   hg_test_settle_choice(app,savings?0:1);
   FieldSystem phone={0};hg_story_bind_field(hg_field_app_story(app),&phone);
   CHECK(Save_VarsFlags_MomsSavingsFlagCheck(SaveArray_Get(phone.saveData,SAVE_FLAGS))==savings);
   durable(&app);hg_story_bind_field(hg_field_app_story(app),&phone);
   CHECK(Save_VarsFlags_MomsSavingsFlagCheck(SaveArray_Get(phone.saveData,SAVE_FLAGS))==savings);
   if(phone_only){char temporary[256];snprintf(temporary,sizeof(temporary),"%s",hg_field_app_save_path(app));hg_field_app_free(app);app=NULL;CHECK(!unlink(temporary));return;}
  }
 }
 int flag;CHECK(hg_story_flag_get(hg_field_app_story(app),FLAG_TALKED_TO_MOM_AFTER_NAMING_RIVAL,&flag)&&flag);
 hg_test_walk_to(app,3,7);hg_test_settle(app);
 u16 value;CHECK(hg_story_var_get(hg_field_app_story(app),VAR_UNK_4132,&value)&&value==1);
 FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);CHECK(Bag_GetQuantity(hg_story_save_bag(field.saveData),ITEM_VS__RECORDER,HEAP_ID_FIELD1)==1);
 durable(&app);hg_test_portal(app,73);hg_test_settle(app);durable(&app);
 CHECK(!hg_field_app_error(app)[0]);CHECK(hg_story_var_get(hg_field_app_story(app),VAR_UNK_4132,&value)&&value==1);
 printf("Starter %u: original Mom call, gatehouse Recorder gift/departure, gatehouse and Violet Save/Continue passed\n",species);fflush(stdout);
 char saved_path[256];snprintf(saved_path,sizeof(saved_path),"%s",hg_field_app_save_path(app));hg_field_app_free(app);app=NULL;
 {snprintf(path,sizeof(path),"build/checkpoints/%u-violet-arrival.sav",species);CHECK(!rename(saved_path,path));app=hg_field_app_continue(path,error,sizeof(error));CHECK(app&&hg_field_app_map(app)==73);hg_field_app_free(app);}
}
int main(int argc,char **argv) {
 if(argc==2&&!strcmp(argv[1],"--probe")){
  char error[256];HGFieldApp *app=hg_field_app_continue("build/checkpoints/155-violet-arrival.sav",error,sizeof(error));CHECK(app);
  hg_test_portal(app,110);hg_test_settle(app);hg_test_portal(app,155);hg_test_settle(app);
  hg_test_approach(app,3050);hg_field_app_interact(app);hg_field_app_step(app);hg_test_settle(app);hg_field_app_free(app);return 0;
 }
 if(argc!=1)return 2;scenario(0,0);scenario(1,1);return 0;
}
