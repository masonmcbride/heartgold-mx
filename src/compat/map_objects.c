#include "compat/map_objects.h"
#include "motion_guest.h"
#include "constants/flags.h"
/* Include the complete unchanged unit so its private event initialization can
 * run at this boundary. This remains whole-source compilation, not a rewrite. */
#include HG_ORIGINAL_MAP_OBJECT

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
