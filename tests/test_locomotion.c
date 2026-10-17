#include "compat/motion.h"
#include "compat/stats.h"
#include "motion_guest.h"
#include "field_data.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
static unsigned notifications;
static void completed(LocalMapObject *object) { (void)object;notifications++; }
static unsigned word(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24; }
static void place(HGFieldResources *resources,LocalMapObject *o,int x,int z) {
    int32_t height;CHECK(hg_field_resources_height(resources,x,z,0,0,&height)==1);
    MapObject_SetCurrentX(o,x);MapObject_SetCurrentZ(o,z);MapObject_SetCurrentY(o,(height>>3)/4096);
    MapObject_SetPreviousX(o,x);MapObject_SetPreviousZ(o,z);MapObject_SetPreviousY(o,(height>>3)/4096);
    VecFx32 v={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(o,&v);
    MapObject_SetMovementCommand(o,255);MapObject_SetMovementStep(o,0);
}
int main(void) {
    LocalMapObject object={0};MapObjectManager manager={0};FieldSystem field={0};PlayerSaveData save;
    manager.objects=&object;manager.objectCount=1;manager.fieldSystem=&field;object.manager=&manager;object.movement=1;
    MapObject_SetFlagsBits(&object,MAPOBJECTFLAG_ACTIVE);sub_0205F47C(&object,completed);PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
    HGFieldResources *resources=hg_field_resources_new(60);CHECK(resources);
    HGMotionSession session={.stats=calloc(1,GameStats_sizeof())};CHECK(session.stats);GameStats_Init(session.stats);
    const unsigned keys[]={0x40,0x80,0x20,0x10};const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
    unsigned walked=0,denied=0;int samples[4][2]={{0}};
    PlayerSaveData_SetRunningShoesFlag(&save,TRUE);
    for(int z=384;z<416;z++)for(int x=672;x<704;x++)for(unsigned d=0;d<4;d++) {
        if(x+dx[d]<672||x+dx[d]>=704||z+dz[d]<384||z+dz[d]>=416)continue;
        place(resources,&object,x,z);LocalMapObject before=object;unsigned mask;
        CHECK(hg_motion_player_collision(resources,avatar,d,&mask));
        unsigned count=GameStats_GetCapped(session.stats,GAME_STAT_STEPS_WALKED);
        HGMotionSession follower_before=session;unsigned callbacks=notifications;HGMotionMove move;
        CHECK(hg_motion_locomotion_begin(resources,avatar,keys[d]|2,&session,&move));
        CHECK(move.direction==(int)d && move.collision_mask==mask && move.moved==!mask);
        CHECK(PlayerAvatar_GetUnk28(avatar)==(d>=2?(int)d:-1));
        CHECK(PlayerAvatar_GetUnk2C(avatar)==(d<2?(int)d:-1));
        CHECK(PlayerAvatar_CheckFlag6(avatar)==!mask);
        CHECK(GameStats_GetCapped(session.stats,GAME_STAT_STEPS_WALKED)==count+!mask);
        CHECK(GameStats_GetCapped(session.stats,GAME_STAT_STEPS_BIKED)==0);
        if(mask) {
            CHECK(!memcmp(&object,&before,sizeof(object)));
            CHECK(!memcmp(session.follower,follower_before.follower,32));denied++;continue;
        }
        samples[d][0]=x;samples[d][1]=z;walked++;
        CHECK(MapObject_GetMovementCommand(&object)==88+d && PlayerAvatar_GetUnk8(avatar)==88+d);
        CHECK(word(session.follower+4)==88+d && word(session.follower+8)==(unsigned)x && word(session.follower+12)==(unsigned)z);
        CHECK(word(session.follower+28)==1);
        for(unsigned tick=1;tick<=4;tick++) {
            int done=77;CHECK(hg_motion_walk_tick(resources,&object,&done)&&done==(tick==4));
            CHECK(object.positionVector.x==before.positionVector.x+dx[d]*(int)tick*16384);
            CHECK(object.positionVector.z==before.positionVector.z+dz[d]*(int)tick*16384);
            CHECK(notifications==callbacks+(tick==4));
            CHECK(GameStats_GetCapped(session.stats,GAME_STAT_STEPS_WALKED)==count+1);
        }
        CHECK(MapObject_GetMovementCommand(&object)==255 && MapObject_GetMovementStep(&object)==0);
        CHECK(!MapObject_GetFlagsBitsMask(&object,0x10));
    }
    CHECK(walked==1590 && denied==2378 && notifications==walked);
    /* Original save ownership and B input must both be present to select run.
     * Follower notification state transitions also execute original code. */
    for(unsigned d=0;d<4;d++)for(unsigned shoes=0;shoes<2;shoes++)for(unsigned button=0;button<2;button++) {
        CHECK(samples[d][0]);place(resources,&object,samples[d][0],samples[d][1]);
        PlayerSaveData_SetRunningShoesFlag(&save,shoes);
        memset(session.follower,0,32);session.follower[28]=3;
        HGMotionMove move;CHECK(hg_motion_locomotion_begin(resources,avatar,keys[d]|(button?2:0),&session,&move)&&move.moved);
        unsigned command=(shoes&&button?88:12)+d,duration=shoes&&button?4:8;
        CHECK(MapObject_GetMovementCommand(&object)==command && word(session.follower+4)==command && word(session.follower+28)==2);
        for(unsigned tick=1;tick<=duration;tick++) { int done;CHECK(hg_motion_walk_tick(resources,&object,&done)&&done==(tick==duration)); }
    }
    /* Unlike the old isolated resolver tests, these use history written by
     * original input processing itself across a real sequence of commands. */
    const unsigned sequence[]={0x40,0x50,0x50,0x40,0x60,0x60,0x20,0xa0,0,0x60};
    const int directions[]={0,3,3,0,2,2,2,1,-1,0};
    for(unsigned i=0;i<sizeof(sequence)/sizeof(*sequence);i++) {
        place(resources,&object,688,398);HGMotionMove move;
        CHECK(hg_motion_locomotion_begin(resources,avatar,sequence[i],&session,&move));CHECK(move.direction==directions[i]);
        if(move.moved) { int done=0;for(unsigned t=0;t<8&&!done;t++)CHECK(hg_motion_walk_tick(resources,&object,&done));CHECK(done); }
        else if(move.direction>=0)MapObject_SetNextFacingDirection(&object,(unsigned)move.direction);
    }
    unsigned stat=GameStats_GetCapped(session.stats,0);HGMotionMove untouched={.moved=77};
    PlayerAvatar_SetState(avatar,1);CHECK(!hg_motion_locomotion_begin(resources,avatar,0x42,&session,&untouched));
    CHECK(untouched.moved==77 && GameStats_GetCapped(session.stats,0)==stat);
    free(session.stats);hg_field_resources_free(resources);PlayerAvatar_FreeToHeap(avatar);
    printf("Original locomotion branch: %u running edges / %u denied; shoes/B gating, four directions, follower notifications, one step statistic per command and stateful diagonal input passed\n",walked,denied);
    return 0;
}
