#include "compat/save_c_records.h"
#include "compat/save_manager.h"
#include "compat/save_records.h"
#include "ds_thumb.h"
#include "save_records_guest.inc"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Separate guest record/heap/stack ranges; never cast native record pointers to
 * 32-bit addresses. BSS remains live across callbacks, as in the original game. */
#define SAVE_RAM_SIZE 0x200000u
#define SAVE_RECORD_ADDRESS 0x02040000u
#define SAVE_HEAP_ADDRESS 0x02090000u
#define SAVE_STACK_ADDRESS 0x021ffff0u
_Static_assert(sizeof(hg_save_guest_image)<SAVE_RECORD_ADDRESS-0x02000000u,"Save guest image overlaps record scratch");
static unsigned char *save_ram;
static const char *save_error;
static unsigned heap_used;
static void *active_record;
static unsigned active_size;
static SaveData *active_save;
static unsigned active_id;
static SaveData *full_save;
static unsigned call_second;
u32 PRandom(u32);
struct SaveTrainerCard *Save_TrainerCard_Get(SaveData *);
extern void sub_0203A01C(WiFiList *);
extern u16 *StringFillEOS(u16 *,u32);

const char *hg_save_records_last_error(void) { return save_error; }
static void *guest_bytes(DSThumb *cpu,unsigned address,unsigned size) {
    return ds_translate(&cpu->memory,address,size);
}
static int save_boundary(DSThumb *cpu,void *unused) {
    (void)unused;const char *name=NULL;
    for(size_t i=0;i<sizeof(hg_save_guest_imports)/sizeof(*hg_save_guest_imports);i++)
        if(cpu->r[15]==hg_save_guest_imports[i].address){name=hg_save_guest_imports[i].name;break;}
    if(!name)return 0;
    if(!strcmp(name,"MIi_CpuClearFast")||!strcmp(name,"MIi_CpuClear32")||!strcmp(name,"MIi_CpuClear16")) {
        unsigned width=!strcmp(name,"MIi_CpuClear16")?2:4;
        unsigned char *p=guest_bytes(cpu,cpu->r[1],cpu->r[2]);
        if(!p||cpu->r[1]%width||cpu->r[2]%width)return -1;
        for(unsigned i=0;i<cpu->r[2];i++)p[i]=(unsigned char)(cpu->r[0]>>(8*(i%width)));
        return 1;
    }
    if(!strcmp(name,"MI_CpuFill8")||!strcmp(name,"memset")) {
        unsigned char *p=guest_bytes(cpu,cpu->r[0],cpu->r[2]);if(!p)return -1;
        memset(p,(unsigned char)cpu->r[1],cpu->r[2]);return 1;
    }
    if(!strcmp(name,"MI_CpuCopy8")||!strcmp(name,"MIi_CpuCopy32")||!strcmp(name,"MIi_CpuCopy16")||!strcmp(name,"MIi_CpuCopyFast")) {
        void *from=guest_bytes(cpu,cpu->r[0],cpu->r[2]),*to=guest_bytes(cpu,cpu->r[1],cpu->r[2]);
        if(!from||!to)return -1;memmove(to,from,cpu->r[2]);return 1;
    }
    if(!strcmp(name,"Heap_Alloc")||!strcmp(name,"Heap_AllocAtEnd")) {
        unsigned n=(cpu->r[1]+3u)&~3u;if(n<cpu->r[1]||heap_used+n>0x60000u)return -1;
        cpu->r[0]=SAVE_HEAP_ADDRESS+heap_used;heap_used+=n;return 1;
    }
    if(!strcmp(name,"Heap_Free")) {
        return cpu->r[0]>=SAVE_HEAP_ADDRESS && cpu->r[0]<SAVE_HEAP_ADDRESS+heap_used?1:-1;
    }
    if(!strcmp(name,"SaveSubstruct_UpdateCRC")) {
        if(!active_record)return -1;
        memcpy(active_record,save_ram+SAVE_RECORD_ADDRESS-0x02000000u,active_size);
        SaveSubstruct_UpdateCRC((int)cpu->r[0]);return 1;
    }
    if(full_save&&!strcmp(name,"SaveArray_Get")) {
        if(cpu->r[0]!=SAVE_RECORD_ADDRESS||cpu->r[1]>=(unsigned)gNumSaveChunkHeaders)return -1;
        unsigned offset=(unsigned)((u8 *)SaveArray_Get(full_save,cpu->r[1])-(u8 *)full_save);
        cpu->r[0]=SAVE_RECORD_ADDRESS+offset;return 1;
    }
    if(!strcmp(name,"PRandom")){cpu->r[0]=PRandom(cpu->r[0]);return 1;}
    if(full_save&&!strcmp(name,"Save_TrainerCard_Get")) {
        if(cpu->r[0]!=SAVE_RECORD_ADDRESS)return -1;
        cpu->r[0]=SAVE_RECORD_ADDRESS+(unsigned)((u8 *)Save_TrainerCard_Get(full_save)-(u8 *)full_save);return 1;
    }
    if(full_save&&!strcmp(name,"Save_PlayerData_GetProfile")) {
        if(cpu->r[0]!=SAVE_RECORD_ADDRESS)return -1;
        cpu->r[0]=SAVE_RECORD_ADDRESS+(unsigned)((u8 *)hg_story_save_profile(full_save)-(u8 *)full_save);return 1;
    }
    if(!strcmp(name,"TrainerCard_GetBadgeShininessArr")) {
        struct SaveTrainerCard *card=guest_bytes(cpu,cpu->r[0],sizeof(*card));if(!card)return -1;
        cpu->r[0]+=(unsigned)((u8 *)TrainerCard_GetBadgeShininessArr(card)-(u8 *)card);return 1;
    }
    if(!strcmp(name,"PlayerProfile_TestBadgeFlag")) {
        PlayerProfile *profile=guest_bytes(cpu,cpu->r[0],sizeof(*profile));if(!profile||cpu->r[1]>=16)return -1;
        cpu->r[0]=PlayerProfile_TestBadgeFlag(profile,cpu->r[1]);return 1;
    }
    if(!strcmp(name,"GetShininessOfBadgeI")) {
        int *array=guest_bytes(cpu,cpu->r[1],8*sizeof(int));if(!array||cpu->r[0]>=8)return -1;
        cpu->r[0]=GetShininessOfBadgeI(cpu->r[0],array);return 1;
    }
    if(!strcmp(name,"SetShininessOfBadgeI")) {
        int *array=guest_bytes(cpu,cpu->r[2],8*sizeof(int));if(!array||cpu->r[0]>=8)return -1;
        SetShininessOfBadgeI(cpu->r[0],cpu->r[1],array);return 1;
    }
    if(!strcmp(name,"SaveArray_Get")) {
        if(!active_save || cpu->r[0]!=0x03000000u || cpu->r[1]!=active_id || !active_record)return -1;
        memcpy(save_ram+SAVE_RECORD_ADDRESS-0x02000000u,active_record,active_size);
        cpu->r[0]=SAVE_RECORD_ADDRESS;return 1;
    }
    if(!strcmp(name,"sub_0203A01C")) {
        void *p=guest_bytes(cpu,cpu->r[0],0x8c0);if(!p)return -1;
        sub_0203A01C(p);return 1;
    }
    if(!strcmp(name,"MailMsg_Init_FromTemplate")) {
        void *p=guest_bytes(cpu,cpu->r[0],8),*t=guest_bytes(cpu,cpu->r[1],8);if(!p||!t)return -1;
        MailMsg_Init_FromTemplate(p,t);return 1;
    }
    if(!strcmp(name,"StringFillEOS")) {
        if(cpu->r[1]>SAVE_RAM_SIZE/2)return -1;
        u16 *p=guest_bytes(cpu,cpu->r[0],cpu->r[1]*2);if(!p||cpu->r[0]%2)return -1;
        u16 *end=StringFillEOS(p,cpu->r[1]);
        cpu->r[0]+=(unsigned)((unsigned char *)end-(unsigned char *)p);return 1;
    }
    save_error=name;return -1;
}
static unsigned save_guest_call(unsigned address,unsigned argument) {
    if(!save_ram){save_ram=calloc(1,SAVE_RAM_SIZE);if(!save_ram)abort();memcpy(save_ram,hg_save_guest_image,sizeof(hg_save_guest_image));}
    heap_used=0;save_error=NULL;
    DSThumb cpu={0};cpu.memory=(DSMainRAM){save_ram,SAVE_RAM_SIZE};
    cpu.code_start=0x02000000u;cpu.code_size=HG_SAVE_GUEST_CODE_SIZE;
    cpu.r[0]=argument;cpu.r[1]=call_second;cpu.r[13]=SAVE_STACK_ADDRESS;cpu.r[14]=0x01fffff1u;cpu.r[15]=address;cpu.return_address=cpu.r[14];
    cpu.boundary=save_boundary;
    DSThumbResult result=ds_thumb_run(&cpu,16000000);
    if(result!=DS_THUMB_RETURN){fprintf(stderr,"Original save callback %08x failed: %s (result %d, PC %08x, instruction %04x)\n",address,save_error?save_error:"guest execution",result,cpu.r[15],cpu.fault_instruction);abort();}
    return cpu.r[0];
}
static unsigned save_guest_size(unsigned address) { return save_guest_call(address,0); }
static void save_guest_init(unsigned address,unsigned size_address,void *record) {
    unsigned size=save_guest_size(size_address);
    if(!record||!size||size>SAVE_HEAP_ADDRESS-SAVE_RECORD_ADDRESS)abort();
    memcpy(save_ram+SAVE_RECORD_ADDRESS-0x02000000u,record,size);
    active_record=record;active_size=size;
    save_guest_call(address,SAVE_RECORD_ADDRESS);
    active_record=NULL;active_size=0;
    memcpy(record,save_ram+SAVE_RECORD_ADDRESS-0x02000000u,size);
}
typedef struct SavePokegear SavePokegear;
typedef struct Bag Bag;
typedef struct LocalFieldData LocalFieldData;
struct SavedMapObjectList;
#include "save_records_registry.inc"

static void save_guest_sync(SaveData *save,unsigned address,unsigned id,int commit) {
    unsigned size=gSaveChunkHeaders[id].sizeFunc();
    active_save=save;active_id=id;active_record=SaveArray_Get(save,(int)id);active_size=size;
    save_guest_call(address,0x03000000u);
    if(commit && active_record)memcpy(active_record,save_ram+SAVE_RECORD_ADDRESS-0x02000000u,active_size);
    active_save=NULL;active_record=NULL;active_size=0;
}
void Save_Frontier_Load(SaveData *save) { save_guest_sync(save,HG_SAVE_GUEST_Save_Frontier_Load,19,0); }
void Save_Frontier_Commit(SaveData *save) { save_guest_sync(save,HG_SAVE_GUEST_Save_Frontier_Commit,19,1); }
void sub_0202C6FC(SaveData *save) { save_guest_sync(save,HG_SAVE_GUEST_sub_0202C6FC,25,0); }
void sub_0202C714(SaveData *save) { save_guest_sync(save,HG_SAVE_GUEST_sub_0202C714,25,1); }

#include "friend_group.h"
#include "unk_02031B0C.h"
static unsigned save_guest_full(SaveData *save,unsigned address,unsigned argument) {
    save_guest_size(HG_SAVE_GUEST_Save_FriendGroup_sizeof);
    memcpy(save_ram+SAVE_RECORD_ADDRESS-0x02000000u,save,sizeof(*save));
    full_save=save;call_second=argument;
    unsigned result=save_guest_call(address,SAVE_RECORD_ADDRESS);
    full_save=NULL;call_second=0;
    memcpy(save,save_ram+SAVE_RECORD_ADDRESS-0x02000000u,sizeof(*save));
    return result;
}
static unsigned save_guest_record(unsigned address,unsigned id,void *record,unsigned argument) {
    unsigned size=gSaveChunkHeaders[id].sizeFunc();
    memcpy(save_ram+SAVE_RECORD_ADDRESS-0x02000000u,record,size);call_second=argument;
    unsigned result=save_guest_call(address,SAVE_RECORD_ADDRESS);call_second=0;
    memcpy(record,save_ram+SAVE_RECORD_ADDRESS-0x02000000u,size);return result;
}
SAV_FRIEND_GRP *Save_FriendGroup_Get(SaveData *save) {
    unsigned result=save_guest_full(save,HG_SAVE_GUEST_Save_FriendGroup_Get,0);
    GF_ASSERT(result>=SAVE_RECORD_ADDRESS&&result-SAVE_RECORD_ADDRESS<sizeof(*save));
    return (void *)((u8 *)save+result-SAVE_RECORD_ADDRESS);
}
SaveApricornBox *Save_ApricornBox_Get(SaveData *save) {
    unsigned result=save_guest_full(save,HG_SAVE_GUEST_Save_ApricornBox_Get,0);
    GF_ASSERT(result>=SAVE_RECORD_ADDRESS&&result-SAVE_RECORD_ADDRESS<sizeof(*save));
    return (void *)((u8 *)save+result-SAVE_RECORD_ADDRESS);
}
void sub_0202C78C(SAV_FRIEND_GRP *record,int days) { save_guest_record(HG_SAVE_GUEST_sub_0202C78C,SAVE_FRIEND_GROUP,record,days); }
u32 sub_0202C7DC(SAV_FRIEND_GRP *record) { return save_guest_record(HG_SAVE_GUEST_sub_0202C7DC,SAVE_FRIEND_GROUP,record,0); }
void sub_02031CCC(SaveApricornBox *record,int days) { save_guest_record(HG_SAVE_GUEST_sub_02031CCC,SAVE_APRICORN_BOX,record,days); }
void sub_0206759C(SaveData *save,int days) { save_guest_full(save,HG_SAVE_GUEST_sub_0206759C,days); }
u32 sub_020674BC(SaveData *save) {
    Save_Frontier_Load(save);
    unsigned result=save_guest_full(save,HG_SAVE_GUEST_sub_020674BC,0);
    Save_Frontier_Commit(save);return result;
}
