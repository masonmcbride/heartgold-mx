#include "compat/motion.h"
#include "motion_guest.h"
#include "ds_thumb.h"
#include "metatile_behavior.h"
#include "field_data.h"
#include "field_guest.inc"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define STATIC_HEIGHT 0x02110000u
#define DISPATCH_ADDRESS 0x02008200u
#define HANDLE 0x03000000u
#define FIELD_ADDRESS 0x02008000u
#define ATTRIBUTE_ADDRESS 0x02009000u
#define MANAGER_HANDLE 0x03010000u
#define OBJECT_BASE 0x03020000u
#define LOADER_HANDLE 0x03000004u
/* Only the original normal-field dispatch offsets are marshaled. No native
 * FieldSystem or object is cast to the DS serialized layout. */
typedef struct { PlayerAvatar *avatar; LocalMapObject *object;
    MapObjectManager *manager; HGFieldResources *field; VecFx32 *vector; HGFieldTile selected; int selected_valid; int new_bark_static;
} MotionBoundary;
static int store(DSThumb *cpu,uint32_t address,uint32_t value,unsigned width) {
    if(address<0x02000000u+sizeof(hg_field_guest_image) ||
       (width!=1 && width!=2 && width!=4) || address%width)return 0;
    uint8_t *p=ds_translate(&cpu->memory,address,width);if(!p)return 0;
    for(unsigned i=0;i<width;i++)p[i]=(uint8_t)(value>>(8*i));return 1;
}
static int load_word(DSThumb *cpu,uint32_t address,uint32_t *value) {
    if(address&3)return 0;const uint8_t *p=ds_translate(&cpu->memory,address,4);if(!p)return 0;
    *value=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;return 1;
}
static int boundary(DSThumb *cpu, void *opaque) {
    MotionBoundary *b=opaque;const char *name=NULL;
    for(size_t i=0;i<sizeof(hg_field_guest_imports)/sizeof(*hg_field_guest_imports);i++)
        if(cpu->r[15]==hg_field_guest_imports[i].address) { name=hg_field_guest_imports[i].name;break; }
    if(cpu->r[15]==STATIC_HEIGHT && b->field && b->new_bark_static) {
        /* Explicit static BDHC provider, not the original dynamic-platform
         * selector. Only tile-center normal-field requests are admitted. */
        if(cpu->r[0]!=FIELD_ADDRESS || (cpu->r[2]&65535)!=32768 ||
           (cpu->r[3]&65535)!=32768 || (int32_t)cpu->r[2]<0 || (int32_t)cpu->r[3]<0)return -1;
        int x=(int32_t)cpu->r[2]/65536,z=(int32_t)cpu->r[3]/65536;int32_t height;
        uint32_t status;
        if(!load_word(cpu,cpu->r[13],&status) ||
           hg_field_resources_height(b->field,x,z,0,(int32_t)cpu->r[1],&height)!=1)return -1;
        if(status && !store(cpu,status,1,1))return -1;
        cpu->r[0]=(uint32_t)height;return 1;
    }
    if(!name)return 0;
    if(!strcmp(name,"Gymmick_CheckCollision") && b->new_bark_static && cpu->r[0]==FIELD_ADDRESS) {
        /* This admitted environment has no active gym gimmick. No other
         * environment is silently treated as having no collision provider. */
        cpu->r[0]=0;return 1;
    }
    if(b->field && !strcmp(name,"ov01_021F654C")) {
        if(cpu->r[0]!=LOADER_HANDLE)return -1;
        b->selected_valid=hg_field_resources_tile(b->field,(int32_t)cpu->r[1],(int32_t)cpu->r[2],&b->selected);
        if(b->selected_valid && !store(cpu,cpu->r[3],0,1))return -1;
        cpu->r[0]=b->selected_valid;return 1;
    }
    if(b->field && !strcmp(name,"ov01_021F65E4")) {
        if(cpu->r[0]!=LOADER_HANDLE || cpu->r[1]!=0 || !b->selected_valid)return -1;
        void *attributes=ds_translate(&cpu->memory,ATTRIBUTE_ADDRESS,2048);if(!attributes)return -1;
        memcpy(attributes,b->selected.land.attributes,2048);cpu->r[0]=ATTRIBUTE_ADDRESS;return 1;
    }
#define TILE_PREDICATE(n) if(!strcmp(name,#n)){if(cpu->r[0]>255)return -1;cpu->r[0]=n((uint8_t)cpu->r[0]);return 1;}
    TILE_PREDICATE(MetatileBehavior_IsJumpNorth)
    TILE_PREDICATE(MetatileBehavior_IsJumpSouth)
    TILE_PREDICATE(MetatileBehavior_IsJumpWest)
    TILE_PREDICATE(MetatileBehavior_IsJumpEast)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceNorth)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceSouth)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceWest)
    TILE_PREDICATE(MetatileBehavior_IsWarpEntranceEast)
    TILE_PREDICATE(MetatileBehavior_IsDoor)
    TILE_PREDICATE(MetatileBehavior_IsSurfableWater)
    TILE_PREDICATE(MetatileBehavior_IsSnow)
    TILE_PREDICATE(MetatileBehavior_IsVeryTallGrass)
    TILE_PREDICATE(MetatileBehavior_IsMud)
    TILE_PREDICATE(sub_0205BA54)
    TILE_PREDICATE(sub_0205B78C)
    TILE_PREDICATE(sub_0205B8F4)
    TILE_PREDICATE(sub_0205B918)
    TILE_PREDICATE(sub_0205B93C)
    TILE_PREDICATE(sub_0205B960)
    if(!strcmp(name,"GetMetatileBehavior_None")){cpu->r[0]=GetMetatileBehavior_None();return 1;}
    if(b->manager) {
        unsigned count=MapObjectManager_GetObjectCount(b->manager);
        LocalMapObject *objects=MapObjectManager_GetObjects2(b->manager);
        if(!strcmp(name,"MapObjectManager_GetObjects2")&&cpu->r[0]==MANAGER_HANDLE) {
            cpu->r[0]=objects==b->object?HANDLE:OBJECT_BASE;return 1;
        }
        if(!strcmp(name,"MapObjectManager_GetObjectCount")&&cpu->r[0]==MANAGER_HANDLE) {
            cpu->r[0]=count;return 1;
        }
        if(!strcmp(name,"MapObjectArray_NextObject")) {
            if(cpu->r[0]&3)return -1;
            uint8_t *p=ds_translate(&cpu->memory,cpu->r[0],4);if(!p)return -1;
            uint32_t handle=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
            unsigned index=count;
            for(unsigned i=0;i<count;i++)if(handle==(objects+i==b->object?HANDLE:OBJECT_BASE+4*i)){index=i;break;}
            if(index==count)return -1;
            LocalMapObject *next=objects+index;MapObjectArray_NextObject(&next);
            uint32_t next_handle=next==b->object?HANDLE:OBJECT_BASE+4*(index+1);
            return store(cpu,cpu->r[0],next_handle,4)?1:-1;
        }
    }
    LocalMapObject *object=cpu->r[0]==HANDLE?b->object:NULL;
    if(!object && b->manager && cpu->r[0]>=OBJECT_BASE && !((cpu->r[0]-OBJECT_BASE)&3)) {
        unsigned index=(cpu->r[0]-OBJECT_BASE)/4;
        if(index<MapObjectManager_GetObjectCount(b->manager))object=MapObjectManager_GetObjects2(b->manager)+index;
    }
    if(object && b->manager && !strcmp(name,"MapObject_GetManager")){cpu->r[0]=MANAGER_HANDLE;return 1;}
    if(object && !strcmp(name,"MapObject_GetFlagsBitsMask")){cpu->r[0]=MapObject_GetFlagsBitsMask(object,cpu->r[1]);return 1;}
    if(cpu->r[0]!=HANDLE && !object)return -1;
    if(b->field && b->object && !strcmp(name,"MapObject_GetFieldSystem")) {cpu->r[0]=FIELD_ADDRESS;return 1;}
#define GET_AVATAR(n) if(!strcmp(name,#n)&&b->avatar){cpu->r[0]=(uint32_t)n(b->avatar);return 1;}
    GET_AVATAR(PlayerAvatar_GetState)
    GET_AVATAR(PlayerAvatar_GetNextFacingDirection)
    GET_AVATAR(PlayerAvatar_GetUnk28)
    GET_AVATAR(PlayerAvatar_GetUnk2C)
#define GET_OBJECT(n) if(!strcmp(name,#n)&&object){cpu->r[0]=(uint32_t)n(object);return 1;}
    if(object && !strcmp(name,"MapObject_CopyPositionVector")) {
        VecFx32 v;MapObject_CopyPositionVector(object,&v);
        if(!store(cpu,cpu->r[1],(uint32_t)v.x,4)||!store(cpu,cpu->r[1]+4,(uint32_t)v.y,4)||
           !store(cpu,cpu->r[1]+8,(uint32_t)v.z,4))return -1;
        return 1;
    }
    GET_OBJECT(MapObject_GetInitialX)
    GET_OBJECT(MapObject_GetInitialZ)
    GET_OBJECT(MapObject_GetXRange)
    GET_OBJECT(MapObject_GetYRange)
    GET_OBJECT(MapObject_CheckFlag28)
    GET_OBJECT(MapObject_GetSpriteID)
    GET_OBJECT(sub_0205F8D0)
    GET_OBJECT(sub_0205F504)
    GET_OBJECT(MapObject_GetPreviousXCoord)
    GET_OBJECT(MapObject_GetPreviousZCoord)
    GET_OBJECT(MapObject_GetXCoord)
    GET_OBJECT(MapObject_GetYCoord)
    GET_OBJECT(MapObject_GetZCoord)
#define SET_OBJECT(n) if(!strcmp(name,#n)&&object){n(object,(int32_t)cpu->r[1]);return 1;}
    SET_OBJECT(MapObject_SetPreviousX)
    SET_OBJECT(MapObject_SetPreviousY)
    SET_OBJECT(MapObject_SetPreviousZ)
    SET_OBJECT(MapObject_AddCurrentX)
    SET_OBJECT(MapObject_AddCurrentY)
    SET_OBJECT(MapObject_AddCurrentZ)
    return -1;
}
static int run(MotionBoundary *b,uint32_t entry,uint32_t arg1,uint32_t arg2,uint32_t arg3,uint32_t *result) {
    uint8_t *ram=calloc(1,65536);if(!ram)return 0;
    memcpy(ram,hg_field_guest_image,sizeof(hg_field_guest_image));
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,65536};
    cpu.code_start=0x02000000;cpu.code_size=HG_FIELD_GUEST_CODE_SIZE;
    cpu.return_address=0x02200001;cpu.r[14]=cpu.return_address;
    cpu.r[13]=0x0200f000;cpu.r[15]=entry;cpu.r[0]=HANDLE;cpu.r[1]=arg1;cpu.r[2]=arg2;cpu.r[3]=arg3;
    if(b->field) {
        store(&cpu,FIELD_ADDRESS+0x2c,LOADER_HANDLE,4);
        store(&cpu,FIELD_ADDRESS+0x60,DISPATCH_ADDRESS,4);
        store(&cpu,DISPATCH_ADDRESS,b->new_bark_static?STATIC_HEIGHT+1:HG_FIELD_GUEST_sub_02054654+1,4);
        store(&cpu,DISPATCH_ADDRESS+4,HG_FIELD_GUEST_sub_020547D8+1,4);
        if(!b->object)cpu.r[0]=FIELD_ADDRESS;
    }
    if(b->vector) {
        cpu.r[0]=arg1;cpu.r[1]=arg2;cpu.r[2]=0x02008400;
        store(&cpu,0x02008404,(uint32_t)b->vector->y,4);
    }
    cpu.boundary=boundary;cpu.boundary_user=b;
    DSThumbResult status=ds_thumb_run(&cpu,10000);
    int ok=status==DS_THUMB_RETURN && cpu.r[13]==0x0200f000;
    if(!ok) {
        const char *name="internal";
        for(size_t i=0;i<sizeof(hg_field_guest_imports)/sizeof(*hg_field_guest_imports);i++)
            if(cpu.r[15]==hg_field_guest_imports[i].address)name=hg_field_guest_imports[i].name;
        fprintf(stderr,"Motion guest failed: status=%d pc=%08x instruction=%04x provider=%s\n",
                status,cpu.r[15],cpu.fault_instruction,name);
    }
    if(ok&&b->vector) {
        uint32_t x,y,z;
        ok=load_word(&cpu,0x02008400,&x)&&load_word(&cpu,0x02008404,&y)&&load_word(&cpu,0x02008408,&z);
        if(ok)*b->vector=(VecFx32){(int32_t)x,(int32_t)y,(int32_t)z};
    }
    if(ok&&result)*result=cpu.r[0];free(ram);return ok;
}
int hg_motion_direction(PlayerAvatar *avatar,uint16_t keys,int *direction) {
    if(!avatar||!direction)return 0;
    MotionBoundary b={.avatar=avatar};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_0205DDD4,0,keys,0,&result))return 0;
    if(result!=UINT32_MAX&&result>3)return 0;
    *direction=(int32_t)result;return 1;
}
int hg_motion_grid_step(LocalMapObject *object,unsigned direction) {
    if(!object||direction>3)return 0;
    LocalMapObject saved=*object;MotionBoundary b={.object=object};
    if(run(&b,HG_FIELD_GUEST_sub_02060F24,direction,0,0,NULL))return 1;
    *object=saved;return 0;
}

int hg_motion_tile(HGFieldResources *field,int x,int z,HGMotionTile *out) {
    if(!field||!out)return 0;
    HGFieldTile tile;if(!hg_field_resources_tile(field,x,z,&tile))return 0;
    MotionBoundary b={.field=field};uint32_t wall,behavior;
    if(!run(&b,HG_FIELD_GUEST_sub_020548C0,(uint32_t)x,(uint32_t)z,0,&wall) ||
       !run(&b,HG_FIELD_GUEST_GetMetatileBehavior,(uint32_t)x,(uint32_t)z,0,&behavior))return 0;
    if(wall>1 || behavior>255)return 0;
    *out=(HGMotionTile){.wall=(int)wall,.behavior=(uint8_t)behavior};return 1;
}
int hg_motion_terrain_barrier(HGFieldResources *field,LocalMapObject *object,unsigned direction,int *blocked) {
    if(!field||!object||!blocked||direction>3)return 0;
    LocalMapObject candidate=*object;
    HGMotionTile current,target;
    if(!hg_motion_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&current) ||
       !hg_motion_grid_step(&candidate,direction) ||
       !hg_motion_tile(field,MapObject_GetXCoord(&candidate),MapObject_GetZCoord(&candidate),&target))return 0;
    /* Refresh the original object's cached current behavior on a private copy. */
    sub_0205F4FC(&candidate,current.behavior);
    MotionBoundary b={.field=field,.object=&candidate};uint32_t directional;
    if(!run(&b,HG_FIELD_GUEST_sub_02060DEC,MapObject_GetXCoord(&candidate),
            MapObject_GetZCoord(&candidate),direction,&directional)||directional>1)return 0;
    *blocked=target.wall || directional;return 1;
}

int hg_motion_occupied(LocalMapObject *object,int x,int y,int z,int *occupied) {
    if(!object||!occupied)return 0;
    MapObjectManager *manager=MapObject_GetManager(object);
    if(!manager||!MapObjectManager_GetObjectCount(manager)||!MapObjectManager_GetObjects2(manager))return 0;
    unsigned count=MapObjectManager_GetObjectCount(manager);
    /* The bridge admits a bounded native object array, never arbitrary pointers. */
    if(count>1024)return 0;
    int found=0;for(unsigned i=0;i<count;i++)if(MapObjectManager_GetObjects2(manager)+i==object)found=1;
    if(!found)return 0;
    MotionBoundary b={.object=object,.manager=manager};uint32_t result;
    if(!run(&b,HG_FIELD_GUEST_sub_02060BFC,(uint32_t)x,(uint32_t)y,(uint32_t)z,&result)||result>1)return 0;
    *occupied=(int)result;return 1;
}

int hg_motion_player_collision(HGFieldResources *field,PlayerAvatar *avatar,unsigned direction,unsigned *mask) {
    if(!field||!avatar||!mask||direction>3)return 0;
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);if(!object)return 0;
    MapObjectManager *manager=MapObject_GetManager(object);
    if(!manager||!MapObjectManager_GetObjectCount(manager)||MapObjectManager_GetObjectCount(manager)>1024)return 0;
    int found=0;LocalMapObject *objects=MapObjectManager_GetObjects2(manager);if(!objects)return 0;
    for(unsigned i=0;i<MapObjectManager_GetObjectCount(manager);i++)if(objects+i==object)found=1;
    if(!found || PlayerAvatar_GetState(avatar)>2)return 0;
    LocalMapObject saved=*object,candidate=*object;
    HGFieldTile current,target;
    if(!hg_field_resources_tile(field,MapObject_GetXCoord(object),MapObject_GetZCoord(object),&current) ||
       !hg_motion_grid_step(&candidate,direction) ||
       !hg_field_resources_tile(field,MapObject_GetXCoord(&candidate),MapObject_GetZCoord(&candidate),&target) ||
       current.map_header!=60 || target.map_header!=60)return 0;
    sub_0205F4FC(object,(uint8_t)current.attribute);
    MotionBoundary b={.avatar=avatar,.object=object,.manager=manager,.field=field,.new_bark_static=1};
    uint32_t result;int ok=run(&b,HG_FIELD_GUEST_sub_0205DA34,HANDLE,direction,0,&result);
    *object=saved;
    if(!ok || (result&~0x2fu))return 0;
    *mask=result;return 1;
}

int hg_motion_walk_step(HGFieldResources *field,PlayerAvatar *avatar,uint16_t keys,HGMotionMove *out) {
    if(!field||!avatar||!out||PlayerAvatar_GetState(avatar)!=0)return 0;
    int direction;if(!hg_motion_direction(avatar,keys,&direction))return 0;
    HGMotionMove move={.direction=direction};
    if(direction<0){*out=move;return 1;}
    if(!hg_motion_player_collision(field,avatar,(unsigned)direction,&move.collision_mask))return 0;
    if(move.collision_mask){*out=move;return 1;}
    LocalMapObject *object=PlayerAvatar_GetMapObject(avatar),candidate=*object;
    if(!hg_motion_grid_step(&candidate,(unsigned)direction))return 0;
    int x=MapObject_GetXCoord(&candidate),z=MapObject_GetZCoord(&candidate);VecFx32 vector;
    MapObject_CopyPositionVector(object,&vector);
    if(hg_field_resources_height(field,x,z,0,vector.y,&vector.y)!=1)return 0;
    MotionBoundary b={.vector=&vector};
    if(!run(&b,HG_FIELD_GUEST_sub_020611C8,(uint32_t)x,(uint32_t)z,0,NULL))return 0;
    MapObject_SetPositionVector(&candidate,&vector);
    MapObject_SetCurrentY(&candidate,(vector.y>>3)/4096);
    MapObject_SetFacingDirectionDirect(&candidate,(unsigned)direction);
    MapObject_SetNextFacingDirection(&candidate,(unsigned)direction);
    *object=candidate;move.moved=1;*out=move;return 1;
}
