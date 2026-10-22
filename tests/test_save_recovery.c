/* Enumerate actual cartridge commands, not guessed offsets/state-machine steps. */
#include "compat/save_c_records.h"
#include "compat/save_manager.h"
#include "ds_save_flash.h"
#include <unistd.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s [%s]\n",__FILE__,__LINE__,#x,error);exit(1);} } while(0)
static char error[256],path[256];
static unsigned char image[DS_SAVE_FLASH_SIZE];
static uint32_t command_size;
static SaveData *live,*base,*out,*before;
static void capture(void){CHECK(ds_save_flash_read(0,image,sizeof(image)));}
static void reset(void){ds_save_flash_fault_after(-1);CHECK(ds_save_flash_write(0,image,sizeof(image)));ds_save_flash_close();CHECK(ds_save_flash_open_existing(path));memcpy(live,base,sizeof(*live));hg_save_manager_bind(live);}
static unsigned coins(SaveData *s){return ((PLAYERDATA*)SaveArray_Get(s,SAVE_PLAYERDATA))->coins;}
static void setcoins(SaveData *s,unsigned n){((PLAYERDATA*)SaveArray_Get(s,SAVE_PLAYERDATA))->coins=n;}
static void load(void){ds_save_flash_close();CHECK(ds_save_flash_open_existing(path));CHECK(hg_save_manager_load(out,error,sizeof(error)));}
static unsigned species(SaveData *s){PCStorage *p=SaveArray_Get(s,SAVE_PCSTORAGE);return GetBoxMonData(&p->boxes[7].mons[3],MON_DATA_SPECIES,NULL);}
static void change(int box){setcoins(live,222);if(box){Pokemon mon;CreateMon(&mon,155,12,32,FALSE,0,OT_ID_PLAYER_ID,0);PCStorage *p=SaveArray_Get(live,SAVE_PCSTORAGE);p->boxes[7].mons[3]=mon.box;p->boxModifiedFlag|=1u<<7;if(box==2)SetAllPCBoxesModified();}}
static void normal(int box){
    reset();change(box);CHECK(hg_save_manager_write(live,error,sizeof(error)));unsigned commands=ds_save_flash_write_count();CHECK(commands>0);
    for(unsigned n=0;n<commands;n++)for(unsigned torn=0;torn<6;torn++){
        uint32_t previous_size=command_size;reset();change(box);memcpy(before,live,sizeof(*live));
        if(torn==3)ds_save_flash_fault_sync_after(n);else ds_save_flash_fault_partial_after(n,torn==0?0:torn==1?1:torn==4?previous_size/2:torn==5?previous_size-1:UINT32_MAX);
        int ok=hg_save_manager_write(live,error,sizeof(error));
        if(!ok)CHECK(!memcmp(live,before,sizeof(*live)));
        command_size=ds_save_flash_last_write_size();load();CHECK(coins(out)==111||coins(out)==222);
        if(box)CHECK(species(out)==(coins(out)==111?152:155));
    }
    printf("Normal %s: %u command boundaries x absent/one-byte/half/final-byte/whole-command/sync-failure interruption\n",box==2?"all-boxes":box?"single-box":"unchanged-box",commands);
}
static void first_save(void){
    memcpy(base,live,sizeof(*base));capture();CHECK(hg_save_manager_write(live,error,sizeof(error)));unsigned commands=ds_save_flash_write_count();
    for(unsigned n=0;n<commands;n++)for(unsigned torn=0;torn<6;torn++){
        uint32_t previous_size=command_size;reset();memcpy(before,live,sizeof(*live));
        if(torn==3)ds_save_flash_fault_sync_after(n);else ds_save_flash_fault_partial_after(n,torn==0?0:torn==1?1:torn==4?previous_size/2:torn==5?previous_size-1:UINT32_MAX);
        int ok=hg_save_manager_write(live,error,sizeof(error));command_size=ds_save_flash_last_write_size();if(!ok)CHECK(!memcmp(live,before,sizeof(*live)));
        ds_save_flash_close();CHECK(ds_save_flash_open_existing(path));memset(out,0xcc,sizeof(*out));memcpy(before,out,sizeof(*out));
        if(hg_save_manager_load(out,error,sizeof(error)))CHECK(coins(out)==111&&species(out)==152);else CHECK(!memcmp(before,out,sizeof(*out)));
    }
    reset();printf("First-ever save: %u command boundaries x 6\n",commands);
}
static void extra(int id,HGSaveExtraMode mode){
    size_t size=gExtraSaveChunkHeaders[id].sizeFunc();u8 *old=calloc(1,size),*next=calloc(1,size),*read=malloc(size);CHECK(old&&next&&read);
    reset();gExtraSaveChunkHeaders[id].initFunc(old);old[size-1]=0x31;
    CHECK(hg_save_manager_write_extra(live,id,old,size,mode,error,sizeof(error)));
    CHECK(hg_save_manager_write(live,error,sizeof(error)));
    /* Establish the second journal generation too, so both mirror slots exist. */
    CHECK(hg_save_manager_write_extra(live,id,old,size,mode,error,sizeof(error)));
    CHECK(hg_save_manager_write(live,error,sizeof(error)));memcpy(base,live,sizeof(*base));capture();
    memcpy(next,old,size);next[size-1]=0x72;
    reset();CHECK(hg_save_manager_write_extra(live,id,next,size,mode,error,sizeof(error)));unsigned commands=ds_save_flash_write_count();
    CHECK(commands>0);
    for(unsigned n=0;n<commands;n++)for(unsigned torn=0;torn<6;torn++){
        uint32_t previous_size=command_size;reset();memcpy(before,live,sizeof(*live));memcpy(next,old,size);next[size-1]=0x72;
        if(torn==3)ds_save_flash_fault_sync_after(n);else ds_save_flash_fault_partial_after(n,torn==0?0:torn==1?1:torn==4?previous_size/2:torn==5?previous_size-1:UINT32_MAX);
        int ok=hg_save_manager_write_extra(live,id,next,size,mode,error,sizeof(error));
        if(!ok)CHECK(!memcmp(live,before,sizeof(*live)));
        command_size=ds_save_flash_last_write_size();load();CHECK(hg_save_manager_read_extra(out,id,read,size,mode,NULL,error,sizeof(error)));
        CHECK(read[size-1]==0x31||read[size-1]==0x72);
        /* Journal refs have not been published: a new orphan is never selected. */
        if(mode==HG_SAVE_EXTRA_JOURNALED)CHECK(read[size-1]==0x31);
    }
    if(mode==HG_SAVE_EXTRA_JOURNALED){
        reset();memcpy(next,old,size);next[size-1]=0x72;CHECK(hg_save_manager_write_extra(live,id,next,size,mode,error,sizeof(error)));setcoins(live,222);CHECK(hg_save_manager_write(live,error,sizeof(error)));unsigned count=ds_save_flash_write_count()-commands;
        for(unsigned n=0;n<count;n++)for(unsigned torn=0;torn<6;torn++){
            uint32_t previous_size=command_size;reset();memcpy(next,old,size);next[size-1]=0x72;CHECK(hg_save_manager_write_extra(live,id,next,size,mode,error,sizeof(error)));setcoins(live,222);
            if(torn==3)ds_save_flash_fault_sync_after(n);else ds_save_flash_fault_partial_after(n,torn==0?0:torn==1?1:torn==4?previous_size/2:torn==5?previous_size-1:UINT32_MAX);(void)hg_save_manager_write(live,error,sizeof(error));command_size=ds_save_flash_last_write_size();load();
            CHECK(hg_save_manager_read_extra(out,id,read,size,mode,NULL,error,sizeof(error)));CHECK(coins(out)==111||coins(out)==222);CHECK(read[size-1]==(coins(out)==111?0x31:0x72));
        }
        printf("Journal %d: %u extra + %u normal publication boundaries x 6\n",id,commands,count);
    }else printf("Mirrored %d: %u command boundaries x 6\n",id,commands);
    for(unsigned operation=0;operation<2;operation++){
        unsigned failures=0;
        for(int n=0;n<32;n++){
            reset();memcpy(before,live,sizeof(*before));memcpy(next,old,size);hg_save_manager_fault_alloc_after(n);
            int ok=operation?hg_save_manager_read_extra(live,id,next,size,mode,NULL,error,sizeof(error)):hg_save_manager_write_extra(live,id,next,size,mode,error,sizeof(error));hg_save_manager_fault_alloc_after(-1);
            if(ok)break;
            CHECK(!memcmp(before,live,sizeof(*before)));CHECK(!memcmp(next,old,size));failures++;
        }
        CHECK(failures>0&&failures<32);
    }
    /* Independently corrupt each extra copy, then both. */
    for(unsigned which=0;which<3;which++){
        reset();u8 bad;
        for(unsigned copy=0;copy<2;copy++)if(which==2||which==copy){u32 off=(gExtraSaveChunkHeaders[id].sector+64*copy)*SAVE_SECTOR_SIZE;CHECK(ds_save_flash_read(off+size,&bad,1));bad^=0x80;CHECK(ds_save_flash_write(off+size,&bad,1));}
        memset(read,0xcc,size);memcpy(next,read,size);
        int ok=hg_save_manager_read_extra(live,id,read,size,mode,NULL,error,sizeof(error));
        if(which==2){CHECK(!ok);CHECK(!memcmp(next,read,size));}else if(mode==HG_SAVE_EXTRA_MIRRORED)CHECK(ok);else if(!ok)CHECK(!memcmp(next,read,size));
    }
    free(read);free(next);free(old);
}
int main(void){
    char dir[]="/tmp/hg-recovery-XXXXXX";CHECK(mkdtemp(dir));snprintf(path,sizeof(path),"%s/game.sav",dir);CHECK(ds_save_flash_open(path));
    live=malloc(sizeof(*live));base=malloc(sizeof(*base));out=malloc(sizeof(*out));before=malloc(sizeof(*before));CHECK(live&&base&&out&&before);hg_save_manager_init_ram(live);hg_save_manager_bind(live);setcoins(live,111);
    Pokemon mon;CreateMon(&mon,152,12,32,FALSE,0,OT_ID_PLAYER_ID,0);PCStorage *p=SaveArray_Get(live,SAVE_PCSTORAGE);p->boxes[7].mons[3]=mon.box;SetAllPCBoxesModified();
    first_save();CHECK(hg_save_manager_write(live,error,sizeof(error)));CHECK(hg_save_manager_write(live,error,sizeof(error)));memcpy(base,live,sizeof(*base));capture();normal(0);normal(1);normal(2);
    /* Cancellation runs the original async state machine and Save_Cancel at
     * every scheduler step, including states with an outstanding CARD lock. */
    unsigned cancelled=0;
    for(unsigned step=0;step<200;step++){
        reset();change(1);memcpy(before,live,sizeof(*live));int done=-1;
        CHECK(hg_save_manager_write_steps(live,step,&done,error,sizeof(error)));
        if(!done)CHECK(!memcmp(before,live,sizeof(*live)));
        command_size=ds_save_flash_last_write_size();load();CHECK(coins(out)==111||coins(out)==222);CHECK(species(out)==(coins(out)==111?152:155));
        if(done)break;cancelled++;
    }
    CHECK(cancelled>0&&cancelled<200);printf("Original asynchronous cancellation: %u scheduler states\n",cancelled);
    /* Fail every original manager allocation until one attempt succeeds. */
    for(unsigned operation=0;operation<2;operation++){
        unsigned failures=0;
        for(int n=0;n<32;n++){
            reset();memcpy(before,operation?out:live,sizeof(*before));hg_save_manager_fault_alloc_after(n);
            int ok=operation?hg_save_manager_load(out,error,sizeof(error)):hg_save_manager_write(live,error,sizeof(error));hg_save_manager_fault_alloc_after(-1);
            if(ok)break;
            CHECK(!memcmp(before,operation?out:live,sizeof(*before)));failures++;
        }
        CHECK(failures>0&&failures<32);printf("%s allocation failures: %u\n",operation?"Load":"Write",failures);
    }
    for(int id=0;id<6;id++)extra(id,HG_SAVE_EXTRA_MIRRORED);
    for(int id=1;id<6;id++)extra(id,HG_SAVE_EXTRA_JOURNALED);
    reset();for(unsigned slot=0;slot<2;slot++){u32 off=(slot*64+live->saveSlotSpecs[0].firstPage)*SAVE_SECTOR_SIZE;u8 b;CHECK(ds_save_flash_read(off,&b,1));b^=0x80;CHECK(ds_save_flash_write(off,&b,1));}
    memset(out,0xcc,sizeof(*out));memcpy(before,out,sizeof(*out));CHECK(!hg_save_manager_load(out,error,sizeof(error)));CHECK(!memcmp(before,out,sizeof(*out)));
    hg_save_manager_bind(NULL);free(before);free(out);free(base);free(live);ds_save_flash_close();CHECK(!unlink(path));CHECK(!rmdir(dir));puts("Recovery: every observed command boundary, torn writes, coherent generation selection, extra-copy fallback and both-invalid rejection pass");return 0;
}
