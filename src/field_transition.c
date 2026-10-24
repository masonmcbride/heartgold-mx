#include "compat/transitions.h"
#include "field_transition.h"
#include "field_events.h"
#include "field_data.h"
#include "motion_guest.h"
#include "metatile_behavior.h"
#include "constants/maps.h"
#include "map_header.h"
#include "compat/resources.h"
#include HG_ORIGINAL_MAP_EVENTS
_Static_assert(sizeof(BgEvent)==20&&sizeof(ObjectEvent)==32&&sizeof(WarpEvent)==12&&sizeof(CoordEvent)==16,"Native event record widths differ from validated disk arrays");
u16 GetMomSpawnId(void) { return (u16)hg_field_guest_mom_spawn(); }
void GF_AssertFail(void) { fputs("Invalid original field transition state\n",stderr);abort(); }

struct HGTransition {
    FieldSystem field;LocalFieldData *local;Location *location;
    PlayerSaveData *previous_player_save;
    HGFieldResources *resources;
    HGWarpDestination request;int requested,failed;
    uint8_t *pending_local,*pending_source;int pending,pending_x,pending_z;
};
int hg_transition_supported(unsigned map) {
    if(map>=MAP_ID_MAX)return 0;
    static signed char cached[MAP_ID_MAX];if(cached[map])return cached[map]>0;
    const uint8_t *bytes;size_t size;HGEventView events;
    int ready=hg_event_load(map,&events)&&
        native_resource_member(NARC_fielddata_script_scr_seq,0,MapHeader_GetScriptsBank(map),&bytes,&size)&&size&&
        native_resource_member(NARC_fielddata_script_scr_seq,0,MapHeader_GetScriptHeaderBank(map),&bytes,&size)&&size;
    cached[map]=ready?1:-1;return ready;
}
static HGTransition *owner(FieldSystem *field) { return (HGTransition *)((char *)field-(size_t)offsetof(HGTransition,field)); }
static LocalFieldData *native_local(SaveData *save) { return ((HGTransition *)(void *)save)->local; }
static int load_events(HGTransition *t,unsigned map) {
    HGEventView view;if(!hg_transition_supported(map)||!hg_event_load(map,&view))return 0;
    /* Original parser follows the four serialized arrays. Validate lengths
     * first, then copy into its aligned, mutable native-owned buffer. */
    size_t size=(size_t)(view.coordinates+16*view.coordinate_count-view.backgrounds)+4;
    if(size>=sizeof(t->field.mapEvents->event_data))return 0;
    memset(t->field.mapEvents,0,sizeof(*t->field.mapEvents));
    memcpy(t->field.mapEvents->event_data,view.backgrounds-4,size);
    MapEvents_ComputeRamHeader(t->field.mapEvents);return 1;
}
static void native_init_events(FieldSystem *field,u32 map) { if(!load_events(owner(field),map))owner(field)->failed=1; }
static u8 native_behavior(FieldSystem *field,int x,int z) {
    HGMotionTile tile;HGTransition *t=owner(field);
    if(!hg_motion_tile(t->resources,x,z,&tile)){t->failed=1;return 255;}return tile.behavior;
}
static BOOL native_wall(FieldSystem *field,int x,int z) {
    HGMotionTile tile;HGTransition *t=owner(field);
    if(!hg_motion_tile(t->resources,x,z,&tile)){t->failed=1;return TRUE;}return tile.wall;
}
static void capture(FieldSystem *field,u32 map,int warp,int x,int z,int direction,unsigned kind) {
    HGTransition *t=owner(field);if(t->requested){t->failed=1;return;}
    t->request=(HGWarpDestination){(int)map,warp,x,z,direction,kind};t->requested=1;
}
static void native_transition(FieldSystem *field,u32 map,int warp,int x,int z,int direction,int kind) { capture(field,map,warp,x,z,direction,(unsigned)kind); }
static void native_exit(FieldSystem *field,u32 map,int warp,int x,int z,int direction) { capture(field,map,warp,x,z,direction,0); }
/* Step probing currently admits downward ladders only. Other branches of the
 * unchanged selector remain bounded; warp-panel tasks have no host owner yet. */
static void *native_warp_panel(FieldSystem *field,u32 map,int warp) { (void)map;(void)warp;owner(field)->failed=1;return NULL; }
#define Save_LocalFieldData_Get native_local
#define Field_InitMapEvents native_init_events
#define GetMetatileBehavior native_behavior
#define sub_020548C0 native_wall
#define NewFieldTransitionEnvironment native_transition
#define sub_02055CD8 native_exit
#define sub_02053E08 native_warp_panel
#include "transition_kernel.inc"
HGFieldLocation hg_field_new_game_location(void) {
    Location location;Location_SetToPlayerRoom(&location);
    return (HGFieldLocation){location.mapId,location.warpId,location.x,location.y,location.direction};
}

HGTransition *hg_transition_new(unsigned map,PlayerAvatar *avatar) {
    if(!avatar||!PlayerAvatar_GetPlayerSaveData(avatar)||!PlayerAvatar_GetMapObject(avatar)||!hg_transition_supported(map))return NULL;
    HGTransition *t=calloc(1,sizeof(*t));if(!t)return NULL;
    t->local=calloc(1,Save_LocalFieldData_sizeof());t->pending_local=malloc(Save_LocalFieldData_sizeof());t->pending_source=malloc(Save_LocalFieldData_sizeof());
    if(!t->local||!t->pending_local||!t->pending_source)goto fail;
    Save_LocalFieldData_Init(t->local);
    t->location=LocalFieldData_GetCurrentPosition(t->local);
    t->field.playerAvatar=avatar;t->field.location=t->location;t->field.saveData=(SaveData *)(void *)t;
    Field_AllocateMapEvents(&t->field,HEAP_ID_DEFAULT);
    if(!load_events(t,map))goto fail;
    *t->location=(Location){(int)map,-1,0,0,0};
    t->previous_player_save=PlayerAvatar_GetPlayerSaveData(avatar);
    *LocalFieldData_GetPlayer(t->local)=*t->previous_player_save;
    avatar->playerSaveData=LocalFieldData_GetPlayer(t->local);return t;
fail:hg_transition_free(t);return NULL;
}
void hg_transition_free(HGTransition *t) {
    if(!t)return;if(t->field.mapEvents)Field_FreeMapEvents(&t->field);
    if(t->previous_player_save) {
        *t->previous_player_save=*LocalFieldData_GetPlayer(t->local);
        t->field.playerAvatar->playerSaveData=t->previous_player_save;
    }
    free(t->local);free(t->pending_local);free(t->pending_source);free(t);
}
static int probe(HGTransition *t,HGFieldResources *resources,unsigned direction,int step,HGWarpDestination *out) {
    if(!t)return 0;t->pending=0;
    if(!resources||direction>3||!out)return 0;t->failed=t->requested=0;t->resources=resources;
    size_t size=Save_LocalFieldData_sizeof();uint8_t saved[size];memcpy(saved,t->local,size);Location before=*t->location;
    t->location->x=(int)PlayerAvatar_GetXCoord(t->field.playerAvatar);t->location->y=(int)PlayerAvatar_GetZCoord(t->field.playerAvatar);
    t->location->direction=(int)PlayerAvatar_GetFacingDirection(t->field.playerAvatar);
    FieldInput input={.transitionDir=(s8)direction};
    int selected;
    if(step){
        u8 behavior=GetMetatileBehavior(&t->field,t->location->x,t->location->y);
        selected=!t->failed&&MetatileBehavior_IsLadderDown(behavior)&&FieldSystem_CheckTransition(&t->field,t->location->x,t->location->y,behavior);
    }else selected=FieldSystem_CheckMapTransition(&t->field,&input);
    int ok=selected&&t->requested&&!t->failed&&hg_transition_supported((unsigned)t->request.map);
    if(ok) {
        HGEventView destination;
        ok=hg_event_load((unsigned)t->request.map,&destination)&&t->request.warp>=0&&(unsigned)t->request.warp<destination.warp_count;
    }
    if(ok&&step)FieldSystem_TrySetMapConnection(&t->field);
    if(ok) {
        Location next={t->request.map,t->request.warp,t->request.x,t->request.z,t->request.direction};
        sub_02052F94(&t->field,&next);
        ok=!t->failed;
        if(ok){t->request.x=t->location->x;t->request.z=t->location->y;memcpy(t->pending_local,t->local,size);}
    }
    memcpy(t->local,saved,size);*t->location=before;
    if(!load_events(t,(unsigned)before.mapId))return 0;
    if(!ok)return 0;
    memcpy(t->pending_source,saved,size);t->pending_x=(int)PlayerAvatar_GetXCoord(t->field.playerAvatar);t->pending_z=(int)PlayerAvatar_GetZCoord(t->field.playerAvatar);
    t->pending=1;*out=t->request;return 1;
}
int hg_transition_probe(HGTransition *t,HGFieldResources *resources,unsigned direction,HGWarpDestination *out){return probe(t,resources,direction,0,out);}
int hg_transition_probe_step(HGTransition *t,HGFieldResources *resources,HGWarpDestination *out){return probe(t,resources,0,1,out);}
int hg_transition_commit(HGTransition *t,const HGWarpDestination *destination) {
    if(!t||!destination||!t->pending||memcmp(destination,&t->request,sizeof(*destination)))return 0;
    if(memcmp(t->local,t->pending_source,Save_LocalFieldData_sizeof())||
       (int)PlayerAvatar_GetXCoord(t->field.playerAvatar)!=t->pending_x||(int)PlayerAvatar_GetZCoord(t->field.playerAvatar)!=t->pending_z||
       MapObject_GetMovementCommand(PlayerAvatar_GetMapObject(t->field.playerAvatar))!=255)return 0;
    if(!load_events(t,(unsigned)destination->map))return 0;
    memcpy(t->local,t->pending_local,Save_LocalFieldData_sizeof());t->pending=0;return 1;
}
int hg_transition_commit_zone(HGTransition *t,unsigned map) {
    if(!t||!hg_transition_supported(map)||!load_events(t,map))return 0;
    t->location->mapId=(int)map;t->pending=0;hg_transition_sync_player(t);return 1;
}
int hg_transition_inspect(HGTransition *t,HGTransitionState *out) {
    if(!t||!out)return 0;
    const Location *locations[]={LocalFieldData_GetCurrentPosition(t->local),LocalFieldData_GetPreviousPosition(t->local),
        LocalFieldData_GetEntrancePosition(t->local),LocalFieldData_GetDynamicWarp(t->local)};
    HGFieldLocation *values[]={&out->current,&out->previous,&out->entrance,&out->dynamic};
    for(unsigned i=0;i<4;i++)*values[i]=(HGFieldLocation){locations[i]->mapId,locations[i]->warpId,locations[i]->x,locations[i]->y,locations[i]->direction};
    return 1;
}
void hg_transition_sync_player(HGTransition *t) {
    if(!t)return;
    t->location->x=(int)PlayerAvatar_GetXCoord(t->field.playerAvatar);
    t->location->y=(int)PlayerAvatar_GetZCoord(t->field.playerAvatar);
    t->location->direction=(int)PlayerAvatar_GetFacingDirection(t->field.playerAvatar);
}

void hg_transition_export_local(const HGTransition *t,void *record) {
    GF_ASSERT(t&&record);memcpy(record,t->local,Save_LocalFieldData_sizeof());
}
void hg_transition_restore_local(HGTransition *t,const void *record) {
    GF_ASSERT(t&&record);memcpy(t->local,record,Save_LocalFieldData_sizeof());
}

int hg_transition_relocate_record(void *record,const HGFieldLocation *destination) {
    if(!record||!destination||destination->warp<-1||destination->map<0||destination->x<0||destination->z<0||destination->direction<0||destination->direction>3||!hg_transition_supported((unsigned)destination->map))return 0;
    HGEventView events;if(!hg_event_load((unsigned)destination->map,&events)||(destination->warp>=0&&(unsigned)destination->warp>=events.warp_count))return 0;
    HGTransition t={0};t.local=malloc(Save_LocalFieldData_sizeof());if(!t.local)return 0;
    memcpy(t.local,record,Save_LocalFieldData_sizeof());t.location=LocalFieldData_GetCurrentPosition(t.local);
    t.field.location=t.location;t.field.saveData=(SaveData *)(void *)&t;
    Field_AllocateMapEvents(&t.field,HEAP_ID_DEFAULT);
    Location next={destination->map,destination->warp,destination->x,destination->z,destination->direction};
    sub_02052F94(&t.field,&next);
    int ok=!t.failed;
    if(ok)memcpy(record,t.local,Save_LocalFieldData_sizeof());
    Field_FreeMapEvents(&t.field);free(t.local);return ok;
}
