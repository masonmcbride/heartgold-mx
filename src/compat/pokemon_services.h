#pragma once
#include "compat/pokemon.h"
#include "motion_guest.h"
/* Shared original memo/location boundary, with an explicit owned date. */
static BOOL hg_mon_location_compatible(void *date,unsigned location){(void)date;int result;GF_ASSERT(hg_motion_location_compatible(location,&result));return result;}
static int hg_mon_map_location(void *date,unsigned type,unsigned index){(void)date;unsigned result;GF_ASSERT(hg_motion_map_location(type,index,&result));return (int)result;}
static void hg_mon_copy_date(void *date,HGMonDate *out){*out=*(HGMonDate *)date;}
static HGPokemonServices hg_mon_services(HGMonDate *date){return (HGPokemonServices){date,hg_mon_location_compatible,hg_mon_map_location,hg_mon_copy_date};}
