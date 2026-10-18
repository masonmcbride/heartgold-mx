#include "field_script.h"
#include "game_font.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "constants/sprites.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
#define W(n) H(n),H((n)>>16)
void SetLCRNGSeed(uint32_t);
uint32_t GetLCRNGSeed(void);
static int flag(HGStoryState *s,unsigned id) { int value;CHECK(hg_story_flag_get(s,id,&value));return value; }
static unsigned var(HGStoryState *s,unsigned id) { uint16_t value;CHECK(hg_story_var_get(s,id,&value));return value; }
static int finish(HGFieldScript *script) {
    CHECK(script);for(unsigned i=0;i<20000;i++){int status=hg_field_script_tick(script);if(status==HG_SCRIPT_DONE)CHECK(hg_field_script_commit(script));if(status!=HG_SCRIPT_RUNNING)return status;}CHECK(0);return -1;
}
int main(void) {
    SetLCRNGSeed(0x12345678);HGStoryState *a=hg_story_new(),*b=hg_story_new();CHECK(a&&b);
    CHECK(GetLCRNGSeed()==0x12345678);CHECK(hg_story_revision(a)==1);
    CHECK(var(a,VAR_LOTO_NUMBER_LO)==0xe97e&&!var(a,VAR_LOTO_NUMBER_HI)); /* Preserve original unpatched high-half bug. */
    CHECK(flag(a,FLAG_HIDE_ELMS_LAB_OFFICER)&&flag(a,FLAG_HIDE_NEW_BARK_MOM)&&flag(a,FLAG_HIDE_PLAYERS_ROOM_GOLD_TROPHY));
    CHECK(!flag(a,FLAG_HIDE_NEW_BARK_RIVAL)&&!flag(a,FLAG_GOT_STARTER)&&!flag(a,0));
    const unsigned maps[]={60,61,62,63,64,65,66,384};
    for(unsigned i=0;i<sizeof(maps)/sizeof(*maps);i++){unsigned entry=123;CHECK(hg_story_map_script(a,maps[i],2,&entry));CHECK(entry==(maps[i]==60?7:maps[i]==384?3:65535));}
    unsigned hook=123;CHECK(hg_story_map_script(a,63,1,&hook)&&hook==1); /* Selected, not executed. */
    const uint8_t fixed_header[]={2,H(7),H(0),0};
    for(size_t n=0;n<sizeof(fixed_header);n++)CHECK(!hg_story_header_script(a,fixed_header,n,2,&hook));
    CHECK(hg_story_header_script(a,fixed_header,sizeof(fixed_header),2,&hook)&&hook==7);
    uint8_t frame_header[]={1,W(1),0,H(VAR_SCENE_ELMS_LAB),H(0),H(2),H(0)};
    CHECK(hg_story_header_script(a,frame_header,sizeof(frame_header),1,&hook)&&hook==2);
    frame_header[6]=0;frame_header[7]=0x80;hook=123;
    CHECK(!hg_story_header_script(a,frame_header,sizeof(frame_header),1,&hook)&&hook==123);
    frame_header[1]=255;frame_header[2]=255;frame_header[3]=255;frame_header[4]=255;
    CHECK(!hg_story_header_script(a,frame_header,sizeof(frame_header),1,&hook));
    CHECK(hg_story_flag_set(a,TEMP_FLAG_BASE,1)&&flag(a,TEMP_FLAG_BASE)&&!flag(b,TEMP_FLAG_BASE));
    CHECK(hg_story_flag_set(b,TEMP_FLAG_BASE+1,1)&&flag(b,TEMP_FLAG_BASE+1)&&!flag(a,TEMP_FLAG_BASE+1));
    CHECK(hg_story_var_set(a,VAR_TEMP_x4000,65535)&&!var(b,VAR_TEMP_x4000));
    uint16_t untouched=4321;int unchanged=123;unsigned long long revision=hg_story_revision(a);
    CHECK(!hg_story_var_get(a,VAR_BASE+NUM_VARS,&untouched)&&untouched==4321);
    CHECK(!hg_story_flag_get(a,NUM_FLAGS,&unchanged)&&unchanged==123);
    CHECK(!hg_story_var_set(a,SPECIAL_VAR_BASE,1)&&!hg_story_flag_set(a,TEMP_FLAG_BASE+NUM_TEMP_FLAGS,1));
    CHECK(hg_story_revision(a)==revision);
    char error[160];
    const uint8_t branch[]={H(41),H(VAR_TEMP_x4000),H(65535),H(39),H(VAR_TEMP_x4000),H(1),
        H(17),H(VAR_TEMP_x4000),H(0),H(28),1,W(6),H(41),H(VAR_TEMP_x4001),H(99),H(30),H(FLAG_MAPTEMP_001),H(2)};
    HGFieldScript *s=hg_field_script_bytes(a,branch,sizeof(branch),0,542,error,sizeof(error));CHECK(s&&!error[0]);
    CHECK(var(a,VAR_TEMP_x4000)==65535&&!flag(a,FLAG_MAPTEMP_001));CHECK(finish(s)==HG_SCRIPT_DONE);
    CHECK(!var(a,VAR_TEMP_x4000)&&!var(a,VAR_TEMP_x4001)&&flag(a,FLAG_MAPTEMP_001));hg_field_script_free(s);
    const uint8_t arithmetic[]={H(0),H(41),H(VAR_TEMP_x4007),H(1),H(40),H(VAR_TEMP_x4007),H(2),
        H(42),H(VAR_TEMP_x4008),H(VAR_TEMP_x4007),H(43),H(VAR_TEMP_x4009),H(VAR_TEMP_x4008),H(43),H(VAR_TEMP_x400A),H(2),
        H(18),H(VAR_TEMP_x4008),H(VAR_TEMP_x4009),H(28),1,W(6),H(41),H(VAR_TEMP_x400A),H(99),
        H(30),H(FLAG_MAPTEMP_003),H(31),H(FLAG_MAPTEMP_003),H(11),0,1,H(28),1,W(6),H(41),H(VAR_TEMP_x400A),H(99),
        H(12),0,1,H(29),0,W(2),H(2),H(39),H(VAR_TEMP_x400A),H(1),H(27)};
    s=hg_field_script_bytes(a,arithmetic,sizeof(arithmetic),0,542,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_DONE);hg_field_script_free(s);
    CHECK(var(a,VAR_TEMP_x4007)==65535&&var(a,VAR_TEMP_x4008)==65535&&var(a,VAR_TEMP_x4009)==65535&&var(a,VAR_TEMP_x400A)==3&&!flag(a,FLAG_MAPTEMP_003));
    const uint8_t call[]={H(26),W(2),H(2),H(41),H(VAR_TEMP_x4002),H(123),H(27)};
    s=hg_field_script_bytes(a,call,sizeof(call),0,542,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_DONE&&var(a,VAR_TEMP_x4002)==123);hg_field_script_free(s);
    const uint8_t empty_return[]={H(27)};s=hg_field_script_bytes(a,empty_return,sizeof(empty_return),0,542,error,sizeof(error));
    CHECK(s&&finish(s)==HG_SCRIPT_DONE);hg_field_script_free(s);
    const uint8_t pause[]={H(3),H(3),H(VAR_TEMP_x4003),H(41),H(VAR_TEMP_x4004),H(42),H(2)};
    s=hg_field_script_bytes(a,pause,sizeof(pause),0,542,error,sizeof(error));CHECK(s);
    for(unsigned i=0;i<4;i++)CHECK(hg_field_script_tick(s)==HG_SCRIPT_RUNNING&&!var(a,VAR_TEMP_x4004));
    CHECK(hg_field_script_tick(s)==HG_SCRIPT_DONE&&!var(a,VAR_TEMP_x4004));CHECK(hg_field_script_commit(s)&&!hg_field_script_commit(s));
    CHECK(var(a,VAR_TEMP_x4004)==42&&!var(a,VAR_TEMP_x4003));hg_field_script_free(s);
    const uint8_t bad[]={H(30),H(FLAG_MAPTEMP_002),H(999),H(2)};
    revision=hg_story_revision(a);CHECK(!hg_field_script_bytes(a,bad,sizeof(bad),0,542,error,sizeof(error))&&error[0]);
    CHECK(!flag(a,FLAG_MAPTEMP_002)&&hg_story_revision(a)==revision&&GetLCRNGSeed()==0x12345678);
    const uint8_t conversation[]={H(30),H(FLAG_MAPTEMP_002),H(45),9,H(50),H(45),5,H(50),H(2)};
    s=hg_field_script_bytes(a,conversation,sizeof(conversation),0,542,error,sizeof(error));CHECK(s);
    CHECK(finish(s)==HG_SCRIPT_MESSAGE&&!strcmp(hg_field_script_message(s)->text,"I like your Bag!")&&!flag(a,FLAG_MAPTEMP_002));
    CHECK(hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_MESSAGE&&strstr(hg_field_script_message(s)->text,"adorable")&&!flag(a,FLAG_MAPTEMP_002));
    CHECK(hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_DONE&&flag(a,FLAG_MAPTEMP_002));hg_field_script_free(s);
    const uint8_t backwards[]={H(22),W(0xfffffffa)};
    CHECK(!hg_field_script_bytes(a,backwards,sizeof(backwards),0,542,error,sizeof(error)));
    CHECK(strstr(error,"budget"));
    const uint8_t loop[]={H(41),H(VAR_TEMP_x4005),H(0),H(39),H(VAR_TEMP_x4005),H(1),
        H(17),H(VAR_TEMP_x4005),H(3),H(28),0,W(0xffffffed),H(2)};
    s=hg_field_script_bytes(a,loop,sizeof(loop),0,542,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_DONE&&var(a,VAR_TEMP_x4005)==3);hg_field_script_free(s);
    const uint8_t back_call[]={H(22),W(8),H(39),H(VAR_TEMP_x4006),H(1),H(27),H(26),W(0xfffffff2),H(2)};
    s=hg_field_script_bytes(a,back_call,sizeof(back_call),0,542,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_DONE&&var(a,VAR_TEMP_x4006)==1);hg_field_script_free(s);
    const uint8_t fallthrough[]={H(17),H(VAR_TEMP_x4000),H(0),H(28),0,W(0xfffffff3)};
    CHECK(!hg_field_script_bytes(a,fallthrough,sizeof(fallthrough),0,542,error,sizeof(error))&&strstr(error,"outside bank"));
    const uint8_t fresh_comparison[]={H(28),1,W(8),H(17),H(VAR_TEMP_x4000),H(0),H(2),H(999)};
    s=hg_field_script_bytes(a,fresh_comparison,sizeof(fresh_comparison),0,542,error,sizeof(error));
    CHECK(s&&finish(s)==HG_SCRIPT_DONE);hg_field_script_free(s);
    const uint8_t invalid_var[]={H(41),H(VAR_BASE+NUM_VARS),H(1),H(2)};
    CHECK(!hg_field_script_bytes(a,invalid_var,sizeof(invalid_var),0,542,error,sizeof(error)));
    /* Preflight is read-only, messages retain a live context, and state becomes
     * authoritative only at original End. An externally changed owner cannot
     * be overwritten by an older script candidate. */
    s=hg_field_script_new(a,63,2,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE);
    CHECK(!strcmp(hg_field_script_message(s)->text,"Professor Elm has been waiting for\nyou, dear. Please, go and help him."));
    CHECK(hg_field_script_message(s)->face_player==1&&hg_field_script_message(s)->select_sound==1500);
    revision=hg_story_revision(a);CHECK(hg_field_script_tick(s)==HG_SCRIPT_MESSAGE&&hg_story_revision(a)==revision);
    CHECK(hg_field_script_ack(s)&&!hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_DONE&&hg_story_revision(a)==revision+1);hg_field_script_free(s);
    s=hg_field_script_new(a,60,2,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE&&!strcmp(hg_field_script_message(s)->text,"I like your Bag!"));
    CHECK(hg_story_var_set(a,VAR_SCENE_NEW_BARK_TOWN_OW,1)&&hg_field_script_tick(s)==HG_SCRIPT_ERROR);
    CHECK(var(a,VAR_SCENE_NEW_BARK_TOWN_OW)==1);hg_field_script_free(s);
    s=hg_field_script_new(a,60,2,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE&&strstr(hg_field_script_message(s)->text,"adorable"));hg_field_script_free(s);
    CHECK(hg_story_flag_set(a,FLAG_GOT_STARTER,1));revision=hg_story_revision(a);
    CHECK(!hg_field_script_new(a,63,2,error,sizeof(error))&&hg_story_revision(a)==revision&&flag(a,FLAG_GOT_STARTER));
    CHECK(hg_story_enter_map(b,60,error,sizeof(error))&&flag(b,FLAG_HIDE_CAMERON));
    CHECK(var(b,VAR_OBJ_0)==SPRITE_HEROINE&&!var(b,VAR_TEMP_x4000));
    CHECK(hg_story_flag_set(b,FLAG_MAPTEMP_001,1)&&hg_story_var_set(b,VAR_TEMP_x4000,13));
    CHECK(hg_story_enter_map(b,63,error,sizeof(error))&&!flag(b,FLAG_MAPTEMP_001)&&!var(b,VAR_TEMP_x4000));
    CHECK(flag(b,TEMP_FLAG_BASE+1));CHECK(hg_story_var_set(b,VAR_OBJ_0,0));
    CHECK(hg_story_enter_map(b,384,error,sizeof(error))&&var(b,VAR_OBJ_0)==SPRITE_HEROINE);
    /* Original BufferPlayersName and party count handlers, with bounded native
     * UTF-8 formatting and an actual empty original-layout inspection party. */
    revision=hg_story_revision(b);
    CHECK(!hg_story_profile_set(b,"TooLongName",0)&&!hg_story_profile_set(b,"",0)&&!hg_story_profile_set(b,"Ethan",2));
    CHECK(!hg_story_profile_set(b,"\xe0\x80\x80",0)&&!hg_story_profile_set(b,"\xed\xa0\x80",0)&&!hg_story_profile_set(b,"\xe2",0));
    CHECK(hg_story_revision(b)==revision&&hg_story_profile_set(b,"Zoé",1));
    s=hg_field_script_new(b,60,16,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE);
    CHECK(!strcmp(hg_field_script_message(s)->text,"Yo, Zoé!\rI hear Professor Elm discovered some\nnew Pokémon."));
    CHECK(hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_DONE);hg_field_script_free(s);
    CHECK(hg_story_var_set(b,VAR_SCENE_ELMS_LAB,1));s=hg_field_script_new(b,61,1,error,sizeof(error));
    CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE&&strstr(hg_field_script_message(s)->text,"Zoé"));
    CHECK(hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_DONE);hg_field_script_free(s);
    const uint8_t party_count[]={H(332),H(VAR_TEMP_x4000),H(2)};
    s=hg_field_script_bytes(b,party_count,sizeof(party_count),0,542,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_DONE&&!var(b,VAR_TEMP_x4000));hg_field_script_free(s);
    const uint8_t unbound_name[]={H(45),12,H(2)},bad_slot[]={H(190),8,H(2)};
    CHECK(!hg_field_script_bytes(b,unbound_name,sizeof(unbound_name),0,542,error,sizeof(error)));
    CHECK(!hg_field_script_bytes(b,bad_slot,sizeof(bad_slot),0,542,error,sizeof(error)));
    /* Elm's real introductory research text exceeds the old 512-byte limit.
     * Preserve all of it through a live original context and name buffering;
     * this fixture does not claim the surrounding cutscene is implemented. */
    const uint8_t research[]={H(190),0,H(132),0,1,H(50),H(2)};
    s=hg_field_script_bytes(b,research,sizeof(research),0,543,error,sizeof(error));CHECK(s&&finish(s)==HG_SCRIPT_MESSAGE);
    const HGScriptMessage *long_text=hg_field_script_message(s);
    CHECK(strlen(long_text->text)>512&&strstr(long_text->text,"Professor Elm: Hi, Zoé!")&&strstr(long_text->text,"Ethan does!")&&strstr(long_text->text,"you could choose from..."));
    CHECK(!strchr(long_text->text,'{')&&!strchr(long_text->text,'\f'));
    HGFont *font=hg_font_new();CHECK(font);size_t consumed=0;unsigned pages=0;uint8_t pixels[256*64*4]={0};
    while(long_text->text[consumed]) {
        size_t length=hg_font_page_end(font,long_text->text+consumed,236,2);CHECK(length&&length<512);
        consumed+=length;CHECK(((unsigned char)long_text->text[consumed]&0xc0)!=0x80);
        char page[512];memcpy(page,long_text->text+consumed-length,length);page[length]=0;
        hg_font_draw(font,page,pixels,256,64,10,8,236,0x202830,0xc0c8c0);pages++;
    }
    CHECK(consumed==strlen(long_text->text)&&pages>1&&pages<64);hg_font_free(font);
    revision=hg_story_revision(b);CHECK(hg_field_script_ack(s)&&finish(s)==HG_SCRIPT_DONE&&hg_story_revision(b)==revision+1);hg_field_script_free(s);
    hg_story_free(a);hg_story_free(b);
    puts("Original story runtime: std_init, persistent flags/vars, RNG/temp isolation, branches/calls/arithmetic, frame wait, live messages, rollback and map reset passed");return 0;
}
