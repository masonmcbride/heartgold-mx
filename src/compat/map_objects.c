#include "compat/map_objects.h"
#include "motion_guest.h"
#include "field_data.h"
#include "constants/flags.h"
/* Include the complete unchanged unit so its private event initialization can
 * run at this boundary. This remains whole-source compilation, not a rewrite. */
#include HG_ORIGINAL_MAP_OBJECT

void sub_02060F78(LocalMapObject *object) { GF_ASSERT(hg_motion_sync_previous(object)); }
void sub_02061070(LocalMapObject *object) {
    FieldSystem *field=MapObject_GetFieldSystem(object);
    GF_ASSERT(field&&hg_motion_update_height(field->native_scene_resources,object));
}
void MapObject_ClearHeldMovement(LocalMapObject *object) { GF_ASSERT(hg_motion_clear_held(object)); }

int hg_map_object_init_event(LocalMapObject *object,MapObjectManager *manager,unsigned map,const ObjectEvent *source) {
    if(!object||!manager||!source||!manager->fieldSystem||source->scriptId==65535||
       source->eventFlag>=NUM_FLAGS||source->facingDirection<0||source->facingDirection>3||
       source->x>32767||source->z>32767||MapObject_CheckActive(object))return 0;
    if(source->eventFlag&&(!manager->fieldSystem->saveData||FieldSystem_FlagCheck(manager->fieldSystem,source->eventFlag)))return 0;
    if(source->spriteId>=101&&source->spriteId<=117&&!manager->fieldSystem->saveData)return 0;
    ObjectEvent event=*source;
    MapObject_InitFromObjectEvent(object,&event,manager->fieldSystem);
    sub_0205EC90(object,manager);
    MapObject_SetMapID(object,map);
    MapObject_SetMovementCommand(object,255);return 1;
}

void hg_map_object_set_map(LocalMapObject *object,unsigned map) { MapObject_SetMapID(object,map); }

void hg_map_object_set_movement(LocalMapObject *object,unsigned movement) { MapObject_SetMovement(object,movement); }

void sub_020611C8(int x,int z,VecFx32 *position) {
    GF_ASSERT(hg_motion_position_center((int)x,(int)z,position));
}
BOOL sub_02061248(FieldSystem *field,VecFx32 *position,BOOL allow_dynamic) {
    GF_ASSERT(field&&field->native_scene_resources&&position);
    int32_t height;
    int kind=hg_field_resources_height(field->native_scene_resources,position->x/65536,position->z/65536,0,position->y,&height);
    if(!kind||(kind==2&&!allow_dynamic))return FALSE;
    position->y=height;return TRUE;
}
int hg_map_object_restore_record(LocalMapObject *object,MapObjectManager *manager,const SavedMapObject *record) {
    if(!object||!manager||!record||!(record->flags&MAPOBJECTFLAG_ACTIVE)||record->currentX<0||record->currentZ<0||record->currentFacing>3||record->nextFacing>3)return 0;
    memset(object,0,sizeof(*object));
    LocalMapObject_InitFromSavedMapObject(object,(SavedMapObject *)record);
    MapObject_ConvertXZToPositionVec(object);
    MapObject_SetManager(object,manager);
    MapObject_SetMovementCommand(object,255);MapObject_SetMovementStep(object,0);
    return 1;
}
