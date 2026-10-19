#include "compat/follow.h"
#include "compat/map_objects.h"
#include "compat/resources.h"
#include "constants/species.h"
#include "constants/follow_mon_idx.h"
#include "constants/pokemon.h"
#include "constants/sprites.h"
#include "constants/std_script.h"
#include "constants/maps.h"
#define FOLLOW_MON_PARAM_SPECIES 0
static LocalMapObject *allocate_object(MapObjectManager *manager,u32 x,u32 z,u32 direction,u32 sprite,u32 movement,u32 map) {
    LocalMapObject *slot=NULL;
    for(unsigned i=0;i<manager->objectCount;i++)if(!MapObject_CheckActive(manager->objects+i)){slot=manager->objects+i;break;}
    if(!slot)return NULL;
    ObjectEvent event={.spriteId=sprite,.movement=movement,.facingDirection=direction,.x=x,.z=z};
    if(!hg_map_object_init_event(slot,manager,map,&event))return NULL;
    return slot;
}
static void read_params(void *out,int archive,int member) {
    const uint8_t *bytes;size_t size;
    GF_ASSERT(archive==NARC_fielddata_tsurepoke_tp_param&&native_resource_member(archive,0,member,&bytes,&size)&&size==4);
    memcpy(out,bytes,4);
}
#define MapObject_Create allocate_object
#define ReadWholeNarcMemberByIdPair read_params
#include "follow_decls.inc"
#include "follow_kernel.inc"
LocalMapObject *hg_follow_create(MapObjectManager *manager,unsigned species,unsigned form,unsigned gender,unsigned direction,int x,int z,int shiny) {
    if(!manager||species<1||species>NATIONAL_DEX_COUNT||form>(unsigned)OverworldModelLookupFormCount(species)||gender>2||direction>3||x<0||z<0)return NULL;
    return FollowMon_CreateMapObject(manager,species,form,gender,direction,x,z,shiny);
}
