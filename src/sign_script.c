#include "compat/script.h"
#include "field_events.h"
#include "field_script.h"
#include "constants/maps.h"
#include <string.h>
/* Legacy single-message inspection API, sharing the persistent interpreter's
 * original handlers and validation. Its private state is never published. */
int hg_field_message(unsigned map,unsigned id,char *message,size_t capacity,HGFieldMessageEffects *effects) {
    if(!message||!capacity)return 0;
    HGStoryState *state=hg_story_new();if(!state)return 0;
    HGFieldScript *script=hg_field_script_new(state,map,id,NULL,0);
    if(!script){hg_story_free(state);return 0;}
    HGScriptMessage result={0};unsigned messages=0;int status=HG_SCRIPT_RUNNING;
    while(status!=HG_SCRIPT_DONE&&status!=HG_SCRIPT_ERROR) {
        status=hg_field_script_tick(script);
        if(status==HG_SCRIPT_MESSAGE) {
            if(messages++){status=HG_SCRIPT_ERROR;break;}
            result=*hg_field_script_message(script);hg_field_script_ack(script);
        }
    }
    hg_field_script_free(script);hg_story_free(state);
    if(status!=HG_SCRIPT_DONE||messages!=1||strlen(result.text)+1>capacity)return 0;
    memcpy(message,result.text,strlen(result.text)+1);
    if(effects)*effects=(HGFieldMessageEffects){result.face_player,result.select_sound};return 1;
}
int hg_sign_script(unsigned id,char *message,size_t capacity) {
    HGEventView events;if(!hg_event_load(MAP_NEW_BARK,&events))return 0;
    for(unsigned i=0;i<events.background_count;i++) {
        const uint8_t *event=events.backgrounds+20*i;
        if((event[0]|(unsigned)event[1]<<8)==id&&(event[2]|(unsigned)event[3]<<8)==1)
            return hg_field_message(MAP_NEW_BARK,id,message,capacity,NULL);
    }
    return 0;
}
