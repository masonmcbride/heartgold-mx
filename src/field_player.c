#include "compat/motion.h"
#include "compat/stats.h"
#include "compat/map_objects.h"
#include "compat/story.h"
#include "motion_guest.h"
#include "field_data.h"
#include "field_player.h"
#include "field_events.h"
typedef struct {
    const uint8_t *bytes;size_t size,offset;unsigned remaining,command;int active;
} ScriptMovement;
typedef struct {
    LocalMapObject objects[64];PlayerAvatar avatar;MapObjectManager manager;
    HGFieldResources *resources;ScriptMovement movement[64];unsigned count,map,target;
} ScriptScene;
struct HGFieldPlayer {
    FieldSystem field;MapObjectManager manager;LocalMapObject objects[64];
    HGFieldView *actor_sprites[63];HGFieldActorCounts actors;
    PlayerSaveData save;PlayerAvatar *avatar;HGFieldResources *resources;
    LocalMapObject step_start;int walking;
    HGMotionSession session;
    HGMotionSession session_start;PlayerAvatar avatar_start;uint8_t *stats_start;
    HGTransition *transitions;unsigned map;
    HGStoryState *story;HGFieldScript *script;unsigned script_actor;char script_error[160];
    ScriptScene *script_start;
    HGFieldView *script_sprites[63];
};
typedef struct {
    LocalMapObject objects[63];HGFieldView *sprites[63];HGFieldActorCounts counts;
} PreparedActors;
static void free_prepared(PreparedActors *actors) {
    if(!actors)return;for(unsigned i=0;i<actors->counts.active;i++)hg_field_view_free(actors->sprites[i]);free(actors);
}
static void release_live_sprite(HGFieldPlayer *p,unsigned index) {
    if(p->actor_sprites[index]!=p->script_sprites[index])hg_field_view_free(p->actor_sprites[index]);
}
static PreparedActors *prepare_actors(HGFieldPlayer *p,HGFieldResources *resources,unsigned map,const HGStoryState *story,int retain) {
    HGEventView events;if(!hg_event_load(map,&events)||events.object_count>63)return NULL;
    PreparedActors *actors=calloc(1,sizeof(*actors));if(!actors)return NULL;
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
        LocalMapObject *existing=NULL;
        if(retain&&map==p->map)for(unsigned n=0;n<p->actors.active;n++)if(MapObject_GetID(p->objects+n+1)==event.id&&MapObject_GetSpriteID(p->objects+n+1)==sprite_id){existing=p->objects+n+1;break;}
        HGFieldView *sprite=hg_character_sprite_load(sprite_id,existing?MapObject_GetFacingDirection(existing):(unsigned)event.facingDirection,NULL,0);
        if(!sprite){actors->counts.unsupported++;continue;}
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
    free(actors); /* Sprite ownership moved; stable live object slots copied. */
}
/* The host renderer owns sprite resources; there is no DS VRAM transfer to
 * perform at completion. Original command dispatch still invokes this native
 * resource notification through the original map-object callback accessor. */
static void movement_finished(LocalMapObject *object) { (void)object; }
static unsigned scene_half(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static void *scene_clone(const void *source) {
    ScriptScene *scene=malloc(sizeof(*scene));if(!scene)return NULL;*scene=*(const ScriptScene *)source;
    scene->manager.objects=scene->objects;scene->avatar.mapObject=scene->objects;
    for(unsigned i=0;i<scene->count;i++){scene->objects[i].manager=&scene->manager;sub_0205F47C(scene->objects+i,movement_finished);}
    return scene;
}
static void scene_bind(void *opaque,FieldSystem *field) {
    ScriptScene *scene=opaque;scene->manager.fieldSystem=field;field->mapObjectManager=&scene->manager;field->playerAvatar=&scene->avatar;
}
static unsigned scene_facing(const void *opaque) { return PlayerAvatar_GetFacingDirection((PlayerAvatar *)&((const ScriptScene *)opaque)->avatar); }
static int scene_face_player(void *opaque) {
    ScriptScene *scene=opaque;if(scene->target>=scene->count)return 0;
    unsigned facing=scene_facing(scene)^1u;LocalMapObject *object=scene->objects+scene->target;
    MapObject_SetFacingDirectionDirect(object,facing);MapObject_SetNextFacingDirection(object,facing);return 1;
}
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
static int scene_tick(void *opaque) {
    ScriptScene *scene=opaque;
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
        int complete;if(!hg_motion_script_tick(scene->resources,object,&complete))return 0;
        HGFieldTile tile;VecFx32 v;MapObject_CopyPositionVector(object,&v);
        if(!hg_field_resources_tile(scene->resources,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&tile)||tile.map_header!=scene->map||
           !hg_field_resources_tile(scene->resources,v.x/65536,v.z/65536,&tile)||tile.map_header!=scene->map)return 0;
        if(complete)job->remaining--;
    }
    return 1;
}
static int publish_scene(HGFieldPlayer *p,const ScriptScene *scene) {
    HGFieldView *sprites[63]={0};
    for(unsigned i=1;i<scene->count;i++)if(MapObject_GetFacingDirection(p->objects+i)!=MapObject_GetFacingDirection((LocalMapObject *)scene->objects+i)) {
        sprites[i-1]=hg_character_sprite_load(MapObject_GetSpriteID(p->objects+i),MapObject_GetFacingDirection((LocalMapObject *)scene->objects+i),NULL,0);
        if(!sprites[i-1]){for(unsigned n=0;n<63;n++)hg_field_view_free(sprites[n]);return 0;}
    }
    memcpy(p->objects,scene->objects,scene->count*sizeof(*p->objects));
    for(unsigned i=0;i<scene->count;i++)p->objects[i].manager=&p->manager;
    for(unsigned i=0;i<63;i++)if(sprites[i]){release_live_sprite(p,i);p->actor_sprites[i]=sprites[i];}
    return 1;
}
static void restore_scene(HGFieldPlayer *p) {
    /* Keep the initial views alive until completion. Rollback needs no new
     * allocation even if preparation of a later facing view failed. */
    memcpy(p->objects,p->script_start->objects,p->script_start->count*sizeof(*p->objects));
    for(unsigned i=0;i<63;i++){release_live_sprite(p,i);p->actor_sprites[i]=p->script_sprites[i];p->script_sprites[i]=NULL;}
}
static void place_player(HGFieldPlayer *p,int x,int z,int32_t height,unsigned direction) {
    MapObject_SetCurrentX(&p->objects[0],x);MapObject_SetCurrentZ(&p->objects[0],z);MapObject_SetCurrentY(&p->objects[0],(height>>3)/4096);
    MapObject_SetPreviousX(&p->objects[0],x);MapObject_SetPreviousZ(&p->objects[0],z);MapObject_SetPreviousY(&p->objects[0],(height>>3)/4096);
    VecFx32 v={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(&p->objects[0],&v);
    MapObject_SetFacingDirectionDirect(&p->objects[0],direction);MapObject_SetNextFacingDirection(&p->objects[0],direction);
}
void hg_field_player_free(HGFieldPlayer *p) {
    if(!p)return;hg_field_script_free(p->script);free(p->script_start);hg_story_free(p->story);hg_transition_free(p->transitions);if(p->avatar)PlayerAvatar_FreeToHeap(p->avatar);
    for(unsigned i=0;i<p->actors.active;i++)release_live_sprite(p,i);
    for(unsigned i=0;i<63;i++)hg_field_view_free(p->script_sprites[i]);
    hg_field_resources_free(p->resources);free(p->session.stats);free(p->stats_start);free(p);
}
HGFieldPlayer *hg_field_player_new(void) { return hg_field_player_new_map(60); }
HGFieldPlayer *hg_field_player_new_map(unsigned map) {
    if(!hg_transition_supported(map))return NULL;
    HGFieldPlayer *p=calloc(1,sizeof(*p));if(!p)return NULL;
    p->map=map;p->resources=hg_field_resources_new(map);if(!p->resources)goto fail;
    p->story=hg_story_new();if(!p->story||!hg_story_enter_map(p->story,map,p->script_error,sizeof(p->script_error)))goto fail;
    hg_story_bind_field(p->story,&p->field);
    p->session.stats=calloc(1,GameStats_sizeof());p->stats_start=malloc(GameStats_sizeof());
    if(!p->session.stats||!p->stats_start)goto fail;
    GameStats_Init(p->session.stats);
    p->manager.objects=&p->objects[0];p->manager.objectCount=1;p->manager.fieldSystem=&p->field;
    p->objects[0].manager=&p->manager;p->objects[0].movement=1;
    MapObject_SetFlagsBits(&p->objects[0],MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&p->save);
    /* Temporary inspection session equipment; not the new-game story award. */
    PlayerSaveData_SetRunningShoesFlag(&p->save,TRUE);
    p->avatar=PlayerAvatar_CreateWithActiveMapObject(&p->manager,&p->save,0);if(!p->avatar)goto fail;
    p->field.playerAvatar=p->avatar;
    MapObject_SetMovementCommand(&p->objects[0],255);sub_0205F47C(&p->objects[0],movement_finished);
    MapObject_SetXRange(&p->objects[0],-1);MapObject_SetYRange(&p->objects[0],-1);
    p->transitions=hg_transition_new(map,p->avatar);if(!p->transitions)goto fail;
    PreparedActors *actors=prepare_actors(p,p->resources,map,p->story,0);if(!actors)goto fail;install_actors(p,actors);
    if(map!=60) {
        HGEventView events;if(!hg_event_load(map,&events)||!events.warp_count)goto fail;
        int x=events.warps[0]|events.warps[1]<<8,z=events.warps[2]|events.warps[3]<<8;int32_t height;HGFieldTile tile;
        if(!hg_field_resources_tile(p->resources,x,z,&tile)||tile.map_header!=map||hg_field_resources_height(p->resources,x,z,0,0,&height)!=1)goto fail;
        place_player(p,x,z,height,0);hg_transition_sync_player(p->transitions);return p;
    }
    /* A native inspection-session spawn, not the original new-game script.
     * Prefer the town center; admit only a tile with an ordinary walking exit. */
    for(int radius=0;radius<32;radius++)for(int z=384;z<416;z++)for(int x=672;x<704;x++) {
        if(abs(x-688)+abs(z-398)!=radius)continue;
        HGMotionTile tile;int32_t height;
        if(!hg_motion_tile(p->resources,x,z,&tile)||tile.wall||
           hg_field_resources_height(p->resources,x,z,0,0,&height)!=1)continue;
        place_player(p,x,z,height,0);
        for(unsigned d=0;d<4;d++){unsigned mask;if(hg_motion_player_collision(p->resources,p->avatar,d,&mask)&&!mask){hg_transition_sync_player(p->transitions);return p;}}
    }
fail:hg_field_player_free(p);return NULL;
}
int hg_field_player_move(HGFieldPlayer *p,uint16_t keys) {
    if(!p||p->walking||p->script)return 0;HGMotionMove move;
    p->step_start=p->objects[0];
    p->avatar_start=*p->avatar;p->session_start=p->session;
    memcpy(p->stats_start,p->session.stats,GameStats_sizeof());
    if(!hg_motion_locomotion_begin(p->resources,p->avatar,keys,&p->session,&move))return 0;
    if(!move.moved&&move.direction>=0) {
        MapObject_SetFacingDirectionDirect(&p->objects[0],(unsigned)move.direction);
        MapObject_SetNextFacingDirection(&p->objects[0],(unsigned)move.direction);
    }
    p->walking=move.moved;
    if(!p->walking)hg_transition_sync_player(p->transitions);
    return move.moved;
}
int hg_field_player_walking(const HGFieldPlayer *p) { return p&&p->walking; }
int hg_field_player_tick(HGFieldPlayer *p) {
    if(!p||!p->walking)return 0;int complete;
    if(!hg_motion_walk_tick(p->resources,&p->objects[0],&complete)) {
        p->objects[0]=p->step_start;*p->avatar=p->avatar_start;p->session=p->session_start;
        memcpy(p->session.stats,p->stats_start,GameStats_sizeof());p->walking=0;return -1;
    }
    p->walking=!complete;if(complete)hg_transition_sync_player(p->transitions);return 1;
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
    if(!p||p->walking||p->script)return 0;p->script_error[0]=0;
    LocalMapObject *target=NULL;if(!hg_motion_facing_object(p->resources,p->avatar,&target))return 0;
    unsigned script=0;p->script_actor=p->actors.active;
    if(target) {
        for(unsigned i=0;i<p->actors.active;i++)if(p->objects+i+1==target){p->script_actor=i;break;}
        if(p->script_actor==p->actors.active||MapObject_GetType(target)==9)return 0;
        script=MapObject_GetScriptID(target);
    } else {
        if(p->map!=60)return 0;HGEventView events;if(!hg_event_load(p->map,&events))return 0;
        uint8_t signs[2000];unsigned count=0;
        for(unsigned i=0;i<events.background_count;i++) {
            const uint8_t *event=events.backgrounds+20*i;
            if((event[2]|(unsigned)event[3]<<8)==1){if(count==100)return 0;memcpy(signs+20*count++,event,20);}
        }
        if(!hg_motion_background_event(p->avatar,signs,count,&script)||script==65535)return 0;
    }
    return hg_field_player_start_script(p,script,target?(int)MapObject_GetID(target):-1);
}
int hg_field_player_start_script(HGFieldPlayer *p,unsigned entry,int actor_id) {
    if(!p||p->walking||p->script)return 0;p->script_error[0]=0;p->script_actor=p->actors.active;
    if(actor_id>=0) {
        for(unsigned i=0;i<p->actors.active;i++)if(MapObject_GetID(p->objects+i+1)==(unsigned)actor_id){p->script_actor=i;break;}
        if(p->script_actor==p->actors.active)return 0;
    }
    p->script_start=calloc(1,sizeof(*p->script_start));if(!p->script_start)return 0;
    ScriptScene *scene=p->script_start;scene->count=p->manager.objectCount;scene->manager=p->manager;scene->avatar=*p->avatar;
    scene->resources=p->resources;scene->map=p->map;scene->target=p->script_actor<p->actors.active?p->script_actor+1:scene->count;
    memcpy(scene->objects,p->objects,scene->count*sizeof(*scene->objects));
    HGScriptSceneServices services={.source=scene,.clone=scene_clone,.destroy=free,.bind=scene_bind,.apply=scene_apply,.tick=scene_tick,.active=scene_active,.facing=scene_facing,.face_player=scene_face_player};
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
    if(status==HG_SCRIPT_DONE) {
        const HGStoryState *candidate=hg_field_script_candidate(p->script);PreparedActors *actors=NULL;
        int changed=hg_story_population_changed(p->story,candidate);
        if(changed)actors=prepare_actors(p,p->resources,p->map,candidate,1);
        if((changed&&!actors)||!hg_field_script_commit(p->script)) {
            free_prepared(actors);snprintf(p->script_error,sizeof(p->script_error),"Cannot publish script state/population");status=HG_SCRIPT_ERROR;
        } else if(actors)install_actors(p,actors);
    }
    if(status==HG_SCRIPT_ERROR||status==HG_SCRIPT_DONE) {
        if(status==HG_SCRIPT_ERROR)restore_scene(p);
        else {
            hg_transition_sync_player(p->transitions);
            for(unsigned i=0;i<63;i++)if(p->script_sprites[i]!=p->actor_sprites[i])hg_field_view_free(p->script_sprites[i]);
            memset(p->script_sprites,0,sizeof(p->script_sprites));
        }
        if(status==HG_SCRIPT_ERROR&&!p->script_error[0])snprintf(p->script_error,sizeof(p->script_error),"%s",hg_field_script_error(p->script));
        hg_field_script_free(p->script);p->script=NULL;free(p->script_start);p->script_start=NULL;
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
    if(!story||!hg_story_enter_map(story,(unsigned)destination->map,p->script_error,sizeof(p->script_error))){hg_story_free(story);hg_field_resources_free(next);return 0;}
    PreparedActors *actors=prepare_actors(p,next,(unsigned)destination->map,story,0);
    if(!actors||hg_story_revision(p->story)!=revision||!hg_transition_commit(p->transitions,destination)) {
        free_prepared(actors);hg_story_free(story);hg_field_resources_free(next);return 0;
    }
    GF_ASSERT(hg_story_commit(p->story,story,revision));hg_story_free(story);
    install_actors(p,actors);
    hg_field_resources_free(p->resources);p->resources=next;p->map=(unsigned)destination->map;
    place_player(p,destination->x,destination->z,height,(unsigned)destination->direction);
    PlayerAvatar_SetUnk28Unk2C(p->avatar,-1,-1);memset(p->session.follower,0,sizeof(p->session.follower));return 1;
}
HGFieldActorCounts hg_field_player_actor_counts(const HGFieldPlayer *p) { return p?p->actors:(HGFieldActorCounts){0}; }
int hg_field_player_actor(const HGFieldPlayer *p,unsigned index,HGFieldActor *out) {
    if(!p||!out||index>=p->actors.active)return 0;
    LocalMapObject *object=(LocalMapObject *)&p->objects[index+1];VecFx32 position,shift;MapObject_CopyPositionVector(object,&position);MapObject_CopyFacingVector(object,&shift);
    *out=(HGFieldActor){.id=MapObject_GetID(object),.sprite=MapObject_GetSpriteID(object),.movement=MapObject_GetMovement(object),
        .script=MapObject_GetScriptID(object),.direction=MapObject_GetFacingDirection(object),
        .x=(int)MapObject_GetXCoord(object),.z=(int)MapObject_GetZCoord(object),
        .billboard={.sprite=p->actor_sprites[index],.world={(position.x+(double)shift.x)/4096.0,(position.y+(double)shift.y)/4096.0,(position.z+(double)shift.z)/4096.0}}};
    return 1;
}
