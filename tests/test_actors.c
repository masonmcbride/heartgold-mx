#include "compat/map_objects.h"
#include "compat/story.h"
#include "prop_runtime.h"
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
typedef struct {
    LocalMapObject objects[2];MapObjectManager manager;PlayerAvatar avatar;HGFieldResources *resources;
} QueryScene;
static void *query_clone(const void *source) {
    QueryScene *scene=malloc(sizeof(*scene));if(!scene)return NULL;*scene=*(const QueryScene *)source;
    scene->manager.objects=scene->objects;scene->avatar.mapObject=scene->objects;
    for(unsigned i=0;i<2;i++)scene->objects[i].manager=&scene->manager;
    return scene;
}
static void query_bind(void *opaque,FieldSystem *field,int preview) {
    (void)preview;
    QueryScene *scene=opaque;scene->manager.fieldSystem=field;
    field->mapObjectManager=&scene->manager;field->playerAvatar=&scene->avatar;field->native_scene_resources=scene->resources;
}
static int query_apply(void *scene,unsigned id,const uint8_t *bytes,size_t length) { (void)scene;(void)id;(void)bytes;(void)length;return 0; }
static int query_tick(void *scene) { (void)scene;return 1; }
static unsigned query_active(const void *scene) { (void)scene;return 0; }
static int query_face(void *scene) { (void)scene;return 0; }
static int query_command(void *scene,ScriptContext *ctx,unsigned op,BOOL *yielded) { (void)scene;return hg_script_scene_command(ctx,op,yielded); }
#define HALF(n) ((n)&255),(((n)>>8)&255)
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
    /* Execute unchanged scene query handlers against real native avatar,
     * object and original variable records. The source object coordinates
     * deliberately differ from the player. */
    HGStoryState *query_story=hg_story_new();CHECK(query_story);hg_story_bind_field(query_story,&field);
    PlayerAvatar query_avatar={0};query_avatar.mapObject=objects;field.playerAvatar=&query_avatar;field.mapObjectManager=&manager;
    set_location(objects,688,400,0,3);
    const uint8_t query_coords[]={0x00,0x40,0x01,0x40};
    ScriptContext query={0};query.fieldSystem=&field;query.script_ptr=query_coords;BOOL yielded=TRUE;
    CHECK(hg_script_scene_command(&query,105,&yielded)&&!yielded&&query.script_ptr==query_coords+4);
    uint16_t value;CHECK(hg_story_var_get(query_story,0x4000,&value)&&value==688);
    CHECK(hg_story_var_get(query_story,0x4001,&value)&&value==400);
    query.script_ptr=query_coords;CHECK(hg_script_scene_command(&query,386,&yielded)&&!yielded);
    CHECK(hg_story_var_get(query_story,0x4000,&value)&&value==3);
    const uint8_t person_coords[]={31,0,0,0x40,1,0x40};query.script_ptr=person_coords;
    CHECK(hg_script_scene_command(&query,106,&yielded)&&!yielded);
    CHECK(hg_story_var_get(query_story,0x4000,&value)&&value==683);
    CHECK(hg_story_var_get(query_story,0x4001,&value)&&value==399);
    const uint8_t partner_coords[]={253,0,0,0x40,1,0x40};query.script_ptr=partner_coords;
    CHECK(hg_script_scene_command(&query,106,&yielded));
    CHECK(hg_story_var_get(query_story,0x4000,&value)&&value==255);
    CHECK(hg_story_var_get(query_story,0x4001,&value)&&value==255);
    const uint8_t missing_coords[]={32,0,0,0x40,1,0x40};query.script_ptr=missing_coords;
    CHECK(!hg_script_scene_command(&query,106,&yielded)&&query.script_ptr==missing_coords);
    CHECK(!hg_script_scene_command(&query,1023,&yielded));
    LocalMapObject placement_before=*object;MapObject_ClearFlagsBits(object,MAPOBJECTFLAG_IGNORE_HEIGHTS);
    field.native_scene_resources=hg_field_resources_new(60);CHECK(field.native_scene_resources);
    const uint8_t place[]={31,0,0xb0,2,0,0,0x90,1,1,0};query.script_ptr=place;
    CHECK(hg_script_scene_command(&query,339,&yielded)&&!yielded&&query.script_ptr==place+10);
    CHECK(MapObject_GetXCoord(object)==688&&MapObject_GetZCoord(object)==400&&MapObject_GetFacingDirection(object)==1);
    CHECK(MapObject_GetPreviousXCoord(object)==688&&MapObject_GetPreviousZCoord(object)==400);
    VecFx32 placed;MapObject_CopyPositionVector(object,&placed);int32_t expected_height;
    CHECK(hg_field_resources_height(field.native_scene_resources,688,400,0,0,&expected_height)==1);
    CHECK(placed.x==688*65536+32768&&placed.z==400*65536+32768&&placed.y==expected_height);
    CHECK(MapObject_GetMovementCommand(object)==255);
    LocalMapObject unchanged_placement=*object;
    uint8_t bad_place[sizeof(place)];memcpy(bad_place,place,sizeof(place));bad_place[8]=4;query.script_ptr=bad_place;
    CHECK(!hg_script_scene_command(&query,339,&yielded)&&query.script_ptr==bad_place&&!memcmp(object,&unchanged_placement,sizeof(*object)));
    bad_place[8]=1;bad_place[2]=255;bad_place[3]=15;
    CHECK(!hg_script_scene_command(&query,339,&yielded)&&!memcmp(object,&unchanged_placement,sizeof(*object)));
    QueryScene source={.manager=manager,.avatar=query_avatar,.resources=field.native_scene_resources};
    memcpy(source.objects,objects,sizeof(source.objects));source.objects[1]=placement_before;
    QueryScene source_before=source;
    HGScriptSceneServices services={.source=&source,.clone=query_clone,.destroy=free,.bind=query_bind,.apply=query_apply,
        .tick=query_tick,.active=query_active,.command=query_command,.face_player=query_face};
    uint8_t program[]={HALF(105),HALF(0x4000),HALF(0x4001),HALF(106),HALF(31),HALF(0x4002),HALF(0x4003),
        HALF(339),HALF(31),HALF(688),HALF(0),HALF(400),HALF(1),HALF(386),HALF(0x4004),HALF(2)};
    char query_error[160]={0};unsigned long long query_revision=hg_story_revision(query_story);
    HGFieldScript *vm=hg_field_script_scene_bytes(query_story,program,sizeof(program),542,&services,query_error,sizeof(query_error));
    CHECK(vm&&!query_error[0]&&!memcmp(&source,&source_before,sizeof(source))&&hg_story_revision(query_story)==query_revision);
    int query_status;do{query_status=hg_field_script_tick(vm);}while(query_status==HG_SCRIPT_RUNNING);
    CHECK(query_status==HG_SCRIPT_DONE&&hg_story_revision(query_story)==query_revision);
    const QueryScene *result=hg_field_script_scene(vm);CHECK(result);
    CHECK(MapObject_GetXCoord((LocalMapObject *)result->objects+1)==688&&MapObject_GetFacingDirection((LocalMapObject *)result->objects+1)==1);
    CHECK(hg_field_script_commit(vm)&&!hg_field_script_commit(vm));
    CHECK(hg_story_var_get(query_story,0x4002,&value)&&value==683);
    CHECK(hg_story_var_get(query_story,0x4003,&value)&&value==399);
    CHECK(hg_story_var_get(query_story,0x4004,&value)&&value==3);
    CHECK(!memcmp(&source,&source_before,sizeof(source)));hg_field_script_free(vm);
    /* Late failure after original placement discards every preview change. */
    program[sizeof(program)-2]=0xff;program[sizeof(program)-1]=0x03;
    query_revision=hg_story_revision(query_story);
    CHECK(!hg_field_script_scene_bytes(query_story,program,sizeof(program),542,&services,query_error,sizeof(query_error)));
    CHECK(strstr(query_error,"Unsupported opcode")&&hg_story_revision(query_story)==query_revision&&!memcmp(&source,&source_before,sizeof(source)));
    /* Original starter count branches execute against real flag records and
     * independent original prop managers. The initial party is empty. */
    HGFieldProps *balls=hg_field_props_new(61);CHECK(balls&&hg_field_props_prepare(balls,141,3));
    CHECK(!hg_field_props_prepare(balls,550,1)&&!hg_field_props_prepare(balls,141,33));
    field.mapPropManager=hg_field_props_manager(balls);query.script_ptr=program;
    CHECK(hg_script_scene_command(&query,621,&yielded)&&!yielded&&hg_field_props_count(balls)==3);
    const VecFx32 ball_positions[]={{131*4096,0,65*4096},{141*4096,0,65*4096},{136*4096,0,72*4096}};
    for(unsigned i=0;i<3;i++) {
        MapProp *ball=MapPropManager_GetMapPropByIndex(field.mapPropManager,(u8)i);VecFx32 position;
        CHECK(MapProp_IsActive(ball)&&MapProp_GetBuildModel(ball)==141);MapProp_GetTranslation(&position,ball);
        CHECK(!memcmp(&position,ball_positions+i,sizeof(position)));
    }
    HGFieldProps *balls_copy=hg_field_props_clone(balls);CHECK(balls_copy&&hg_field_props_count(balls_copy)==3);
    HGFieldView *ball_view=hg_field_props_view(balls_copy);CHECK(ball_view&&hg_field_view_triangles(ball_view)>0);
    hg_field_props_free(balls);hg_field_props_free(balls_copy);hg_field_view_free(ball_view);
    /* Count-only party fixture: no Pokemon is constructed or claimed usable. */
    Party *count_fixture=hg_story_save_party(field.saveData);count_fixture->core.curCount=1;
    balls=hg_field_props_new(61);CHECK(balls&&hg_field_props_prepare(balls,141,3));field.mapPropManager=hg_field_props_manager(balls);
    CHECK(hg_script_scene_command(&query,621,&yielded)&&hg_field_props_count(balls)==2);hg_field_props_free(balls);count_fixture->core.curCount=0;
    CHECK(hg_story_flag_set(query_story,FLAG_MET_PASSERBY_BOY,1));
    balls=hg_field_props_new(61);CHECK(balls&&hg_field_props_prepare(balls,141,3));field.mapPropManager=hg_field_props_manager(balls);
    CHECK(hg_script_scene_command(&query,621,&yielded)&&hg_field_props_count(balls)==1);hg_field_props_free(balls);
    CHECK(hg_story_flag_set(query_story,FLAG_GOT_TM51_FROM_FALKNER,1));
    balls=hg_field_props_new(61);CHECK(balls&&hg_field_props_prepare(balls,141,3));field.mapPropManager=hg_field_props_manager(balls);
    CHECK(hg_script_scene_command(&query,621,&yielded)&&!hg_field_props_count(balls));hg_field_props_free(balls);field.mapPropManager=NULL;
    hg_field_resources_free(field.native_scene_resources);field.native_scene_resources=NULL;*object=placement_before;
    hg_story_free(query_story);field.saveData=NULL;field.playerAvatar=NULL;

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
        if(maps[m]==61) {int x,z;CHECK(hg_field_player_position(player,&x,&z,NULL)&&x==4&&z==14);}
        CHECK(hg_field_player_prop_count(player)==(maps[m]==61?3:0));
        CHECK((hg_field_player_props(player)!=NULL)==(maps[m]==61));
        HGFieldBillboard billboards[65]={0};
        for(unsigned i=0;i<counts.active;i++) {
            HGFieldActor actor;CHECK(hg_field_player_actor(player,i,&actor));unsigned matches=0;
            for(unsigned e=0;e<events.object_count;e++) {
                ObjectEvent source;memcpy(&source,events.objects+32*e,32);
                if(source.id!=actor.id)continue;int hidden;CHECK(hg_story_flag_get(hg_field_player_story(player),source.eventFlag,&hidden)&&!hidden);
                unsigned sprite=source.spriteId;if(sprite>=101&&sprite<=117){uint16_t graphic;CHECK(hg_story_var_get(hg_field_player_story(player),VAR_OBJ_GFX_BASE+sprite-101,&graphic));sprite=graphic;}
                CHECK(actor.sprite==sprite&&actor.movement==source.movement&&actor.script==source.scriptId);
                /* Lab resume moves Elm from serialized (6,5) to (4,5). */
                unsigned expected_x=maps[m]==61&&source.id==0?4:source.x;
                CHECK(actor.x==expected_x&&actor.z==source.z&&actor.direction==(unsigned)source.facingDirection);
                CHECK(actor.billboard.world[0]==expected_x*16+8&&actor.billboard.world[2]==source.z*16+8);matches++;
            }
            CHECK(matches==1&&actor.billboard.sprite);billboards[i]=actor.billboard;
        }
        HGFieldActor unchanged={.id=12345};CHECK(!hg_field_player_actor(player,counts.active,&unchanged)&&unchanged.id==12345);
        HGFieldView *view=hg_field_view_new(maps[m]);CHECK(view);DSCamera camera;unsigned char with[256*192*4],without[sizeof(with)];
        double world[3];CHECK(hg_field_player_position(player,NULL,NULL,world));hg_field_view_focus(view,world,&camera,256,192);
        CHECK(hg_field_view_render(view,&camera,without,256,192));
        CHECK(hg_field_view_render_actors(view,billboards,counts.active,&camera,with,256,192));
        if(maps[m]==61) {
            double desk[3]={136,0,68};hg_field_view_focus(view,desk,&camera,256,192);camera.scale=3;
            const HGFieldView *props=hg_field_player_props(player);CHECK(props);
            billboards[counts.active]=(HGFieldBillboard){.sprite=props,.fixed=1};
            for(unsigned angle=0;angle<3;angle++) {
                camera.yaw=angle*0.6;
                CHECK(hg_field_view_render_actors(view,billboards,counts.active,&camera,without,256,192));
                CHECK(hg_field_view_render_actors(view,billboards,counts.active+1,&camera,with,256,192));
                CHECK(memcmp(with,without,sizeof(with)));
            }
        }
        if(maps[m]==63)CHECK(memcmp(with,without,sizeof(with)));
        if(!counts.active)CHECK(!memcmp(with,without,sizeof(with)));
        if(counts.active) {
            HGFieldBillboard invalid=billboards[0];invalid.world[0]=NAN;
            CHECK(!hg_field_view_render_actors(view,&invalid,1,&camera,with,256,192));
            CHECK(!hg_field_view_render_actors(view,NULL,1,&camera,with,256,192));
            CHECK(!hg_field_view_render_actors(view,billboards,66,&camera,with,256,192));
        }
        printf("Map %u: %u active, %u conditional, %u unsupported / %u events\n",maps[m],counts.active,counts.conditional,counts.unsupported,counts.events);
        active+=counts.active;conditional+=counts.conditional;unsupported+=counts.unsupported;
        hg_field_view_free(view);hg_field_player_free(player);
    }
    CHECK(active==14&&conditional==10&&unsupported==0);
    /* Re-enter through original portal admission: fresh prop owners follow
     * original persistent flags without accumulating duplicate balls. */
    HGFieldPlayer *lab=hg_field_player_new_map(61);CHECK(lab);HGStoryState *lab_story=hg_field_player_story(lab);
    const unsigned counts_by_visit[]={3,1,0,3};
    for(unsigned visit=0;visit<4;visit++) {
        CHECK(hg_field_player_prop_count(lab)==counts_by_visit[visit]);
        HGWarpDestination door;CHECK(hg_field_player_probe_transition(lab,0x80,&door)&&door.map==60);
        CHECK(hg_field_player_transition(lab,&door)&&!hg_field_player_prop_count(lab)&&!hg_field_player_props(lab));
        CHECK(hg_field_player_story(lab)==lab_story);
        if(visit==3)break;
        CHECK(hg_story_flag_set(lab_story,FLAG_MET_PASSERBY_BOY,visit==0));
        CHECK(hg_story_flag_set(lab_story,FLAG_GOT_TM51_FROM_FALKNER,visit==1));
        CHECK(hg_field_player_probe_transition(lab,0x40,&door)&&door.map==61);
        if(visit==2) {
            /* Scene 8 requires the currently hidden friend. A failed incoming
             * resume leaves current map, population, coordinates and state. */
            CHECK(hg_story_var_set(lab_story,VAR_SCENE_ELMS_LAB,8));
            unsigned long long before=hg_story_revision(lab_story);HGFieldActor actor_before,actor_after;
            CHECK(hg_field_player_actor(lab,0,&actor_before));int before_x,before_z,after_x,after_z;
            CHECK(hg_field_player_position(lab,&before_x,&before_z,NULL));
            CHECK(!hg_field_player_transition(lab,&door));
            CHECK(hg_story_revision(lab_story)==before&&!hg_field_player_prop_count(lab));
            CHECK(hg_field_player_position(lab,&after_x,&after_z,NULL)&&after_x==before_x&&after_z==before_z);
            CHECK(hg_field_player_actor(lab,0,&actor_after)&&actor_after.billboard.sprite==actor_before.billboard.sprite);
            CHECK(hg_story_var_set(lab_story,VAR_SCENE_ELMS_LAB,0));
            CHECK(hg_field_player_probe_transition(lab,0x40,&door));
        }
        CHECK(hg_field_player_transition(lab,&door));
    }
    hg_field_player_free(lab);
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
    CHECK(hg_field_player_actor_counts(player).active==5&&hg_field_player_actor(player,0,&after));
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
    CHECK(hg_field_player_actor_counts(player).active==6);unsigned hook;
    CHECK(hg_story_map_script(state,60,2,&hook)&&hook!=65535);
    CHECK(hg_field_player_start_script(player,hook,-1));int status;
    do{status=hg_field_player_interaction_tick(player,message,sizeof(message));}while(status==HG_SCRIPT_RUNNING);
    CHECK(status==HG_SCRIPT_DONE&&hg_field_player_actor_counts(player).active==5);
    int hidden;CHECK(hg_story_flag_get(state,FLAG_HIDE_CAMERON,&hidden)&&hidden);
    for(unsigned i=0;i<hg_field_player_actor_counts(player).active;i++){CHECK(hg_field_player_actor(player,i,&after));CHECK(after.x!=695||after.z!=403);}
    hg_field_player_free(player);
    printf("Original event actors: %u active, %u conditional, %u unsupported; source metadata, Q12 initialization, real occupancy and shared-depth rendering passed\n",active,conditional,unsupported);
    return 0;
}
