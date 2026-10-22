#include "save_manager.h"
#include "../ds_save_flash.h"
#include <setjmp.h>
#include "stats.h"

_Static_assert(sizeof(SaveData) == 0x2330c, "original SaveData layout");
_Static_assert(offsetof(SaveData, arrayHeaders) == 0x23014, "original record directory");
#define MI_CpuClearFast(p,n) memset((p),0,(n))
#define MI_CpuFillFast(p,v,n) memset((p),(v),(n))
#define MI_CpuFill8(p,v,n) memset((p),(v),(n))
#define BOX_ALL_MODIFIED_FLAG ((1U << 18) - 1)
#define OS_LOCK_ID_ERROR (-1)
#define CARD_BACKUP_TYPE_NOT_USE 0
#define CARD_BACKUP_TYPE_FLASH_4MBITS_EX 1
#define CARD_BACKUP_TYPE_FLASH_4MBITS 2
#define CARD_RESULT_SUCCESS 0
#define CARD_RESULT_TIMEOUT 1
#define CARD_RESULT_NO_RESPONSE 2
typedef int CARDBackupType;
typedef struct SAVE_MISC_DATA SAVE_MISC_DATA;
u16 GF_CalcCRC16(const void *,u32);
u32 PRandom(u32);
u32 PCStorage_GetNumBoxes(void);
u32 PCStorage_GetSizeOfBox(void);
u32 Save_GetPCBoxModifiedFlags(SaveData *);
void Save_ResetPCBoxModifiedFlags(SaveData *);
void Save_SetAllPCBoxesModified(SaveData *);
void sub_020271A0(SaveData *);
SAVE_MISC_DATA *Save_Misc_Get(SaveData *);
BOOL SaveMisc_CheckExtraChunksExist(SAVE_MISC_DATA *);
void SaveMisc_SetExtraChunksExist(SAVE_MISC_DATA *);
void sub_0202AC38(SAVE_MISC_DATA *,int,u32 *,u32 *,u8 *);
void sub_0202AC60(SAVE_MISC_DATA *,int,u32,u32,u8);
void Save_Frontier_Load(SaveData *);
void Save_Frontier_Commit(SaveData *);
void sub_0202C6FC(SaveData *);
void sub_0202C714(SaveData *);

/* Nitro's callback may run before enqueue returns. Complete writes durably in
 * that permitted case: upstream's blocking path spins on the callback flag and
 * does not poll CARD_WaitBackupAsync. No worker mutates gameplay-owned buffers. */
static int backup_result = CARD_RESULT_SUCCESS;
static int lock_allocated;
static int backup_locked;
static int OS_GetLockID(void) { GF_ASSERT(!lock_allocated); lock_allocated=1; return 1; }
static void OS_ReleaseLockID(int id) { GF_ASSERT(id==1 && lock_allocated && !backup_locked); lock_allocated=0; }
static void CARD_LockBackup(int id) { GF_ASSERT(id==1 && lock_allocated && !backup_locked); backup_locked=1; }
static void CARD_UnlockBackup(int id) { GF_ASSERT(id==1 && backup_locked); backup_locked=0; }
static BOOL CARD_IdentifyBackup(CARDBackupType type) {
    unsigned char byte;
    GF_ASSERT(backup_locked);
    return type==CARD_BACKUP_TYPE_FLASH_4MBITS_EX && ds_save_flash_read(0,&byte,1);
}
static BOOL CARD_ReadBackup(u32 off,void *data,u32 size) {
    GF_ASSERT(backup_locked);
    BOOL ok=ds_save_flash_read(off,data,size);
    backup_result=ok?CARD_RESULT_SUCCESS:CARD_RESULT_NO_RESPONSE;
    return ok;
}
static void CARD_ReadBackupAsync(u32 off,void *data,u32 size,void (*callback)(void *),void *arg) {
    CARD_ReadBackup(off,data,size); if(callback)callback(arg);
}
static void CARD_WriteAndVerifyBackupAsync(u32 off,const void *data,u32 size,void (*callback)(void *),void *arg) {
    GF_ASSERT(backup_locked);
    backup_result=ds_save_flash_write(off,data,size)?CARD_RESULT_SUCCESS:CARD_RESULT_TIMEOUT;
    if(callback)callback(arg);
}
static BOOL CARD_WaitBackupAsync(void) { return backup_result==CARD_RESULT_SUCCESS; }
static BOOL CARD_TryWaitBackupAsync(void) { return TRUE; }
static void CARD_CancelBackupAsync(void) { /* All enqueued transfers have already completed. */ }
static int CARD_GetResultCode(void) { return backup_result; }
/* Original handlers free the global save before entering their fatal screens.
 * Never return into code holding that freed owner. */
/* A scope transfers the original fatal-screen path back to the native UI.
 * The original manager has already freed its heap owner on this path. */
static struct {
    jmp_buf jump;
    int active;
    SaveData *previous;
    SaveData *owned;
    char *error;
    size_t error_size;
    void *allocations[32];
    unsigned allocation_count;
    u8 previous_sleep_flags;
} save_scope;
static _Noreturn void save_transport_error(const char *kind,int code) {
    if(save_scope.active) {
        save_scope.owned=NULL; /* Freed by the original fatal error path. */
        if(save_scope.error && save_scope.error_size)
            snprintf(save_scope.error,save_scope.error_size,"%s (%d): %s",kind,code,ds_save_flash_last_error());
        longjmp(save_scope.jump,1);
    }
    fprintf(stderr,"%s (%d): %s\n",kind,code,ds_save_flash_last_error());
    abort();
}
static _Noreturn void ShowSaveDataReadError(enum HeapID heap) {
    (void)heap; save_transport_error("Save read failed",0);
}
static _Noreturn void ShowSaveDataWriteError(enum HeapID heap,int code) {
    (void)heap; save_transport_error("Save write failed",code);
}
/* The original fatal screen never resumes, so its temporary buffers normally
 * die with the process. Track manager allocations within native recovery scopes
 * to release those buffers when returning to the application after an error. */
static int allocation_fault=-1;
void hg_save_manager_fault_alloc_after(int count) { allocation_fault=count; }
static void *manager_alloc(enum HeapID heap,u32 size,int at_end) {
    if(save_scope.active && allocation_fault==0) {
        if(save_scope.error && save_scope.error_size)snprintf(save_scope.error,save_scope.error_size,"Injected save allocation failure");
        longjmp(save_scope.jump,1);
    }
    void *p=at_end?Heap_AllocAtEnd(heap,size):Heap_Alloc(heap,size);
    if(!p && save_scope.active) {
        if(save_scope.error && save_scope.error_size)snprintf(save_scope.error,save_scope.error_size,"Save allocation failed");
        longjmp(save_scope.jump,1);
    }
    if(save_scope.active && allocation_fault>0)allocation_fault--;
    if(save_scope.active) {
        GF_ASSERT(save_scope.allocation_count<NELEMS(save_scope.allocations));
        save_scope.allocations[save_scope.allocation_count++]=p;
    }
    return p;
}
static void manager_free(void *p) {
    if(save_scope.active)for(unsigned i=0;i<save_scope.allocation_count;i++)
        if(save_scope.allocations[i]==p) {
            save_scope.allocations[i]=save_scope.allocations[--save_scope.allocation_count];
            break;
        }
    Heap_Free(p);
}
#define Heap_Alloc(h,n) manager_alloc((h),(n),0)
#define Heap_AllocAtEnd(h,n) manager_alloc((h),(n),1)
#define Heap_Free(p) manager_free(p)
#include "original_save_manager.inc"
#undef Heap_Alloc
#undef Heap_AllocAtEnd
#undef Heap_Free

/* Native owner bytes remain authoritative. Guest-backed records19/25 have
 * original process-global mirrors; refresh those mirrors when selecting an
 * owner, rather than letting a fixture/transaction overwrite another owner. */
static void select_owner(SaveData *save) {
    sSaveDataPtr=save;
    if(save) {
        Save_Frontier_Load(save);
        sub_0202C6FC(save);
    }
}

void hg_save_manager_init_ram(SaveData *save) {
    memset(save,0,sizeof(*save));
    save->isNewGame=TRUE;
    save->sectorCleanFlag[0]=1;
    save->sectorCleanFlag[1]=1;
    SaveData_InitSubstructs(save->arrayHeaders);
    SaveData_InitSlotSpecs(save->saveSlotSpecs,save->arrayHeaders);
    SaveData *previous=sSaveDataPtr;
    sSaveDataPtr=save;
    Save_InitDynamicRegion(save);
    select_owner(previous);
}
void hg_save_manager_release(SaveData *save) {
    if(sSaveDataPtr==save)sSaveDataPtr=NULL;
    Heap_Free(save);
}

int hg_save_manager_validate_ram(const SaveData *save) {
    if(!save) return 0;
    struct SaveArrayHeader expected[SAVE_BLOCK_NUM];
    struct SaveSlotSpec slots[2];
    SaveData_InitSubstructs(expected);
    SaveData_InitSlotSpecs(slots,expected);
    for(int i=0;i<SAVE_BLOCK_NUM;i++) {
        const struct SaveArrayHeader *a=&save->arrayHeaders[i], *b=&expected[i];
        if(a->id!=b->id || a->size!=b->size || a->offset!=b->offset || a->slot!=b->slot) return 0;
    }
    for(int i=0;i<2;i++) {
        const struct SaveSlotSpec *a=&save->saveSlotSpecs[i], *b=&slots[i];
        if(a->id!=b->id || a->firstPage!=b->firstPage || a->numPages!=b->numPages || a->offset!=b->offset || a->size!=b->size) return 0;
    }
    return save->lastGoodSector<2 && save->lastGoodSaveSlot<2;
}
SaveData *hg_save_manager_exchange(SaveData *save) {
    SaveData *previous=sSaveDataPtr;select_owner(save);return previous;
}
void hg_save_manager_bind(SaveData *save) { (void)hg_save_manager_exchange(save); }

static int save_scope_begin(char *error,size_t error_size) {
    if(error && error_size)error[0]=0;
    if(save_scope.active) {
        if(error && error_size)snprintf(error,error_size,"Nested save operation rejected");
        return 0;
    }
    save_scope.active=1;
    save_scope.previous=sSaveDataPtr;
    save_scope.previous_sleep_flags=gSystem.lidClosedPauseDisabled;
    save_scope.owned=NULL;
    save_scope.allocation_count=0;
    save_scope.error=error;
    save_scope.error_size=error_size;
    return 1;
}
static int save_scope_finish(int success,const char *error) {
    if(!success && error && save_scope.error && save_scope.error_size)
        snprintf(save_scope.error,save_scope.error_size,"%s",error);
    if(save_scope.owned)manager_free(save_scope.owned);
    while(save_scope.allocation_count)manager_free(save_scope.allocations[0]);
    select_owner(save_scope.previous);
    gSystem.lidClosedPauseDisabled=save_scope.previous_sleep_flags;
    save_scope.active=0;
    save_scope.owned=NULL;
    return success;
}
int hg_save_manager_write(SaveData *live,char *error,size_t error_size) {
    if(!save_scope_begin(error,error_size))return 0;
    if(setjmp(save_scope.jump))return save_scope_finish(0,NULL);
    if(!hg_save_manager_validate_ram(live))return save_scope_finish(0,"Invalid live save directory");
    save_scope.owned=manager_alloc(HEAP_ID_1,sizeof(SaveData),0);
    memcpy(save_scope.owned,live,sizeof(SaveData));
    select_owner(save_scope.owned);
    save_scope.owned->flashChipDetected=SaveDetectFlash();
    if(SaveGameNormal(save_scope.owned)!=WRITE_STATUS_SUCCESS)
        return save_scope_finish(0,"Save flash unavailable or write incomplete");
    memcpy(live,save_scope.owned,sizeof(SaveData));
    return save_scope_finish(1,NULL);
}
int hg_save_manager_load(SaveData *out,char *error,size_t error_size) {
    if(!save_scope_begin(error,error_size))return 0;
    if(setjmp(save_scope.jump))return save_scope_finish(0,NULL);
    if(!out)return save_scope_finish(0,"Missing save destination");
    save_scope.owned=SaveData_New();
    if(!save_scope.owned->flashChipDetected)return save_scope_finish(0,"Save flash unavailable");
    if(!Save_FileExists(save_scope.owned))
        return save_scope_finish(0,(Save_GetStatusFlags(save_scope.owned)&2)?"Both save sectors are invalid":"No saved game exists");
    if(!SaveData_TryLoadOnContinue(save_scope.owned))return save_scope_finish(0,"Saved game failed original continue validation");
    memcpy(out,save_scope.owned,sizeof(SaveData));
    return save_scope_finish(1,NULL);
}
int hg_save_manager_write_steps(SaveData *live,unsigned max_steps,int *completed,char *error,size_t error_size) {
    if(completed)*completed=0;
    if(!save_scope_begin(error,error_size))return 0;
    if(setjmp(save_scope.jump))return save_scope_finish(0,NULL);
    if(!completed || !hg_save_manager_validate_ram(live))return save_scope_finish(0,"Invalid incremental save arguments");
    save_scope.owned=manager_alloc(HEAP_ID_1,sizeof(SaveData),0);
    memcpy(save_scope.owned,live,sizeof(SaveData));select_owner(save_scope.owned);
    save_scope.owned->flashChipDetected=SaveDetectFlash();
    Save_PrepareForAsyncWrite(save_scope.owned,2);
    for(unsigned i=0;i<max_steps;i++) {
        int result=Save_WriteFileAsync(save_scope.owned);
        if(result==WRITE_STATUS_SUCCESS) {
            memcpy(live,save_scope.owned,sizeof(SaveData));*completed=1;return save_scope_finish(1,NULL);
        }
        if(result!=WRITE_STATUS_CONTINUE && result!=WRITE_STATUS_NEXT)return save_scope_finish(0,"Incremental save failed");
    }
    Save_Cancel(save_scope.owned);
    return save_scope_finish(1,NULL);
}

/* Nitro's save CRC is CRC-16/CCITT-FALSE (poly1021, initial FFFF).
 * Accept an absent precomputed table during early native owner initialization;
 * the result is independent of the optional acceleration table. */
u16 MATH_CalcCRC16CCITT(const MATHCRC16Table *table,const void *data,u32 size) {
    (void)table;
    const u8 *bytes=data; u16 crc=0xffff;
    for(u32 i=0;i<size;i++) {
        crc^=(u16)bytes[i]<<8;
        for(int bit=0;bit<8;bit++)crc=(u16)((crc<<1)^((crc&0x8000)?0x1021:0));
    }
    return crc;
}
void MATH_CRC16CCITTInitTable(MATHCRC16Table *table) {
    GF_ASSERT(table);
    for(unsigned i=0;i<256;i++) {
        u16 crc=(u16)(i<<8);
        for(int bit=0;bit<8;bit++)crc=(u16)((crc<<1)^((crc&0x8000)?0x1021:0));
        table->entries[i]=crc;
    }
}
void Sys_SetSleepDisableFlag(int flags) { gSystem.lidClosedPauseDisabled|=flags; }
void Sys_ClearSleepDisableFlag(int flags) { gSystem.lidClosedPauseDisabled&=~flags; }

static int extra_arguments(SaveData *save,int id,const void *payload,size_t payload_size,HGSaveExtraMode mode) {
    if(!hg_save_manager_validate_ram(save) || id<0 || id>=gNumExtraSaveChunkHeaders || !payload)
        return 0;
    if(mode!=HG_SAVE_EXTRA_MIRRORED && mode!=HG_SAVE_EXTRA_JOURNALED)return 0;
    if(mode==HG_SAVE_EXTRA_JOURNALED && id==0)return 0;
    return payload_size==gExtraSaveChunkHeaders[id].sizeFunc();
}
int hg_save_manager_write_extra(SaveData *live,int id,void *payload,size_t payload_size,HGSaveExtraMode mode,char *error,size_t error_size) {
    if(!save_scope_begin(error,error_size))return 0;
    if(setjmp(save_scope.jump))return save_scope_finish(0,NULL);
    if(!extra_arguments(live,id,payload,payload_size,mode))return save_scope_finish(0,"Invalid extra save arguments");
    save_scope.owned=manager_alloc(HEAP_ID_1,sizeof(SaveData),0);
    memcpy(save_scope.owned,live,sizeof(SaveData));
    select_owner(save_scope.owned);
    /* Footer and journal generation are original manager responsibilities. */
    void *buffer=manager_alloc(HEAP_ID_1,(u32)payload_size+sizeof(struct SaveArrayFooter),0);
    memcpy(buffer,payload,payload_size);
    int status=mode==HG_SAVE_EXTRA_JOURNALED?sub_02028230(save_scope.owned,id,buffer):WriteExtraSaveChunk(save_scope.owned,id,buffer);
    if(status!=WRITE_STATUS_SUCCESS)return save_scope_finish(0,"Extra save write incomplete");
    memcpy(payload,buffer,payload_size);
    memcpy(live,save_scope.owned,sizeof(SaveData));
    return save_scope_finish(1,NULL);
}
int hg_save_manager_read_extra(SaveData *live,int id,void *payload,size_t payload_size,HGSaveExtraMode mode,int *recovered,char *error,size_t error_size) {
    if(!save_scope_begin(error,error_size))return 0;
    if(setjmp(save_scope.jump))return save_scope_finish(0,NULL);
    if(!extra_arguments(live,id,payload,payload_size,mode))return save_scope_finish(0,"Invalid extra save arguments");
    save_scope.owned=manager_alloc(HEAP_ID_1,sizeof(SaveData),0);
    memcpy(save_scope.owned,live,sizeof(SaveData));
    select_owner(save_scope.owned);
    int status,rollback=0;
    void *buffer=mode==HG_SAVE_EXTRA_JOURNALED?sub_020284A4(save_scope.owned,HEAP_ID_1,id,&status,&rollback):ReadExtraSaveChunk(save_scope.owned,HEAP_ID_1,id,&status);
    if(status!=1)return save_scope_finish(0,"No valid extra save record");
    memcpy(payload,buffer,payload_size);
    memcpy(live,save_scope.owned,sizeof(SaveData));
    if(recovered)*recovered=rollback;
    return save_scope_finish(1,NULL);
}
