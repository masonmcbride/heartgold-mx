#include "compat/save_c_records.h"
#include "compat/save_manager.h"
#include "ds_save_flash.h"
#include <unistd.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1); } } while(0)
static char error[256];
static void reopen(const char *path,SaveData *save) {
    ds_save_flash_close();CHECK(ds_save_flash_open(path));
    CHECK(hg_save_manager_load(save,error,sizeof(error)));
}
int main(void) {
    char dir[]="/tmp/hg-save-acceptance-XXXXXX",path[256];CHECK(mkdtemp(dir));
    snprintf(path,sizeof(path),"%s/game.sav",dir);
    CHECK(ds_save_flash_open(path));
    SaveData *live=calloc(1,sizeof(*live)),*loaded=malloc(sizeof(*loaded)),*before=malloc(sizeof(*before));CHECK(live&&loaded&&before);
    hg_save_manager_init_ram(live);CHECK(hg_save_manager_validate_ram(live));
    CHECK(gNumSaveChunkHeaders==42&&gNumExtraSaveChunkHeaders==6);
    CHECK(DWC_CheckUserData(SaveArray_Get(live,SAVE_UNK_25)));
    CHECK(SaveArray_CalcCRC16(live,"123456789",9)==0x29b1);
    CHECK(((Party *)SaveArray_Get(live,SAVE_PARTY))->core.curCount==0);
    CHECK(hg_save_manager_write(live,error,sizeof(error)));
    reopen(path,loaded);
    CHECK(((Party *)SaveArray_Get(loaded,SAVE_PARTY))->core.curCount==0);
    /* Real encrypted Pokemon populate a full party and all 540 storage slots.
     * This is a persistence fixture, not a claim that the PC menu is ported. */
    memcpy(live,loaded,sizeof(*live));hg_save_manager_bind(live);
    Party *party=SaveArray_Get(live,SAVE_PARTY);PCStorage *storage=SaveArray_Get(live,SAVE_PCSTORAGE);
    Pokemon mon;CreateMon(&mon,152,12,32,FALSE,0,OT_ID_PLAYER_ID,0);
    for(unsigned i=0;i<6;i++)CHECK(Party_AddMon(party,&mon));
    for(unsigned box=0;box<NUM_BOXES;box++)for(unsigned slot=0;slot<MONS_PER_BOX;slot++)storage->boxes[box].mons[slot]=mon.box;
    SetAllPCBoxesModified();
    PLAYERDATA *player=SaveArray_Get(live,SAVE_PLAYERDATA);player->coins=4321;player->igt.hours=123;player->igt.minutes=45;player->igt.seconds=56;
    CHECK(hg_save_manager_write(live,error,sizeof(error)));reopen(path,loaded);
    CHECK(Party_GetCount(SaveArray_Get(loaded,SAVE_PARTY))==6);
    storage=SaveArray_Get(loaded,SAVE_PCSTORAGE);
    for(unsigned box=0;box<NUM_BOXES;box++)for(unsigned slot=0;slot<MONS_PER_BOX;slot++)CHECK(GetBoxMonData(&storage->boxes[box].mons[slot],MON_DATA_SPECIES,NULL)==152);
    player=SaveArray_Get(loaded,SAVE_PLAYERDATA);CHECK(player->coins==4321&&player->igt.hours==123&&player->igt.minutes==45&&player->igt.seconds==56);
    /* Every original payload participates, including late-game records whose
     * gameplay is not yet available. Keep semantic defaults and mark a byte at
     * each payload tail, then compare the complete payload after restart. */
    memcpy(live,loaded,sizeof(*live));hg_save_manager_bind(live);
    for(int id=0;id<42;id++) {
        u32 size=gSaveChunkHeaders[id].sizeFunc();CHECK(size>0);
        ((u8 *)SaveArray_Get(live,id))[size-1]^=(u8)(id+1);
        SaveSubstruct_UpdateCRC(id);
    }
    CHECK(hg_save_manager_write(live,error,sizeof(error)));
    memcpy(before,live,sizeof(*before));reopen(path,loaded);
    for(int id=0;id<42;id++) {
        u32 size=gSaveChunkHeaders[id].sizeFunc();
        if(memcmp(SaveArray_Get(before,id),SaveArray_Get(loaded,id),size))
            fprintf(stderr,"Record %d changed across original save/load\n",id);
        CHECK(!memcmp(SaveArray_Get(before,id),SaveArray_Get(loaded,id),size));
    }
    /* A failed save must preserve the live owner, then allow an ordinary retry. */
    memcpy(live,loaded,sizeof(*live));memcpy(before,live,sizeof(*before));
    ds_save_flash_fault_after(0);
    CHECK(!hg_save_manager_write(live,error,sizeof(error))&&error[0]);
    CHECK(!memcmp(live,before,sizeof(*live)));
    ds_save_flash_fault_after(-1);CHECK(hg_save_manager_write(live,error,sizeof(error)));
    reopen(path,loaded);
    /* All six original extra callbacks and transport paths, with canaries
     * around each record. This includes the lazy Hall of Fame region. */
    for(int id=0;id<6;id++) {
        size_t length=gExtraSaveChunkHeaders[id].sizeFunc();
        u8 *guard=malloc(length+32),*copy=malloc(length);CHECK(guard&&copy);
        memset(guard,0xcc,length+32);memset(guard+16,0,length);
        hg_save_manager_bind(live);gExtraSaveChunkHeaders[id].initFunc(guard+16);
        for(unsigned k=0;k<16;k++)CHECK(guard[k]==0xcc&&guard[length+16+k]==0xcc);
        CHECK(hg_save_manager_write_extra(live,id,guard+16,length,HG_SAVE_EXTRA_MIRRORED,error,sizeof(error)));
        CHECK(hg_save_manager_read_extra(live,id,copy,length,HG_SAVE_EXTRA_MIRRORED,NULL,error,sizeof(error)));
        CHECK(!memcmp(copy,guard+16,length));
        memcpy(before,live,sizeof(*before));ds_save_flash_fault_after(0);
        CHECK(!hg_save_manager_write_extra(live,id,guard+16,length,HG_SAVE_EXTRA_MIRRORED,error,sizeof(error)));
        CHECK(!memcmp(live,before,sizeof(*live)));ds_save_flash_fault_after(-1);
        CHECK(hg_save_manager_read_extra(live,id,copy,length,HG_SAVE_EXTRA_MIRRORED,NULL,error,sizeof(error)));
        CHECK(!memcmp(copy,guard+16,length));
        if(id) {
            CHECK(hg_save_manager_write_extra(live,id,guard+16,length,HG_SAVE_EXTRA_JOURNALED,error,sizeof(error)));
            CHECK(hg_save_manager_write(live,error,sizeof(error)));
            reopen(path,loaded);memcpy(live,loaded,sizeof(*live));
            int recovered=-1;
            CHECK(hg_save_manager_read_extra(live,id,copy,length,HG_SAVE_EXTRA_JOURNALED,&recovered,error,sizeof(error)));
            CHECK(!memcmp(copy,guard+16,length));
        }
        free(copy);free(guard);
    }
    /* The latest general-data sector is corrupt: recover the prior generation
     * using original footer/CRC selection, rather than accepting damaged bytes. */
    u32 offset=(live->lastGoodSector*64+live->saveSlotSpecs[0].firstPage)*SAVE_SECTOR_SIZE;
    u8 byte;CHECK(ds_save_flash_read(offset,&byte,1));byte^=0x80;
    CHECK(ds_save_flash_write(offset,&byte,1));reopen(path,loaded);
    CHECK(Save_FileExists(loaded));
    puts("Original manager: empty/full party, 540 boxed Pokemon, coins/IGT, all42 payloads, six mirrored/five journaled extras, failure/retry and corruption recovery pass");
    hg_save_manager_bind(NULL);free(before);free(loaded);free(live);ds_save_flash_close();CHECK(!unlink(path));CHECK(!rmdir(dir));
    return 0;
}
