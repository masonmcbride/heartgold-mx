#include "compat/motion.h"
#include "compat/stats.h"
#include "motion_guest.h"
#include "field_data.h"
#include "field_player.h"
#include "field_events.h"
struct HGFieldPlayer {
    FieldSystem field;MapObjectManager manager;LocalMapObject object;
    PlayerSaveData save;PlayerAvatar *avatar;HGFieldResources *resources;
    LocalMapObject step_start;int walking;
    HGMotionSession session;
    HGMotionSession session_start;PlayerAvatar avatar_start;uint8_t *stats_start;
    HGTransition *transitions;unsigned map;
};
/* The host renderer owns sprite resources; there is no DS VRAM transfer to
 * perform at completion. Original command dispatch still invokes this native
 * resource notification through the original map-object callback accessor. */
static void movement_finished(LocalMapObject *object) { (void)object; }
static void place_player(HGFieldPlayer *p,int x,int z,int32_t height,unsigned direction) {
    MapObject_SetCurrentX(&p->object,x);MapObject_SetCurrentZ(&p->object,z);MapObject_SetCurrentY(&p->object,(height>>3)/4096);
    MapObject_SetPreviousX(&p->object,x);MapObject_SetPreviousZ(&p->object,z);MapObject_SetPreviousY(&p->object,(height>>3)/4096);
    VecFx32 v={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(&p->object,&v);
    MapObject_SetFacingDirectionDirect(&p->object,direction);MapObject_SetNextFacingDirection(&p->object,direction);
}
void hg_field_player_free(HGFieldPlayer *p) {
    if(!p)return;hg_transition_free(p->transitions);if(p->avatar)PlayerAvatar_FreeToHeap(p->avatar);
    hg_field_resources_free(p->resources);free(p->session.stats);free(p->stats_start);free(p);
}
HGFieldPlayer *hg_field_player_new(void) { return hg_field_player_new_map(60); }
HGFieldPlayer *hg_field_player_new_map(unsigned map) {
    if(!hg_transition_supported(map))return NULL;
    HGFieldPlayer *p=calloc(1,sizeof(*p));if(!p)return NULL;
    p->map=map;p->resources=hg_field_resources_new(map);if(!p->resources)goto fail;
    p->session.stats=calloc(1,GameStats_sizeof());p->stats_start=malloc(GameStats_sizeof());
    if(!p->session.stats||!p->stats_start)goto fail;
    GameStats_Init(p->session.stats);
    p->manager.objects=&p->object;p->manager.objectCount=1;p->manager.fieldSystem=&p->field;
    p->object.manager=&p->manager;p->object.movement=1;
    MapObject_SetFlagsBits(&p->object,MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&p->save);
    /* Temporary inspection session equipment; not the new-game story award. */
    PlayerSaveData_SetRunningShoesFlag(&p->save,TRUE);
    p->avatar=PlayerAvatar_CreateWithActiveMapObject(&p->manager,&p->save,0);if(!p->avatar)goto fail;
    p->field.playerAvatar=p->avatar;
    MapObject_SetMovementCommand(&p->object,255);sub_0205F47C(&p->object,movement_finished);
    MapObject_SetXRange(&p->object,-1);MapObject_SetYRange(&p->object,-1);
    p->transitions=hg_transition_new(map,p->avatar);if(!p->transitions)goto fail;
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
    if(!p||p->walking)return 0;HGMotionMove move;
    p->step_start=p->object;
    p->avatar_start=*p->avatar;p->session_start=p->session;
    memcpy(p->stats_start,p->session.stats,GameStats_sizeof());
    if(!hg_motion_locomotion_begin(p->resources,p->avatar,keys,&p->session,&move))return 0;
    if(!move.moved&&move.direction>=0) {
        MapObject_SetFacingDirectionDirect(&p->object,(unsigned)move.direction);
        MapObject_SetNextFacingDirection(&p->object,(unsigned)move.direction);
    }
    p->walking=move.moved;
    if(!p->walking)hg_transition_sync_player(p->transitions);
    return move.moved;
}
int hg_field_player_walking(const HGFieldPlayer *p) { return p&&p->walking; }
int hg_field_player_tick(HGFieldPlayer *p) {
    if(!p||!p->walking)return 0;int complete;
    if(!hg_motion_walk_tick(p->resources,&p->object,&complete)) {
        p->object=p->step_start;*p->avatar=p->avatar_start;p->session=p->session_start;
        memcpy(p->session.stats,p->stats_start,GameStats_sizeof());p->walking=0;return -1;
    }
    p->walking=!complete;if(complete)hg_transition_sync_player(p->transitions);return 1;
}
int hg_field_player_position(const HGFieldPlayer *p,int *x,int *z,double world[3]) {
    if(!p)return 0;VecFx32 v;MapObject_CopyPositionVector((LocalMapObject *)&p->object,&v);
    if(x)*x=(int)MapObject_GetXCoord((LocalMapObject *)&p->object);
    if(z)*z=(int)MapObject_GetZCoord((LocalMapObject *)&p->object);
    if(world){world[0]=v.x/4096.0;world[1]=v.y/4096.0;world[2]=v.z/4096.0;}return 1;
}

int hg_field_player_facing(const HGFieldPlayer *p) { return p?(int)PlayerAvatar_GetFacingDirection(p->avatar):0; }
int hg_field_player_running(const HGFieldPlayer *p) {
    if(!p||!p->walking)return 0;unsigned command=MapObject_GetMovementCommand((LocalMapObject *)&p->object);
    return command>=88&&command<=91;
}
unsigned hg_field_player_steps(const HGFieldPlayer *p) { return p?GameStats_GetCapped(p->session.stats,0):0; }

int hg_field_player_interact(HGFieldPlayer *p,char *message,size_t capacity) {
    if(!p||p->map!=60||!message||!capacity)return 0;HGEventView events;
    if(!hg_event_load(60,&events))return 0;
    uint8_t signs[2000];unsigned count=0;
    for(unsigned i=0;i<events.background_count;i++) {
        const uint8_t *event=events.backgrounds+20*i;
        if((event[2]|(unsigned)event[3]<<8)==1){if(count==100)return 0;memcpy(signs+20*count++,event,20);}
    }
    unsigned script;
    if(!hg_motion_background_event(p->avatar,signs,count,&script)||script==65535)return 0;
    return hg_sign_script(script,message,capacity);
}
int hg_field_player_probe_transition(HGFieldPlayer *p,uint16_t keys,HGWarpDestination *out) {
    if(!p||p->walking||!out)return 0;int direction;
    if(!hg_motion_direction(p->avatar,keys,&direction)||direction<0)return 0;
    /* Native inspection input turns immediately. Probe the intended facing
     * before ordinary height/collision admission: exit tiles can face beyond
     * the room's BDHC surface. The original transition selector still decides. */
    LocalMapObject saved=p->object;
    MapObject_SetFacingDirectionDirect(&p->object,(unsigned)direction);MapObject_SetNextFacingDirection(&p->object,(unsigned)direction);
    int selected=hg_transition_probe(p->transitions,p->resources,(unsigned)direction,out);
    p->object=saved;return selected;
}
int hg_field_player_transition(HGFieldPlayer *p,const HGWarpDestination *destination) {
    if(!p||p->walking||!destination||!hg_transition_supported((unsigned)destination->map)||destination->x<0||destination->z<0||destination->x>32767||destination->z>32767||destination->direction<0||destination->direction>3)return 0;
    HGFieldResources *next=hg_field_resources_new((unsigned)destination->map);if(!next)return 0;
    HGFieldTile tile;int32_t height;
    if(!hg_field_resources_tile(next,destination->x,destination->z,&tile)||tile.map_header!=(unsigned)destination->map||
       hg_field_resources_height(next,destination->x,destination->z,0,0,&height)!=1||!hg_transition_commit(p->transitions,destination)) {
        hg_field_resources_free(next);return 0;
    }
    hg_field_resources_free(p->resources);p->resources=next;p->map=(unsigned)destination->map;
    place_player(p,destination->x,destination->z,height,(unsigned)destination->direction);
    PlayerAvatar_SetUnk28Unk2C(p->avatar,-1,-1);memset(p->session.follower,0,sizeof(p->session.follower));return 1;
}
