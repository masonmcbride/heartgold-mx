#include "compat/motion.h"
#include "compat/story.h"
#include "field_data.h"
#include "field_transition.h"
#include "choose_starter.h"
#include "compat/follow.h"
#include "compat/map_objects.h"
#include "motion_guest.h"
#include "field_events.h"
#include "field_view.h"
static HGEventView scene_events(FieldSystem *field) {
    HGEventView events;GF_ASSERT(field&&field->location&&hg_event_load(field->location->mapId,&events));return events;
}
static unsigned native_object_event_count(FieldSystem *field) { return scene_events(field).object_count; }
static const ObjectEvent *native_object_events(FieldSystem *field) { return (const ObjectEvent *)scene_events(field).objects; }
static LocalMapObject *native_script_show(MapObjectManager *manager,u16 id,u32 count,u32 map,const ObjectEvent *events) {
    LocalMapObject *existing=MapObjectManager_GetFirstActiveObjectByID(manager,id);if(existing)return existing;
    if(manager->objectCount>=64)return NULL;
    for(unsigned i=0;i<count;i++)if(events[i].id==id) {
        LocalMapObject *slot=manager->objects+manager->objectCount;memset(slot,0,sizeof(*slot));
        if(!hg_map_object_init_event(slot,manager,map,events+i)||!hg_motion_settle(manager->fieldSystem->native_scene_resources,slot))return NULL;
        manager->objectCount++;return slot;
    }
    return NULL;
}
static void native_script_remove(LocalMapObject *object) { MapObject_ClearFlagsBits(object,MAPOBJECTFLAG_ACTIVE); }
#define Field_GetNumObjectEvents native_object_event_count
#define Field_GetObjectEvents native_object_events
#define MapObject_CreateFromObjectEventWithId native_script_show
#define MapObject_Remove native_script_remove
#define MapObject_Delete native_script_delete
void sub_02061070(LocalMapObject *);
static BOOL native_follow_active(FieldSystem *field) {
    return MapObjectManager_GetFirstActiveObjectByID(field->mapObjectManager,obj_partner_poke)!=NULL;
}
static LocalMapObject *native_follow_object(FieldSystem *field) { return MapObjectManager_GetFirstActiveObjectByID(field->mapObjectManager,obj_partner_poke); }
static void native_follow_reset(LocalMapObject *object) {
    FieldSystem *field=MapObject_GetFieldSystem(object);
    GF_ASSERT(hg_motion_follow_tick(field->native_scene_resources,field->playerAvatar,object,field->native_follow_session,2,48));
}
static void native_follow_change(LocalMapObject *object,u32 movement) {
    FieldSystem *field=MapObject_GetFieldSystem(object);GF_ASSERT(movement==48||movement==55||movement==56);
    hg_map_object_set_movement(object,movement);
    GF_ASSERT(hg_motion_follow_tick(field->native_scene_resources,field->playerAvatar,object,field->native_follow_session,1,movement));
}
static void native_follow_position(LocalMapObject *player,LocalMapObject *follower,unsigned direction,unsigned facing) {
    FieldSystem *field=MapObject_GetFieldSystem(player);GF_ASSERT(hg_motion_follow_position(field->native_scene_resources,field->playerAvatar,follower,direction,facing));
}
static void native_follow_appear(LocalMapObject *object) {
    /* Host presentation publishes the real actor; DS NNS emergence effect is
     * not executed. Original script still owns the ten-frame wait. */
    sub_02069DC8(object,FALSE);
}
#define FollowMon_IsActive native_follow_active
#define FollowMon_GetMapObject native_follow_object
#define sub_020659CC native_follow_reset
#define sub_0205FC94 native_follow_change
#define ov01_02205720 native_follow_position
#define ov01_02205784 native_follow_appear
#define SaveArray_Party_Get hg_story_save_party
#include "compat/prop_animation.h"
#include "save_local_field_data.h"
/* The scene owns a real original local-field record; script save-array lookup
 * is scoped to this field context rather than the transition owner. */
#define Save_LocalFieldData_Get(save) (ctx->fieldSystem->native_local_field)
#include "screen_fade_types.inc"
#define PM_LCD_TOP 0
#define PM_LCD_BOTTOM 1
/* Native presentation has no DS hardware-window masks. The unchanged script
 * still issues both reset requests; the host display is already unmasked. */
#define ResetVisibleHardwareWindows(screen) ((void)(screen))
#define BeginNormalPaletteFade(mode,main,sub,color,steps,speed,heap) GF_ASSERT(hg_motion_fade_begin(ctx->fieldSystem->native_fade,main,color,steps,speed))
#define IsPaletteFadeFinished() (!ctx->fieldSystem->native_fade->active)
#define Field_PlayerAvatar_ApplyTransitionFlags(avatar) GF_ASSERT(hg_motion_avatar_transition(avatar))
#include "compat/healing.h"
static void native_handoff(FieldSystem *field){GF_ASSERT(hg_motion_handoff_begin(field,field->native_motion_task));}
#define ov01_02205AEC native_handoff
#define CallTask_LeaveOverworld(task) (ctx->fieldSystem->native_field_suspended=TRUE)
#define PokecenterAnimCreate(field,count) ((field)->native_healing=hg_healing_new(field,count))
#include "script_scene_decls.inc"
#include "script_scene_kernel.inc"
static const ScrCmdFunc scene_commands[1024]={
#include "script_scene_table.inc"
};
/* Typed native scene owner. Operand bounds are validated by the script VM
 * before reaching the unchanged original handlers. No guest pointers cross. */
int hg_script_scene_command(ScriptContext *context,unsigned op,BOOL *yielded) {
    if(!context||!context->fieldSystem||!context->fieldSystem->playerAvatar||
       !context->fieldSystem->mapObjectManager||!yielded||op>=NELEMS(scene_commands)||!scene_commands[op])return 0;
    /* The original missing-person assertion only admits an absent follower. */
    if(op==106) {
        u16 id=FieldSystem_VarGet(context->fieldSystem,(u16)(context->script_ptr[0]|(u16)context->script_ptr[1]<<8));
        if(id!=obj_partner_poke&&!MapObjectManager_GetFirstActiveObjectByID(context->fieldSystem->mapObjectManager,id))return 0;
    }
    if(op==101) {
        u16 id=FieldSystem_VarGet(context->fieldSystem,(u16)(context->script_ptr[0]|(u16)context->script_ptr[1]<<8));
        if(!MapObjectManager_GetFirstActiveObjectByID(context->fieldSystem->mapObjectManager,id))return 0;
    }
    if(op==98||op==99) {
        u16 id=(u16)(context->script_ptr[0]|(u16)context->script_ptr[1]<<8);
        if(id!=obj_partner_poke&&!MapObjectManager_GetFirstActiveObjectByID(context->fieldSystem->mapObjectManager,id))return 0;
    }
    if(op==100) {
        u16 id=FieldSystem_VarGet(context->fieldSystem,(u16)(context->script_ptr[0]|(u16)context->script_ptr[1]<<8));
        HGEventView events=scene_events(context->fieldSystem);
        if(!native_script_show(context->fieldSystem->mapObjectManager,id,events.object_count,context->fieldSystem->location->mapId,(const ObjectEvent *)events.objects))return 0;
    }
    if(op==339) {
        FieldSystem *field=context->fieldSystem;unsigned v[5];
        for(unsigned i=0;i<5;i++)v[i]=FieldSystem_VarGet(field,(u16)(context->script_ptr[2*i]|(u16)context->script_ptr[2*i+1]<<8));
        LocalMapObject *object=MapObjectManager_GetFirstActiveObjectByID(field->mapObjectManager,v[0]);
        HGFieldTile tile;int32_t height;
        if(!object||v[1]>32767||v[2]>32767||v[3]>32767||v[4]>3||!field->native_scene_resources||
           !hg_field_resources_tile(field->native_scene_resources,v[1],v[3],&tile)||(!hg_transition_supported(tile.map_header)||(object==PlayerAvatar_GetMapObject(field->playerAvatar)&&tile.map_header!=MapObject_GetMapID(object)))||
           (object==PlayerAvatar_GetMapObject(field->playerAvatar)&&hg_field_resources_height(field->native_scene_resources,v[1],v[3],0,(int32_t)v[2]*32768,&height)!=1))return 0;
    }

    if(op==436&&(!context->fieldSystem->native_fade||context->fieldSystem->native_fade->active||!context->fieldSystem->native_fade->brightness))return 0;
    if(op==188) {
        unsigned flags=context->script_ptr[0]|(unsigned)context->script_ptr[1]<<8;
        if(flags&~((1u<<0)|(1u<<8)|(1u<<10)|(1u<<11)))return 0;
    }
    if(op==189) {
        PlayerAvatar avatar=*context->fieldSystem->playerAvatar;
        LocalMapObject object=*avatar.mapObject;PlayerSaveData save;avatar.mapObject=&object;
        if(avatar.playerSaveData){save=*avatar.playerSaveData;avatar.playerSaveData=&save;}
        if(!hg_motion_avatar_transition(&avatar))return 0;
        HGFieldView *sprite=hg_avatar_sprite_load(MapObject_GetSpriteID(&object),MapObject_GetFacingDirection(&object),NULL,0);
        if(!sprite)return 0;hg_field_view_free(sprite);
    }
    if(op==621&&!context->fieldSystem->mapPropManager)return 0;
    *yielded=scene_commands[op](context);return 1;
}
