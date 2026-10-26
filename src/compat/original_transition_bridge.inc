/* Explicit DS FieldSystem projection; no native pointers enter ARM registers. */
#include "ds_thumb.h"
#include "original_transition_guest.inc"
#define TRANSITION_BASE 0x02000000u
#define TRANSITION_FIELD (TRANSITION_BASE+0x8000u)
enum { TRANSITION_SAVE=0x03000000u, TRANSITION_FLAGS, TRANSITION_ROAMERS, TRANSITION_LOCAL,
       TRANSITION_LOCATION=0x03001000u, TRANSITION_PLAYER=0x03002000u };
static int transition_import(DSThumb *cpu,void *opaque) {
    FieldSystem *field=opaque;const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_transition_imports);i++)
        if(cpu->r[15]==hg_transition_imports[i].address)name=hg_transition_imports[i].name;
    if(!name){open_contract="transition: unmapped guest import";return -1;}
    if(!field){open_contract=name;return -1;}
    u32 a=cpu->r[0],b=cpu->r[1],c=cpu->r[2];
    SaveVarsFlags *flags=Save_VarsFlags_Get(field->saveData);
    RoamerSaveData *roamers=Save_Roamers_Get(field->saveData);
#define REQUIRE(x) do {if(!(x)){open_contract=name;return -1;}}while(0)
#define IMPORT(n) if(!strcmp(name,#n))
    IMPORT(Save_VarsFlags_Get){REQUIRE(a==TRANSITION_SAVE);cpu->r[0]=TRANSITION_FLAGS;}
    else IMPORT(ClearFlag972){REQUIRE(a==TRANSITION_FLAGS);ClearFlag972(flags);}
    else IMPORT(StrengthFlagAction){REQUIRE(a==TRANSITION_FLAGS);cpu->r[0]=StrengthFlagAction(flags,b);}
    else IMPORT(Save_Roamers_Get){REQUIRE(a==TRANSITION_SAVE);cpu->r[0]=TRANSITION_ROAMERS;}
    else IMPORT(RoamerSave_SetFlute){REQUIRE(a==TRANSITION_ROAMERS);RoamerSave_SetFlute(roamers,b);}
    else IMPORT(UpdatePlayerLocationHistoryIfAnyRoamersActive){REQUIRE(a==TRANSITION_ROAMERS);UpdatePlayerLocationHistoryIfAnyRoamersActive(roamers,b);}
    else IMPORT(Save_UpdateRoamersLocation){REQUIRE(a==TRANSITION_ROAMERS);Save_UpdateRoamersLocation(roamers);}
    else IMPORT(MapHeader_IsCave){cpu->r[0]=MapHeader_IsCave(a);}
    else IMPORT(MapHeader_IsBikeAllowed){cpu->r[0]=MapHeader_IsBikeAllowed(a);}
    else IMPORT(SysFlagFlashClear){REQUIRE(a==TRANSITION_FLAGS);SysFlagFlashClear(flags);}
    else IMPORT(SysFlagDefogClear){REQUIRE(a==TRANSITION_FLAGS);SysFlagDefogClear(flags);}
    else IMPORT(Save_VarsFlags_CheckSafariSysFlag){REQUIRE(a==TRANSITION_FLAGS);cpu->r[0]=Save_VarsFlags_CheckSafariSysFlag(flags);}
    else IMPORT(Save_VarsFlags_FlypointFlagAction){REQUIRE(a==TRANSITION_FLAGS);cpu->r[0]=Save_VarsFlags_FlypointFlagAction(flags,b,c);}
    else IMPORT(Save_LocalFieldData_Get){REQUIRE(a==TRANSITION_SAVE);cpu->r[0]=TRANSITION_LOCAL;}
    else IMPORT(LocalFieldData_GetPlayer){REQUIRE(a==TRANSITION_LOCAL);cpu->r[0]=TRANSITION_PLAYER;}
    else {open_contract=name;return -1;}
#undef IMPORT
#undef REQUIRE
    return 1;
}
static void transition_put(u8 *ram,unsigned offset,u32 value){for(unsigned i=0;i<4;i++)ram[offset+i]=(u8)(value>>(i*8));}
static u32 transition_run(FieldSystem *field,u32 function,u32 argument) {
    u8 *ram=calloc(1,65536);GF_ASSERT(ram&&sizeof(hg_transition_image)<0x8000);
    memcpy(ram,hg_transition_image,sizeof(hg_transition_image));
    transition_put(ram,0x800c,TRANSITION_SAVE);transition_put(ram,0x8020,TRANSITION_LOCATION);
    if(field){
    transition_put(ram,0x80ac,field->unkAC);
    memcpy(ram+0x807c,&field->reverseTurnFrameSteps,2);memcpy(ram+0x807e,&field->encounterInhibitSteps,2);
    }
    struct PlayerSaveData *player=field?LocalFieldData_GetPlayer(Save_LocalFieldData_Get(field->saveData)):NULL;
    DSNativeRegion regions[]={{TRANSITION_LOCATION,field?(u8 *)field->location:NULL,sizeof(Location)},
        {TRANSITION_PLAYER,(u8 *)player,sizeof(*player)}};
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,65536,regions,NELEMS(regions)};
    cpu.code_start=TRANSITION_BASE;cpu.code_size=HG_TRANSITION_CODE_SIZE;
    cpu.r[0]=function==HG_TRANSITION_MapHeader_GetSpawnIdForDeathWarp?argument:TRANSITION_FIELD;
    cpu.r[13]=TRANSITION_BASE+0xf000;cpu.return_address=0x02010009;cpu.r[14]=cpu.return_address;
    cpu.r[15]=function;cpu.boundary=transition_import;cpu.boundary_user=field;
    DSThumbResult result=ds_thumb_run(&cpu,100000);
    if(field){memcpy(&field->reverseTurnFrameSteps,ram+0x807c,2);memcpy(&field->encounterInhibitSteps,ram+0x807e,2);}
    free(ram);
    if(result!=DS_THUMB_RETURN||cpu.r[13]!=TRANSITION_BASE+0xf000)
        unsupported(open_contract?open_contract:"transition assembly execution/stack failure");
    return cpu.r[0];
}
static void sub_02067AE4(FieldSystem *field){(void)transition_run(field,HG_TRANSITION_sub_02067AE4,0);}
static void sub_02067A88(FieldSystem *field){(void)transition_run(field,HG_TRANSITION_sub_02067A88,0);}
static u32 transition_spawn(u32 map){return transition_run(NULL,HG_TRANSITION_MapHeader_GetSpawnIdForDeathWarp,map);}
