#include "compat/field_queries.h"
#include "compat/pokemon.h"
#include "ds_thumb.h"
#include "constants/save_arrays.h"
SaveVarsFlags *hg_story_save_vars(SaveData *);
BOOL CheckGameClearFlag(SaveVarsFlags *);
#include "field_queries_guest.inc"
#define BASE 0x02000000u
#define FIELD 0x03000000u
#define SAVE 0x03000004u
#define FLAGS 0x03000008u
#define DEX 0x0300000cu
#define FRONTIER (BASE+0x8000)
static int boundary(DSThumb *cpu,void *opaque){
    FieldSystem *field=opaque;const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_query_guest_imports);i++)if(cpu->r[15]==hg_query_guest_imports[i].address){name=hg_query_guest_imports[i].name;break;}
    if(!name)return -1;
#define IS(n) (!strcmp(name,n))
    if(IS("FieldSystem_GetSaveData")&&cpu->r[0]==FIELD){cpu->r[0]=SAVE;return 1;}
    if(cpu->r[0]==SAVE){
        if(IS("Save_GameStats_Get")){cpu->r[0]=0x03000010u;return 1;}
        if(IS("Save_VarsFlags_Get")){cpu->r[0]=FLAGS;return 1;}
        if(IS("Save_Pokedex_Get")){cpu->r[0]=DEX;return 1;}
        if(IS("Save_Frontier_GetStatic")){cpu->r[0]=FRONTIER;return 1;}
    }
    if(IS("CheckGameClearFlag")&&cpu->r[0]==FLAGS){cpu->r[0]=CheckGameClearFlag(hg_story_save_vars(field->saveData));return 1;}
    if(IS("Save_VarsFlags_CheckFlagInArray")&&cpu->r[0]==FLAGS&&cpu->r[1]<NUM_FLAGS){cpu->r[0]=Save_VarsFlags_CheckFlagInArray(hg_story_save_vars(field->saveData),cpu->r[1]);return 1;}
    if(IS("Pokedex_NationalDexIsComplete")&&cpu->r[0]==DEX){cpu->r[0]=Pokedex_NationalDexIsComplete(hg_story_save_dex(field->saveData));return 1;}
    return -1;
}
unsigned hg_field_trainer_stars(FieldSystem *field){
    GF_ASSERT(field&&field->saveData&&sizeof(hg_query_guest_image)<0x8000);
    u8 ram[65536]={0};memcpy(ram,hg_query_guest_image,sizeof(hg_query_guest_image));
    /* Original static Frontier owner is rebound to this session's saved record. */
    memcpy(ram+0x8000,SaveArray_Get(field->saveData,SAVE_UNK_19),0x1628);
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,sizeof(ram)};cpu.code_start=BASE;cpu.code_size=HG_QUERY_GUEST_CODE_SIZE;
    cpu.r[0]=FIELD;cpu.r[15]=HG_QUERY_GUEST_sub_020691E8;cpu.r[13]=BASE+0xf000;cpu.return_address=0x02200001;cpu.r[14]=cpu.return_address;cpu.boundary=boundary;cpu.boundary_user=field;
    DSThumbResult result=ds_thumb_run(&cpu,10000);GF_ASSERT(result==DS_THUMB_RETURN&&cpu.r[13]==BASE+0xf000&&cpu.r[0]<=5);return cpu.r[0];
}
