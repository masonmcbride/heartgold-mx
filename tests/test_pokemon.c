#include "compat/pokemon.h"
#include <stdio.h>
#include "motion_guest.h"
#include "map_header.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static BOOL compatible(void *owner,unsigned location) { (void)owner;int result;CHECK(hg_motion_location_compatible(location,&result));return result; }
static int map_location(void *owner,unsigned type,unsigned index) { (void)owner;unsigned result;CHECK(hg_motion_map_location(type,index,&result));return (int)result; }
static void date(void *owner,HGMonDate *out) { (void)owner;*out=(HGMonDate){26,10,17,6}; }
int main(void) {
    HGPokemonServices previous=hg_pokemon_services_exchange((HGPokemonServices){.location_compatible=compatible,.map_location=map_location,.date=date});
    PlayerProfile profile;PlayerProfile_Init(&profile);profile.id=0x12345678;profile.gender=1;
    const u16 name[]={0x12b,0xffff};memcpy(profile.name,name,sizeof(name));
    const unsigned species[]={152,155,158},hp[]={20,19,20},moves[]={33,33,10};
    for(unsigned n=0;n<3;n++) {
        Pokemon mon;SetLCRNGSeed(0x12345678);CreateMon(&mon,species[n],5,32,FALSE,0,OT_ID_PLAYER_ID,0);
        CHECK(GetMonData(&mon,MON_DATA_SPECIES,NULL)==species[n]);
        CHECK(GetMonData(&mon,MON_DATA_LEVEL,NULL)==5);
        CHECK(GetMonData(&mon,MON_DATA_MAX_HP,NULL)==hp[n]);
        CHECK(GetMonData(&mon,MON_DATA_HP,NULL)==GetMonData(&mon,MON_DATA_MAX_HP,NULL));
        CHECK(GetMonData(&mon,MON_DATA_MOVE1,NULL)==moves[n]&&GetMonData(&mon,MON_DATA_MOVE1_PP,NULL)==35);
        CHECK(!mon.box.checksumFailed&&!mon.box.partyDecrypted&&!mon.box.boxDecrypted);
        sub_020720FC(&mon,&profile,BALL_POKE,MapHeader_GetMapSec(61),12,HEAP_ID_FIELD2);
        CHECK(GetMonData(&mon,MON_DATA_OT_ID,NULL)==profile.id&&GetMonData(&mon,MON_DATA_OT_GENDER,NULL)==1);
        CHECK(GetMonData(&mon,MON_DATA_MET_LOCATION,NULL)==MapHeader_GetMapSec(61));
        CHECK(GetMonData(&mon,MON_DATA_MET_YEAR,NULL)==26&&GetMonData(&mon,MON_DATA_MET_MONTH,NULL)==10&&GetMonData(&mon,MON_DATA_MET_DAY,NULL)==17);
        u16 ot_name[8];GetMonData(&mon,MON_DATA_OT_NAME,ot_name);CHECK(ot_name[0]==name[0]&&ot_name[1]==0xffff);
        Pokedex dex;Save_Pokedex_Init(&dex);CHECK(!Pokedex_CheckMonCaughtFlag(&dex,species[n]));
        Pokedex_SetMonCaughtFlag(&dex,&mon);CHECK(Pokedex_CheckMonCaughtFlag(&dex,species[n]));
        Pokemon encrypted=mon;for(unsigned i=0;i<100;i++)CHECK(GetMonData(&mon,MON_DATA_SPECIES,NULL)==species[n]);
        CHECK(!memcmp(&mon,&encrypted,sizeof(mon)));
        Party party;Party_InitWithMaxSize(&party,6);CHECK(!party.core.curCount);
        CHECK(Party_AddMon(&party,&mon)&&party.core.curCount==1);
        CHECK(GetMonData(Party_GetMonByIndex(&party,0),MON_DATA_SPECIES,NULL)==species[n]);
        for(unsigned i=1;i<6;i++)CHECK(Party_AddMon(&party,&mon));
        CHECK(party.core.curCount==6&&!Party_AddMon(&party,&mon));
        printf("Original Pokemon %u: HP %u, move %u, encrypted round-trip and party insertion passed\n",species[n],GetMonData(&mon,MON_DATA_HP,NULL),GetMonData(&mon,MON_DATA_MOVE1,NULL));
    }
    hg_pokemon_services_exchange(previous);return 0;
}
