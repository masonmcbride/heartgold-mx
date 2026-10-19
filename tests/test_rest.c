#include "compat/pokemon.h"
#include "field_script.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
static void complete(HGFieldScript *s) { int result=HG_SCRIPT_RUNNING;for(unsigned i=0;i<32&&result==HG_SCRIPT_RUNNING;i++)result=hg_field_script_tick(s);CHECK(result==HG_SCRIPT_DONE);CHECK(hg_field_script_commit(s)); }
int main(void) {
    HGStoryState *live=hg_story_new();CHECK(live);FieldSystem field={0};hg_story_bind_field(live,&field);
    Pokemon mon;CreateMon(&mon,152,5,32,FALSE,0,OT_ID_PLAYER_ID,0);CHECK(Party_AddMon(hg_story_save_party(field.saveData),&mon));
    Pokemon *member=Party_GetMonByIndex(hg_story_save_party(field.saveData),0);u32 hp=1,status=8;u8 pp=0;
    SetMonData(member,MON_DATA_HP,&hp);SetMonData(member,MON_DATA_STATUS,&status);SetMonData(member,MON_DATA_MOVE1_PP,&pp);
    const uint8_t rejected[]={H(282),H(291),H(573),H(65535)};
    const uint8_t program[]={H(282),H(291),H(573),H(2)};char error[256];
    CHECK(!hg_field_script_bytes(live,rejected,sizeof(rejected),0,0,error,sizeof(error)));
    CHECK(GetMonData(member,MON_DATA_HP,NULL)==1&&GetMonData(member,MON_DATA_STATUS,NULL)==8);
    CHECK(!Pokedex_IsEnabled(hg_story_save_dex(field.saveData))&&!Save_SysInfo_GetMysteryGiftActive(hg_story_save_sys_info(field.saveData)));
    HGFieldScript *script=hg_field_script_bytes(live,program,sizeof(program),0,0,error,sizeof(error));CHECK(script);
    CHECK(GetMonData(member,MON_DATA_HP,NULL)==1&&!Pokedex_IsEnabled(hg_story_save_dex(field.saveData)));
    HGStoryState *copy=hg_story_clone(live);CHECK(copy);complete(script);hg_field_script_free(script);
    CHECK(GetMonData(member,MON_DATA_HP,NULL)==GetMonData(member,MON_DATA_MAX_HP,NULL));
    CHECK(!GetMonData(member,MON_DATA_STATUS,NULL)&&GetMonData(member,MON_DATA_MOVE1_PP,NULL)==35);
    CHECK(Pokedex_IsEnabled(hg_story_save_dex(field.saveData))&&Save_SysInfo_GetMysteryGiftActive(hg_story_save_sys_info(field.saveData))==1);
    FieldSystem private_field={0};hg_story_bind_field(copy,&private_field);
    CHECK(GetMonData(Party_GetMonByIndex(hg_story_save_party(private_field.saveData),0),MON_DATA_HP,NULL)==1);
    CHECK(!Pokedex_IsEnabled(hg_story_save_dex(private_field.saveData))&&!Save_SysInfo_GetMysteryGiftActive(hg_story_save_sys_info(private_field.saveData)));
    script=hg_field_script_bytes(copy,program,sizeof(program),0,0,error,sizeof(error));CHECK(script);
    CHECK(hg_story_profile_set(copy,"Gold",0));CHECK(hg_field_script_tick(script)==HG_SCRIPT_ERROR);CHECK(!hg_field_script_commit(script));hg_field_script_free(script);
    CHECK(!Pokedex_IsEnabled(hg_story_save_dex(private_field.saveData)));
    hg_story_free(copy);hg_story_free(live);
    puts("Original rest, Pokedex and Mystery Gift: preflight isolation, publication, clones and stale rejection passed");return 0;
}
