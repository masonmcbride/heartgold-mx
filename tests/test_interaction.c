#include "compat/motion.h"
#include "field_events.h"
#include "field_app.h"
#include "motion_guest.h"
#include "ds_resources.h"
#include "compat/resources.h"
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void) {
    HGEventView view;CHECK(hg_event_load(60,&view));
    CHECK(view.background_count==5&&view.object_count==10&&view.warp_count==5&&view.coordinate_count==4);
    unsigned valid=0,missing=0;
    for(unsigned i=0;i<491;i++) {
        const uint8_t *bytes;size_t size;CHECK(native_resource_member(NARC_fielddata_eventdata_zone_event,0,i,&bytes,&size));
        if(!size){missing++;continue;}
        HGEventView decoded;CHECK(hg_event_decode(bytes,size,&decoded));valid++;
        for(size_t n=0;n<size;n++)CHECK(!hg_event_decode(bytes,n,&decoded));
    }
    CHECK(valid==464&&missing==27);
    LocalMapObject object={0};MapObjectManager manager={0};FieldSystem field={0};PlayerSaveData save;
    manager.objects=&object;manager.objectCount=1;manager.fieldSystem=&field;object.manager=&manager;object.movement=1;
    MapObject_SetFlagsBits(&object,MAPOBJECTFLAG_ACTIVE);PlayerSaveData_Init(&save);
    PlayerAvatar *avatar=PlayerAvatar_CreateWithActiveMapObject(&manager,&save,0);CHECK(avatar);
    const int dx[]={0,0,-1,1},dz[]={-1,1,0,0};
    for(unsigned i=0;i<4;i++)for(unsigned d=0;d<4;d++) {
        const uint8_t *event=view.backgrounds+20*i;
        int x=(int)(event[4]|(unsigned)event[5]<<8),z=(int)(event[8]|(unsigned)event[9]<<8);
        MapObject_SetCurrentX(&object,x-dx[d]);MapObject_SetCurrentZ(&object,z-dz[d]);
        PlayerAvatar_SetFacingDirection(avatar,d);unsigned script=0;
        CHECK(hg_motion_background_event(avatar,event,1,&script));
        CHECK(script==(unsigned)(event[0]|event[1]<<8));
        uint8_t directed[20];memcpy(directed,event,20);
        for(unsigned rule=0;rule<7;rule++) {
            directed[16]=rule;directed[17]=0;
            CHECK(hg_motion_background_event(avatar,directed,1,&script));
            int expected=rule==4||(d==0&&(rule==0||rule==6))||(d==1&&(rule==3||rule==6))||
                (d==2&&(rule==2||rule==5))||(d==3&&(rule==1||rule==5));
            CHECK(script==(expected?(unsigned)(event[0]|event[1]<<8):65535));
        }
        MapObject_SetCurrentX(&object,x+2);CHECK(hg_motion_background_event(avatar,event,1,&script)&&script==65535);
    }
    char message[512];CHECK(hg_sign_script(14,message,sizeof(message)));CHECK(!strcmp(message,"Elm Pokémon Lab"));
    CHECK(hg_sign_script(15,message,sizeof(message)));CHECK(!strcmp(message,"New Bark Town\nWinds of a New Beginning"));
    for(unsigned id=1;id<=18;id++) {
        strcpy(message,"unchanged");int accepted=hg_sign_script(id,message,sizeof(message));
        CHECK(accepted==(id==14||id==15));if(!accepted)CHECK(!strcmp(message,"unchanged"));
    }
    strcpy(message,"unchanged");CHECK(!hg_sign_script(8,message,sizeof(message))&&!strcmp(message,"unchanged"));
    CHECK(!hg_sign_script(14,message,3)&&!strcmp(message,"unchanged"));CHECK(!hg_sign_script(0,message,sizeof(message)));
    PlayerAvatar_FreeToHeap(avatar);
    char error[256];HGFieldApp *app=hg_field_app_new(60,error,sizeof(error));CHECK(app);
    const unsigned route[]={0x20,0x20,0x80,0x80,0x20};
    for(unsigned i=0;i<5;i++) {
        hg_field_app_input(app,route[i]);hg_field_app_step(app);hg_field_app_input(app,0);
        for(unsigned j=0;j<7;j++)hg_field_app_step(app);
    }
    int px,pz;CHECK(hg_field_app_player(app,&px,&pz));
    CHECK(px==686&&pz==400);
    hg_field_app_interact(app);hg_field_app_step(app);
    CHECK(!strcmp(hg_field_app_dialogue(app),"New Bark Town\nWinds of a New Beginning"));
    DSCamera camera;hg_field_app_camera(app,&camera,320,240);unsigned char shown[320*240*4],closed[sizeof(shown)];
    CHECK(hg_field_app_render(app,&camera,shown,320,240));
    hg_field_app_input(app,0x10);for(unsigned i=0;i<16;i++)hg_field_app_step(app);
    int frozenx,frozenz;CHECK(hg_field_app_player(app,&frozenx,&frozenz)&&frozenx==px&&frozenz==pz);
    hg_field_app_interact(app);hg_field_app_step(app);hg_field_app_input(app,0);
    CHECK(!hg_field_app_dialogue(app)[0]);CHECK(hg_field_app_render(app,&camera,closed,320,240));
    CHECK(memcmp(shown,closed,sizeof(shown)));hg_field_app_free(app);
    printf("Original events: %u validated / %u unresolved; facing selection and original sign bytecode/message dispatch passed\n",valid,missing);return 0;
}
