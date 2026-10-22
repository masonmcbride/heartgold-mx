#pragma once
#include "compat/motion.h"
#include "metatile_behavior.h"
#include "field_player.h"
#include "field_app.h"
#include "field_data.h"
#include "map_matrix.h"
#include "motion_guest.h"
#include "compat/field_control.h"
/* Story/travel scenarios use the real Run command; the dedicated field-battle
 * scenario verifies fighting and EXP. No healing or outcome flags are injected. */
static void hg_test_battle_input(HGFieldApp *app){
    int state=hg_field_app_battle_state(app);HGBattleMonView mon;
    if(state==HG_BATTLE_COMMAND&&hg_field_app_battle_mon(app,0,&mon))CHECK(hg_field_app_battle_choose(app,hg_field_app_battle_trainer(app)?0:hg_field_app_battle_command_count(app)-1));
    else if(state==HG_BATTLE_MOVES){for(unsigned i=0;i<4;i++)if(hg_field_app_battle_choose(app,i))return;CHECK(0);}
    else hg_field_app_interact(app);
}
/* Drain real automatic scripts through application ticks and acknowledgments.
 * Two idle frames also exercise the next header check after completion. */
static unsigned hg_test_settle_observe(HGFieldApp *app,unsigned choice,void (*observe)(HGFieldApp *,void *),void *context) {
    unsigned idle=0,pages=0;hg_field_app_clear_input(app);
    for(unsigned ticks=0;ticks<8192;ticks++) {
        hg_field_app_step(app);
        if(strstr(hg_field_app_dialogue(app),"isn’t available yet")||strstr(hg_field_app_dialogue(app),"Battle stopped"))fprintf(stderr,"Automatic event failed: %s\n",hg_field_app_error(app));
        CHECK(!strstr(hg_field_app_dialogue(app),"isn’t available yet"));CHECK(!strstr(hg_field_app_dialogue(app),"Battle stopped"));
        if(hg_field_app_battle_active(app)){hg_test_battle_input(app);idle=0;continue;}
        if(hg_field_app_dialogue(app)[0]){if(observe)observe(app,context);pages++;hg_field_app_interact(app);}
        else if(hg_field_app_menu_pending(app)) {
            CHECK(hg_field_app_menu_choice_supported(app,choice));
            if(!choice)hg_field_app_input(app,0x40);
            hg_field_app_interact(app);
        }
        if(!hg_field_app_interaction_busy(app)&&!hg_field_app_dialogue(app)[0]){if(++idle==2)return pages;}else idle=0;
    }
    CHECK(0);return 0;
}
static unsigned hg_test_settle_choice(HGFieldApp *app,unsigned choice) { return hg_test_settle_observe(app,choice,NULL,NULL); }
static unsigned hg_test_settle(HGFieldApp *app) { return hg_test_settle_choice(app,1); }
/* Staged door/stair travel fades out, swaps the scene, then fades back in. A
 * single input step now only begins that animation; advance application ticks
 * until the destination map is installed and the fade finishes (or the bounded
 * budget expires). Left mid-fade, the next input would be swallowed. */
static unsigned hg_test_travel(HGFieldApp *app,unsigned expect,unsigned budget) {
    unsigned ticks=0;
    for(;ticks<budget&&(hg_field_app_map(app)!=expect||hg_field_app_transition_active(app));ticks++)hg_field_app_step(app);
    return ticks;
}
/* Reusable scenario routing: discover with original collision/targeting, then
 * play every step through actual application input. Only the current 32x32
 * map header footprint is searched, including its connected chunks. This is test automation, not runtime pathfinding. */
static void hg_test_route_impl(HGFieldApp *app,int goalx,int goalz,int target_script,unsigned replans) {
    CHECK(replans<8);
    unsigned map=hg_field_app_map(app);
    int currentx,currentz;CHECK(hg_field_app_player(app,&currentx,&currentz));
    HGFieldLocation current={.map=(int)map,.x=currentx,.z=currentz,.direction=0};
    HGFieldPlayer *reference=hg_field_app_is_exploring(app)?hg_field_player_new_explore(map):hg_field_player_resume(hg_field_app_story(app),&current,hg_field_app_running_shoes(app));CHECK(reference);
    LocalMapObject objects[64]={0};FieldSystem field={0};MapObjectManager manager={.objects=objects,.objectCount=1,.fieldSystem=&field};
    objects[0].manager=&manager;objects[0].movement=1;MapObject_SetFlagsBits(objects,MAPOBJECTFLAG_ACTIVE);
    MapObject_SetXRange(objects,-1);MapObject_SetYRange(objects,-1);PlayerSaveData save;PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);field.playerAvatar=avatar;
    unsigned target=0;
    for(unsigned i=0;i<hg_field_player_actor_counts(reference).active;i++) {
        HGFieldActor actor;CHECK(hg_field_player_actor(reference,i,&actor));LocalMapObject *object=objects+manager.objectCount++;
        object->manager=&manager;MapObject_SetFlagsBits(object,MAPOBJECTFLAG_ACTIVE);MapObject_SetID(object,actor.id);
        int32_t height=(int32_t)(actor.billboard.world[1]*4096);int y=(height>>3)/4096;
        MapObject_SetCurrentX(object,actor.x);MapObject_SetPreviousX(object,actor.x);MapObject_SetCurrentY(object,y);MapObject_SetPreviousY(object,y);
        MapObject_SetCurrentZ(object,actor.z);MapObject_SetPreviousZ(object,actor.z);
        VecFx32 position={actor.x*65536+32768,height,actor.z*65536+32768};MapObject_SetPositionVector(object,&position);
        if((int)actor.script==target_script){CHECK(!target);target=manager.objectCount-1;}
    }
    if(target_script>=0&&target_script<2000)CHECK(target);
    HGFieldResources *terrain=hg_field_resources_new(map);CHECK(terrain);
    hg_story_bind_field(hg_field_app_story(app),&field);field.native_scene_resources=terrain;
    HGTransition *transitions=hg_transition_new(map,avatar);CHECK(transitions);
    int startx,startz;CHECK(hg_field_app_player(app,&startx,&startz));
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);
    int basex=32767,basez=32767,endx=0,endz=0;
    for(unsigned z=0;z<MapMatrix_GetHeight(matrix);z++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,x,z)==map) {
        if((int)x*32<basex)basex=(int)x*32;if((int)z*32<basez)basez=(int)z*32;
        if((int)(x+1)*32>endx)endx=(int)(x+1)*32;if((int)(z+1)*32>endz)endz=(int)(z+1)*32;
    }
    MapMatrix_Free(matrix);int width=endx-basex,height_tiles=endz-basez,cells=width*height_tiles;
    CHECK(width>0&&height_tiles>0&&cells<=32768&&startx>=basex&&startx<endx&&startz>=basez&&startz<endz);
    int *previous=malloc((size_t)cells*sizeof(int)),*direction=malloc((size_t)cells*sizeof(int)),*queue=malloc((size_t)cells*sizeof(int));
    unsigned *route=malloc((size_t)cells*sizeof(unsigned));CHECK(previous&&direction&&queue&&route);
    for(int i=0;i<cells;i++)previous[i]=-1;
    int start=(startz-basez)*width+startx-basex,goal=-1,face=-1,read=0,write=1;queue[0]=start;previous[start]=start;
    const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};const unsigned keys[]={0x40,0x80,0x20,0x10};
    while(read<write&&goal<0) {
        int at=queue[read++],x=basex+at%width,z=basez+at/width;int32_t height;
        CHECK(hg_field_resources_height(terrain,x,z,0,0,&height)==1);
        MapObject_SetCurrentX(objects,x);MapObject_SetPreviousX(objects,x);MapObject_SetCurrentZ(objects,z);MapObject_SetPreviousZ(objects,z);
        MapObject_SetCurrentY(objects,(height>>3)/4096);MapObject_SetPreviousY(objects,(height>>3)/4096);
        VecFx32 position={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(objects,&position);
        if(target_script==-1&&x==goalx&&z==goalz){goal=at;break;}
        for(unsigned d=0;d<4&&goal<0;d++) {
            MapObject_SetFacingDirectionDirect(objects,d);MapObject_SetNextFacingDirection(objects,d);
            if(target_script<=-2) {
                HGWarpDestination destination;
                if(hg_transition_probe(transitions,terrain,d,&destination)&&destination.map==-target_script-2){goal=at;face=(int)d;break;}
                HGFieldTile tile;unsigned collision;
                if(hg_field_resources_tile(terrain,x+dx[d],z+dz[d],&tile)&&tile.map_header==(unsigned)(-target_script-2)&&
                   hg_motion_player_collision(terrain,avatar,d,&collision)&&!collision){goal=at;face=(int)d;break;}
            }
            if(target_script>=0) {
                LocalMapObject *facing;CHECK(hg_motion_facing_object(terrain,avatar,&facing));
                if(target&&facing==objects+target){goal=at;face=(int)d;break;}
                if(target_script>=2000&&!target&&!facing) {
                    unsigned script;
                    if(hg_field_metatile_script(&field,map,&script)&&
                       (script==(unsigned)target_script||(target_script==65534&&script>=2500&&script<=2507))){goal=at;face=(int)d;break;}
                }
            }
            int nx=x+dx[d],nz=z+dz[d];if(nx<basex||nx>=endx||nz<basez||nz>=endz)continue;
            int next=(nz-basez)*width+nx-basex;if(previous[next]>=0)continue;unsigned mask;HGWarpDestination warp;
            HGFieldTile next_tile;
            if(!hg_field_resources_tile(terrain,nx,nz,&next_tile)||next_tile.map_header!=map||hg_field_resources_height(terrain,nx,nz,0,0,&height)!=1||hg_transition_probe(transitions,terrain,d,&warp)||
               !hg_motion_player_collision(terrain,avatar,d,&mask)||mask)continue;
            previous[next]=at;direction[next]=(int)d;queue[write++]=next;
        }
    }
    if(goal<0)fprintf(stderr,"No route: map %u, goal %d,%d, target script %d\n",map,goalx,goalz,target_script);CHECK(goal>=0);
    unsigned length=0;for(int at=goal;at!=start;at=previous[at])route[length++]=(unsigned)direction[at];
    printf("Map %u scenario route: ",map);for(unsigned i=length;i;i--)putchar("wsad"[route[i-1]]);if(face>=0)printf("; face %c","wsad"[face]);putchar('\n');
    while(length) {
        hg_field_app_input(app,(uint16_t)keys[route[--length]]);hg_field_app_step(app);hg_field_app_clear_input(app);
        for(unsigned tick=1;tick<8;tick++)hg_field_app_step(app);CHECK(hg_field_app_map(app)==map);
        if(hg_field_app_interaction_busy(app)) {
            int battle_interruption=hg_field_app_battle_active(app);int battle_x,battle_z;hg_field_app_player(app,&battle_x,&battle_z);
            hg_test_settle(app);
            int after_battle_x,after_battle_z;if(battle_interruption&&hg_field_app_map(app)==map&&hg_field_app_player(app,&after_battle_x,&after_battle_z)&&after_battle_x==battle_x&&after_battle_z==battle_z)continue;
            hg_transition_free(transitions);PlayerAvatar_FreeToHeap(avatar);hg_field_resources_free(terrain);hg_field_player_free(reference);
            free(previous);free(direction);free(queue);free(route);
            hg_test_route_impl(app,goalx,goalz,target_script,replans+1);return;
        }
    }
    int x,z;CHECK(hg_field_app_player(app,&x,&z)&&x==basex+goal%width&&z==basez+goal/width);
    if(face>=0) {
        hg_field_app_input(app,(uint16_t)keys[face]);hg_field_app_step(app);hg_field_app_clear_input(app);
        int afterx,afterz;
        if(target_script<=-2) {
            hg_test_travel(app,(unsigned)(-target_script-2),32);
            if(hg_field_app_map(app)!=(unsigned)(-target_script-2))fprintf(stderr,"Portal/header crossing failed: %s\n",hg_field_app_error(app));
            CHECK(hg_field_app_map(app)==(unsigned)(-target_script-2));
        }
        else CHECK(hg_field_app_player(app,&afterx,&afterz)&&x==afterx&&z==afterz);
    }
    free(previous);free(direction);free(queue);free(route);
    hg_transition_free(transitions);PlayerAvatar_FreeToHeap(avatar);hg_field_resources_free(terrain);hg_field_player_free(reference);
}
static void hg_test_walk_to(HGFieldApp *app,int x,int z) { hg_test_route_impl(app,x,z,-1,0); }
static void hg_test_portal(HGFieldApp *app,unsigned map) { hg_test_route_impl(app,0,0,-(int)map-2,0); }
static void hg_test_approach(HGFieldApp *app,unsigned script) { hg_test_route_impl(app,0,0,(int)script,0); }

void hg_test_prepare_defeat(HGFieldApp *);
