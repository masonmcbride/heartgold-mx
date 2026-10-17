#include "compat/motion.h"
#include "motion_guest.h"
#include "field_data.h"
#include "field_player.h"
struct HGFieldPlayer {
    FieldSystem field;MapObjectManager manager;LocalMapObject object;
    PlayerSaveData save;PlayerAvatar *avatar;HGFieldResources *resources;
};
void hg_field_player_free(HGFieldPlayer *p) {
    if(!p)return;if(p->avatar)PlayerAvatar_FreeToHeap(p->avatar);
    hg_field_resources_free(p->resources);free(p);
}
HGFieldPlayer *hg_field_player_new(void) {
    HGFieldPlayer *p=calloc(1,sizeof(*p));if(!p)return NULL;
    p->resources=hg_field_resources_new(60);if(!p->resources)goto fail;
    p->manager.objects=&p->object;p->manager.objectCount=1;p->manager.fieldSystem=&p->field;
    p->object.manager=&p->manager;p->object.movement=1;
    MapObject_SetFlagsBits(&p->object,MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&p->save);
    p->avatar=PlayerAvatar_CreateWithActiveMapObject(&p->manager,&p->save,0);if(!p->avatar)goto fail;
    p->field.playerAvatar=p->avatar;
    MapObject_SetXRange(&p->object,-1);MapObject_SetYRange(&p->object,-1);
    /* A native inspection-session spawn, not the original new-game script.
     * Prefer the town center; admit only a tile with an ordinary walking exit. */
    for(int radius=0;radius<32;radius++)for(int z=384;z<416;z++)for(int x=672;x<704;x++) {
        if(abs(x-688)+abs(z-398)!=radius)continue;
        HGMotionTile tile;int32_t height;
        if(!hg_motion_tile(p->resources,x,z,&tile)||tile.wall||
           hg_field_resources_height(p->resources,x,z,0,0,&height)!=1)continue;
        MapObject_SetCurrentX(&p->object,x);MapObject_SetCurrentZ(&p->object,z);
        MapObject_SetCurrentY(&p->object,(height>>3)/4096);
        MapObject_SetPreviousX(&p->object,x);MapObject_SetPreviousZ(&p->object,z);
        MapObject_SetPreviousY(&p->object,MapObject_GetYCoord(&p->object));
        VecFx32 v={x*65536+32768,height,z*65536+32768};MapObject_SetPositionVector(&p->object,&v);
        for(unsigned d=0;d<4;d++){unsigned mask;if(hg_motion_player_collision(p->resources,p->avatar,d,&mask)&&!mask)return p;}
    }
fail:hg_field_player_free(p);return NULL;
}
int hg_field_player_move(HGFieldPlayer *p,uint16_t keys) {
    if(!p)return 0;HGMotionMove move;int direction;
    if(hg_motion_direction(p->avatar,keys,&direction)&&direction>=0) {
        MapObject_SetFacingDirectionDirect(&p->object,(unsigned)direction);
        MapObject_SetNextFacingDirection(&p->object,(unsigned)direction);
    }
    if(!hg_motion_walk_step(p->resources,p->avatar,keys,&move))return 0;
    return move.moved;
}
int hg_field_player_position(const HGFieldPlayer *p,int *x,int *z,double world[3]) {
    if(!p)return 0;VecFx32 v;MapObject_CopyPositionVector((LocalMapObject *)&p->object,&v);
    if(x)*x=(int)MapObject_GetXCoord((LocalMapObject *)&p->object);
    if(z)*z=(int)MapObject_GetZCoord((LocalMapObject *)&p->object);
    if(world){world[0]=v.x/4096.0;world[1]=v.y/4096.0;world[2]=v.z/4096.0;}return 1;
}

int hg_field_player_facing(const HGFieldPlayer *p) { return p?(int)PlayerAvatar_GetFacingDirection(p->avatar):0; }
