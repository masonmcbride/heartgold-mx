#include "compat/motion.h"
#include "compat/stats.h"
#include "compat/map_objects.h"
#include "compat/story.h"
#include "motion_guest.h"
#include "field_data.h"
#include "field_player.h"
#include "field_events.h"
#include "map_matrix.h"
#include "prop_runtime.h"
#include "compat/starter.h"
#include "compat/prop_animation.h"
#include "compat/pokemon.h"
#include "compat/follow.h"
#include "compat/field_control.h"
#include "constants/init_script_types.h"
#include "save_local_field_data.h"
typedef struct PreparedActors PreparedActors;
typedef struct {
    const uint8_t *bytes;size_t size,offset;unsigned remaining,command;int active;HGMotionEffect effect;
} ScriptMovement;
typedef struct {
    LocalMapObject objects[64];PlayerAvatar avatar;PlayerSaveData player_save;MapObjectManager manager;
    HGMotionFade fade;LocalFieldData *local_field;HGFieldResources *resources;HGFieldProps *props;HGStarter *starter;HGPropAnimations *prop_animations;Location location;int preview,props_changed;HGMotionSession follow;ScriptMovement movement[64];unsigned count,map,target;
} ScriptScene;
struct HGFieldPlayer {
    int exploring;HGMotionFade fade;
    FieldSystem field;MapObjectManager manager;LocalMapObject objects[64];
    HGFieldView *actor_sprites[63];HGFieldActorCounts actors;
    PlayerSaveData save;PlayerAvatar *avatar;HGFieldResources *resources;
    LocalMapObject step_start,step_objects[64];int walking;unsigned animation_tick;
    HGMotionSession session;
    HGMotionSession session_start;PlayerAvatar avatar_start;uint8_t *stats_start;
    HGTransition *transitions;unsigned map;
    HGStoryState *story;HGFieldScript *script;unsigned script_actor;char script_error[160];
    ScriptScene *script_start;HGFieldProps *props;HGFieldView *props_view;
    HGFieldView *script_sprites[63];
    unsigned automatic_entry;
    unsigned long long rejected_revision;
    int automatic,rejected_frame;
    HGStoryState *zone_story;HGFieldResources *zone_resources;PreparedActors *zone_actors;
    unsigned zone_map;unsigned long long zone_revision;
    unsigned step_map;int step_valid,coordinate_blocked,coordinate_x,coordinate_z;
};
struct PreparedActors {
    LocalMapObject objects[63];HGFieldView *sprites[63];HGFieldActorCounts counts;
    LocalMapObject player;PlayerAvatar avatar;PlayerSaveData player_save;int resume_prepared;
};
static void free_prepared(PreparedActors *actors) {
    if(!actors)return;for(unsigned i=0;i<actors->counts.active;i++)hg_field_view_free(actors->sprites[i]);free(actors);
}
static void discard_zone(HGFieldPlayer *p) {
    if(p->story){FieldSystem live={0};hg_story_bind_field(p->story,&live);p->session.stats=hg_story_save_stats(live.saveData);}
    hg_story_free(p->zone_story);hg_field_resources_free(p->zone_resources);free_prepared(p->zone_actors);
    p->zone_story=NULL;p->zone_resources=NULL;p->zone_actors=NULL;
}
static void restore_step(HGFieldPlayer *p) {
    if(!p->step_valid)return;
    if(p->step_map!=p->map){p->coordinate_blocked=1;p->coordinate_x=MapObject_GetXCoord(p->step_objects);p->coordinate_z=MapObject_GetZCoord(p->step_objects);return;}
    memcpy(p->objects,p->step_objects,sizeof(p->objects));*p->avatar=p->avatar_start;p->session=p->session_start;
    memcpy(p->session.stats,p->stats_start,GameStats_sizeof());p->walking=0;hg_transition_sync_player(p->transitions);
}
static void release_live_sprite(HGFieldPlayer *p,unsigned index) {
    if(p->actor_sprites[index]!=p->script_sprites[index])hg_field_view_free(p->actor_sprites[index]);
}
static LocalMapObject *add_follower(LocalMapObject *,MapObjectManager *,HGFieldResources *,unsigned,Party *,int,int,unsigned);
static PreparedActors *prepare_actors(HGFieldPlayer *p,HGFieldResources *resources,unsigned map,const HGStoryState *story,int retain) {
    HGEventView events;if(!hg_event_load(map,&events)||events.object_count>63)return NULL;
    PreparedActors *actors=calloc(1,sizeof(*actors));if(!actors)return NULL;
    if(p->exploring){actors->counts.events=events.object_count;return actors;}
    _Static_assert(sizeof(ObjectEvent)==32,"Event objects must retain serialized scalar layout");
    actors->counts.events=events.object_count;
    SaveData *previous_save=p->field.saveData;hg_story_bind_field((HGStoryState *)story,&p->field);
    for(unsigned i=0;i<events.object_count;i++) {
        ObjectEvent event;memcpy(&event,events.objects+32*i,sizeof(event));
        int hidden;
        if(!hg_story_flag_get(story,event.eventFlag,&hidden)){actors->counts.unsupported++;continue;}
        if(hidden){actors->counts.conditional++;continue;}
        HGFieldTile tile;int32_t height;
        if(event.x>32767||event.z>32767||event.facingDirection<0||event.facingDirection>3||
           !hg_field_resources_tile(resources,event.x,event.z,&tile)||tile.map_header!=map||
           hg_field_resources_height(resources,event.x,event.z,0,event.y,&height)!=1) {
            actors->counts.unsupported++;continue;
        }
        unsigned sprite_id=event.spriteId;
        if(sprite_id>=101&&sprite_id<=117)sprite_id=FieldSystem_VarGetObjectEventGraphicsId(&p->field,(u16)(sprite_id-101));
        if(sprite_id==SPRITE_BONGURI) {
            unsigned kind;int available;
            if(!hg_story_apricorn_tree(story,event.param[0],&kind,&available)||!hg_motion_apricorn_sprite(kind,available,&sprite_id)){actors->counts.unsupported++;continue;}
            event.spriteId=(u16)sprite_id;
        }
        LocalMapObject *existing=NULL;
        if(retain&&map==p->map)for(unsigned n=0;n<p->actors.active;n++)if(MapObject_GetID(p->objects+n+1)==event.id&&MapObject_GetSpriteID(p->objects+n+1)==sprite_id){existing=p->objects+n+1;break;}
        char resource_error[128];HGFieldView *sprite=hg_character_sprite_load(sprite_id,existing?MapObject_GetFacingDirection(existing):(unsigned)event.facingDirection,resource_error,sizeof(resource_error));
        if(!sprite){actors->counts.unsupported++;if(!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"Map %u actor %u sprite %u: %.80s",map,event.id,sprite_id,resource_error);continue;}
        LocalMapObject *object=&actors->objects[actors->counts.active];
        if(!hg_map_object_init_event(object,&p->manager,map,&event)) {
            hg_field_view_free(sprite);actors->counts.unsupported++;continue;
        }
        /* Original initializer supplies event metadata/grid placement. The
         * admitted static BDHC provider resolves the settled surface height. */
        VecFx32 position;MapObject_CopyPositionVector(object,&position);position.y=height;
        MapObject_SetPositionVector(object,&position);MapObject_SetCurrentY(object,(height>>3)/4096);
        MapObject_SetPreviousY(object,(height>>3)/4096);
        if(existing)*object=*existing;
        actors->sprites[actors->counts.active++]=sprite;
    }
    Party *party=hg_story_save_party(p->field.saveData);
    if(Party_GetCount(party)) {
        if(actors->counts.active>=63){p->field.saveData=previous_save;free_prepared(actors);return NULL;}
        LocalMapObject *existing=retain?MapObjectManager_GetFirstActiveObjectByID(&p->manager,obj_partner_poke):NULL;
        int x=existing?MapObject_GetXCoord(existing):0,z=existing?MapObject_GetZCoord(existing):0;
        unsigned direction=existing?MapObject_GetFacingDirection(existing):0;
        if(!existing) {
            if(map==p->map){x=MapObject_GetXCoord(p->objects);z=MapObject_GetZCoord(p->objects);direction=MapObject_GetFacingDirection(p->objects);}
            else if(events.warp_count){x=events.warps[0]|events.warps[1]<<8;z=events.warps[2]|events.warps[3]<<8;}
        }
        LocalMapObject *object=actors->objects+actors->counts.active;
        if(!add_follower(object,&p->manager,resources,map,party,x,z,direction)){p->field.saveData=previous_save;free_prepared(actors);return NULL;}
        if(existing)*object=*existing;
        hg_map_object_set_map(object,map);
        sub_02069DC8(object,FALSE);MapObject_UnpauseMovement(object);
        HGFieldView *sprite=hg_character_sprite_load(MapObject_GetSpriteID(object),direction,NULL,0);
        if(!sprite){p->field.saveData=previous_save;free_prepared(actors);return NULL;}
        actors->sprites[actors->counts.active++]=sprite;
    }
    p->field.saveData=previous_save;
    return actors;
}
static void install_actors(HGFieldPlayer *p,PreparedActors *actors) {
    for(unsigned i=0;i<p->actors.active;i++)release_live_sprite(p,i);
    memset(p->objects+1,0,sizeof(p->objects)-sizeof(p->objects[0]));
    memset(p->actor_sprites,0,sizeof(p->actor_sprites));
    memcpy(p->objects+1,actors->objects,actors->counts.active*sizeof(*actors->objects));
    memcpy(p->actor_sprites,actors->sprites,actors->counts.active*sizeof(*actors->sprites));
    p->actors=actors->counts;p->manager.objectCount=1+p->actors.active;
    if(actors->resume_prepared) {
        PlayerSaveData *save=PlayerAvatar_GetPlayerSaveData(p->avatar);
        p->objects[0]=actors->player;p->objects[0].manager=&p->manager;
        *p->avatar=actors->avatar;p->avatar->mapObject=p->objects;p->avatar->playerSaveData=save;*save=actors->player_save;
    }
    free(actors); /* Sprite ownership moved; stable live object slots copied. */
}
/* The host renderer owns sprite resources; there is no DS VRAM transfer to
 * perform at completion. Original command dispatch still invokes this native
 * resource notification through the original map-object callback accessor. */
static void movement_finished(LocalMapObject *object) { (void)object; }
static unsigned scene_half(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static void *scene_clone(const void *source) {
    ScriptScene *scene=malloc(sizeof(*scene));if(!scene)return NULL;*scene=*(const ScriptScene *)source;
    scene->starter=NULL;scene->props=NULL;scene->prop_animations=NULL;scene->local_field=NULL;
    if(((const ScriptScene *)source)->local_field) {
        scene->local_field=malloc(Save_LocalFieldData_sizeof());if(!scene->local_field){free(scene);return NULL;}
        memcpy(scene->local_field,((const ScriptScene *)source)->local_field,Save_LocalFieldData_sizeof());
    }
    if(((const ScriptScene *)source)->prop_animations){scene->prop_animations=hg_prop_animations_clone(((const ScriptScene *)source)->prop_animations);if(!scene->prop_animations){free(scene->local_field);free(scene);return NULL;}}
    if(((const ScriptScene *)source)->props) {
        scene->props=hg_field_props_clone(((const ScriptScene *)source)->props);
        if(!scene->props){hg_prop_animations_free(scene->prop_animations);free(scene->local_field);free(scene);return NULL;}
    }
    scene->manager.objects=scene->objects;scene->manager.objectCount=scene->count;scene->avatar.mapObject=scene->objects;
    scene->player_save=*(((const ScriptScene *)source)->avatar.playerSaveData);scene->avatar.playerSaveData=&scene->player_save;
    for(unsigned i=0;i<scene->count;i++){scene->objects[i].manager=&scene->manager;sub_0205F47C(scene->objects+i,movement_finished);}
    return scene;
}
static void scene_destroy(void *opaque) {
    ScriptScene *scene=opaque;if(scene){free(scene->local_field);hg_prop_animations_free(scene->prop_animations);hg_starter_free(scene->starter);hg_field_props_free(scene->props);}free(scene);
}
static void scene_bind(void *opaque,FieldSystem *field,int preview) {
    ScriptScene *scene=opaque;scene->preview=preview;field->native_fade=&scene->fade;field->native_local_field=scene->local_field;field->native_prop_animations=scene->prop_animations;scene->location.mapId=scene->map;field->location=&scene->location;scene->manager.fieldSystem=field;field->mapObjectManager=&scene->manager;field->playerAvatar=&scene->avatar;field->native_scene_resources=scene->resources;field->mapPropManager=hg_field_props_manager(scene->props);scene->follow.stats=hg_story_save_stats(field->saveData);field->native_follow_session=&scene->follow;
}
static int scene_command(void *opaque,ScriptContext *context,unsigned op,BOOL *yielded) {
    ScriptScene *scene=opaque;
    if(context->fieldSystem->playerAvatar!=&scene->avatar)return 0;
    if(op>=307&&op<=311) {
        if(!scene->prop_animations)scene->prop_animations=hg_prop_animations_new(scene->map);
        context->fieldSystem->native_prop_animations=scene->prop_animations;
        if(!hg_prop_animations_validate(scene->prop_animations,scene->resources,context,op))return 0;
    }
    if(op==174) {
        const uint8_t *p=context->script_ptr;
        if(scene->fade.active||!scene_half(p)||scene_half(p)>3600||!scene_half(p+2)||scene_half(p+2)>60||scene_half(p+4)>1||(scene_half(p+6)!=0&&scene_half(p+6)!=32767))return 0;
    }
    if(op==167) {
        if(scene->starter)return 0;
        scene->starter=hg_starter_begin(context->fieldSystem,scene->preview);if(!scene->starter)return 0;
    }
    if(op==621) {
        if(scene->map!=61)return 0;
        if(!scene->props)scene->props=hg_field_props_new(scene->map);
        if(!hg_field_props_prepare(scene->props,141,3))return 0;
        context->fieldSystem->mapPropManager=hg_field_props_manager(scene->props);scene->props_changed=1;
    }
    if(op==293||op==280) {
        if(!scene->local_field){scene->local_field=calloc(1,Save_LocalFieldData_sizeof());if(!scene->local_field)return 0;Save_LocalFieldData_Init(scene->local_field);}
        context->fieldSystem->native_local_field=scene->local_field;
        *LocalFieldData_GetPlayer(scene->local_field)=scene->player_save;
        LocalFieldData_SetBlackoutSpawn(scene->local_field,*hg_story_save_blackout_spawn(context->fieldSystem->saveData));
    }
    if(!hg_script_scene_command(context,op,yielded))return 0;
    if(op==293)scene->player_save=*LocalFieldData_GetPlayer(scene->local_field);
    if(op==280)*hg_story_save_blackout_spawn(context->fieldSystem->saveData)=LocalFieldData_GetBlackoutSpawn(scene->local_field);
    if(op==100||op==101) {
        unsigned count=1,target=64;
        for(unsigned i=1;i<scene->manager.objectCount;i++)if(MapObject_CheckActive(scene->objects+i)) {
            if(i==scene->target)target=count;
            if(count!=i){scene->objects[count]=scene->objects[i];scene->movement[count]=scene->movement[i];}
            count++;
        }
        scene->count=scene->manager.objectCount=count;scene->target=target;
        for(unsigned i=0;i<count;i++)sub_0205F47C(scene->objects+i,movement_finished);
        if(op==100)for(unsigned i=1;i<count;i++) {
            HGFieldView *view=hg_character_sprite_load(MapObject_GetSpriteID(scene->objects+i),MapObject_GetFacingDirection(scene->objects+i),NULL,0);
            if(!view)return 0;hg_field_view_free(view);
        }
    }
    return 1;
}
static int scene_face_player(void *opaque) {
    ScriptScene *scene=opaque;if(scene->target>=scene->count)return 2; /* Original FacePlayer permits no actor. */
    unsigned facing=PlayerAvatar_GetFacingDirection(&scene->avatar)^1u;LocalMapObject *object=scene->objects+scene->target;
    MapObject_SetFacingDirectionDirect(object,facing);MapObject_SetNextFacingDirection(object,facing);return 1;
}
static int scene_busy(const void *opaque) { if(((const ScriptScene *)opaque)->fade.active)return 1;if(hg_prop_animations_busy(((const ScriptScene *)opaque)->prop_animations))return 1;const HGStarter *s=((const ScriptScene *)opaque)->starter;return hg_starter_pending(s)?2:hg_starter_busy(s); }
static unsigned scene_active(const void *opaque) {
    const ScriptScene *scene=opaque;unsigned active=0;for(unsigned i=0;i<scene->count;i++)active+=scene->movement[i].active!=0;return active;
}
static int scene_apply(void *opaque,unsigned person,const uint8_t *bytes,size_t size) {
    ScriptScene *scene=opaque;unsigned index=scene->count;
    if(person==obj_player)index=0;
    else for(unsigned i=1;i<scene->count;i++)if(MapObject_GetID(scene->objects+i)==person){index=i;break;}
    /* Original ApplyMovement permits an absent partner. No follower actor has
     * been created in this inspection session. Other missing actors fail. */
    if(index==scene->count)return person==obj_partner_poke;
    if(scene->movement[index].active)return 0;
    for(size_t at=0;at+4<=size&&at<1024;at+=4) {
        unsigned command=scene_half(bytes+at),repeat=scene_half(bytes+at+2);
        if(command==254) {
            if(repeat)return 0;scene->movement[index]=(ScriptMovement){.bytes=bytes,.size=at+4,.active=1};return 1;
        }
        if(!hg_motion_script_command(command)||!repeat||repeat>256)return 0;
    }
    return 0;
}
static LocalMapObject *add_follower(LocalMapObject *slot,MapObjectManager *manager,HGFieldResources *resources,unsigned map,Party *party,int x,int z,unsigned facing) {
    if(!party||!Party_GetCount(party))return NULL;
    Pokemon *mon=Party_GetMonByIndex(party,0);
    MapObjectManager allocation=*manager;allocation.objects=slot;allocation.objectCount=1;
    memset(slot,0,sizeof(*slot));LocalMapObject *object=hg_follow_create(&allocation,GetMonData(mon,MON_DATA_SPECIES,NULL),GetMonData(mon,MON_DATA_FORM,NULL),GetMonData(mon,MON_DATA_GENDER,NULL),facing,x,z,MonIsShiny(mon));
    if(!object)return NULL;object->manager=manager;hg_map_object_set_map(object,map);sub_0205F47C(object,movement_finished);
    if(!hg_motion_settle(resources,object))return NULL;
    return object;
}
static int scene_tick(void *opaque) {
    ScriptScene *scene=opaque;
    if(!hg_motion_fade_tick(&scene->fade)||!hg_prop_animations_tick(scene->prop_animations))return 0;
    if(scene->starter&&hg_starter_busy(scene->starter)&&!hg_starter_tick(scene->starter))return 0;
    if(scene->starter&&!hg_starter_busy(scene->starter)&&!MapObjectManager_GetFirstActiveObjectByID(&scene->manager,obj_partner_poke)) {
        if(scene->count>=64)return 0;
        scene->manager.objectCount=scene->count+1;
        LocalMapObject *object=add_follower(scene->objects+scene->count,&scene->manager,scene->resources,scene->map,hg_story_save_party(scene->manager.fieldSystem->saveData),MapObject_GetXCoord(scene->objects),MapObject_GetZCoord(scene->objects),MapObject_GetFacingDirection(scene->objects));
        if(!object)return 0;scene->count++;
        hg_field_props_free(scene->props);scene->props=hg_field_props_new(scene->map);
        if(!hg_field_props_prepare(scene->props,141,3))return 0;
        scene->manager.fieldSystem->mapPropManager=hg_field_props_manager(scene->props);scene->props_changed=1;
        ScriptContext ctx={.fieldSystem=scene->manager.fieldSystem};BOOL yielded;
        if(!hg_script_scene_command(&ctx,621,&yielded))return 0;
        if(!hg_motion_follow_tick(scene->resources,&scene->avatar,object,&scene->follow,1,48))return 0;
    }
    for(unsigned i=1;i<scene->count;i++)if(MapObject_GetID(scene->objects+i)==obj_partner_poke&&!MapObject_CheckMovementPaused(scene->objects+i)) {
        if(!hg_motion_follow_tick(scene->resources,&scene->avatar,scene->objects+i,&scene->follow,0,MapObject_GetMovement(scene->objects+i)))return 0;
    }
    for(unsigned i=0;i<scene->count;i++) {
        ScriptMovement *job=scene->movement+i;if(!job->active)continue;LocalMapObject *object=scene->objects+i;
        if(!job->remaining) {
            if(job->offset+4>job->size)return 0;
            job->command=scene_half(job->bytes+job->offset);job->remaining=scene_half(job->bytes+job->offset+2);job->offset+=4;
            if(job->command==254){job->active=0;continue;}
            MapObject_SetMovementCommand(object,job->command);MapObject_SetMovementStep(object,0);MapObject_ClearFlagsBits(object,0x20);
        } else if(MapObject_GetMovementCommand(object)==255) {
            MapObject_SetMovementCommand(object,job->command);MapObject_SetMovementStep(object,0);MapObject_ClearFlagsBits(object,0x20);
        }
        int complete;if(!hg_motion_script_effect_tick(scene->resources,object,&job->effect,&complete)){fprintf(stderr,"Movement tick failed map %u actor %u command %u step %u at %d,%d\n",scene->map,MapObject_GetID(object),job->command,MapObject_GetMovementStep(object),MapObject_GetXCoord(object),MapObject_GetZCoord(object));return 0;}
        HGFieldTile tile;VecFx32 v;MapObject_CopyPositionVector(object,&v);
        if(!hg_field_resources_tile(scene->resources,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&tile)||tile.map_header!=scene->map||
           !hg_field_resources_tile(scene->resources,v.x/65536,v.z/65536,&tile)||tile.map_header!=scene->map)return 0;
        if(complete)job->remaining--;
    }
    return 1;
}
static int publish_scene(HGFieldPlayer *p,const ScriptScene *scene) {
    HGFieldView *sprites[63]={0};
    for(unsigned i=1;i<scene->count;i++)if(i>=p->manager.objectCount||MapObject_GetSpriteID(p->objects+i)!=MapObject_GetSpriteID((LocalMapObject *)scene->objects+i)||MapObject_GetFacingDirection(p->objects+i)!=MapObject_GetFacingDirection((LocalMapObject *)scene->objects+i)) {
        sprites[i-1]=hg_character_sprite_load(MapObject_GetSpriteID((LocalMapObject *)scene->objects+i),MapObject_GetFacingDirection((LocalMapObject *)scene->objects+i),NULL,0);
        if(!sprites[i-1]){for(unsigned n=0;n<63;n++)hg_field_view_free(sprites[n]);return 0;}
    }
    for(unsigned i=scene->count;i<p->manager.objectCount;i++){release_live_sprite(p,i-1);p->actor_sprites[i-1]=NULL;}
    p->manager.objectCount=scene->count;p->actors.active=scene->count-1;memcpy(p->session.follower,scene->follow.follower,32);
    p->script_actor=scene->target>0&&scene->target<scene->count?scene->target-1:p->actors.active;
    PlayerSaveData *save=PlayerAvatar_GetPlayerSaveData(p->avatar);*p->avatar=scene->avatar;p->avatar->mapObject=p->objects;p->avatar->playerSaveData=save;
    memcpy(p->objects,scene->objects,scene->count*sizeof(*p->objects));
    for(unsigned i=0;i<scene->count;i++)p->objects[i].manager=&p->manager;
    for(unsigned i=0;i<63;i++)if(sprites[i]){release_live_sprite(p,i);p->actor_sprites[i]=sprites[i];}
    return 1;
}
static void restore_scene(HGFieldPlayer *p) {
    /* Keep the initial views alive until completion. Rollback needs no new
     * allocation even if preparation of a later facing view failed. */
    p->manager.objectCount=p->script_start->count;p->actors.active=p->script_start->count-1;
    p->script_actor=p->script_start->target>0&&p->script_start->target<p->script_start->count?p->script_start->target-1:p->actors.active;
    memcpy(p->session.follower,p->script_start->follow.follower,32);
    PlayerSaveData *save=PlayerAvatar_GetPlayerSaveData(p->avatar);*p->avatar=p->script_start->avatar;p->avatar->mapObject=p->objects;p->avatar->playerSaveData=save;
    memcpy(p->objects,p->script_start->objects,p->script_start->count*sizeof(*p->objects));
    for(unsigned i=0;i<63;i++){release_live_sprite(p,i);p->actor_sprites[i]=p->script_sprites[i];p->script_sprites[i]=NULL;}
}
static void place_object(LocalMapObject *object,int x,int z,int32_t height,unsigned direction) {
    MapObject_SetCurrentX(object,x);MapObject_SetCurrentZ(object,z);MapObject_SetCurrentY(object,(height>>3)/4096);
    MapObject_SetPreviousX(object,x);MapObject_SetPreviousZ(object,z);MapObject_SetPreviousY(object,(height>>3)/4096);
    VecFx32 v={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(object,&v);
    MapObject_SetFacingDirectionDirect(object,direction);MapObject_SetNextFacingDirection(object,direction);
}
static void place_player(HGFieldPlayer *p,int x,int z,int32_t height,unsigned direction) {
    place_object(p->objects,x,z,height,direction);
}
/* Map entry is prepared entirely against the incoming resources/story. The
 * live player, population and prop owners remain unchanged until publication. */
static int prepare_resume(HGFieldPlayer *p,PreparedActors *actors,HGStoryState *story,HGFieldResources *resources,
                          unsigned map,int x,int z,int32_t height,unsigned direction,HGFieldProps **props,HGFieldView **view) {
    if(p->exploring)return 1;
    *props=NULL;*view=NULL;unsigned entry;
    if(!hg_story_map_script(story,map,INIT_SCRIPT_ON_RESUME,&entry))return 0;
    if(entry==65535)return 1;
    ScriptScene source={.manager=p->manager,.avatar=*p->avatar,.resources=resources,.count=actors->counts.active+1,.map=map,.target=64};
    source.objects[0]=p->objects[0];hg_map_object_set_map(source.objects,map);place_object(source.objects,x,z,height,direction);
    memcpy(source.objects+1,actors->objects,actors->counts.active*sizeof(*actors->objects));
    HGScriptSceneServices services={.source=&source,.clone=scene_clone,.destroy=scene_destroy,.bind=scene_bind,.apply=scene_apply,
        .tick=scene_tick,.active=scene_active,.busy=scene_busy,.command=scene_command,.face_player=scene_face_player};
    HGFieldScript *script=hg_field_script_scene_new(story,map,entry,&services,p->script_error,sizeof(p->script_error));
    if(!script)return 0;
    int status;unsigned ticks=0;
    do{status=hg_field_script_tick(script);}while(status==HG_SCRIPT_RUNNING&&++ticks<1024);
    ScriptScene *result=(ScriptScene *)hg_field_script_scene(script);
    if(status!=HG_SCRIPT_DONE||!result){hg_field_script_free(script);return 0;}
    HGFieldView *ready=hg_field_props_count(result->props)?hg_field_props_view(result->props):NULL;
    if(hg_field_props_count(result->props)&&!ready){snprintf(p->script_error,sizeof(p->script_error),"Cannot render incoming script props");hg_field_script_free(script);return 0;}
    HGFieldView *sprites[63]={0};
    /* Resume scripts may create/delete actors. Prepare their complete private
     * population before publishing either story or rendering ownership. */
    for(unsigned i=0;i+1<result->count;i++) {
        LocalMapObject *object=result->objects+i+1;
        sprites[i]=hg_character_sprite_load(MapObject_GetSpriteID(object),MapObject_GetFacingDirection(object),NULL,0);
        if(!sprites[i]){for(unsigned n=0;n<63;n++)hg_field_view_free(sprites[n]);hg_field_view_free(ready);hg_field_script_free(script);return 0;}
    }
    if(!hg_field_script_commit(script)){for(unsigned n=0;n<63;n++)hg_field_view_free(sprites[n]);hg_field_view_free(ready);hg_field_script_free(script);return 0;}
    actors->player=result->objects[0];actors->avatar=result->avatar;actors->player_save=result->player_save;actors->resume_prepared=1;
    for(unsigned i=0;i<actors->counts.active;i++)hg_field_view_free(actors->sprites[i]);
    memcpy(actors->sprites,sprites,sizeof(sprites));actors->counts.active=result->count-1;
    memcpy(actors->objects,result->objects+1,actors->counts.active*sizeof(*actors->objects));
    for(unsigned i=0;i<actors->counts.active;i++)actors->objects[i].manager=&p->manager;
    *props=result->props;result->props=NULL;*view=ready;hg_field_script_free(script);return 1;
}
void hg_field_player_free(HGFieldPlayer *p) {
    if(!p)return;discard_zone(p);hg_field_script_free(p->script);free(p->script_start);hg_story_free(p->story);hg_transition_free(p->transitions);if(p->avatar)PlayerAvatar_FreeToHeap(p->avatar);
    for(unsigned i=0;i<p->actors.active;i++)release_live_sprite(p,i);
    for(unsigned i=0;i<63;i++)hg_field_view_free(p->script_sprites[i]);
    hg_field_props_free(p->props);hg_field_view_free(p->props_view);
    hg_field_resources_free(p->resources);free(p->stats_start);free(p);
}
HGFieldPlayer *hg_field_player_new(void) { return hg_field_player_new_map(60); }
static HGFieldPlayer *player_new_map(unsigned map,const HGFieldLocation *start,int exploring) {
    if(!hg_transition_supported(map))return NULL;
    HGFieldPlayer *p=calloc(1,sizeof(*p));if(!p)return NULL;
    p->exploring=exploring;p->map=map;p->resources=hg_field_resources_new(map);if(!p->resources)goto fail;
    p->story=hg_story_new();if(!p->story||(!exploring&&!hg_story_enter_map(p->story,map,p->script_error,sizeof(p->script_error))))goto fail;
    hg_story_bind_field(p->story,&p->field);
    p->session.stats=hg_story_save_stats(p->field.saveData);p->stats_start=malloc(GameStats_sizeof());
    if(!p->session.stats||!p->stats_start)goto fail;
    p->manager.objects=&p->objects[0];p->manager.objectCount=1;p->manager.fieldSystem=&p->field;
    p->objects[0].manager=&p->manager;p->objects[0].movement=1;
    MapObject_SetID(p->objects,obj_player);hg_map_object_set_map(p->objects,map);
    MapObject_SetFlagsBits(&p->objects[0],MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&p->save);
    /* Temporary inspection session equipment; not the new-game story award. */
    PlayerSaveData_SetRunningShoesFlag(&p->save,start?FALSE:TRUE);
    p->avatar=PlayerAvatar_CreateWithActiveMapObject(&p->manager,&p->save,0);if(!p->avatar)goto fail;
    p->field.playerAvatar=p->avatar;p->field.mapObjectManager=&p->manager;p->field.native_scene_resources=p->resources;p->field.native_follow_session=&p->session;
    MapObject_SetMovementCommand(&p->objects[0],255);sub_0205F47C(&p->objects[0],movement_finished);
    MapObject_SetXRange(&p->objects[0],-1);MapObject_SetYRange(&p->objects[0],-1);
    p->transitions=hg_transition_new(map,p->avatar);if(!p->transitions)goto fail;
    PreparedActors *actors=prepare_actors(p,p->resources,map,p->story,0);if(!actors)goto fail;
    if(actors->counts.unsupported){free_prepared(actors);goto fail;}
    if(exploring) {
        HGEventView entrances;if(!hg_event_load(map,&entrances)){free_prepared(actors);goto fail;}
        int anchorX=entrances.warp_count?(entrances.warps[0]|entrances.warps[1]<<8):-1;
        int anchorZ=entrances.warp_count?(entrances.warps[2]|entrances.warps[3]<<8):-1;
        MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);int found=0;
        for(unsigned cz=0;cz<MapMatrix_GetHeight(matrix)&&!found;cz++)for(unsigned cx=0;cx<MapMatrix_GetWidth(matrix)&&!found;cx++)if(MapMatrix_GetMapHeader(matrix,cx,cz)==map) {
            int ax=anchorX>=0?anchorX-(int)cx*32:16,az=anchorZ>=0?anchorZ-(int)cz*32:16;
            if(anchorX>=0&&(ax<0||ax>=32||az<0||az>=32))continue;
            for(int radius=0;radius<=62&&!found;radius++)for(int z=0;z<32&&!found;z++)for(int x=0;x<32&&!found;x++) {
                if(abs(x-ax)+abs(z-az)!=radius)continue;
                int wx=(int)cx*32+x,wz=(int)cz*32+z;HGMotionTile tile;int32_t height;
                if(!hg_motion_tile(p->resources,wx,wz,&tile)||tile.wall||hg_field_resources_height(p->resources,wx,wz,0,0,&height)!=1)continue;
                place_player(p,wx,wz,height,0);
                for(unsigned d=0;d<4;d++){unsigned mask;if(hg_motion_player_collision(p->resources,p->avatar,d,&mask)&&!mask){found=1;break;}}
            }
        }
        MapMatrix_Free(matrix);if(!found){free_prepared(actors);goto fail;}
        install_actors(p,actors);hg_transition_sync_player(p->transitions);return p;
    }
    if(map!=60) {
        HGEventView events;if(!hg_event_load(map,&events)||!events.warp_count){free_prepared(actors);goto fail;}
        int x=start?start->x:events.warps[0]|events.warps[1]<<8,z=start?start->z:events.warps[2]|events.warps[3]<<8;unsigned direction=start?(unsigned)start->direction:0;int32_t height;HGFieldTile tile;
        if(!hg_field_resources_tile(p->resources,x,z,&tile)||tile.map_header!=map||hg_field_resources_height(p->resources,x,z,0,0,&height)!=1){free_prepared(actors);goto fail;}
        place_player(p,x,z,height,direction);
        if(!prepare_resume(p,actors,p->story,p->resources,map,x,z,height,direction,&p->props,&p->props_view)){free_prepared(actors);goto fail;}
        install_actors(p,actors);hg_transition_sync_player(p->transitions);return p;
    }
    /* A native inspection-session spawn, not the original new-game script.
     * Prefer the town center; admit only a tile with an ordinary walking exit. */
    for(int radius=0;radius<32;radius++)for(int z=384;z<416;z++)for(int x=672;x<704;x++) {
        if(abs(x-688)+abs(z-398)!=radius)continue;
        HGMotionTile tile;int32_t height;
        if(!hg_motion_tile(p->resources,x,z,&tile)||tile.wall||
           hg_field_resources_height(p->resources,x,z,0,0,&height)!=1)continue;
        place_player(p,x,z,height,0);
        for(unsigned d=0;d<4;d++){unsigned mask;if(hg_motion_player_collision(p->resources,p->avatar,d,&mask)&&!mask){
            if(!prepare_resume(p,actors,p->story,p->resources,map,x,z,height,0,&p->props,&p->props_view)){free_prepared(actors);goto fail;}
            install_actors(p,actors);hg_transition_sync_player(p->transitions);return p;
        }}
    }
    free_prepared(actors);
fail:hg_field_player_free(p);return NULL;
}
HGFieldPlayer *hg_field_player_new_map(unsigned map) { return player_new_map(map,NULL,0); }
HGFieldPlayer *hg_field_player_new_game(void) { HGFieldLocation start=hg_field_new_game_location();return player_new_map((unsigned)start.map,&start,0); }
int hg_field_player_move(HGFieldPlayer *p,uint16_t keys) {
    if(!p||p->walking||p->script||p->zone_story)return 0;HGMotionMove move;
    if(p->coordinate_blocked) {
        int direction;LocalMapObject probe=p->objects[0];
        if(!hg_motion_direction(p->avatar,keys,&direction)||direction<0||!hg_motion_grid_step(&probe,(unsigned)direction)||
           MapObject_GetXCoord(&probe)!=p->coordinate_x||MapObject_GetZCoord(&probe)!=p->coordinate_z)return 0;
    }
    if(keys&0xf0)p->script_error[0]=0;
    p->step_start=p->objects[0];memcpy(p->step_objects,p->objects,sizeof(p->objects));
    p->avatar_start=*p->avatar;p->session_start=p->session;
    memcpy(p->stats_start,p->session.stats,GameStats_sizeof());
    p->step_map=p->map;p->step_valid=1;
    int direction;HGFieldTile target;LocalMapObject probe=p->objects[0];
    if(!hg_motion_direction(p->avatar,keys,&direction))return 0;
    int crossing=direction>=0&&hg_motion_grid_step(&probe,(unsigned)direction)&&
        hg_field_resources_tile(p->resources,MapObject_GetXCoord(&probe),MapObject_GetZCoord(&probe),&target)&&target.map_header!=p->map;
    int began=0;
    if(crossing) {
        p->script_error[0]=0;p->zone_revision=hg_story_revision(p->story);p->zone_map=target.map_header;
        p->zone_resources=hg_field_resources_new(p->zone_map);p->zone_story=hg_story_clone(p->story);
        if(!p->zone_resources||!p->zone_story||(!p->exploring&&!hg_story_enter_zone(p->zone_story,p->zone_map,p->script_error,sizeof(p->script_error)))||
           !(p->zone_actors=prepare_actors(p,p->zone_resources,p->zone_map,p->zone_story,1))||p->zone_actors->counts.unsupported) {
            if(p->zone_actors&&p->zone_actors->counts.unsupported) {
                char detail[160];snprintf(detail,sizeof(detail),"%s",p->script_error);
                snprintf(p->script_error,sizeof(p->script_error),"Destination map %u has %u unsupported actors: %.90s",p->zone_map,p->zone_actors->counts.unsupported,detail);
            } else if(!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"Cannot prepare destination field lifecycle/actors");
            discard_zone(p);return 0;
        }
        /* Collision sees both source and incoming populations before taking
         * the step. These handles live only for this synchronous dispatch. */
        LocalMapObject objects[128];unsigned count=p->manager.objectCount;
        memcpy(objects,p->objects,count*sizeof(*objects));memcpy(objects+count,p->zone_actors->objects,p->zone_actors->counts.active*sizeof(*objects));
        MapObjectManager manager=p->manager;manager.objects=objects;manager.objectCount=count+p->zone_actors->counts.active;
        FieldSystem field=p->field;manager.fieldSystem=&field;PlayerAvatar avatar=*p->avatar;avatar.mapObject=objects;field.playerAvatar=&avatar;field.mapObjectManager=&manager;
        for(unsigned i=0;i<manager.objectCount;i++)objects[i].manager=&manager;
        /* The crossing step belongs to the staged destination state. Updating
         * the live statistics here would be overwritten when it publishes. */
        FieldSystem staged={0};hg_story_bind_field(p->zone_story,&staged);
        p->session.stats=hg_story_save_stats(staged.saveData);
        began=hg_motion_locomotion_begin(p->zone_resources,&avatar,keys,&p->session,&move);
        if(began){p->objects[0]=objects[0];p->objects[0].manager=&p->manager;*p->avatar=avatar;p->avatar->mapObject=p->objects;}
        if(!began||!move.moved)discard_zone(p);
    } else began=hg_motion_locomotion_begin(p->resources,p->avatar,keys,&p->session,&move);
    if(!began){memcpy(p->session.stats,p->stats_start,GameStats_sizeof());return 0;}
    if(!move.moved&&move.direction>=0) {
        MapObject_SetFacingDirectionDirect(&p->objects[0],(unsigned)move.direction);
        MapObject_SetNextFacingDirection(&p->objects[0],(unsigned)move.direction);
    }
    p->walking=move.moved;
    if(move.moved)p->coordinate_blocked=0;
    if(!p->walking)hg_transition_sync_player(p->transitions);
    return move.moved;
}
int hg_field_player_pending_map(const HGFieldPlayer *p) { return p&&p->zone_story?(int)p->zone_map:-1; }
void hg_field_player_cancel_step(HGFieldPlayer *p) {
    if(!p||!p->zone_story)return;
    memcpy(p->objects,p->step_objects,sizeof(p->objects));*p->avatar=p->avatar_start;p->session=p->session_start;
    memcpy(p->session.stats,p->stats_start,GameStats_sizeof());p->walking=0;discard_zone(p);hg_transition_sync_player(p->transitions);
    if(!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"Cannot publish destination field resources/state");
}
int hg_field_player_commit_zone(HGFieldPlayer *p) {
    if(!p||!p->zone_story||p->walking||p->script||hg_story_revision(p->story)!=p->zone_revision||!hg_transition_commit_zone(p->transitions,p->zone_map))return 0;
    GF_ASSERT(hg_story_commit(p->story,p->zone_story,p->zone_revision));
    for(unsigned i=0;i<p->zone_actors->counts.active;i++)if(MapObject_GetID(p->zone_actors->objects+i)==obj_partner_poke) {
        LocalMapObject *follower=MapObjectManager_GetFirstActiveObjectByID(&p->manager,obj_partner_poke);
        if(follower){p->zone_actors->objects[i]=*follower;hg_map_object_set_map(p->zone_actors->objects+i,p->zone_map);}
    }
    install_actors(p,p->zone_actors);p->zone_actors=NULL;
    hg_field_resources_free(p->resources);p->resources=p->zone_resources;p->zone_resources=NULL;p->field.native_scene_resources=p->resources;
    p->map=p->zone_map;hg_map_object_set_map(p->objects,p->map);p->rejected_frame=0;
    discard_zone(p);return 1;
}
int hg_field_player_walking(const HGFieldPlayer *p) { return p&&p->walking; }
static int tick_followers(HGFieldPlayer *p) {
    int active=0;p->animation_tick++;
    for(unsigned i=1;i<p->manager.objectCount;i++)if(MapObject_GetID(p->objects+i)==obj_partner_poke) {
        LocalMapObject *object=p->objects+i;
        LocalMapObject before=*object;
        if(!MapObject_CheckMovementPaused(object)&&!hg_motion_follow_tick(p->resources,p->avatar,object,&p->session,0,48))return -1;
        unsigned command=MapObject_GetMovementCommand(object),frame=1+4*MapObject_GetFacingDirection(object);
        if(command!=255)frame+=(p->animation_tick/4)%2;
        if(!hg_player_sprite_frame(p->actor_sprites[i-1],frame))return -1;
        active|=memcmp(&before,object,sizeof(before))!=0;
    }
    return active;
}
int hg_field_player_tick(HGFieldPlayer *p) {
    if(!p||p->script)return 0;
    int complete=1,was_walking=p->walking,followers=0;
    if((p->walking&&!hg_motion_player_tick(p->resources,&p->objects[0],&complete))||((followers=tick_followers(p))<0)) {
        if(was_walking) {
            memcpy(p->objects,p->step_objects,sizeof(p->objects));*p->avatar=p->avatar_start;p->session=p->session_start;
            memcpy(p->session.stats,p->stats_start,GameStats_sizeof());p->walking=0;
            discard_zone(p);
        }
        snprintf(p->script_error,sizeof(p->script_error),"Cannot execute field object frame");return -1;
    }
    if(was_walking&&complete&&!hg_motion_clear_held(p->objects))return -1;
    p->walking=was_walking&&!complete;if(complete)hg_transition_sync_player(p->transitions);return was_walking||followers>0;
}
int hg_field_player_position(const HGFieldPlayer *p,int *x,int *z,double world[3]) {
    if(!p)return 0;VecFx32 v;MapObject_CopyPositionVector((LocalMapObject *)&p->objects[0],&v);
    if(x)*x=(int)MapObject_GetXCoord((LocalMapObject *)&p->objects[0]);
    if(z)*z=(int)MapObject_GetZCoord((LocalMapObject *)&p->objects[0]);
    if(world){VecFx32 shift;MapObject_CopyFacingVector((LocalMapObject *)&p->objects[0],&shift);
        world[0]=(v.x+(double)shift.x)/4096.0;world[1]=(v.y+(double)shift.y)/4096.0;world[2]=(v.z+(double)shift.z)/4096.0;}return 1;
}

int hg_field_player_facing(const HGFieldPlayer *p) { return p?(int)PlayerAvatar_GetFacingDirection(p->avatar):0; }
int hg_field_player_running(const HGFieldPlayer *p) {
    if(!p||!p->walking)return 0;unsigned command=MapObject_GetMovementCommand((LocalMapObject *)&p->objects[0]);
    return command>=88&&command<=91;
}
unsigned hg_field_player_steps(const HGFieldPlayer *p) { return p?GameStats_GetCapped(p->session.stats,0):0; }

int hg_field_player_interaction_begin(HGFieldPlayer *p) {
    if(p&&p->exploring)return 0;
    if(!p||p->walking||p->script)return 0;p->script_error[0]=0;
    LocalMapObject *target=NULL;if(!hg_motion_facing_object(p->resources,p->avatar,&target))return 0;
    unsigned script=0;p->script_actor=p->actors.active;
    if(target) {
        for(unsigned i=0;i<p->actors.active;i++)if(p->objects+i+1==target){p->script_actor=i;break;}
        if(p->script_actor==p->actors.active||MapObject_GetType(target)==9)return 0;
        script=MapObject_GetScriptID(target);
    } else {
        HGEventView events;if(!hg_event_load(p->map,&events))return 0;
        uint8_t signs[2000];unsigned count=0;
        for(unsigned i=0;i<events.background_count;i++) {
            const uint8_t *event=events.backgrounds+20*i;
            if((event[2]|(unsigned)event[3]<<8)<=1){if(count==100)return 0;memcpy(signs+20*count++,event,20);}
        }
        if(!hg_motion_background_event(p->avatar,signs,count,&script))return 0;
        if(script==65535&&!hg_field_metatile_script(&p->field,p->map,&script)) {
            snprintf(p->script_error,sizeof(p->script_error),"Unsupported metatile interaction service");return 0;
        }
        if(script==65535)return 0;
    }
    return hg_field_player_start_script(p,script,target?(int)MapObject_GetID(target):-1);
}
int hg_field_player_start_script(HGFieldPlayer *p,unsigned entry,int actor_id) {
    if(p&&p->exploring)return 0;
    if(!p||p->walking||p->script)return 0;p->script_error[0]=0;p->script_actor=p->actors.active;
    if(actor_id>=0) {
        for(unsigned i=0;i<p->actors.active;i++)if(MapObject_GetID(p->objects+i+1)==(unsigned)actor_id){p->script_actor=i;break;}
        if(p->script_actor==p->actors.active)return 0;
    }
    p->script_start=calloc(1,sizeof(*p->script_start));if(!p->script_start)return 0;
    ScriptScene *scene=p->script_start;scene->count=p->manager.objectCount;scene->manager=p->manager;scene->avatar=*p->avatar;
    scene->player_save=*PlayerAvatar_GetPlayerSaveData(p->avatar);scene->avatar.playerSaveData=&scene->player_save;
    scene->fade=p->fade;scene->follow=p->session;scene->resources=p->resources;scene->props=p->props;scene->map=p->map;scene->target=p->script_actor<p->actors.active?p->script_actor+1:64;
    memcpy(scene->objects,p->objects,scene->count*sizeof(*scene->objects));
    HGScriptSceneServices services={.source=scene,.clone=scene_clone,.destroy=scene_destroy,.bind=scene_bind,.apply=scene_apply,.tick=scene_tick,.active=scene_active,.busy=scene_busy,.command=scene_command,.face_player=scene_face_player};
    p->script=hg_field_script_scene_new(p->story,p->map,entry,&services,p->script_error,sizeof(p->script_error));
    if(!p->script){free(p->script_start);p->script_start=NULL;if(!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"No admitted script for this interaction");}
    else memcpy(p->script_sprites,p->actor_sprites,sizeof(p->script_sprites));return p->script!=NULL;
}
int hg_field_player_refresh_actors(HGFieldPlayer *p) {
    if(!p||p->walking||p->script)return 0;PreparedActors *actors=prepare_actors(p,p->resources,p->map,p->story,1);
    if(!actors)return 0;install_actors(p,actors);return 1;
}
int hg_field_player_interaction_tick(HGFieldPlayer *p,char *message,size_t capacity) {
    if(!p||!p->script)return HG_SCRIPT_DONE;
    int status=hg_field_script_tick(p->script);
    if(status!=HG_SCRIPT_ERROR&&!publish_scene(p,hg_field_script_scene(p->script))) {
        snprintf(p->script_error,sizeof(p->script_error),"Cannot present script actors");status=HG_SCRIPT_ERROR;
    }
    if(status==HG_SCRIPT_MESSAGE) {
        const HGScriptMessage *text=hg_field_script_message(p->script);
        if(!message||!capacity||strlen(text->text)+1>capacity){snprintf(p->script_error,sizeof(p->script_error),"Message buffer is too small");status=HG_SCRIPT_ERROR;}
        else {
            if(text->face_player&&p->script_actor>=p->actors.active){snprintf(p->script_error,sizeof(p->script_error),"Script requires an interaction actor");status=HG_SCRIPT_ERROR;}
            if(status==HG_SCRIPT_MESSAGE){memcpy(message,text->text,strlen(text->text)+1);return status;}
        }
    }
    if(status==HG_SCRIPT_DONE&&p->automatic==1) {
        unsigned selected;
        if(!hg_story_map_script((HGStoryState *)hg_field_script_candidate(p->script),p->map,INIT_SCRIPT_ON_FRAME_TABLE,&selected)||selected==p->automatic_entry) {
            snprintf(p->script_error,sizeof(p->script_error),"Frame script did not clear its original condition");status=HG_SCRIPT_ERROR;
        }
    }
    if(status==HG_SCRIPT_DONE) {
        const HGStoryState *candidate=hg_field_script_candidate(p->script);PreparedActors *actors=NULL;
        int changed=hg_story_population_changed(p->story,candidate);
        if(changed)actors=prepare_actors(p,p->resources,p->map,candidate,1);
        ScriptScene *scene=(ScriptScene *)hg_field_script_scene(p->script);HGFieldView *props_view=NULL;
        if(scene->props_changed&&hg_field_props_count(scene->props))props_view=hg_field_props_view(scene->props);
        if((scene->props_changed&&hg_field_props_count(scene->props)&&!props_view)||(changed&&!actors)||!hg_field_script_commit(p->script)) {
            hg_field_view_free(props_view);
            free_prepared(actors);snprintf(p->script_error,sizeof(p->script_error),"Cannot publish script state/population");status=HG_SCRIPT_ERROR;
        } else {
            p->fade=scene->fade;*PlayerAvatar_GetPlayerSaveData(p->avatar)=scene->player_save;
            if(actors)install_actors(p,actors);
            if(scene->props_changed) {
                hg_field_props_free(p->props);hg_field_view_free(p->props_view);
                p->props=scene->props;scene->props=NULL;p->props_view=props_view;
            }
        }
    }
    if(status==HG_SCRIPT_ERROR||status==HG_SCRIPT_DONE) {
        int coordinate_failed=p->automatic==2&&status==HG_SCRIPT_ERROR;
        if(p->automatic==1) {
            unsigned next=65535;
            if(status==HG_SCRIPT_ERROR||(hg_story_map_script(p->story,p->map,INIT_SCRIPT_ON_FRAME_TABLE,&next)&&next==p->automatic_entry)) {
                p->rejected_frame=1;p->rejected_revision=hg_story_revision(p->story);
            }
        }
        p->automatic=0;
        if(status==HG_SCRIPT_ERROR)restore_scene(p);
        else {
            hg_transition_sync_player(p->transitions);
            for(unsigned i=0;i<63;i++)if(p->script_sprites[i]!=p->actor_sprites[i])hg_field_view_free(p->script_sprites[i]);
            memset(p->script_sprites,0,sizeof(p->script_sprites));
        }
        if(status==HG_SCRIPT_ERROR&&!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"%s",hg_field_script_error(p->script));
        hg_field_script_free(p->script);p->script=NULL;free(p->script_start);p->script_start=NULL;
        if(coordinate_failed)restore_step(p);
    }
    return status;
}
int hg_field_player_interaction_ack(HGFieldPlayer *p) {
    return p&&p->script&&hg_field_script_ack(p->script);
}
int hg_field_player_interaction_busy(const HGFieldPlayer *p) { return p&&p->script; }
const char *hg_field_player_interaction_error(const HGFieldPlayer *p) { return p?p->script_error:"Missing player"; }
HGStoryState *hg_field_player_story(HGFieldPlayer *p) { return p?p->story:NULL; }
int hg_field_player_interact(HGFieldPlayer *p,char *message,size_t capacity) {
    if(!message||!capacity||!hg_field_player_interaction_begin(p))return 0;
    int status;do{status=hg_field_player_interaction_tick(p,message,capacity);}while(status==HG_SCRIPT_RUNNING);
    return status==HG_SCRIPT_MESSAGE;
}
int hg_field_player_probe_transition(HGFieldPlayer *p,uint16_t keys,HGWarpDestination *out) {
    if(!p||p->walking||p->script||!out)return 0;int direction;
    if(!hg_motion_direction(p->avatar,keys,&direction)||direction<0)return 0;
    /* Native inspection input turns immediately. Probe the intended facing
     * before ordinary height/collision admission: exit tiles can face beyond
     * the room's BDHC surface. The original transition selector still decides. */
    LocalMapObject saved=p->objects[0];
    MapObject_SetFacingDirectionDirect(&p->objects[0],(unsigned)direction);MapObject_SetNextFacingDirection(&p->objects[0],(unsigned)direction);
    int selected=hg_transition_probe(p->transitions,p->resources,(unsigned)direction,out);
    p->objects[0]=saved;return selected;
}
int hg_field_player_transition(HGFieldPlayer *p,const HGWarpDestination *destination) {
    if(!p||p->walking||p->script||!destination||!hg_transition_supported((unsigned)destination->map)||destination->x<0||destination->z<0||destination->x>32767||destination->z>32767||destination->direction<0||destination->direction>3)return 0;
    HGFieldResources *next=hg_field_resources_new((unsigned)destination->map);if(!next)return 0;
    HGFieldTile tile;int32_t height;
    if(!hg_field_resources_tile(next,destination->x,destination->z,&tile)||tile.map_header!=(unsigned)destination->map||
       hg_field_resources_height(next,destination->x,destination->z,0,0,&height)!=1) {
        hg_field_resources_free(next);return 0;
    }
    unsigned long long revision=hg_story_revision(p->story);HGStoryState *story=hg_story_clone(p->story);
    if(!story||(!p->exploring&&!hg_story_enter_map(story,(unsigned)destination->map,p->script_error,sizeof(p->script_error)))){hg_story_free(story);hg_field_resources_free(next);return 0;}
    PreparedActors *actors=prepare_actors(p,next,(unsigned)destination->map,story,0);
    HGFieldProps *props=NULL;HGFieldView *props_view=NULL;
    if(actors)for(unsigned i=0;i<actors->counts.active;i++)if(MapObject_GetID(actors->objects+i)==obj_partner_poke)
        place_object(actors->objects+i,destination->x,destination->z,height,(unsigned)destination->direction);
    if(!actors||actors->counts.unsupported||!prepare_resume(p,actors,story,next,(unsigned)destination->map,destination->x,destination->z,height,(unsigned)destination->direction,&props,&props_view)||hg_story_revision(p->story)!=revision||!hg_transition_commit(p->transitions,destination)) {
        hg_field_props_free(props);hg_field_view_free(props_view);free_prepared(actors);hg_story_free(story);hg_field_resources_free(next);return 0;
    }
    GF_ASSERT(hg_story_commit(p->story,story,revision));hg_story_free(story);
    int resumed=actors->resume_prepared;install_actors(p,actors);
    hg_field_props_free(p->props);hg_field_view_free(p->props_view);p->props=props;p->props_view=props_view;
    hg_field_resources_free(p->resources);p->resources=next;p->field.native_scene_resources=next;p->map=(unsigned)destination->map;hg_map_object_set_map(p->objects,p->map);
    if(!resumed)place_player(p,destination->x,destination->z,height,(unsigned)destination->direction);
    hg_transition_sync_player(p->transitions);
    PlayerAvatar_SetUnk28Unk2C(p->avatar,-1,-1);memset(p->session.follower,0,sizeof(p->session.follower));p->rejected_frame=0;p->automatic=0;p->coordinate_blocked=0;return 1;
}
HGFieldActorCounts hg_field_player_actor_counts(const HGFieldPlayer *p) { return p?p->actors:(HGFieldActorCounts){0}; }
int hg_field_player_actor(const HGFieldPlayer *p,unsigned index,HGFieldActor *out) {
    if(!p||!out||index>=p->actors.active)return 0;
    LocalMapObject *object=(LocalMapObject *)&p->objects[index+1];VecFx32 position,shift;MapObject_CopyPositionVector(object,&position);MapObject_CopyFacingVector(object,&shift);
    *out=(HGFieldActor){.id=MapObject_GetID(object),.sprite=MapObject_GetSpriteID(object),.movement=MapObject_GetMovement(object),
        .hidden=MapObject_CheckVisible(object),.script=MapObject_GetScriptID(object),.direction=MapObject_GetFacingDirection(object),
        .x=(int)MapObject_GetXCoord(object),.z=(int)MapObject_GetZCoord(object),
        .billboard={.sprite=p->actor_sprites[index],.world={(position.x+(double)shift.x)/4096.0,(position.y+(double)shift.y)/4096.0,(position.z+(double)shift.z)/4096.0}}};
    if(p->script) {
        const ScriptScene *scene=hg_field_script_scene(p->script);
        if(scene&&index+1<scene->count) {
            const HGMotionEffect *effect=&scene->movement[index+1].effect;
            out->emote=effect->active&&effect->visible;out->emote_offset=(int32_t)effect->state[7]/4096.0;
        }
    }
    return 1;
}

const HGFieldView *hg_field_player_props(const HGFieldPlayer *p) { return p?p->props_view:NULL; }
unsigned hg_field_player_prop_count(const HGFieldPlayer *p) { return p?hg_field_props_count(p->props):0; }

int hg_field_player_coordinate_begin(HGFieldPlayer *p) {
    if(p&&p->exploring)return 0;
    if(!p||p->walking||p->script)return 0;
    HGEventView events;unsigned entry;
    if(!hg_event_load(p->map,&events)||!hg_motion_coordinate_event(&p->field,p->avatar,events.coordinates,events.coordinate_count,&entry)){snprintf(p->script_error,sizeof(p->script_error),"Cannot select coordinate event");restore_step(p);return -1;}
    if(entry==65535)return 0;
    if(!hg_field_player_start_script(p,entry,-1)){restore_step(p);return -1;}
    p->automatic=2;return 1;
}

int hg_field_player_frame_begin(HGFieldPlayer *p) {
    if(p&&p->exploring)return 0;
    if(!p||p->walking||p->script)return 0;
    unsigned long long revision=hg_story_revision(p->story);
    if(p->rejected_frame&&p->rejected_revision==revision)return 0;
    p->rejected_frame=0;unsigned entry;
    if(!hg_story_map_script(p->story,p->map,INIT_SCRIPT_ON_FRAME_TABLE,&entry)) {
        snprintf(p->script_error,sizeof(p->script_error),"Cannot select map frame script");
        p->rejected_frame=1;p->rejected_revision=revision;return -1;
    }
    if(entry==65535)return 0;
    if(!hg_field_player_start_script(p,entry,-1)) {
        p->rejected_frame=1;p->rejected_revision=revision;return -1;
    }
    p->automatic=1;p->automatic_entry=entry;return 1;
}

static HGStarter *active_starter(const HGFieldPlayer *p) {
    const ScriptScene *scene=p&&p->script?hg_field_script_scene(p->script):NULL;return scene?scene->starter:NULL;
}
int hg_field_player_starter_pending(const HGFieldPlayer *p) { return hg_starter_pending(active_starter(p)); }
int hg_field_player_starter_option(HGFieldPlayer *p,unsigned index,HGStarterOption *out) { return hg_starter_option(active_starter(p),index,out); }
int hg_field_player_choose_starter(HGFieldPlayer *p,unsigned index) { return hg_starter_choose(active_starter(p),index); }
double hg_field_player_starter_fade(const HGFieldPlayer *p) { return hg_starter_fade(active_starter(p)); }

int hg_field_player_menu_pending(const HGFieldPlayer *p) { return p&&hg_field_script_menu_pending(p->script); }
int hg_field_player_menu_choose(HGFieldPlayer *p,unsigned choice) { return p&&hg_field_script_menu_choose(p->script,choice); }
int hg_field_player_menu_choice_supported(HGFieldPlayer *p,unsigned choice) { return p&&hg_field_script_menu_choice_supported(p->script,choice); }

int hg_field_player_follower(const HGFieldPlayer *p,unsigned *species,int *x,int *z) {
 if(!p)return 0;for(unsigned i=1;i<p->manager.objectCount;i++)if(MapObject_GetID((LocalMapObject *)p->objects+i)==obj_partner_poke) {
  LocalMapObject *o=(LocalMapObject *)p->objects+i;if(species)*species=MapObject_GetParam(o,0);if(x)*x=MapObject_GetXCoord(o);if(z)*z=MapObject_GetZCoord(o);return 1;
 }return 0;
}

int hg_field_player_present_props(const HGFieldPlayer *p,HGFieldView *view) {
    HGPropPose poses[16];unsigned count=0;
    if(p&&p->script){const ScriptScene *scene=hg_field_script_scene(p->script);if(scene)count=hg_prop_animations_poses(scene->prop_animations,poses);}
    return hg_field_view_prop_poses(view,poses,count);
}

int hg_field_player_running_shoes(const HGFieldPlayer *p) { return p&&PlayerSaveData_CheckRunningShoes(PlayerAvatar_GetPlayerSaveData(p->avatar)); }

HGFieldPlayer *hg_field_player_new_explore(unsigned map) { return player_new_map(map,NULL,1); }

int hg_field_player_brightness(const HGFieldPlayer *p) {
    if(!p)return 0;const ScriptScene *scene=p->script?hg_field_script_scene(p->script):NULL;
    return scene?scene->fade.brightness:p->fade.brightness;
}
