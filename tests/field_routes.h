#pragma once
#include "compat/motion.h"
#include "field_player.h"
#include "field_app.h"
#include "field_data.h"
#include "motion_guest.h"
/* Reusable scenario routing: discover with original collision/targeting, then
 * play every step through actual application input. Only the current 32x32
 * static chunk is searched. This is test automation, not runtime pathfinding. */
static void hg_test_route(HGFieldApp *app,int goalx,int goalz,int target_script) {
    unsigned map=hg_field_app_map(app);HGFieldPlayer *reference=hg_field_player_new_map(map);CHECK(reference);
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
    if(target_script>=0)CHECK(target);
    HGFieldResources *terrain=hg_field_resources_new(map);CHECK(terrain);
    HGTransition *transitions=hg_transition_new(map,avatar);CHECK(transitions);
    int startx,startz;CHECK(hg_field_app_player(app,&startx,&startz));int basex=(startx/32)*32,basez=(startz/32)*32;
    int previous[1024],direction[1024],queue[1024];for(unsigned i=0;i<1024;i++)previous[i]=-1;
    int start=(startz-basez)*32+startx-basex,goal=-1,face=-1,read=0,write=1;queue[0]=start;previous[start]=start;
    const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};const unsigned keys[]={0x40,0x80,0x20,0x10};
    while(read<write&&goal<0) {
        int at=queue[read++],x=basex+at%32,z=basez+at/32;int32_t height;
        CHECK(hg_field_resources_height(terrain,x,z,0,0,&height)==1);
        MapObject_SetCurrentX(objects,x);MapObject_SetPreviousX(objects,x);MapObject_SetCurrentZ(objects,z);MapObject_SetPreviousZ(objects,z);
        MapObject_SetCurrentY(objects,(height>>3)/4096);MapObject_SetPreviousY(objects,(height>>3)/4096);
        VecFx32 position={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(objects,&position);
        if(target_script<0&&x==goalx&&z==goalz){goal=at;break;}
        for(unsigned d=0;d<4&&goal<0;d++) {
            MapObject_SetFacingDirectionDirect(objects,d);MapObject_SetNextFacingDirection(objects,d);
            if(target_script>=0) {
                LocalMapObject *facing;CHECK(hg_motion_facing_object(terrain,avatar,&facing));
                if(facing==objects+target){goal=at;face=(int)d;break;}
            }
            int nx=x+dx[d],nz=z+dz[d];if(nx<basex||nx>=basex+32||nz<basez||nz>=basez+32)continue;
            int next=(nz-basez)*32+nx-basex;if(previous[next]>=0)continue;unsigned mask;HGWarpDestination warp;
            if(hg_field_resources_height(terrain,nx,nz,0,0,&height)!=1||hg_transition_probe(transitions,terrain,d,&warp)||
               !hg_motion_player_collision(terrain,avatar,d,&mask)||mask)continue;
            previous[next]=at;direction[next]=(int)d;queue[write++]=next;
        }
    }
    if(goal<0)fprintf(stderr,"No route: map %u, goal %d,%d, target script %d\n",map,goalx,goalz,target_script);CHECK(goal>=0);
    unsigned route[1024],length=0;for(int at=goal;at!=start;at=previous[at])route[length++]=(unsigned)direction[at];
    printf("Map %u scenario route: ",map);for(unsigned i=length;i;i--)putchar("wsad"[route[i-1]]);if(face>=0)printf("; face %c","wsad"[face]);putchar('\n');
    while(length) {
        hg_field_app_input(app,(uint16_t)keys[route[--length]]);hg_field_app_step(app);hg_field_app_clear_input(app);
        for(unsigned tick=1;tick<8;tick++)hg_field_app_step(app);CHECK(hg_field_app_map(app)==map);
    }
    int x,z;CHECK(hg_field_app_player(app,&x,&z)&&x==basex+goal%32&&z==basez+goal/32);
    if(face>=0) {
        hg_field_app_input(app,(uint16_t)keys[face]);hg_field_app_step(app);hg_field_app_clear_input(app);
        int afterx,afterz;CHECK(hg_field_app_player(app,&afterx,&afterz)&&x==afterx&&z==afterz);
    }
    hg_transition_free(transitions);PlayerAvatar_FreeToHeap(avatar);hg_field_resources_free(terrain);hg_field_player_free(reference);
}
static void hg_test_walk_to(HGFieldApp *app,int x,int z) { hg_test_route(app,x,z,-1); }
static void hg_test_approach(HGFieldApp *app,unsigned script) { hg_test_route(app,0,0,(int)script); }
