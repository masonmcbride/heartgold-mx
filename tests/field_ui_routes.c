/* Keep original C object interfaces out of Objective-C's BOOL ABI. The native
 * window smoke check shares the connected scenario planner with C tests. */
#include "compat/pokemon.h"
#include "compat/motion.h"
#include "compat/story.h"
#include "field_app.h"
#include "field_events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"Native UI route failed at %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
#include "field_routes.h"
void hg_ui_test_approach(HGFieldApp *app,unsigned script) { hg_test_approach(app,script); }

void hg_ui_test_return_to_elm(HGFieldApp *app){hg_test_portal(app,33);hg_test_portal(app,60);hg_test_portal(app,61);}
HGFieldApp *hg_ui_test_tower_ladder(void){
    char error[256];HGFieldApp *app=hg_field_app_continue_copy("build/checkpoints/155-violet-arrival.sav","/tmp","hg-ladder-ui",error,sizeof(error));CHECK(app);
    hg_test_portal(app,110);hg_test_portal(app,155);CHECK(hg_field_app_map(app)==155);return app;
}
void hg_ui_test_tower_descend(HGFieldApp *app){hg_test_portal(app,110);CHECK(hg_field_app_map(app)==110&&!hg_field_app_error(app)[0]);}


/* A legal one-HP party fixture; battle outcome still comes from the original
 * engine. Shared by the C recovery check and native keyboard/window smoke. */
void hg_test_prepare_defeat(HGFieldApp *app){
    CHECK(hg_field_app_map(app)==33);int sx,sz,gx=-1,gz=-1;CHECK(hg_field_app_player(app,&sx,&sz));
    for(int r=0;r<48&&gx<0;r++)for(int z=sz-r;z<=sz+r&&gx<0;z++)for(int x=sx-r;x<=sx+r;x++){
        if(abs(x-sx)+abs(z-sz)!=r)continue;u16 a,b;
        if(hg_field_attribute(33,x,z,&a)&&hg_field_attribute(33,x+1,z,&b)&&MetatileBehavior_IsTallGrass(a&255)&&MetatileBehavior_IsTallGrass(b&255)){gx=x;gz=z;break;}
    }
    CHECK(gx>=0);hg_test_walk_to(app,gx,gz);hg_test_settle(app);
    FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);Party *party=hg_story_save_party(field.saveData);CHECK(Party_GetCount(party)==1);
    u32 hp=1;SetMonData(Party_GetMonByIndex(party,0),MON_DATA_HP,&hp);
}

void hg_ui_test_approach_center_door(HGFieldApp *app){
    CHECK(hg_field_app_map(app)==67);HGEventView events;CHECK(hg_event_load(67,&events));
    for(unsigned i=0;i<events.warp_count;i++){const uint8_t *p=events.warps+12*i;
        if((p[4]|(unsigned)p[5]<<8)==69){hg_test_walk_to(app,p[0]|(unsigned)p[1]<<8,(p[2]|(unsigned)p[3]<<8)+1);return;}
    }CHECK(0);
}

void hg_ui_test_prepare_nurse(HGFieldApp *app){
    CHECK(hg_field_app_map(app)==69);hg_test_approach(app,1);
    FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);u32 hp=1;SetMonData(Party_GetMonByIndex(hg_story_save_party(field.saveData),0),MON_DATA_HP,&hp);hg_story_touch(hg_field_app_story(app));
}
int hg_ui_test_nurse_healed(HGFieldApp *app){
    FieldSystem field={0};hg_story_bind_field(hg_field_app_story(app),&field);Pokemon *mon=Party_GetMonByIndex(hg_story_save_party(field.saveData),0);
    return GetMonData(mon,MON_DATA_HP,NULL)==GetMonData(mon,MON_DATA_MAX_HP,NULL);
}

void hg_ui_test_prepare_mart(HGFieldApp *app){
    hg_test_portal(app,67);hg_test_travel(app,67,128);hg_test_settle(app);hg_test_portal(app,68);hg_test_travel(app,68,80);hg_test_approach(app,1);
}

void hg_ui_test_prepare_mother(HGFieldApp *app){
    hg_test_portal(app,67);hg_test_settle(app);hg_test_portal(app,34);hg_test_settle(app);
    hg_test_walk_to(app,553,293);hg_test_settle(app);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);
    for(unsigned tick=1;tick<8;tick++)hg_field_app_step(app);
}
