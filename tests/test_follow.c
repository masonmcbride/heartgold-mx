#include "compat/follow.h"
#include "compat/map_objects.h"
#include "compat/stats.h"
#include "motion_guest.h"
#include "field_data.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
static void done(LocalMapObject *o) { (void)o; }
static void place(HGFieldResources *r,LocalMapObject *o,int x,int z) {
 int32_t h;CHECK(hg_field_resources_height(r,x,z,0,0,&h)==1);
 MapObject_SetCurrentX(o,x);MapObject_SetCurrentZ(o,z);MapObject_SetCurrentY(o,(h>>3)/4096);
 MapObject_SetPreviousX(o,x);MapObject_SetPreviousZ(o,z);MapObject_SetPreviousY(o,(h>>3)/4096);
 VecFx32 v={x*65536+32768,h,z*65536+32768};MapObject_SetPositionVector(o,&v);MapObject_SetMovementCommand(o,255);sub_0205F47C(o,done);
}
int main(void) {
 HGFieldResources *r=hg_field_resources_new(61);CHECK(r);
 LocalMapObject objects[2]={0};FieldSystem field={0};MapObjectManager manager={.objects=objects,.objectCount=2,.fieldSystem=&field};
 objects[0].manager=&manager;objects[0].movement=1;MapObject_SetFlagsBits(objects,MAPOBJECTFLAG_ACTIVE);MapObject_SetID(objects,obj_player);
 PlayerSaveData save;PlayerSaveData_Init(&save);PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
 field.playerAvatar=avatar;field.mapObjectManager=&manager;field.native_scene_resources=r;place(r,objects,8,7);
 CHECK(FollowMon_GetSpriteID(152,0,0)==581);CHECK(FollowMon_GetSpriteID(155,0,0)==585);CHECK(FollowMon_GetSpriteID(158,0,0)==588);
 LocalMapObject *follower=hg_follow_create(&manager,152,0,0,2,9,7,0);CHECK(follower==objects+1);hg_map_object_set_map(follower,61);place(r,follower,9,7);sub_02069DC8(follower,FALSE);
 CHECK(MapObject_GetID(follower)==obj_partner_poke&&MapObject_GetParam(follower,0)==152);
 HGMotionSession session={0};CHECK(hg_motion_follow_tick(r,avatar,follower,&session,1,55));
 MapObject_SetMovementCommand(objects,14);MapObject_SetMovementStep(objects,0);
 int player_complete=0;for(unsigned i=0;i<32;i++) {
  if(!player_complete)CHECK(hg_motion_walk_tick(r,objects,&player_complete));
  CHECK(hg_motion_follow_tick(r,avatar,follower,&session,0,55));
 }
 CHECK(MapObject_GetXCoord(objects)==7&&MapObject_GetXCoord(follower)==8);
 CHECK(MapObject_GetZCoord(follower)==7);CHECK(MapObject_GetPreviousXCoord(follower)==8);
 session.stats=calloc(1,GameStats_sizeof());CHECK(session.stats);GameStats_Init(session.stats);
 CHECK(hg_motion_follow_tick(r,avatar,follower,&session,1,48));
 HGMotionMove move;CHECK(hg_motion_locomotion_begin(r,avatar,0x20,&session,&move)&&move.moved);
 player_complete=0;for(unsigned i=0;i<32;i++) {
  if(!player_complete)CHECK(hg_motion_walk_tick(r,objects,&player_complete));
  CHECK(hg_motion_follow_tick(r,avatar,follower,&session,0,48));
 }
 CHECK(MapObject_GetXCoord(objects)==6&&MapObject_GetXCoord(follower)==7);
 const unsigned keys[]={0x40,0x80,0x20,0x10};const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
 PlayerSaveData_SetRunningShoesFlag(&save,TRUE);
 for(unsigned run=0;run<2;run++)for(unsigned direction=0;direction<4;direction++) {
  /* The eastern run starts one tile west of the blocked (7,9) tile. */
  int basex=direction==3?4:5,basez=9;
  place(r,objects,basex,basez);place(r,follower,basex,basez);memset(session.follower,0,32);
  MapObject_SetFacingDirectionDirect(objects,direction);MapObject_SetNextFacingDirection(objects,direction);
  CHECK(hg_motion_follow_tick(r,avatar,follower,&session,1,48));
  for(unsigned step=0;step<2;step++) {
   int begun=hg_motion_locomotion_begin(r,avatar,keys[direction]|(run?2:0),&session,&move);if(!begun||!move.moved)fprintf(stderr,"Follow vector run=%u dir=%u step=%u begun=%d mask=%u\n",run,direction,step,begun,move.collision_mask);CHECK(begun&&move.moved);
   int complete=0;for(unsigned tick=0;tick<24;tick++) {
    int finished=0;if(!complete)CHECK(hg_motion_player_tick(r,objects,&finished));
    CHECK(hg_motion_follow_tick(r,avatar,follower,&session,0,48));
    if(finished){complete=1;CHECK(hg_motion_clear_held(objects));}
   }
   CHECK(complete);
  }
  CHECK(MapObject_GetXCoord(objects)==basex+2*dx[direction]&&MapObject_GetZCoord(objects)==basez+2*dz[direction]);
  CHECK(MapObject_GetXCoord(follower)==basex+dx[direction]&&MapObject_GetZCoord(follower)==basez+dz[direction]);
  CHECK(MapObject_GetMovementCommand(follower)==255);
 }
 /* A real ledge must notify the normal follower controller as well as move
  * the player. Continue walking to drain the queued follower jump. */
 HGFieldResources *ledge=hg_field_resources_new(117);CHECK(ledge);
 field.native_scene_resources=ledge;
 place(ledge,objects,38,19);place(ledge,follower,38,18);
 MapObject_SetFacingDirectionDirect(objects,1);MapObject_SetNextFacingDirection(objects,1);
 memset(session.follower,0,sizeof(session.follower));
 CHECK(hg_motion_follow_tick(ledge,avatar,follower,&session,1,48));
 unsigned follower_jump=0;
 for(unsigned step=0;step<2;step++) {
  CHECK(hg_motion_locomotion_begin(ledge,avatar,0x80,&session,&move)&&move.moved);
  if(!step)CHECK(MapObject_GetMovementCommand(objects)==57);
  int complete=0;
  for(unsigned tick=0;tick<48;tick++) {
   int finished=0;if(!complete)CHECK(hg_motion_player_tick(ledge,objects,&finished));
   CHECK(hg_motion_follow_tick(ledge,avatar,follower,&session,0,48));
   int32_t arc;CHECK(hg_motion_follow_height(follower,&arc));
   if(arc>0)follower_jump=1;
   if(finished){complete=1;CHECK(hg_motion_clear_held(objects));}
  }
  CHECK(complete&&MapObject_GetZCoord(objects)==21+(int)step);
  CHECK(MapObject_GetZCoord(follower)==(step?21:19));
 }
 CHECK(follower_jump);
 int32_t landed_height;CHECK(hg_motion_follow_height(follower,&landed_height)&&!landed_height);
 hg_field_resources_free(ledge);
 free(session.stats);
 PlayerAvatar_FreeToHeap(avatar);hg_field_resources_free(r);puts("Original follower creation and movement passed");return 0;
}
