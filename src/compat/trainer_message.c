#include "compat/native.h"
#include "compat/resources.h"
#include "compat/trainer_message.h"
#include "constants/trainers.h"
#include "files/msgdata/msg.naix"
#include <limits.h>
static unsigned table_size(NarcId archive,int member) {
    const u8 *bytes;size_t size;
    GF_ASSERT(archive==NARC_poketool_trmsg_trtbl&&member==0&&
        native_resource_member(archive,0,member,&bytes,&size)&&size<=UINT16_MAX);
    return (unsigned)size;
}
static void table_read(void *out,NarcId archive,int member,u32 offset,u32 length) {
    const u8 *bytes;
    GF_ASSERT(archive==NARC_poketool_trmsg_trtblofs&&member==0&&length==2&&
        native_resource_range(archive,0,member,offset,length,&bytes));
    memcpy(out,bytes,length);
}
static void message_member(NarcId archive,unsigned bank,unsigned member,enum HeapID heap,HGTrainerMessageRef *out) {
    GF_ASSERT(archive==NARC_msgdata_msg&&bank==msg_0728&&heap==HEAP_ID_FIELD2&&out);
    *out=(HGTrainerMessageRef){bank,member,1};
}
static void empty_message(HGTrainerMessageRef *out) { *out=(HGTrainerMessageRef){0}; }
/* String storage belongs to native presentation. The unchanged original
 * lookup selects its message member; decoding/formatting happens afterward. */
#define String HGTrainerMessageRef
#define GetTrainerMessageByIdPair original_trainer_message
#define GetNarcMemberSizeByIdPair table_size
#define ReadFromNarcMemberByIdPair table_read
#define ReadMsgData_NewNarc_ExistingString message_member
#define String_SetEmpty empty_message
#include "trainer_message_kernel.inc"
int hg_trainer_message_ref(unsigned trainer,unsigned message,HGTrainerMessageRef *out) {
    if(!out||trainer<FIRST_TRAINER_INDEX||trainer>=LAST_TRAINER_INDEX||message>UINT16_MAX)return 0;
    const u8 *table,*offsets;size_t size,offset_size;
    if(!native_resource_member(NARC_poketool_trmsg_trtbl,0,0,&table,&size)||size>UINT16_MAX||size%4||
       !native_resource_member(NARC_poketool_trmsg_trtblofs,0,0,&offsets,&offset_size)||trainer*2+2>offset_size)return 0;
    unsigned offset=offsets[trainer*2]|offsets[trainer*2+1]<<8;
    if(offset>size||offset%4)return 0;
    HGTrainerMessageRef selected={0};
    original_trainer_message(trainer,message,&selected,HEAP_ID_FIELD2);
    if(selected.found){const u8 *text;size_t length;if(!native_resource_member(NARC_msgdata_msg,selected.bank,selected.message,&text,&length)||length<1||text[length-1])return 0;}
    *out=selected;return 1;
}
