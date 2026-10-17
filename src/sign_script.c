#include "compat/script.h"
#include "field_events.h"
#include "compat/resources.h"
#include "constants/std_script.h"
#include "map_header.h"
#include "constants/maps.h"
#include <string.h>
/* Only finite, fully preflighted sign programs are admitted. Original script.c
 * owns dispatch/yield/end state; the host owns immediate sign presentation. */
typedef struct { ScriptContext context;unsigned message,found,failed; } SignScript;
static unsigned half(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static int operands(unsigned op) {
    switch(op){case 2:return 0;case 20:return 2;case 55:return 6;case 56:return 3;
        case 57:return 1;case 58:return 0;case 59:return 3;case 60:return 2;default:return -1;}
}
static BOOL end_command(ScriptContext *ctx){StopScript(ctx);return FALSE;}
static BOOL sign_command(ScriptContext *ctx) {
    SignScript *sign=(SignScript *)ctx;unsigned op=half(ctx->script_ptr-2);int count=operands(op);
    if(op==55||op==59){sign->message=ScriptReadByte(ctx);sign->found++;count--;}
    while(count-->0)(void)ScriptReadByte(ctx);
    return FALSE;
}
static BOOL unsupported_command(ScriptContext *ctx) {
    ((SignScript *)ctx)->failed=1;StopScript(ctx);return TRUE;
}
static BOOL wait_ready(ScriptContext *ctx){(void)ctx;return TRUE;}
static BOOL call_signpost(ScriptContext *ctx) {
    if(ScriptReadHalfword(ctx)!=std_signpost)return unsupported_command(ctx);
    SetupNativeScript(ctx,wait_ready);return TRUE;
}
int hg_sign_script(unsigned id,char *message,size_t capacity) {
    if(!message||!capacity||id==0||id>18 || MapHeader_GetScriptsBank(MAP_NEW_BARK)!=842 ||
       MapHeader_GetMsgBank(MAP_NEW_BARK)!=542)return 0;const uint8_t *bytes;size_t size;
    if(!native_resource_file("t20_script.bin",&bytes,&size)||size<74)return 0;
    size_t table=4*(id-1);uint32_t relative=(uint32_t)bytes[table]|(uint32_t)bytes[table+1]<<8|
        (uint32_t)bytes[table+2]<<16|(uint32_t)bytes[table+3]<<24;
    uint64_t start=(uint64_t)table+4+relative;if(start>=size)return 0;
    size_t cursor=(size_t)start;unsigned messages=0,commands=0;int ended=0;
    while(cursor+2<=size&&commands++<32) {
        unsigned op=half(bytes+cursor);cursor+=2;int length=operands(op);
        if(length<0||(size_t)length>size-cursor)return 0;
        if(op==20&&half(bytes+cursor)!=std_signpost)return 0;
        if(op==55||op==59)messages++;
        cursor+=(unsigned)length;if(op==2){ended=1;break;}
    }
    if(!ended||messages!=1)return 0;
    ScrCmdFunc table_commands[61];for(unsigned i=0;i<61;i++)table_commands[i]=unsupported_command;
    for(unsigned i=55;i<=60;i++)table_commands[i]=sign_command;
    table_commands[2]=end_command;table_commands[20]=call_signpost;
    SignScript sign={0};InitScriptContext(&sign.context,table_commands,61);
    SetupBytecodeScript(&sign.context,bytes+start);
    /* Three turns cover bytecode -> native presentation yield -> bytecode end. */
    for(unsigned i=0;i<3;i++)RunScriptCommand(&sign.context);
    if(sign.failed||sign.context.mode!=SCRIPT_MODE_STOPPED||sign.found!=1)return 0;
    const uint8_t *text;size_t length;
    if(!native_resource_member(NARC_msgdata_msg,MapHeader_GetMsgBank(MAP_NEW_BARK),(int)sign.message,&text,&length)||!length||text[length-1]||
       memchr(text,'{',length)||length>capacity)return 0;
    memcpy(message,text,length);return 1;
}
