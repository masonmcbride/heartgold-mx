/* Separate service fixtures: never a gameplay-produced trainer victory. */
#include "compat/story.h"
#include "compat/save_manager.h"
#include "compat/pokemon.h"
#include "compat/phone.h"
#include "gear_phone.h"
#include "ds_save_flash.h"
#include "trainer_data.h"
#include "field_player.h"
#include "compat/motion.h"
#include "save_local_field_data.h"
#include "compat/overlays.h"
#include "compat/trainer_message.h"
#include "compat/resources.h"
#include "constants/vars.h"
#include "constants/flags.h"
#include "constants/phone_contacts.h"
#include "constants/std_script.h"
#include "constants/sndseq.h"
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
static void trainer_messages(void) {
 const u8 *table;size_t size;CHECK(native_resource_member(NARC_poketool_trmsg_trtbl,0,0,&table,&size)&&size%4==0);
 unsigned covered=0;
 for(unsigned i=0;i<size;i+=4) {
  unsigned trainer=table[i]|table[i+1]<<8,kind=table[i+2]|table[i+3]<<8;
  if(trainer<FIRST_TRAINER_INDEX||trainer>=LAST_TRAINER_INDEX)continue;
  HGTrainerMessageRef ref;CHECK(hg_trainer_message_ref(trainer,kind,&ref)&&ref.found&&ref.bank==728&&ref.message==i/4);covered++;
 }
 CHECK(covered>100);
 HGTrainerMessageRef ref={123,456,1},before=ref;
 CHECK(!hg_trainer_message_ref(LAST_TRAINER_INDEX,0,&ref)&&!memcmp(&ref,&before,sizeof(ref)));
 CHECK(hg_trainer_message_ref(51,65535,&ref)&&!ref.found);
 HGStoryState *owner=hg_story_new();char error[256];
 const u8 message[]={H(52),H(HG_SCRIPT_TRAINER_MESSAGE),H(51),H(TRMSG_INTRO),H(49),H(53),H(2)};
 HGFieldScript *script=hg_field_script_bytes(owner,message,sizeof(message),0,0,error,sizeof(error));CHECK(script);
 CHECK(hg_field_script_tick(script)==HG_SCRIPT_MESSAGE);
 CHECK(strlen(hg_field_script_message(script)->text)>20&&hg_field_script_ack(script));
 CHECK(hg_field_script_tick(script)==HG_SCRIPT_DONE&&hg_field_script_commit(script));
 hg_field_script_free(script);hg_story_free(owner);
}
static void finish_scene(HGFieldPlayer *player) {
 char text[4096];int state=HG_SCRIPT_RUNNING;
 for(unsigned i=0;i<256&&state!=HG_SCRIPT_DONE&&state!=HG_SCRIPT_ERROR;i++){state=hg_field_player_interaction_tick(player,text,sizeof(text));if(state==HG_SCRIPT_MESSAGE)CHECK(hg_field_player_interaction_ack(player));}
 if(state!=HG_SCRIPT_DONE)fprintf(stderr,"Scene: %s\n",hg_field_player_interaction_error(player));
 CHECK(state==HG_SCRIPT_DONE);
}
static void actor_services(void) {
 HGFieldPlayer *player=hg_field_player_new_map(155);CHECK(player);
 HGStoryState *story=hg_field_player_story(player);u16 value;
 /* Nico and Edmond have different object IDs and original facing movements. */
 const u8 query[]={H(574),H(VAR_TEMP_x4000),H(0),H(574),H(VAR_TEMP_x4001),H(1),H(574),H(VAR_TEMP_x4002),H(63),H(2)};
 CHECK(hg_field_player_start_bytes(player,query,sizeof(query),0));
 CHECK(hg_story_var_get(story,VAR_TEMP_x4001,&value)&&!value);finish_scene(player);
 CHECK(hg_story_var_get(story,VAR_TEMP_x4000,&value)&&value==15);
 CHECK(hg_story_var_get(story,VAR_TEMP_x4001,&value)&&value==16);
 CHECK(hg_story_var_get(story,VAR_TEMP_x4002,&value)&&!value);
 const u8 rejected[]={H(109),H(1),H(14),H(109),H(1),H(51),H(2)};
 unsigned long long revision=hg_story_revision(story);
 CHECK(!hg_field_player_start_bytes(player,rejected,sizeof(rejected),0));
 CHECK(hg_story_revision(story)==revision);
 CHECK(hg_field_player_start_bytes(player,query,sizeof(query),0));finish_scene(player);
 CHECK(hg_story_var_get(story,VAR_TEMP_x4001,&value)&&value==16);
 const u8 reset[]={H(109),H(1),H(17),H(574),H(VAR_TEMP_x4003),H(1),H(454),H(2)};
 CHECK(hg_field_player_start_bytes(player,reset,sizeof(reset),0));finish_scene(player);
 CHECK(hg_story_var_get(story,VAR_TEMP_x4003,&value)&&value==17&&!hg_overlay_live());
 const u8 greeting[]={H(HG_SCRIPT_ENCOUNTER_MUSIC),H(51),H(HG_SCRIPT_TRAINER_MESSAGE),H(51),H(TRMSG_INTRO),H(49),H(53),H(2)};
 HGStoryMusic music=hg_story_music(story);
 if(!hg_field_player_start_bytes(player,greeting,sizeof(greeting),0)){fprintf(stderr,"Trainer greeting: %s\n",hg_field_player_interaction_error(player));exit(1);}
 CHECK(hg_story_music(story).sequence==music.sequence);finish_scene(player);
 CHECK(hg_story_music(story).sequence==SEQ_GS_EYE_BOUZU);
 HGStoryState *saved=hg_story_clone(story);CHECK(saved&&hg_field_player_prepare_save(player,saved));
 FieldSystem field={0};hg_story_bind_field(saved,&field);
 char path[]="/tmp/hg-trainer-actors-XXXXXX",error[256];int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));
 CHECK(ds_save_flash_open(path)&&hg_save_manager_write(field.saveData,error,sizeof(error)));ds_save_flash_close();
 SaveData *records=malloc(sizeof(*records));CHECK(records&&ds_save_flash_open_existing(path)&&hg_save_manager_load(records,error,sizeof(error)));ds_save_flash_close();
 HGStoryState *loaded=hg_story_from_save_records(records);free(records);CHECK(loaded&&!unlink(path));
 hg_story_free(saved);saved=loaded;hg_story_bind_field(saved,&field);
 Location *where=LocalFieldData_GetCurrentPosition(SaveArray_Get(field.saveData,SAVE_LOCAL_FIELD_DATA));
 HGFieldLocation at={where->mapId,where->warpId,where->x,where->y,where->direction};
 HGFieldPlayer *resumed=hg_field_player_resume(saved,&at,0);CHECK(resumed&&hg_field_player_restore_save_objects(resumed));
 CHECK(hg_field_player_start_bytes(resumed,query,sizeof(query),0));finish_scene(resumed);
 CHECK(hg_story_var_get(hg_field_player_story(resumed),VAR_TEMP_x4001,&value)&&value==17);
 hg_field_player_free(resumed);hg_story_free(saved);hg_field_player_free(player);
}
int main(void){
 actor_services();trainer_messages();
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
 /* Lookup metadata survives preflight replay without creating a phone app.
  * A missing trainer must clear a previous caller instead of leaking it. */
 unsigned count;const PhoneBookEntry *entries=hg_phone_entries(&count);CHECK(count);
 unsigned caller=0;while(caller<count&&!entries[caller].trainerId)caller++;CHECK(caller<count);
 const u8 phone[]={H(460),H(entries[caller].trainerId),H(VAR_TEMP_x4002),H(460),H(51),H(VAR_TEMP_x4003),H(2)};
 script=hg_field_script_bytes(owner,phone,sizeof(phone),0,0,error,sizeof(error));CHECK(script);hg_field_script_free(script);CHECK(hg_story_var_get(owner,VAR_TEMP_x4002,&value)&&value==0);
 finish(hg_field_script_bytes(owner,phone,sizeof(phone),0,0,error,sizeof(error)));
 CHECK(hg_story_var_get(owner,VAR_TEMP_x4002,&value)&&value==entries[caller].id);
 CHECK(hg_story_var_get(owner,VAR_TEMP_x4003,&value)&&value==PHONE_CONTACT_NONE);
 const u8 bad_phone[]={H(460),H(51),H(1),H(2)};CHECK(!hg_field_script_bytes(owner,bad_phone,sizeof(bad_phone),0,0,error,sizeof(error))&&error[0]);
 char path[]="/tmp/hg-trainer-flags-XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);close(fd);CHECK(!unlink(path));CHECK(ds_save_flash_open(path));CHECK(hg_save_manager_write(field.saveData,error,sizeof(error)));ds_save_flash_close();
 SaveData *restored=malloc(sizeof(*restored));CHECK(restored&&ds_save_flash_open_existing(path));CHECK(hg_save_manager_load(restored,error,sizeof(error)));ds_save_flash_close();CHECK(TrainerFlagCheck(restored,51)&&!TrainerFlagCheck(restored,20));free(restored);CHECK(!unlink(path));hg_story_free(owner);
 puts("Original trainer identity/attributes, private flag cancellation, stale rejection, flag predicates and durable reopen pass");return 0;
}
