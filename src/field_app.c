#include "field_app.h"
#include "field_player.h"
#include "game_font.h"
#include "ds_frame_clock.h"
#include <math.h>
#include "compat/field_tasks.h"
struct HGFieldApp {
    FieldSystem runtime;DSFrameClock frame_clock;
    int initialized;
    HGFieldView *view,*player_sprite;
    HGFieldPlayer *player;HGFont *font;
    char dialogue[512];int interact_pending;
    uint16_t held_keys,pending_keys;unsigned move_delay,walk_serial,frame;
    unsigned long long revision;
    double visual[3],from[3];
    unsigned map;
    int stop;
    char error[256];
};
static int scene_init(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);
    app->view=hg_field_view_load(app->map,app->error,sizeof(app->error));if(app->view && hg_transition_supported(app->map)) {
        app->player=hg_field_player_new_map(app->map);app->player_sprite=hg_player_sprite_load(app->error,sizeof(app->error));
        if(!app->player||!app->player_sprite){if(!app->error[0])snprintf(app->error,sizeof(app->error),"Cannot initialize player for map %u",app->map);hg_field_view_free(app->view);app->view=NULL;}
    }
    if(app->player) {
        hg_field_player_position(app->player,NULL,NULL,app->visual);
        memcpy(app->from,app->visual,sizeof(app->from));
        app->font=hg_font_new();
        if(!app->font){snprintf(app->error,sizeof(app->error),"Cannot initialize original dialogue font");hg_field_view_free(app->view);app->view=NULL;}
        app->frame=1;hg_player_sprite_frame(app->player_sprite,app->frame);
    }
    app->initialized=1;return TRUE;
}
static int scene_exec(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);if(app->stop)return TRUE;
    if(app->move_delay)app->move_delay--;
    if(app->player && !(app->held_keys&0xf0) && !(app->pending_keys&0xf0) && !hg_field_player_walking(app->player))
        hg_field_player_move(app->player,0); /* Original idle axis-history update. */
    if(app->player && ((app->held_keys|app->pending_keys)&0xf0) && !app->dialogue[0] && !app->interact_pending && !app->move_delay && !hg_field_player_walking(app->player)) {
        memcpy(app->from,app->visual,sizeof(app->from));
        uint16_t keys=app->held_keys|app->pending_keys;app->pending_keys=0;
        HGWarpDestination destination;
        if(hg_field_player_probe_transition(app->player,keys,&destination)) {
            HGFieldView *next=hg_field_view_load((unsigned)destination.map,app->error,sizeof(app->error));
            if(next&&hg_field_player_transition(app->player,&destination)) {
                hg_field_view_free(app->view);app->view=next;app->map=(unsigned)destination.map;
                app->held_keys=app->pending_keys=0;app->move_delay=0;app->dialogue[0]=0;app->revision++;
                hg_field_player_position(app->player,NULL,NULL,app->visual);memcpy(app->from,app->visual,sizeof(app->from));
            } else { hg_field_view_free(next);app->move_delay=8; }
        } else if(hg_field_player_move(app->player,keys)) {
            app->walk_serial++;
        } else app->move_delay=8; /* Host retry policy for blocked attempts. */
    }
    if(hg_field_player_walking(app->player)) {
        hg_field_player_tick(app->player);
        hg_field_player_position(app->player,NULL,NULL,app->visual);app->revision++;
    }
    if(app->interact_pending && !hg_field_player_walking(app->player)) {
        app->interact_pending=0;app->held_keys=app->pending_keys=0;
        if(app->dialogue[0]){app->dialogue[0]=0;app->revision++;}
        else if(hg_field_player_interact(app->player,app->dialogue,sizeof(app->dialogue)))app->revision++;
    }
    if(app->player) {
        static const unsigned idle[]={1,5,9,13};
        static const unsigned stride[][2]={{2,4},{6,8},{10,12},{14,16}};
        unsigned facing=(unsigned)hg_field_player_facing(app->player),frame=idle[facing];
        if(hg_field_player_walking(app->player)) {
            double distance=fabs(app->visual[0]-app->from[0])+fabs(app->visual[2]-app->from[2]);
            if(hg_field_player_running(app->player))frame=idle[facing]+16;
            if(distance<=8)frame=stride[facing][app->walk_serial&1];
            if(hg_field_player_running(app->player)&&distance<=8)frame+=16;
        }
        if(frame!=app->frame){app->frame=frame;hg_player_sprite_frame(app->player_sprite,frame);app->revision++;}
    }
    return FALSE;
}
static int scene_exit(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);hg_field_view_free(app->view);app->view=NULL;hg_field_player_free(app->player);app->player=NULL;hg_field_view_free(app->player_sprite);app->player_sprite=NULL;hg_font_free(app->font);app->font=NULL;return TRUE;
}
static BOOL run_scene_application(TaskManager *task) {
    HGFieldApp *app=TaskManager_GetEnvironment(task);u32 *state=TaskManager_GetStatePtr(task);
    if(*state==0) {
        static const OverlayManagerTemplate template={scene_init,scene_exec,scene_exit,FS_OVERLAY_ID_NONE};
        *state=1;CallApplicationAsTask(task,&template,app);return FALSE;
    }
    return TRUE;
}
HGFieldApp *hg_field_app_new(unsigned map,char *error,size_t size) {
    if(error && size)error[0]=0;
    HGFieldApp *app=calloc(1,sizeof(*app));if(!app) { if(error && size)snprintf(error,size,"Out of memory");return NULL; }
    app->map=map;FieldSystem_CreateTask(&app->runtime,run_scene_application,app);
    /* First frame calls a child task; second launches/initializes its app. */
    hg_field_tasks_tick(&app->runtime);hg_field_tasks_tick(&app->runtime);GF_ASSERT(app->initialized);
    if(!app->view) { if(error && size)snprintf(error,size,"%s",app->error);hg_field_app_free(app);return NULL; }
    return app;
}
void hg_field_app_free(HGFieldApp *app) {
    if(!app)return;app->stop=1;
    /* Exec completes, then exit, then the original waiting task unwinds its
     * child and parent. Never forcibly free a task with an unknown environment. */
    for(unsigned i=0;i<5 && !hg_field_tasks_idle(&app->runtime);i++)hg_field_tasks_tick(&app->runtime);
    GF_ASSERT(hg_field_tasks_idle(&app->runtime));free(app);
}
void hg_field_app_step(HGFieldApp *app) { if(app)hg_field_tasks_tick(&app->runtime); }
unsigned long long hg_field_app_ticks(const HGFieldApp *app) { return app?app->runtime.ticks:0; }
const HGFieldView *hg_field_app_view(const HGFieldApp *app) { return app?app->view:NULL; }
int hg_field_app_render(HGFieldApp *app,const DSCamera *camera,uint8_t *pixels,unsigned width,unsigned height) {
    if(!app || !camera || !pixels)return 0;
    int ok=app->player?hg_field_view_render_player(app->view,app->player_sprite,app->visual,camera,pixels,width,height):
        hg_field_view_render(app->view,camera,pixels,width,height);
    if(ok && app->dialogue[0] && app->font && width>=256 && height>=80) {
        unsigned char panel[256*64*4];
        for(unsigned y=0;y<64;y++)for(unsigned x=0;x<256;x++) {
            unsigned char *p=panel+4*(y*256+x);int edge=x<2||x>=254||y<2||y>=62;
            p[0]=edge?48:247;p[1]=edge?64:249;p[2]=edge?80:236;p[3]=255;
        }
        hg_font_draw(app->font,app->dialogue,panel,256,64,10,8,236,0x202830,0xc0c8c0);
        hg_font_draw(app->font,"Enter / Space to close",panel,256,64,10,44,236,0x405060,0xc0c8c0);
        unsigned scale=width>=512&&height>=160?2:1;
        unsigned left=(width-256*scale)/2,top=height-64*scale-8;
        for(unsigned y=0;y<64*scale;y++)for(unsigned x=0;x<256*scale;x++)
            memcpy(pixels+4*((top+y)*width+left+x),panel+4*((y/scale)*256+x/scale),4);
    }
    return ok;
}

void hg_field_app_input(HGFieldApp *app,uint16_t keys) {
    if(!app)return;keys&=0xf2;
    if(!(app->held_keys&0xf0))app->move_delay=0;
    /* Only directional edges queue an action; a short Shift press alone must
     * not start a move or turn a later walk into an unintended run. */
    uint16_t pressed=(keys&~app->held_keys)&0xf0;
    if(pressed)app->pending_keys|=pressed|(keys&2);app->held_keys=keys;
}
void hg_field_app_clear_input(HGFieldApp *app) { if(app)app->held_keys=app->pending_keys=0; }
int hg_field_app_player(const HGFieldApp *app,int *x,int *z) { return app&&hg_field_player_position(app->player,x,z,NULL); }
void hg_field_app_camera(const HGFieldApp *app,DSCamera *camera,unsigned w,unsigned h) {
    if(!app)return;
    if(app->player)hg_field_view_focus(app->view,app->visual,camera,w,h);
    else hg_field_view_camera(app->view,camera,w,h);
}

unsigned long long hg_field_app_revision(const HGFieldApp *app) { return app?app->revision:0; }
int hg_field_app_presentation(const HGFieldApp *app,double world[3],unsigned *frame) {
    if(!app||!app->player)return 0;
    if(world)memcpy(world,app->visual,sizeof(app->visual));if(frame)*frame=app->frame;return 1;
}

void hg_field_app_interact(HGFieldApp *app) { if(app&&app->player){app->held_keys=app->pending_keys=0;app->interact_pending=1;} }
const char *hg_field_app_dialogue(const HGFieldApp *app) { return app?app->dialogue:""; }

unsigned hg_field_app_advance(HGFieldApp *app,uint64_t monotonic_ns) {
    if(!app)return 0;unsigned count=ds_frame_clock_advance(&app->frame_clock,monotonic_ns);
    for(unsigned i=0;i<count;i++)hg_field_app_step(app);return count;
}
unsigned hg_field_app_steps(const HGFieldApp *app) { return app?hg_field_player_steps(app->player):0; }
unsigned hg_field_app_map(const HGFieldApp *app) { return app?app->map:UINT32_MAX; }
