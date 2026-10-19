#include "compat/pokemon.h"
#include "compat/resources.h"
#include "pokemon_names.inc"
_Static_assert(sizeof(Party)==0x5b0,"Original party record layout changed");
_Static_assert(sizeof(BoxPokemon)==0x88&&sizeof(Pokemon)==0xec&&offsetof(Pokemon,party)==0x88,"Original Pokemon record layout changed");
static void native_mon_read_range(void *out,int archive,int member,u32 offset,u32 length) {
    const uint8_t *bytes;size_t size;
    GF_ASSERT(native_resource_member(archive,0,member,&bytes,&size)&&offset<=size&&length<=size-offset);
    memcpy(out,bytes+offset,length);
}
static void native_mon_read_whole(void *out,int archive,int member) {
    const uint8_t *bytes;size_t size;
    GF_ASSERT(native_resource_member(archive,0,member,&bytes,&size));
    GF_ASSERT((archive==NARC_poketool_personal_personal&&size==sizeof(BASE_STATS))||
        (archive==NARC_poketool_personal_growtbl&&size==101*4)||
        (archive==NARC_poketool_personal_wotbl&&!(size&1)&&size<=MAX_LEARNED_MOVES*sizeof(u16)&&size>=2)||
        (archive==NARC_poketool_waza_waza_tbl&&size==sizeof(MoveTbl)));
    if(archive==NARC_poketool_personal_wotbl) {
        int terminated=0;for(size_t i=0;i+1<size;i+=2)terminated|=bytes[i]==255&&bytes[i+1]==255;
        GF_ASSERT(terminated);
    }
    memcpy(out,bytes,size);
}
static void native_species_array(u16 species,enum HeapID heap,u16 *out) {
    (void)heap;GF_ASSERT(species<NELEMS(species_names));memcpy(out,species_names[species],sizeof(species_names[species]));
}
static String *native_species_name(u16 species,enum HeapID heap) {
    GF_ASSERT(species<NELEMS(species_names));String *name=String_New(11,heap);
    CopyU16ArrayToString(name,species_names[species]);return name;
}
static HGPokemonServices services;
HGPokemonServices hg_pokemon_services_exchange(HGPokemonServices next) { HGPokemonServices old=services;services=next;return old; }
static BOOL native_location(unsigned location) { GF_ASSERT(services.location_compatible);return services.location_compatible(services.owner,location); }
static int native_map_location(unsigned type,unsigned index) { GF_ASSERT(services.map_location);return services.map_location(services.owner,type,index); }
static void native_date(RTCDate *date) { GF_ASSERT(services.date);services.date(services.owner,date); }
#define GF_RTC_CopyDate native_date
#define sub_02017FE4 native_map_location
#define ReadWholeNarcMemberByIdPair native_mon_read_whole
#define ReadFromNarcMemberByIdPair native_mon_read_range
#define GetSpeciesNameIntoArray native_species_array
#define GetSpeciesName native_species_name
#define LocationIsDiamondPearlCompatible native_location
#include "pokemon_kernel.inc"

const char *hg_pokemon_species_name(unsigned species) { return species<NELEMS(species_utf8)?species_utf8[species]:NULL; }
