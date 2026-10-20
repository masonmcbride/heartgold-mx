#include "compat/story.h"
#include "compat/phone.h"
#include "constants/flags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
static const uint8_t call[]={H(30),H(FLAG_GOT_ELMS_PANIC_CALL),H(430),H(1),H(2),H(0),H(431),H(2)};
static int flag(HGStoryState *owner) { int set;CHECK(hg_story_flag_get(owner,FLAG_GOT_ELMS_PANIC_CALL,&set));return set; }
static void reach_message(HGFieldScript *script,const char *name) {
    unsigned ticks=0;int status;do{status=hg_field_script_tick(script);CHECK(status!=HG_SCRIPT_ERROR);}while(status!=HG_SCRIPT_MESSAGE&&++ticks<128);
    CHECK(ticks<128&&hg_field_script_phone_active(script));const HGScriptMessage *message=hg_field_script_message(script);CHECK(message&&strstr(message->text,name)&&strstr(message->text,"disaster"));
}
int main(void) {
    CHECK(hg_phone_admit(1,2,0)&&hg_phone_admit(1,2,4));CHECK(!hg_phone_admit(0,2,0)&&!hg_phone_admit(1,0,0)&&!hg_phone_admit(1,2,5));
    for(unsigned gender=0;gender<2;gender++) {
        HGStoryState *owner=hg_story_new();CHECK(owner);CHECK(hg_story_profile_set(owner,gender?"Kris":"Ethan",gender));unsigned long long revision=hg_story_revision(owner);char error[256];
        HGFieldScript *script=hg_field_script_bytes(owner,call,sizeof(call),0,0,error,sizeof(error));CHECK(script&&!flag(owner));reach_message(script,gender?"Kris":"Ethan");
        for(unsigned i=0;i<12;i++)CHECK(hg_field_script_tick(script)==HG_SCRIPT_MESSAGE);
        CHECK(hg_story_revision(owner)==revision&&!flag(owner));hg_field_script_free(script);CHECK(!flag(owner)&&hg_story_revision(owner)==revision);
        uint8_t rejected[sizeof(call)];memcpy(rejected,call,sizeof(call));rejected[sizeof(call)-2]=255;rejected[sizeof(call)-1]=255;
        CHECK(!hg_field_script_bytes(owner,rejected,sizeof(rejected),0,0,error,sizeof(error))&&!flag(owner));
        script=hg_field_script_bytes(owner,call,sizeof(call),0,0,error,sizeof(error));CHECK(script);reach_message(script,gender?"Kris":"Ethan");
        CHECK(hg_story_profile_set(owner,gender?"Lyra":"Gold",gender));CHECK(hg_field_script_tick(script)==HG_SCRIPT_ERROR&&!hg_field_script_commit(script));hg_field_script_free(script);CHECK(!flag(owner));
        script=hg_field_script_bytes(owner,call,sizeof(call),0,0,error,sizeof(error));CHECK(script);int status=HG_SCRIPT_RUNNING;unsigned ticks=0;
        do{status=hg_field_script_tick(script);if(status==HG_SCRIPT_MESSAGE)CHECK(hg_field_script_ack(script));CHECK(status!=HG_SCRIPT_ERROR);CHECK(!flag(owner));}while(status!=HG_SCRIPT_DONE&&++ticks<512);
        CHECK(ticks<512&&!hg_field_script_phone_active(script)&&hg_field_script_commit(script)&&flag(owner));hg_field_script_free(script);hg_story_free(owner);
    }
    puts("Original phone messages/gender, application wait, private cancellation, preflight/stale rejection and publication passed");return 0;
}
