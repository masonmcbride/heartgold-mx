#include "compat/motion.h"
#include <stdio.h>
#include "motion_guest.h"
#include "field_data.h"
#include "metatile_behavior.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static unsigned completions;
static void completed(LocalMapObject *object) { (void)object;completions++; }
int main(void) {
    LocalMapObject object={0};MapObjectManager manager={0};FieldSystem field={0};PlayerSaveData save;
    manager.objects=&object;manager.objectCount=1;manager.fieldSystem=&field;object.manager=&manager;object.movement=1;
    MapObject_SetFlagsBits(&object,MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
    CHECK(PlayerAvatar_GetMapObject(avatar)==&object);CHECK(PlayerAvatar_GetState(avatar)==0);
    MapObject_SetCurrentX(&object,688);MapObject_SetCurrentZ(&object,403);
    CHECK(PlayerAvatar_GetXCoord(avatar)==688 && PlayerAvatar_GetZCoord(avatar)==403);
    PlayerAvatar_SetFacingDirection(avatar,3);CHECK(PlayerAvatar_GetFacingDirection(avatar)==3);
    CHECK(offsetof(LocalMapObject,positionVector)==0x70);
    int direction=99;
    const unsigned keys[]={0x40,0x80,0x20,0x10};
    const int dx[]={0,0,-1,1}, dz[]={-1,1,0,0};
    for(unsigned d=0;d<4;d++) {
        CHECK(hg_motion_direction(avatar,keys[d],&direction)&&direction==(int)d);
        int x=MapObject_GetXCoord(&object),z=MapObject_GetZCoord(&object);
        CHECK(hg_motion_grid_step(&object,d));
        CHECK(MapObject_GetPreviousXCoord(&object)==x && MapObject_GetPreviousZCoord(&object)==z);
        CHECK(MapObject_GetXCoord(&object)==x+dx[d] && MapObject_GetZCoord(&object)==z+dz[d]);
    }
    CHECK(hg_motion_direction(avatar,0,&direction)&&direction==-1);
    /* Exhaust every D-pad combination, including contradictory/diagonal keys.
     * The original resolver must choose one held axis, or idle. */
    for(unsigned mask=0;mask<16;mask++) {
        CHECK(hg_motion_direction(avatar,(uint16_t)(mask<<4),&direction));
        CHECK(direction>=-1 && direction<=3);
        CHECK(mask ? direction>=0 && ((mask<<4)&keys[direction]) : direction==-1);
    }
    /* Original diagonal policy preserves the last axis when both inputs are
     * unchanged, and switches to the newly pressed axis otherwise. */
    MapObject_SetNextFacingDirection(&object,3);
    PlayerAvatar_SetUnk28Unk2C(avatar,3,0);
    CHECK(hg_motion_direction(avatar,0x50,&direction)&&direction==3);
    PlayerAvatar_SetUnk28Unk2C(avatar,3,-1);
    CHECK(hg_motion_direction(avatar,0x50,&direction)&&direction==0);
    PlayerAvatar_SetUnk28Unk2C(avatar,-1,0);
    CHECK(hg_motion_direction(avatar,0x50,&direction)&&direction==3);
    CHECK(!hg_motion_direction(NULL,0,&direction));
    CHECK(!hg_motion_grid_step(NULL,0));
    CHECK(!hg_motion_grid_step(&object,4));
    CHECK(PlayerAvatar_GetXCoord(avatar)==688 && PlayerAvatar_GetZCoord(avatar)==403);
    LocalMapObject actors[3]={0};MapObjectManager crowd={0};
    crowd.objects=actors;crowd.objectCount=3;
    for(unsigned i=0;i<3;i++) {
        actors[i].manager=&crowd;
        MapObject_SetFlagsBits(&actors[i],MAPOBJECTFLAG_ACTIVE);
        MapObject_SetCurrentX(&actors[i],10+(int)i);
        MapObject_SetCurrentZ(&actors[i],20);
        MapObject_SetPreviousX(&actors[i],10+(int)i);
        MapObject_SetPreviousZ(&actors[i],19);
    }
    for(unsigned owner=0;owner<3;owner++)for(int y=-3;y<=3;y++)
        for(int z=18;z<=21;z++)for(int x=9;x<=13;x++) {
            int occupied=-1;
            CHECK(hg_motion_occupied(&actors[owner],x,y,z,&occupied));
            int expected=x>=10 && x<=12 && x!=10+(int)owner && (z==19||z==20) && y>-2 && y<2;
            CHECK(occupied==expected);
        }
    int occupied=-1;
    MapObject_ClearFlagsBits(&actors[1],MAPOBJECTFLAG_ACTIVE);
    CHECK(hg_motion_occupied(&actors[0],11,0,20,&occupied)&&!occupied);
    MapObject_SetFlagsBits(&actors[1],MAPOBJECTFLAG_ACTIVE|(1<<18));
    CHECK(hg_motion_occupied(&actors[0],11,0,20,&occupied)&&!occupied);
    crowd.objectCount=0;occupied=77;
    CHECK(!hg_motion_occupied(&actors[0],11,0,20,&occupied)&&occupied==77);
    HGFieldResources *resources=hg_field_resources_new(60);CHECK(resources);
    unsigned walls=0,open=0,barriers=0,player_queries=0,water_edges=0,height_cases=0,committed=0,denied=0;
    LocalMapObject walk_sample={0};unsigned sample_direction=0;
    MapObject_SetXRange(&object,-1);MapObject_SetYRange(&object,-1);
    BOOL (*leaving[])(u8)={sub_0205B8F4,sub_0205B918,sub_0205B93C,sub_0205B960};
    BOOL (*entering[])(u8)={sub_0205B918,sub_0205B8F4,sub_0205B960,sub_0205B93C};
    for(int z=0;z<32;z++)for(int x=0;x<32;x++) {
        int worldx=21*32+x,worldz=12*32+z;
        HGFieldTile decoded;HGMotionTile queried;
        CHECK(hg_field_resources_tile(resources,worldx,worldz,&decoded));
        CHECK(hg_motion_tile(resources,worldx,worldz,&queried));
        CHECK(queried.behavior==(decoded.attribute&255));
        CHECK(queried.wall==((decoded.attribute>>15)&1));
        if(queried.wall)walls++;else open++;
        MapObject_SetCurrentX(&object,worldx);MapObject_SetCurrentZ(&object,worldz);
        for(unsigned d=0;d<4;d++) {
            if(x+dx[d]<0||x+dx[d]>=32||z+dz[d]<0||z+dz[d]>=32)continue;
            HGMotionTile target;int blocked=-1;LocalMapObject before=object;
            CHECK(hg_motion_tile(resources,worldx+dx[d],worldz+dz[d],&target));
            CHECK(hg_motion_terrain_barrier(resources,&object,d,&blocked));
            CHECK(!memcmp(&object,&before,sizeof(object)));
            int expected=target.wall || target.behavior==GetMetatileBehavior_None() ||
                         leaving[d](queried.behavior) || entering[d](target.behavior);
            CHECK(blocked==expected);barriers+=blocked;
            int32_t height;CHECK(hg_field_resources_height(resources,worldx,worldz,0,0,&height)==1);
            VecFx32 position={worldx*65536+32768,height,worldz*65536+32768};
            MapObject_SetPositionVector(&object,&position);
            MapObject_SetCurrentY(&object,(height>>3)/4096);
            LocalMapObject player_before=object;unsigned mask=99;
            CHECK(hg_motion_player_collision(resources,avatar,d,&mask));
            CHECK(!memcmp(&object,&player_before,sizeof(object)));
            int32_t target_height;
            CHECK(hg_field_resources_height(resources,worldx+dx[d],worldz+dz[d],0,height,&target_height)==1);
            int high_step=llabs((long long)target_height-height)>=20*4096;
            CHECK(!!(mask&1)==!!(blocked||high_step));
            CHECK(!(mask&2)); /* The single active object must exclude itself. */
            unsigned original_mask=mask;
            BOOL water=MetatileBehavior_IsSurfableWater(target.behavior);
            if(water) {
                CHECK(mask&0x20);
                PlayerAvatar_SetState(avatar,1);unsigned bike=99;
                CHECK(hg_motion_player_collision(resources,avatar,d,&bike));
                CHECK((bike&1) && (bike&0x20));
                PlayerAvatar_SetState(avatar,0);water_edges++;
            }
            if(!blocked && !water && !height_cases) {
                const int deltas[]={-81920,-81919,81919,81920};
                for(unsigned h=0;h<4;h++) {
                    position.y=target_height+deltas[h];MapObject_SetPositionVector(&object,&position);
                    CHECK(hg_motion_player_collision(resources,avatar,d,&mask));
                    CHECK(!!(mask&1)==(llabs(deltas[h])>=81920));
                    height_cases++;
                }
                MapObject_SetPositionVector(&object,&player_before.positionVector);
            }
            LocalMapObject walk_before=object;HGMotionMove move={0};
            CHECK(hg_motion_walk_step(resources,avatar,(uint16_t)keys[d],&move));
            CHECK(move.direction==(int)d && move.collision_mask==original_mask);
            CHECK(move.moved==!original_mask);
            if(move.moved) {
                CHECK(MapObject_GetXCoord(&object)==worldx+dx[d]);
                CHECK(MapObject_GetZCoord(&object)==worldz+dz[d]);
                CHECK(MapObject_GetPreviousXCoord(&object)==(unsigned)worldx);
                CHECK(MapObject_GetPreviousZCoord(&object)==(unsigned)worldz);
                VecFx32 moved;MapObject_CopyPositionVector(&object,&moved);
                CHECK(moved.x==(worldx+dx[d])*65536+32768);
                CHECK(moved.z==(worldz+dz[d])*65536+32768);
                CHECK(moved.y==target_height);
                if(!committed){walk_sample=walk_before;sample_direction=d;}
                committed++;
            } else {CHECK(!memcmp(&object,&walk_before,sizeof(object)));denied++;}
            object=walk_before;
            /* Execute the original command table and state machine, rather
             * than testing a duplicate native eight-tick interpolation. */
            MapObject_SetMovementCommand(&object,255);sub_0205F47C(&object,completed);
            LocalMapObject timed_before=object;unsigned callbacks=completions;
            CHECK(hg_motion_walk_begin(resources,avatar,(uint16_t)keys[d],&move));
            CHECK(move.moved==!original_mask && move.collision_mask==original_mask);
            if(move.moved) {
                CHECK(object.positionVector.x==position.x && object.positionVector.z==position.z);
                HGMotionMove duplicate={.moved=77};LocalMapObject active=object;
                CHECK(!hg_motion_walk_begin(resources,avatar,(uint16_t)keys[d],&duplicate));
                CHECK(duplicate.moved==77 && !memcmp(&object,&active,sizeof(object)));
                for(int tick=1;tick<=8;tick++) {
                    int done=99;CHECK(hg_motion_walk_tick(resources,&object,&done));
                    CHECK(done==(tick==8));
                    CHECK(object.positionVector.x==position.x+dx[d]*tick*8192);
                    CHECK(object.positionVector.z==position.z+dz[d]*tick*8192);
                    int32_t expected_height;
                    CHECK(hg_field_resources_height_at(resources,object.positionVector.x,object.positionVector.z,0,
                          object.positionVector.y,&expected_height)==1);
                    CHECK(object.positionVector.y==expected_height);
                    CHECK(completions==callbacks+(tick==8));
                    CHECK(MapObject_GetXCoord(&object)==worldx+dx[d]);
                    CHECK(MapObject_GetZCoord(&object)==worldz+dz[d]);
                }
                CHECK(MapObject_GetMovementCommand(&object)==255 && MapObject_GetMovementStep(&object)==0);
                CHECK(MapObject_GetPreviousXCoord(&object)==MapObject_GetXCoord(&object));
                CHECK(MapObject_GetPreviousZCoord(&object)==MapObject_GetZCoord(&object));
                CHECK(object.positionVector.y==target_height);
                int intact=77;LocalMapObject ended=object;
                CHECK(!hg_motion_walk_tick(resources,&object,&intact) && intact==77);
                CHECK(!memcmp(&object,&ended,sizeof(object)));
            } else CHECK(!memcmp(&object,&timed_before,sizeof(object)) && completions==callbacks);
            object=walk_before;
            player_queries++;

        }
    }
    HGMotionTile untouched={.wall=77,.behavior=42};
    CHECK(!hg_motion_tile(resources,-1,0,&untouched)&&untouched.wall==77&&untouched.behavior==42);
    CHECK(walls && open && barriers && player_queries==3968 && water_edges && height_cases==4 && committed && denied);
    CHECK(completions==committed);
    printf("Original New Bark attribute dispatch: %u walls, %u open, %u directed barriers\n",walls,open,barriers);
    /* Occupancy must reach the full player mask and stop a connected step. */
    LocalMapObject pair[2]={walk_sample,walk_sample};MapObjectManager pair_manager={0};
    pair_manager.objects=pair;pair_manager.objectCount=2;pair_manager.fieldSystem=&field;
    pair[0].manager=&pair_manager;pair[1].manager=&pair_manager;
    PlayerSaveData pair_save;PlayerSaveData_Init(&pair_save);
    PlayerAvatar *pair_avatar=PlayerAvatar_CreateWithActiveMapObject(&pair_manager,&pair_save,0);
    CHECK(pair_avatar && PlayerAvatar_GetMapObject(pair_avatar)==&pair[0]);
    CHECK(hg_motion_grid_step(&pair[1],sample_direction));
    unsigned occupied_mask=0;
    CHECK(hg_motion_player_collision(resources,pair_avatar,sample_direction,&occupied_mask));
    CHECK(occupied_mask&2);
    LocalMapObject pair_before=pair[0];HGMotionMove stopped;
    CHECK(hg_motion_walk_step(resources,pair_avatar,(uint16_t)keys[sample_direction],&stopped));
    CHECK(!stopped.moved && (stopped.collision_mask&2));
    CHECK(!memcmp(&pair[0],&pair_before,sizeof(pair_before)));
    PlayerAvatar_FreeToHeap(pair_avatar);
    unsigned intact=77;LocalMapObject saved_object=object;
    MapObject_SetCurrentX(&object,-1);
    CHECK(!hg_motion_player_collision(resources,avatar,0,&intact)&&intact==77);
    object=saved_object;
    HGMotionMove idle;CHECK(hg_motion_walk_step(resources,avatar,0,&idle)&&idle.direction==-1&&!idle.moved);
    CHECK(!memcmp(&object,&saved_object,sizeof(object)));
    PlayerAvatar_SetState(avatar,1);HGMotionMove refused={.moved=77};
    CHECK(!hg_motion_walk_step(resources,avatar,0x10,&refused)&&refused.moved==77);
    PlayerAvatar_SetState(avatar,0);
    object=walk_sample;MapObject_SetMovementCommand(&object,255);sub_0205F47C(&object,completed);
    HGMotionMove begun;CHECK(hg_motion_walk_begin(resources,avatar,(uint16_t)keys[sample_direction],&begun)&&begun.moved);
    LocalMapObject admitted=object;object.positionVector.x=-65536;
    LocalMapObject bad_height=object;int failed_tick=77;unsigned before_failure=completions;
    CHECK(!hg_motion_walk_tick(resources,&object,&failed_tick)&&failed_tick==77);
    CHECK(!memcmp(&object,&bad_height,sizeof(object)) && completions==before_failure);
    object=admitted;
    sub_0205F47C(&object,NULL);LocalMapObject no_owner=object;int complete=77;
    CHECK(!hg_motion_walk_tick(resources,&object,&complete)&&complete==77);
    CHECK(!memcmp(&object,&no_owner,sizeof(object)));
    int32_t no_height=77;
    CHECK(hg_field_resources_height_at(resources,-1,0,0,0,&no_height)==-1 && no_height==77);
    CHECK(hg_field_resources_height(resources,32768,0,0,0,&no_height)==-1 && no_height==77);
    printf("Original player collision: %u edges, %u committed / %u denied; %u bike-water checks; height threshold checked\n",player_queries,committed,denied,water_edges);
    printf("Original normal-walk commands: %u complete eight-tick walks; every Q12 position, terrain height and completion callback checked\n",completions);
    hg_field_resources_free(resources);
    PlayerAvatar_FreeToHeap(avatar);puts("Original avatar/object state executed");return 0;
}
