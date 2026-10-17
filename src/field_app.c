#include "field_app.h"
#include "field_player.h"
#include "compat/field_tasks.h"
struct HGFieldApp {
    FieldSystem runtime;
    int initialized;
    HGFieldView *view,*player_sprite;
    HGFieldPlayer *player;
    uint16_t held_keys;unsigned move_delay,walk_remaining,walk_serial,frame;
    unsigned long long revision;
    double visual[3],from[3],target[3];
    unsigned map;
    int stop;
    char error[256];
};
static int scene_init(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);
    app->view=hg_field_view_load(app->map,app->error,sizeof(app->error));if(app->view && app->map==60) {
        app->player=hg_field_player_new();app->player_sprite=hg_player_sprite_load(app->error,sizeof(app->error));
        if(!app->player||!app->player_sprite){if(!app->error[0])snprintf(app->error,sizeof(app->error),"Cannot initialize New Bark player");hg_field_view_free(app->view);app->view=NULL;}
    }
    if(app->player) {
        hg_field_player_position(app->player,NULL,NULL,app->visual);
        memcpy(app->from,app->visual,sizeof(app->from));memcpy(app->target,app->visual,sizeof(app->target));
        app->frame=1;hg_player_sprite_frame(app->player_sprite,app->frame);
    }
    app->initialized=1;return TRUE;
}
static int scene_exec(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);if(app->stop)return TRUE;
    if(app->move_delay)app->move_delay--;
    if(app->player && app->held_keys && !app->move_delay && !app->walk_remaining) {
        memcpy(app->from,app->visual,sizeof(app->from));
        if(hg_field_player_move(app->player,app->held_keys)) {
            hg_field_player_position(app->player,NULL,NULL,app->target);
            app->walk_remaining=8;app->walk_serial++;
        }
        app->move_delay=8;
    }
    if(app->player) {
        static const unsigned idle[]={1,5,9,13};
        static const unsigned stride[][2]={{2,4},{6,8},{10,12},{14,16}};
        unsigned facing=(unsigned)hg_field_player_facing(app->player),frame=idle[facing];
        if(app->walk_remaining) {
            unsigned progress=9-app->walk_remaining;
            for(unsigned i=0;i<3;i++)app->visual[i]=app->from[i]+(app->target[i]-app->from[i])*progress/8.0;
            if(progress<=4)frame=stride[facing][app->walk_serial&1];
            app->walk_remaining--;app->revision++;
        }
        if(frame!=app->frame){app->frame=frame;hg_player_sprite_frame(app->player_sprite,frame);app->revision++;}
    }
    return FALSE;
}
static int scene_exit(OverlayManager *manager,int *state) {
    (void)state;HGFieldApp *app=OverlayManager_GetArgs(manager);hg_field_view_free(app->view);app->view=NULL;hg_field_player_free(app->player);app->player=NULL;hg_field_view_free(app->player_sprite);app->player_sprite=NULL;return TRUE;
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
    if(app->player)
        return hg_field_view_render_player(app->view,app->player_sprite,app->visual,camera,pixels,width,height);
    return hg_field_view_render(app->view,camera,pixels,width,height);
}

void hg_field_app_input(HGFieldApp *app,uint16_t keys) { if(app){if(!app->held_keys)app->move_delay=0;app->held_keys=keys&0xf0;} }
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
