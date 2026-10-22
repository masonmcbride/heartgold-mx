#include "compat/blackout.h"
#include "compat/save_manager.h"
#include "compat/motion.h"
#include "field_types_def.h"
#include "motion_guest.h"
#include "field_view.h"
#include "save_local_field_data.h"
#include "game_name.h"
#include "constants/flags.h"
#include "constants/std_script.h"
static char error[256];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,error);exit(1);}}while(0)
int main(void){
    u8 *pixels=malloc(256*192*4);CHECK(pixels);
    for(unsigned facing=0;facing<4;facing++){
        HGFieldView *nurse=hg_character_sprite_load(335,facing,error,sizeof(error));CHECK(nurse&&hg_field_view_triangles(nurse));
        DSCamera camera;hg_field_view_camera(nurse,&camera,256,192);CHECK(hg_field_view_render(nurse,&camera,pixels,256,192));hg_field_view_free(nurse);
    }
    LocalMapObject object={0};PlayerAvatar avatar={.mapObject=&object};
    for(unsigned gender=0;gender<2;gender++) {
        avatar.gender=gender;
        const unsigned bits[]={1u<<8,1u,1u<<11,1u<<10,1u};
        const unsigned states[]={10,0,12,3,0};
        for(unsigned i=0;i<NELEMS(bits);i++) {
            PlayerAvatar_SetTransitionFlags(&avatar,bits[i]);CHECK(hg_motion_avatar_transition(&avatar));
            CHECK(!PlayerAvatar_GetTransitionFlags(&avatar)&&MapObject_GetSpriteID(&object)==PlayerAvatar_GetSpriteByStateAndGender(states[i],gender));
            HGFieldView *pose=hg_avatar_sprite_load(MapObject_GetSpriteID(&object),0,error,sizeof(error));CHECK(pose);
            DSCamera camera;hg_field_view_camera(pose,&camera,256,192);CHECK(hg_field_view_render(pose,&camera,pixels,256,192));hg_field_view_free(pose);
        }
        PlayerAvatar_SetTransitionFlags(&avatar,2);PlayerAvatar before=avatar;LocalMapObject previous=object;
        CHECK(!hg_motion_avatar_transition(&avatar)&&!memcmp(&before,&avatar,sizeof(avatar))&&!memcmp(&previous,&object,sizeof(object)));
        PlayerAvatar_SetTransitionFlags(&avatar,0);
    }
    free(pixels);
    SaveData *save=malloc(sizeof(*save)),*before=malloc(sizeof(*save));CHECK(save&&before);hg_save_manager_init_ram(save);
    CHECK(hg_name_encode("Ethan",hg_story_save_profile(save)->name,7));
    Party *party=hg_story_save_party(save);Pokemon mon;CreateMon(&mon,152,15,31,TRUE,123,OT_ID_PRESET,42);
    u32 zero=0,status=8;SetMonData(&mon,MON_DATA_HP,&zero);SetMonData(&mon,MON_DATA_STATUS,&status);SetMonData(&mon,MON_DATA_MOVE1_PP,&zero);CHECK(Party_AddMon(party,&mon));
    LocalFieldData *local=SaveArray_Get(save,SAVE_LOCAL_FIELD_DATA);*LocalFieldData_GetCurrentPosition(local)=(Location){33,-1,658,402,3};
    int output[5]={7,7,7,7,7},unchanged[5];memcpy(unchanged,output,sizeof(output));CHECK(!hg_field_guest_spawn(0,0,output)&&!memcmp(output,unchanged,sizeof(output)));
    for(unsigned spawn=1;spawn<=30;spawn++){CHECK(hg_field_guest_spawn(spawn,0,output));CHECK(output[0]>=0&&output[0]<540&&output[1]==-1&&output[2]>=0&&output[3]>=0&&output[4]==0);CHECK(hg_field_guest_spawn(spawn,1,output)&&output[1]==-1&&output[4]==1);}
    for(unsigned spawn=1;spawn<=2;spawn++){
        LocalFieldData_SetBlackoutSpawn(local,spawn);*before=*save;
        HGBlackout *n=hg_blackout_new(save,NULL,error,sizeof(error));CHECK(n);CHECK(!hg_blackout_ack(n));
        hg_blackout_free(n);CHECK(!memcmp(save,before,sizeof(*save)));
        n=hg_blackout_new(save,NULL,error,sizeof(error));CHECK(n);unsigned ticks=0,pages=0;int state=HG_BLACKOUT_RUNNING;
        while(state!=HG_BLACKOUT_DONE&&ticks++<256){state=hg_blackout_tick(n);if(state==HG_BLACKOUT_FAILED)snprintf(error,sizeof(error),"%s",hg_blackout_error(n));CHECK(state!=HG_BLACKOUT_FAILED);if(state==HG_BLACKOUT_MESSAGE){CHECK(strstr(hg_blackout_message(n),"Ethan"));CHECK(hg_blackout_ack(n));pages++;}CHECK(!memcmp(save,before,sizeof(*save)));}
        CHECK(ticks<256&&pages&&hg_blackout_brightness(n)==0);
        CHECK(hg_blackout_script(n)==(spawn==1?std_whited_out_to_mom:std_whited_out_to_pokecenter));
        const SaveData *result=hg_blackout_save(n);Location *position=LocalFieldData_GetCurrentPosition(SaveArray_Get((SaveData *)result,SAVE_LOCAL_FIELD_DATA));
        CHECK(position->mapId==(spawn==1?63:69)&&position->x==(spawn==1?6:8)&&position->y==(spawn==1?8:13)&&position->direction==0);
        Location *previous=LocalFieldData_GetPreviousPosition(SaveArray_Get((SaveData *)result,SAVE_LOCAL_FIELD_DATA));CHECK(previous->mapId==33&&previous->x==658&&previous->y==402);
        Location *special=LocalFieldData_GetSpecialSpawnWarpPtr(SaveArray_Get((SaveData *)result,SAVE_LOCAL_FIELD_DATA));
        CHECK(special->mapId==(spawn==1?60:67)&&special->x==(spawn==1?695:564)&&special->y==(spawn==1?397:392)&&special->direction==1);
        Pokemon *healed=Party_GetMonByIndex(hg_story_save_party((SaveData *)result),0);
        CHECK(GetMonData(healed,MON_DATA_HP,NULL)==GetMonData(healed,MON_DATA_MAX_HP,NULL)&&!GetMonData(healed,MON_DATA_STATUS,NULL));
        CHECK(GetMonData(healed,MON_DATA_MOVE1_PP,NULL)==GetMonData(healed,MON_DATA_MOVE1_MAX_PP,NULL));
        CHECK(hg_save_manager_validate_ram(result));hg_blackout_free(n);
    }
    free(save);free(before);puts("Original blackout task: all spawn selectors, private/cancelled state, death/special warp, healing, presentation acknowledgement and standard-script selection pass");
}
