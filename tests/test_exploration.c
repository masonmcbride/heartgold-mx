#include "compat/motion.h"
#include "field_player.h"
#include "field_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
#include "field_routes.h"
int main(void) {
    const unsigned maps[]={64,60,33,67,34,143,13,35,73,76,78};
    const uint16_t directions[]={0x40,0x10,0x80,0x20};
    for(unsigned m=0;m<sizeof(maps)/sizeof(*maps);m++) {
        unsigned map=maps[m];HGFieldPlayer *p=hg_field_player_new_explore(map);
        if(!p)fprintf(stderr,"explorer map %u failed\n",map);CHECK(p);
        CHECK(hg_field_player_actor_counts(p).active==0);
        CHECK(!hg_field_player_frame_begin(p)&&!hg_field_player_coordinate_begin(p));
        CHECK(!hg_field_player_interaction_begin(p)&&!hg_field_player_start_script(p,0,-1));
        hg_field_player_free(p);
        char error[256];HGFieldApp *app=hg_field_app_new_explore(map,error,sizeof(error));CHECK(app);
        CHECK(hg_field_app_is_exploring(app)&&hg_field_app_running_shoes(app));
        int x,z;CHECK(hg_field_app_player(app,&x,&z));int moved=0;
        for(unsigned d=0;d<4&&!moved;d++) {
            hg_field_app_input(app,directions[d]);hg_field_app_step(app);hg_field_app_input(app,0);
            for(unsigned i=0;i<16;i++)hg_field_app_step(app);
            int nx,nz;CHECK(hg_field_app_player(app,&nx,&nz));moved=x!=nx||z!=nz;
        }
        CHECK(moved);hg_field_app_interact(app);
        for(unsigned i=0;i<64;i++)hg_field_app_step(app);
        CHECK(!hg_field_app_interaction_busy(app)&&!hg_field_app_dialogue(app)[0]);
        uint8_t pixels[256*192*4];DSCamera c;hg_field_app_camera(app,&c,256,192);CHECK(hg_field_app_render(app,&c,pixels,256,192));
        hg_field_app_free(app);
    }
    char error[256];HGFieldApp *route=hg_field_app_new_explore(60,error,sizeof(error));CHECK(route);
    HGStoryState *owner=hg_field_app_story(route);
    const unsigned travel[]={61,60,33,67,34,143,34,67,33,60};
    for(unsigned i=0;i<sizeof(travel)/sizeof(*travel);i++) {
        hg_test_portal(route,travel[i]);hg_test_settle(route);
        CHECK(hg_field_app_story(route)==owner);
        CHECK(!hg_field_app_error(route)[0]);
    }
    hg_field_app_advance(route,UINT64_C(1000000000));
    unsigned long long ticks=hg_field_app_ticks(route);hg_field_app_resume(route);
    CHECK(!hg_field_app_advance(route,UINT64_C(3600000000000))&&hg_field_app_ticks(route)==ticks);
    hg_field_app_free(route);
    puts("Exploration: original terrain movement, no actors/events, eleven rendered maps passed");return 0;
}
