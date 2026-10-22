/* Separate service fixtures: never a gameplay-produced trainer victory. */
#include "compat/story.h"
#include "compat/save_manager.h"
#include "compat/pokemon.h"
#include "ds_save_flash.h"
#include "trainer_data.h"
#include "constants/vars.h"
#include "constants/flags.h"
#include "constants/std_script.h"
#include <unistd.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
#define W(n) H(n),H((n)>>16)
u16 ScriptNumToTrainerNum(u16);
BOOL TrainerFlagCheck(SaveData *,u32);
BOOL TrainerNumIsDouble(u32);
static void finish(HGFieldScript *script) {
 CHECK(script);int status=HG_SCRIPT_RUNNING;for(unsigned i=0;i<100&&status==HG_SCRIPT_RUNNING;i++)status=hg_field_script_tick(script);CHECK(status==HG_SCRIPT_DONE&&hg_field_script_commit(script));hg_field_script_free(script);
}
int main(void){
 CHECK(ScriptNumToTrainerNum(3050)==51&&ScriptNumToTrainerNum(5050)==51&&ScriptNumToTrainerNum(3019)==20);
 CHECK(!TrainerNumIsDouble(51)&&TrainerData_GetAttr(51,TRATTR_NPOKE)==3&&TrainerData_GetAttr(51,TRATTR_CLASS)==TRAINERCLASS_SAGE);
 CHECK(!TrainerNumIsDouble(20)&&TrainerData_GetAttr(20,TRATTR_NPOKE)==2);
 HGStoryState *owner=hg_story_new();CHECK(owner);FieldSystem field={0};hg_story_bind_field(owner,&field);CHECK(!TrainerFlagCheck(field.saveData,51));char error[256];
 const u8 set[]={H(36),H(51),H(2)};HGFieldScript *script=hg_field_script_bytes(owner,set,sizeof(set),0,0,error,sizeof(error));CHECK(script&&!TrainerFlagCheck(field.saveData,51));hg_field_script_free(script);CHECK(!TrainerFlagCheck(field.saveData,51));
 script=hg_field_script_bytes(owner,set,sizeof(set),0,0,error,sizeof(error));CHECK(script);CHECK(hg_story_var_set(owner,VAR_TEMP_x4000,7));while(hg_field_script_tick(script)==HG_SCRIPT_RUNNING){}CHECK(!hg_field_script_commit(script)&&!TrainerFlagCheck(field.saveData,51));hg_field_script_free(script);
 finish(hg_field_script_bytes(owner,set,sizeof(set),0,0,error,sizeof(error)));CHECK(TrainerFlagCheck(field.saveData,51));
 const u8 check[]={H(38),H(51),H(28),1,W(6),H(41),H(VAR_TEMP_x4001),H(99),H(2)};
 finish(hg_field_script_bytes(owner,check,sizeof(check),0,0,error,sizeof(error)));u16 value;CHECK(hg_story_var_get(owner,VAR_TEMP_x4001,&value)&&value==0);
 const u8 invalid[]={H(36),H(LAST_TRAINER_INDEX),H(2)};CHECK(!hg_field_script_bytes(owner,invalid,sizeof(invalid),0,0,error,sizeof(error))&&error[0]);
 const u8 no_context[]={H(212),H(VAR_TEMP_x4000),H(2)};CHECK(!hg_field_script_bytes(owner,no_context,sizeof(no_context),0,0,error,sizeof(error))&&error[0]);
 char path[]="/tmp/hg-trainer-flags-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));CHECK(ds_save_flash_open(path));CHECK(hg_save_manager_write(field.saveData,error,sizeof(error)));ds_save_flash_close();
 SaveData *restored=malloc(sizeof(*restored));CHECK(restored&&ds_save_flash_open_existing(path));CHECK(hg_save_manager_load(restored,error,sizeof(error)));ds_save_flash_close();CHECK(TrainerFlagCheck(restored,51)&&!TrainerFlagCheck(restored,20));free(restored);CHECK(!unlink(path));hg_story_free(owner);
 puts("Original trainer identity/attributes, private flag cancellation, stale rejection, flag predicates and durable reopen pass");return 0;
}
