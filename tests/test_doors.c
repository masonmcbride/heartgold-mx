#include "compat/transitions.h"
#include "compat/prop_animation.h"
#include "compat/resources.h"
#include "field_view.h"
#include "field_data.h"
#include "field_events.h"
#include "motion_guest.h"
#include "constants/sndseq.h"
#include "field_app.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#include "field_routes.h"
static unsigned half(const u8 *p){return p[0]|(unsigned)p[1]<<8;}
int main(void){
    HGEventView events;CHECK(hg_event_load(67,&events));int x=-1,z=-1;
    for(unsigned i=0;i<events.warp_count;i++){const u8 *p=events.warps+12*i;if(half(p+4)==69){x=half(p);z=half(p+2)+1;break;}}
    CHECK(x>=0&&z>=0);HGFieldResources *resources=hg_field_resources_new(67);HGPropAnimations *animations=hg_prop_animations_new(67);CHECK(resources&&animations);
    unsigned sound=0;int status=hg_prop_animation_open_door(animations,resources,x,z,1,&sound);fprintf(stderr,"Center door at %d,%d: status %d sound %u\n",x,z,status,sound);CHECK(status==1);
    CHECK(sound==SEQ_SE_DP_DOOR10);
    HGPropPose poses[16];CHECK(hg_prop_animations_poses(animations,poses)==1);fprintf(stderr,"Door model %u\n",poses[0].model);
    const u8 *curve;size_t curve_size;DSJointAnimation decoded;
    CHECK(native_resource_member(NARC_a_1_0_6,0,35,&curve,&curve_size)&&ds_joint_animation_open(curve,curve_size,&decoded));
    for(size_t cut=0;cut<curve_size;cut++)CHECK(!ds_joint_animation_open(curve,cut,&decoded));
    u8 *bad=malloc(curve_size);CHECK(bad);memcpy(bad,curve,curve_size);
    /* Independent malformed scale-curve offset must not publish a partial view. */
    memset(bad+0x74,255,4);DSJointAnimation untouched={.frames=123};CHECK(!ds_joint_animation_open(bad,curve_size,&untouched)&&untouched.frames==123);free(bad);
    HGFieldView *view=hg_field_view_new(67);CHECK(view);DSCamera camera;hg_field_view_focus(view,poses[0].world,&camera,256,192);
    u8 *closed=malloc(256*192*4),*opened=malloc(256*192*4),*reset=malloc(256*192*4);CHECK(closed&&opened&&reset);
    CHECK(hg_field_view_render(view,&camera,closed,256,192));unsigned ticks=0;
    while(!hg_prop_animation_finished(animations,1)){CHECK(hg_prop_animations_tick(animations));CHECK(++ticks<=10);}
    CHECK(ticks==9&&hg_prop_animations_poses(animations,poses)==1);
    CHECK(poses[0].pose.nodes[0].m[0]==410.0/4096&&poses[0].pose.nodes[0].m[3]==9);
    CHECK(hg_field_view_prop_poses(view,poses,1)&&hg_field_view_render(view,&camera,opened,256,192)&&memcmp(opened,closed,256*192*4));
    CHECK(hg_prop_animation_play(animations,1,1));ticks=0;while(!hg_prop_animation_finished(animations,1)){CHECK(hg_prop_animations_tick(animations));CHECK(++ticks<=10);}
    CHECK(hg_prop_animations_poses(animations,poses)==1&&hg_field_view_prop_poses(view,poses,1)&&hg_field_view_render(view,&camera,reset,256,192)&&!memcmp(closed,reset,256*192*4));
    CHECK(hg_prop_animation_unload(animations,1)&&hg_field_view_prop_poses(view,NULL,0));
    CHECK(hg_field_view_render(view,&camera,reset,256,192)&&!memcmp(closed,reset,256*192*4));
    hg_prop_animations_free(animations);hg_field_resources_free(resources);hg_field_view_free(view);
    char error[256];HGFieldApp *app=hg_field_app_new_explore(67,error,sizeof(error));CHECK(app);hg_test_walk_to(app,x,z);
    hg_field_app_camera(app,&camera,256,192);CHECK(hg_field_view_render(hg_field_app_view(app),&camera,closed,256,192));
    unsigned steps=hg_field_app_steps(app);hg_field_app_input(app,0x40);hg_field_app_step(app);hg_field_app_clear_input(app);
    CHECK(hg_field_app_transition_active(app)&&hg_field_app_map(app)==67&&hg_field_app_brightness(app)==0);
    int visible=0;for(unsigned t=0;t<64&&hg_field_app_transition_active(app);t++){
        CHECK(!hg_field_app_error(app)[0]);CHECK(hg_field_app_render(app,&camera,opened,256,192));
        if(hg_field_app_map(app)==67&&hg_field_app_brightness(app)==0){CHECK(hg_field_view_render(hg_field_app_view(app),&camera,reset,256,192));visible|=memcmp(closed,reset,256*192*4)!=0;}
        hg_field_app_step(app);
    }
    CHECK(visible&&hg_field_app_map(app)==69&&!hg_field_app_transition_active(app)&&hg_field_app_brightness(app)==0&&hg_field_app_steps(app)==steps);
    /* The same owner drives the destination half: opening, threshold step,
     * closing and restoration. This must run in the public application loop. */
    hg_field_app_input(app,0x80);hg_field_app_step(app);hg_field_app_clear_input(app);
    CHECK(hg_field_app_transition_active(app));
    int exit_open=0,exit_closed=0,threshold=0;unsigned ticks_out=0;
    for(;ticks_out<80&&hg_field_app_transition_active(app);ticks_out++) {
        CHECK(!hg_field_app_error(app)[0]);
        if(hg_field_app_map(app)==67) {
            int px,pz;CHECK(hg_field_app_player(app,&px,&pz));
            threshold|=px==x&&pz==z;
            CHECK(hg_field_app_render(app,&camera,opened,256,192));
            /* Test the retained field geometry, excluding actor/fade pixels. */
            CHECK(hg_field_view_render(hg_field_app_view(app),&camera,reset,256,192));
            if(!exit_open){memcpy(closed,reset,256*192*4);exit_open=1;}
            else exit_closed|=memcmp(closed,reset,256*192*4)!=0;
        }
        hg_field_app_step(app);
    }
    int px,pz;CHECK(hg_field_app_player(app,&px,&pz));
    CHECK(exit_open&&exit_closed&&threshold&&px==x&&pz==z);
    CHECK(!hg_field_app_transition_active(app)&&hg_field_app_brightness(app)==0&&hg_field_app_steps(app)==steps);
    /* Removing the finished animation must restore the exact static field. */
    HGFieldView *static_view=hg_field_view_new(67);CHECK(static_view);
    CHECK(hg_field_app_render(app,&camera,opened,256,192));
    CHECK(hg_field_view_render(hg_field_app_view(app),&camera,closed,256,192));
    CHECK(hg_field_view_render(static_view,&camera,reset,256,192)&&!memcmp(closed,reset,256*192*4));
    hg_field_view_free(static_view);
    hg_field_app_free(app);free(closed);free(opened);free(reset);
    puts("Building doors: original sound/curves, visible entry and exit, threshold movement, closed restoration and retained statistics pass");
}
