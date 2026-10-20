#include "field_script.h"
#include "constants/flags.h"
#include "constants/sndseq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
static int has(HGStoryState *owner,unsigned flag){int value;CHECK(hg_story_flag_get(owner,flag,&value));return value;}
int main(void) {
    const uint8_t script[]={
        H(30),H(FLAG_GOT_BAG),H(78),H(SEQ_SE_PL_KIRAKIRA),H(79),
        H(30),H(FLAG_GOT_TRAINER_CARD),
        H(30),H(FLAG_GOT_SAVE_BUTTON),
        H(30),H(FLAG_GOT_OPTIONS_BUTTON),H(2)};
    const unsigned flags[]={FLAG_GOT_BAG,FLAG_GOT_TRAINER_CARD,FLAG_GOT_SAVE_BUTTON,FLAG_GOT_OPTIONS_BUTTON};
    const char *names[]={"Bag","Trainer Card","Save button","Options button"};
    HGStoryState *owner=hg_story_new();CHECK(owner);char error[256];
    uint64_t revision=hg_story_revision(owner);
    HGFieldScript *vm=hg_field_script_bytes(owner,script,sizeof(script),0,0,error,sizeof(error));CHECK(vm);
    CHECK(hg_story_revision(owner)==revision&&!has(owner,FLAG_GOT_BAG));
    CHECK(hg_field_script_tick(vm)==HG_SCRIPT_MESSAGE);
    CHECK(strstr(hg_field_script_message(vm)->text,"Bag"));
    for(unsigned i=0;i<60;i++)CHECK(hg_field_script_tick(vm)==HG_SCRIPT_MESSAGE);
    hg_field_script_free(vm);CHECK(!has(owner,FLAG_GOT_BAG));
    for(unsigned replay=0;replay<2;replay++) {
        vm=hg_field_script_bytes(owner,script,sizeof(script),0,0,error,sizeof(error));CHECK(vm);
        unsigned seen=0,ticks=0;int status;
        do {
            status=hg_field_script_tick(vm);CHECK(status!=HG_SCRIPT_ERROR);
            if(status==HG_SCRIPT_MESSAGE){CHECK(!replay&&seen<4);const HGScriptMessage *message=hg_field_script_message(vm);CHECK(message&&strstr(message->text,names[seen]));
                CHECK(!has(owner,flags[seen]));seen++;CHECK(hg_field_script_ack(vm));}
        }while(status!=HG_SCRIPT_DONE&&++ticks<1000);
        CHECK(status==HG_SCRIPT_DONE&&seen==(replay?0:4));CHECK(hg_field_script_commit(vm));hg_field_script_free(vm);
        for(unsigned i=0;i<4;i++)CHECK(has(owner,flags[i]));
    }
    hg_story_free(owner);puts("Unlock receipts: live flag transitions, fanfare, acknowledgment, cancellation, preflight isolation and no duplicate grants passed");
}
