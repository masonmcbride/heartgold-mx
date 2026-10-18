#include "compat/transitions.h"
#include "field_transition.h"
#include "field_events.h"
#include "field_app.h"
#include "field_data.h"
#include "motion_guest.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
static unsigned half(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static void completed(LocalMapObject *object){(void)object;}
typedef struct { LocalMapObject object;MapObjectManager manager;FieldSystem field;PlayerSaveData save;PlayerAvatar *avatar; } Actor;
static void init(Actor *a) {
    memset(a,0,sizeof(*a));a->manager.objects=&a->object;a->manager.objectCount=1;a->manager.fieldSystem=&a->field;
    a->object.manager=&a->manager;a->object.movement=1;MapObject_SetFlagsBits(&a->object,MAPOBJECTFLAG_ACTIVE);
    sub_0205F47C(&a->object,completed);MapObject_SetXRange(&a->object,-1);MapObject_SetYRange(&a->object,-1);
    PlayerSaveData_Init(&a->save);a->avatar=PlayerAvatar_CreateWithActiveMapObject(&a->manager,&a->save,0);CHECK(a->avatar);
}
static int place(Actor *a,HGFieldResources *resources,int x,int z,unsigned direction) {
    int32_t height;if(hg_field_resources_height(resources,x,z,0,0,&height)!=1)return 0;
    MapObject_SetCurrentX(&a->object,x);MapObject_SetCurrentZ(&a->object,z);MapObject_SetCurrentY(&a->object,(height>>3)/4096);
    MapObject_SetPreviousX(&a->object,x);MapObject_SetPreviousZ(&a->object,z);MapObject_SetPreviousY(&a->object,(height>>3)/4096);
    VecFx32 position={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(&a->object,&position);
    MapObject_SetFacingDirectionDirect(&a->object,direction);MapObject_SetNextFacingDirection(&a->object,direction);
    MapObject_SetMovementCommand(&a->object,255);MapObject_SetMovementStep(&a->object,0);return 1;
}
static const unsigned keys[]={0x40,0x80,0x20,0x10};
static const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
#include "field_routes.h"
int main(void) {
    const unsigned maps[]={60,61,62,63,64,65,66,384};unsigned portals=0,selected_maps=0;
    HGFieldLocation approaches[8][6]={0};
    for(unsigned m=0;m<sizeof(maps)/sizeof(*maps);m++) {
        Actor actor;init(&actor);PlayerSaveData *original=PlayerAvatar_GetPlayerSaveData(actor.avatar);
        HGTransition *transition=hg_transition_new(maps[m],actor.avatar);CHECK(transition);
        CHECK(PlayerAvatar_GetPlayerSaveData(actor.avatar)!=original);
        HGFieldResources *resources=hg_field_resources_new(maps[m]);CHECK(resources);HGEventView events;CHECK(hg_event_load(maps[m],&events));
        unsigned resolved=0;
        for(unsigned w=0;w<events.warp_count;w++) {
            const uint8_t *event=events.warps+12*w;int wx=(int)half(event),wz=(int)half(event+2),found=0;
            for(int oz=-1;oz<=1&&!found;oz++)for(int ox=-1;ox<=1&&!found;ox++)for(unsigned d=0;d<4&&!found;d++) {
                HGMotionTile tile;if(!hg_motion_tile(resources,wx+ox,wz+oz,&tile)||tile.wall||!place(&actor,resources,wx+ox,wz+oz,d))continue;
                HGTransitionState before,after;CHECK(hg_transition_inspect(transition,&before));HGWarpDestination destination;
                int selected=hg_transition_probe(transition,resources,d,&destination);
                CHECK(hg_transition_inspect(transition,&after)&&!memcmp(&before,&after,sizeof(before)));
                if(!selected||destination.map!=(int)half(event+4)||destination.warp!=(int)half(event+6))continue;
                HGEventView target;CHECK(hg_event_load((unsigned)destination.map,&target));CHECK((unsigned)destination.warp<target.warp_count);
                const uint8_t *anchor=target.warps+12*destination.warp;
                CHECK(destination.x==(int)half(anchor)&&destination.z==(int)half(anchor+2)&&destination.direction==(int)d);
                HGWarpDestination altered=destination;altered.x++;CHECK(!hg_transition_commit(transition,&altered));
                CHECK(hg_transition_inspect(transition,&after)&&!memcmp(&before,&after,sizeof(before)));
                MapObject_AddCurrentX(&actor.object,1);CHECK(!hg_transition_commit(transition,&destination));MapObject_AddCurrentX(&actor.object,-1);
                PlayerSaveData *player=PlayerAvatar_GetPlayerSaveData(actor.avatar);BOOL shoes=PlayerSaveData_CheckRunningShoes(player);
                PlayerSaveData_SetRunningShoesFlag(player,!shoes);CHECK(!hg_transition_commit(transition,&destination));PlayerSaveData_SetRunningShoesFlag(player,shoes);
                CHECK(hg_transition_commit(transition,&destination));CHECK(hg_transition_inspect(transition,&after));
                CHECK(after.current.map==destination.map&&after.current.x==destination.x&&after.current.z==destination.z);
                CHECK(after.previous.map==(int)maps[m]&&after.previous.x==wx+ox&&after.previous.z==wz+oz);
                CHECK(after.entrance.map==(int)maps[m]&&after.entrance.warp==(int)w&&after.entrance.x==wx&&after.entrance.z==wz);
                CHECK(!hg_transition_commit(transition,&destination));
                approaches[m][w]=(HGFieldLocation){(int)maps[m],(int)w,wx+ox,wz+oz,(int)d};
                found=1;portals++;resolved++;
                /* Continue testing the other source portals with a fresh owner. */
                hg_transition_free(transition);transition=hg_transition_new(maps[m],actor.avatar);CHECK(transition);
            }
            if(!found)fprintf(stderr,"Unresolved portal: map=%u warp=%u tile=%d,%d target=%u/%u\n",maps[m],w,wx,wz,half(event+4),half(event+6));
            CHECK(found);
        }
        CHECK(resolved==events.warp_count);selected_maps++;
        /* Admit interiors through the same original static collision and
         * running pipeline, checking every surface-backed neighboring edge. */
        if(maps[m]!=60) {
            HGMotionSession motion={.stats=calloc(1,GameStats_sizeof())};CHECK(motion.stats);GameStats_Init(motion.stats);
            PlayerSaveData_SetRunningShoesFlag(PlayerAvatar_GetPlayerSaveData(actor.avatar),TRUE);
            unsigned admitted=0,blocked=0;
            for(int z=0;z<32;z++)for(int x=0;x<32;x++) {
                HGMotionTile tile;if(!hg_motion_tile(resources,x,z,&tile)||tile.wall)continue;
                if(!place(&actor,resources,x,z,0))continue;
                for(unsigned d=0;d<4;d++) {
                    int nx=x+dx[d],nz=z+dz[d];int32_t height;
                    if(nx<0||nx>=32||nz<0||nz>=32||hg_field_resources_height(resources,nx,nz,0,0,&height)!=1)continue;
                    CHECK(place(&actor,resources,x,z,d));unsigned mask;CHECK(hg_motion_player_collision(resources,actor.avatar,d,&mask));
                    HGMotionMove move;CHECK(hg_motion_locomotion_begin(resources,actor.avatar,keys[d]|2,&motion,&move));
                    CHECK(move.moved==!mask&&move.collision_mask==mask);
                    if(!move.moved){blocked++;continue;}admitted++;
                    for(unsigned tick=1;tick<=4;tick++) {
                        int done;CHECK(hg_motion_walk_tick(resources,&actor.object,&done)&&done==(tick==4));
                        CHECK(actor.object.positionVector.x==x*65536+32768+dx[d]*(int)tick*16384);
                        CHECK(actor.object.positionVector.z==z*65536+32768+dz[d]*(int)tick*16384);
                    }
                }
            }
            CHECK(admitted&&GameStats_GetCapped(motion.stats,0)==admitted);
            printf("Interior %u: %u surface-backed edges admitted / %u denied\n",maps[m],admitted,blocked);free(motion.stats);
        }
        hg_transition_free(transition);CHECK(PlayerAvatar_GetPlayerSaveData(actor.avatar)==original);
        PlayerAvatar_FreeToHeap(actor.avatar);hg_field_resources_free(resources);
    }
    char error[256];HGFieldApp *app=hg_field_app_new(60,error,sizeof(error));CHECK(app);
    hg_test_walk_to(app,695,397);unsigned steps=hg_field_app_steps(app);const HGFieldView *town=hg_field_app_view(app);
    hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(hg_field_app_map(app)==63&&hg_field_app_view(app)!=town&&hg_field_app_steps(app)==steps);
    int x,z;CHECK(hg_field_app_player(app,&x,&z)&&x==3&&z==10);
    unsigned char pixels[256*192*4];DSCamera camera;hg_field_app_camera(app,&camera,256,192);
    CHECK(hg_field_app_render(app,&camera,pixels,256,192));
    hg_field_app_input(app,0x80);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(hg_field_app_map(app)==60&&hg_field_app_steps(app)==steps);
    CHECK(hg_field_app_player(app,&x,&z)&&x==695&&z==396);
    hg_field_app_input(app,0x82);hg_field_app_step(app);hg_field_app_input(app,0);
    for(unsigned i=0;i<3;i++)hg_field_app_step(app);
    CHECK(hg_field_app_map(app)==60&&hg_field_app_steps(app)==steps+1);
    CHECK(hg_field_app_player(app,&x,&z)&&x==695&&z==397);
    hg_field_app_free(app);
    /* Actual application stairs, through collision-checked walking and the
     * original selector, preserve the same owner and statistic across floors. */
    app=hg_field_app_new(63,error,sizeof(error));CHECK(app);
    HGFieldLocation upstairs=approaches[3][1],downstairs=approaches[4][0];
    hg_test_walk_to(app,upstairs.x,upstairs.z);steps=hg_field_app_steps(app);
    hg_field_app_input(app,keys[upstairs.direction]);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(hg_field_app_map(app)==64&&hg_field_app_steps(app)==steps);
    hg_field_app_camera(app,&camera,256,192);CHECK(hg_field_app_render(app,&camera,pixels,256,192));
    hg_test_walk_to(app,downstairs.x,downstairs.z);steps=hg_field_app_steps(app);
    hg_field_app_input(app,keys[downstairs.direction]);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(hg_field_app_map(app)==63&&hg_field_app_steps(app)==steps);hg_field_app_free(app);
    CHECK(!hg_transition_supported(67)&&!hg_transition_supported(540));
    printf("Original warp kernel: %u portals across %u maps, original anchors/local-field records, staged commit/rejection; app town-house-town round trip and retained running/statistics passed\n",portals,selected_maps);
    return 0;
}
