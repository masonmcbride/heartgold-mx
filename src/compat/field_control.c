#include "compat/field_control.h"
#include "compat/pokemon.h"
#include "motion_guest.h"
#include "metatile_behavior.h"
#include "constants/std_script.h"
#include "constants/maps.h"
#include "constants/flags.h"
typedef struct { FieldSystem field;int failed; } InteractionBoundary;
static InteractionBoundary *boundary(FieldSystem *field) { return (InteractionBoundary *)(void *)field; }
static u8 native_behavior(FieldSystem *field,int x,int z) {
    HGMotionTile tile;
    if(!hg_motion_tile(field->native_scene_resources,x,z,&tile)){boundary(field)->failed=1;return 255;}
    return tile.behavior;
}
static BOOL native_wall(FieldSystem *field,int x,int z) {
    HGMotionTile tile;
    if(!hg_motion_tile(field->native_scene_resources,x,z,&tile)){boundary(field)->failed=1;return TRUE;}
    return tile.wall;
}
static BOOL native_surf(PlayerAvatar *avatar,u8 standing,u8 facing) {
    int eligible=0;
    /* The caller checks the static predicate once before the original selector
     * so an unavailable guest import cannot become a negative gameplay answer. */
    GF_ASSERT(hg_motion_can_surf(avatar,standing,facing,&eligible));return eligible;
}
static BOOL native_rock_climb(int facing,int direction) { int eligible=0;GF_ASSERT(hg_motion_rock_climb(facing,direction,&eligible));return eligible; }
static int native_unavailable(FieldSystem *field) { boundary(field)->failed=1;return 0; }
static BOOL native_safari_available(FieldSystem *field) {
    /* Original ov02_0224E308 recognizes map 357. Its save-dependent placement
     * capability is not admitted; all other maps have no Safari placement. */
    return field->location->mapId==MAP_SAFARI_ZONE_ENTRANCE_EXTERIOR?native_unavailable(field):FALSE;
}
static BOOL native_safari_flag(SaveVarsFlags *vars) { return Save_VarsFlags_CheckFlagInArray(vars,FLAG_SYS_SAFARI); }
SaveVarsFlags *hg_story_save_vars(SaveData *);
int GetIdxOfFirstPartyMonWithMove(Party *,u16);
#define GetMetatileBehavior native_behavior
#define sub_020548C0 native_wall
#define Field_PlayerCanSurfOnTile native_surf
#define MetatileBehavior_IsRockClimbInDirection native_rock_climb
#define Save_PlayerData_GetProfile hg_story_save_profile
#define SaveArray_Party_Get hg_story_save_party
#define Save_VarsFlags_Get hg_story_save_vars
#define Save_VarsFlags_CheckSafariSysFlag native_safari_flag
#define ov02_0224E35C native_safari_available
#define ov02_0224E4CC(...) native_unavailable(fieldSystem)
#define ov02_0224E4DC(...) native_unavailable(fieldSystem)
#define sub_0202F620(...) native_unavailable(fieldSystem)
#include "interaction_control_kernel.inc"
int hg_field_metatile_script(FieldSystem *source,unsigned map,unsigned *script) {
    if(!source||!source->playerAvatar||!source->saveData||!source->native_scene_resources||!script||map>=MAP_ID_MAX)return 0;
    InteractionBoundary b={.field=*source};Location location={.mapId=(int)map};b.field.location=&location;
    u8 standing=native_behavior(&b.field,PlayerAvatar_GetXCoord(source->playerAvatar),PlayerAvatar_GetZCoord(source->playerAvatar));
    u8 facing=FieldSystem_FacingMetatileBehavior(&b.field);int eligible;
    if(b.failed||!hg_motion_can_surf(source->playerAvatar,standing,facing,&eligible))return 0;
    unsigned selected=GetInteractedMetatileScript(&b.field,facing);
    if(b.failed)return 0;*script=selected;return 1;
}
