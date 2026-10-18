#include "compat/map_objects.h"
#include "field_player.h"
#include "field_app.h"
#include "field_events.h"
#include "field_data.h"
#include "motion_guest.h"
#include "constants/sprites.h"
#include "constants/vars.h"
#include "constants/flags.h"
#include "metatile_behavior.h"
#include <math.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
static void walk(HGFieldPlayer *player,unsigned keys) {
    CHECK(hg_field_player_move(player,(uint16_t)keys));
    for(unsigned i=0;i<8;i++)CHECK(hg_field_player_tick(player)==1);
    CHECK(!hg_field_player_walking(player));
}
static void set_location(LocalMapObject *object,int x,int z,int32_t height,unsigned direction) {
    MapObject_SetCurrentX(object,x);MapObject_SetCurrentZ(object,z);MapObject_SetFacingDirectionDirect(object,direction);
    VecFx32 position={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(object,&position);
}
int main(void) {
    /* Independent synthetic event verifies the original initializer's complete
     * scalar metadata and fixed-point placement, then original occupancy over
     * the actual initialized object rather than a separate collider list. */
    FieldSystem field={0};LocalMapObject objects[2]={0};
    MapObjectManager manager={.objects=objects,.objectCount=2,.fieldSystem=&field};
    objects[0].manager=&manager;MapObject_SetFlagsBits(objects,MAPOBJECTFLAG_ACTIVE);
    ObjectEvent event={.id=31,.spriteId=325,.movement=3,.type=2,.scriptId=123,.facingDirection=2,
        .param={17,29,65535},.xRange=-1,.yRange=4,.x=683,.z=399,.y=65536};
    CHECK(hg_map_object_init_event(objects+1,&manager,60,&event));
    LocalMapObject *object=objects+1;
    CHECK(MapObject_GetManager(object)==&manager&&MapObject_GetMapID(object)==60&&MapObject_CheckActive(object));
    CHECK(MapObject_GetID(object)==31&&MapObject_GetSpriteID(object)==325&&MapObject_GetMovement(object)==3);
    CHECK(MapObject_GetType(object)==2&&MapObject_GetEventFlag(object)==0&&MapObject_GetScriptID(object)==123);
    CHECK(MapObject_GetInitialFacingDirection(object)==2&&MapObject_GetFacingDirection(object)==2&&MapObject_GetNextFacingDirection(object)==2);
    CHECK(MapObject_GetParam(object,0)==17&&MapObject_GetParam(object,1)==29&&MapObject_GetParam(object,2)==65535);
    CHECK(MapObject_GetXRange(object)==-1&&MapObject_GetYRange(object)==4);
    CHECK(MapObject_GetXCoord(object)==683&&MapObject_GetPreviousXCoord(object)==683&&MapObject_GetInitialX(object)==683);
    CHECK(MapObject_GetZCoord(object)==399&&MapObject_GetPreviousZCoord(object)==399&&MapObject_GetInitialZ(object)==399);
    CHECK(MapObject_GetYCoord(object)==2&&MapObject_GetPreviousYCoord(object)==2&&MapObject_GetInitialY(object)==2);
    VecFx32 position;MapObject_CopyPositionVector(object,&position);
    CHECK(position.x==683*65536+32768&&position.y==65536&&position.z==399*65536+32768);
    CHECK(MapObject_GetMovementCommand(object)==255&&MapObject_GetMovementStep(object)==0);
    CHECK(!MapObject_TestFlagsBits(object,0x10)&&MapObject_TestFlagsBits(object,0x20));
    int occupied;CHECK(hg_motion_occupied(objects,683,2,399,&occupied)&&occupied);
    CHECK(hg_motion_occupied(objects,683,4,399,&occupied)&&!occupied);
    MapObject_ClearFlagsBits(object,MAPOBJECTFLAG_ACTIVE);
    CHECK(hg_motion_occupied(objects,683,2,399,&occupied)&&!occupied);
    LocalMapObject saved=*object;event.eventFlag=1;
    CHECK(!hg_map_object_init_event(object,&manager,60,&event)&&!memcmp(&saved,object,sizeof(saved)));
    event.eventFlag=0;event.spriteId=101;
    CHECK(!hg_map_object_init_event(object,&manager,60,&event)&&!memcmp(&saved,object,sizeof(saved)));
    event.spriteId=325;event.facingDirection=-1;
    CHECK(!hg_map_object_init_event(object,&manager,60,&event)&&!memcmp(&saved,object,sizeof(saved)));
    objects[0].movement=1;PlayerSaveData save;PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
    HGFieldResources *terrain=hg_field_resources_new(60);CHECK(terrain);
    const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
    MapObject_SetFlagsBits(object,MAPOBJECTFLAG_ACTIVE);
    for(unsigned direction=0;direction<4;direction++) {
        set_location(objects,683-dx[direction],399-dz[direction],0,direction);
        set_location(object,683,399,0,0);LocalMapObject *target=NULL;
        LocalMapObject before[2];memcpy(before,objects,sizeof(before));
        CHECK(hg_motion_facing_object(terrain,avatar,&target)&&target==object);
        CHECK(!memcmp(before,objects,sizeof(before)));
        MapObject_SetFlagsBits(object,MAPOBJECTFLAG_UNK19);
        CHECK(hg_motion_facing_object(terrain,avatar,&target)&&!target);
        MapObject_ClearFlagsBits(object,MAPOBJECTFLAG_UNK19);
        set_location(object,683,399,32767,0);CHECK(hg_motion_facing_object(terrain,avatar,&target)&&target==object);
        set_location(object,683,399,32768,0);CHECK(hg_motion_facing_object(terrain,avatar,&target)&&!target);
        MapObject_ClearFlagsBits(object,MAPOBJECTFLAG_ACTIVE);
        CHECK(hg_motion_facing_object(terrain,avatar,&target)&&!target);MapObject_SetFlagsBits(object,MAPOBJECTFLAG_ACTIVE);
    }
    hg_field_resources_free(terrain);
    /* Find real counter attributes, then test the unchanged selector's second
     * facing-tile step in all directions. Synthetic actors isolate selection
     * rules; this is distinct from testing a traversable player approach. */
    terrain=hg_field_resources_new(68);CHECK(terrain);unsigned counters=0;
    for(int z=1;z<31;z++)for(int x=1;x<31;x++) {
        HGMotionTile tile;CHECK(hg_motion_tile(terrain,x,z,&tile));if(!sub_0205B700(tile.behavior))continue;
        counters++;
        for(unsigned direction=0;direction<4;direction++) {
            set_location(objects,x-dx[direction],z-dz[direction],0,direction);
            set_location(object,x+dx[direction],z+dz[direction],0,0);LocalMapObject *target=NULL;
            CHECK(hg_motion_facing_object(terrain,avatar,&target)&&target==object);
            set_location(object,x,z,0,0);CHECK(hg_motion_facing_object(terrain,avatar,&target)&&!target);
        }
    }
    CHECK(counters);hg_field_resources_free(terrain);PlayerAvatar_FreeToHeap(avatar);

    const unsigned maps[]={60,61,62,63,64,65,66,384};unsigned active=0,conditional=0,unsupported=0;
    for(unsigned m=0;m<NELEMS(maps);m++) {
        HGFieldPlayer *player=hg_field_player_new_map(maps[m]);CHECK(player);
        HGEventView events;CHECK(hg_event_load(maps[m],&events));HGFieldActorCounts counts=hg_field_player_actor_counts(player);
        CHECK(counts.events==events.object_count&&counts.active+counts.conditional+counts.unsupported==counts.events);
        unsigned expected_conditional=0;
        for(unsigned i=0;i<events.object_count;i++){ObjectEvent source;memcpy(&source,events.objects+32*i,32);int hidden;CHECK(hg_story_flag_get(hg_field_player_story(player),source.eventFlag,&hidden));expected_conditional+=(unsigned)hidden;}
        CHECK(expected_conditional==counts.conditional);
        HGFieldBillboard billboards[64];
        for(unsigned i=0;i<counts.active;i++) {
            HGFieldActor actor;CHECK(hg_field_player_actor(player,i,&actor));unsigned matches=0;
            for(unsigned e=0;e<events.object_count;e++) {
                ObjectEvent source;memcpy(&source,events.objects+32*e,32);
                if(source.id!=actor.id)continue;int hidden;CHECK(hg_story_flag_get(hg_field_player_story(player),source.eventFlag,&hidden)&&!hidden);
                unsigned sprite=source.spriteId;if(sprite>=101&&sprite<=117){uint16_t graphic;CHECK(hg_story_var_get(hg_field_player_story(player),VAR_OBJ_GFX_BASE+sprite-101,&graphic));sprite=graphic;}
                CHECK(actor.sprite==sprite&&actor.movement==source.movement&&actor.script==source.scriptId);
                CHECK(actor.x==source.x&&actor.z==source.z&&actor.direction==(unsigned)source.facingDirection);
                CHECK(actor.billboard.world[0]==source.x*16+8&&actor.billboard.world[2]==source.z*16+8);matches++;
            }
            CHECK(matches==1&&actor.billboard.sprite);billboards[i]=actor.billboard;
        }
        HGFieldActor unchanged={.id=12345};CHECK(!hg_field_player_actor(player,counts.active,&unchanged)&&unchanged.id==12345);
        HGFieldView *view=hg_field_view_new(maps[m]);CHECK(view);DSCamera camera;unsigned char with[256*192*4],without[sizeof(with)];
        double world[3];CHECK(hg_field_player_position(player,NULL,NULL,world));hg_field_view_focus(view,world,&camera,256,192);
        CHECK(hg_field_view_render(view,&camera,without,256,192));
        CHECK(hg_field_view_render_actors(view,billboards,counts.active,&camera,with,256,192));
        if(maps[m]==63)CHECK(memcmp(with,without,sizeof(with)));
        if(!counts.active)CHECK(!memcmp(with,without,sizeof(with)));
        if(counts.active) {
            HGFieldBillboard invalid=billboards[0];invalid.world[0]=NAN;
            CHECK(!hg_field_view_render_actors(view,&invalid,1,&camera,with,256,192));
            CHECK(!hg_field_view_render_actors(view,NULL,1,&camera,with,256,192));
            CHECK(!hg_field_view_render_actors(view,billboards,65,&camera,with,256,192));
        }
        printf("Map %u: %u active, %u conditional, %u unsupported / %u events\n",maps[m],counts.active,counts.conditional,counts.unsupported,counts.events);
        active+=counts.active;conditional+=counts.conditional;unsupported+=counts.unsupported;
        hg_field_view_free(view);hg_field_player_free(player);
    }
    CHECK(active==12&&conditional==10&&unsupported==2);
    /* An actual player approaches the town woman. Original object occupancy
     * prevents entering her tile, without counting or beginning a step. */
    HGFieldPlayer *player=hg_field_player_new();CHECK(player);
    for(unsigned i=0;i<4;i++)walk(player,0x20);walk(player,0x80);
    int x,z;CHECK(hg_field_player_position(player,&x,&z,NULL)&&x==684&&z==399);
    unsigned steps=hg_field_player_steps(player);CHECK(!hg_field_player_move(player,0x20));
    CHECK(!hg_field_player_walking(player)&&hg_field_player_steps(player)==steps);
    CHECK(hg_field_player_position(player,&x,&z,NULL)&&x==684&&z==399);
    CHECK(hg_field_player_facing(player)==2);walk(player,0x40);hg_field_player_free(player);
    /* Population replacement is part of the same staged handoff as terrain
     * and local-field state. A rejected destination retains borrowed sprites;
     * a real house round trip supplies Mom, then restores town actors. */
    player=hg_field_player_new();CHECK(player);
    walk(player,0x40);for(unsigned i=0;i<7;i++)walk(player,0x10);
    HGWarpDestination destination;CHECK(hg_field_player_probe_transition(player,0x40,&destination));
    HGFieldActor before,after;CHECK(hg_field_player_actor(player,0,&before));
    HGWarpDestination altered=destination;altered.x++;
    CHECK(!hg_field_player_transition(player,&altered));CHECK(hg_field_player_actor(player,0,&after));
    CHECK(before.billboard.sprite==after.billboard.sprite&&before.id==after.id&&before.x==after.x&&before.z==after.z);
    CHECK(hg_field_player_transition(player,&destination));CHECK(hg_field_player_actor_counts(player).active==1);
    CHECK(hg_field_player_actor(player,0,&after)&&after.sprite==SPRITE_GSMAMA&&after.x==6&&after.z==7);
    CHECK(hg_field_player_probe_transition(player,0x80,&destination)&&hg_field_player_transition(player,&destination));
    CHECK(hg_field_player_actor_counts(player).active==4&&hg_field_player_actor(player,0,&after));
    CHECK(after.sprite==before.sprite&&after.id==before.id&&after.x==before.x&&after.z==before.z);
    hg_field_player_free(player);
    /* Preflight cannot move live objects. A stale owner during an actual
     * rival movement restores all actor poses and the player's position. */
    {
    player=hg_field_player_new();CHECK(player);HGStoryState *state=hg_field_player_story(player);char message[512];int status;
    HGFieldActor original[63];unsigned rival=63,count=hg_field_player_actor_counts(player).active;
    for(unsigned i=0;i<count;i++){CHECK(hg_field_player_actor(player,i,original+i));if(original[i].script==1)rival=i;}
    CHECK(rival<count);int startx,startz;double startworld[3];CHECK(hg_field_player_position(player,&startx,&startz,startworld));
    CHECK(hg_field_player_start_script(player,1,(int)original[rival].id));
    int x,z;double world[3];CHECK(hg_field_player_position(player,&x,&z,world)&&x==startx&&z==startz&&!memcmp(world,startworld,sizeof(world)));
    CHECK(hg_field_player_interaction_tick(player,message,sizeof(message))==HG_SCRIPT_MESSAGE&&strstr(message,"famous Elm"));
    CHECK(hg_field_player_interaction_ack(player));
    do{status=hg_field_player_interaction_tick(player,message,sizeof(message));}while(status==HG_SCRIPT_RUNNING);
    CHECK(status==HG_SCRIPT_MESSAGE&&strstr(message,"What are you staring at?"));CHECK(hg_field_player_interaction_ack(player));
    for(unsigned i=0;i<12;i++)CHECK(hg_field_player_interaction_tick(player,message,sizeof(message))==HG_SCRIPT_RUNNING);
    CHECK(hg_story_var_set(state,VAR_TEMP_x4000,123));
    CHECK(hg_field_player_interaction_tick(player,message,sizeof(message))==HG_SCRIPT_ERROR&&!hg_field_player_interaction_busy(player));
    CHECK(hg_field_player_position(player,&x,&z,world)&&x==startx&&z==startz&&!memcmp(world,startworld,sizeof(world)));
    for(unsigned i=0;i<count;i++){CHECK(hg_field_player_actor(player,i,&after));CHECK(after.x==original[i].x&&after.z==original[i].z&&after.direction==original[i].direction&&after.billboard.sprite==original[i].billboard.sprite);}
    hg_field_player_free(player);
    }
    player=hg_field_player_new_map(65);CHECK(player);
    for(unsigned i=0;i<3;i++)walk(player,0x40);CHECK(!hg_field_player_move(player,0x10));
    CHECK(hg_field_player_actor(player,0,&before));char message[512];
    CHECK(hg_field_player_interact(player,message,sizeof(message))&&strstr(message,"PIKACHU"));
    CHECK(hg_field_player_actor(player,0,&after)&&after.direction==2&&after.x==before.x&&after.z==before.z);
    hg_field_player_free(player);
    /* Real original map-entry script hides Cameron. Stage a visible actor via
     * its actual flag, then execute the selected hook and verify publication
     * changes both population and rendering ownership, retaining other poses. */
    player=hg_field_player_new();CHECK(player);HGStoryState *state=hg_field_player_story(player);
    CHECK(hg_story_flag_set(state,FLAG_HIDE_CAMERON,0)&&hg_field_player_refresh_actors(player));
    CHECK(hg_field_player_actor_counts(player).active==5);unsigned hook;
    CHECK(hg_story_map_script(state,60,2,&hook)&&hook!=65535);
    CHECK(hg_field_player_start_script(player,hook,-1));int status;
    do{status=hg_field_player_interaction_tick(player,message,sizeof(message));}while(status==HG_SCRIPT_RUNNING);
    CHECK(status==HG_SCRIPT_DONE&&hg_field_player_actor_counts(player).active==4);
    int hidden;CHECK(hg_story_flag_get(state,FLAG_HIDE_CAMERON,&hidden)&&hidden);
    for(unsigned i=0;i<hg_field_player_actor_counts(player).active;i++){CHECK(hg_field_player_actor(player,i,&after));CHECK(after.x!=695||after.z!=403);}
    hg_field_player_free(player);
    printf("Original event actors: %u active, %u conditional, %u unsupported; source metadata, Q12 initialization, real occupancy and shared-depth rendering passed\n",active,conditional,unsupported);
    return 0;
}
