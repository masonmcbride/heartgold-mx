#include "compat/overlays.h"
#include "field_app.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1); } } while(0)
typedef struct { HGFieldView *scene;unsigned init,exec,exit;int module; } Scenario;
static int init(OverlayManager *man,int *state) {
    Scenario *s=OverlayManager_GetArgs(man);CHECK(s);CHECK(hg_overlay_live()==(unsigned)(s->module>=0));
    CHECK(*state==(int)s->init);s->init++;
    if((*state)++==0) {
        unsigned *data=OverlayManager_CreateAndGetData(man,sizeof(unsigned),HEAP_ID_DEFAULT);*data=0x12345678;
        CHECK(OverlayManager_GetData(man)==data);return FALSE;
    }
    char error[256];s->scene=hg_field_view_load(63,error,sizeof(error));CHECK(s->scene);return TRUE;
}
static int exec(OverlayManager *man,int *state) {
    Scenario *s=OverlayManager_GetArgs(man);CHECK(*state==(int)s->exec);s->exec++;
    unsigned char frame[64*64*4];DSCamera camera;hg_field_view_camera(s->scene,&camera,64,64);
    CHECK(hg_field_view_render(s->scene,&camera,frame,64,64));
    CHECK(*(unsigned *)OverlayManager_GetData(man)==0x12345678);
    return ++*state==3;
}
static int finish(OverlayManager *man,int *state) {
    Scenario *s=OverlayManager_GetArgs(man);CHECK(*state==(int)s->exit);s->exit++;
    if((*state)++==0)return FALSE;
    hg_field_view_free(s->scene);s->scene=NULL;OverlayManager_FreeData(man);CHECK(!OverlayManager_GetData(man));return TRUE;
}
int main(void) {
    CHECK(!hg_overlay_register(-1));CHECK(!hg_overlay_register(256));CHECK(!hg_overlay_unregister(77));
    CHECK(hg_overlay_register(77));CHECK(!hg_overlay_register(77));
    for(unsigned pass=0;pass<3;pass++) {
        Scenario scenario={.module=pass==1?FS_OVERLAY_ID_NONE:77};
        OverlayManagerTemplate template={init,exec,finish,scenario.module};
        OverlayManager *man=OverlayManager_New(&template,&scenario,HEAP_ID_DEFAULT);CHECK(man);
        CHECK(!OverlayManager_GetData(man));
        for(unsigned frame=0;frame<7;frame++) {
            CHECK(OverlayManager_Run(man)==(frame==6));
            if(frame<6 && scenario.module>=0)CHECK(!hg_overlay_unregister(77));
        }
        CHECK(scenario.init==2 && scenario.exec==3 && scenario.exit==2 && !scenario.scene);
        CHECK(hg_overlay_live()==0);OverlayManager_Delete(man);
    }
    CHECK(hg_overlay_unregister(77));CHECK(!hg_overlay_unregister(77));
    char error[256];CHECK(!hg_field_app_new(540,error,sizeof(error)) && error[0]);
    for(unsigned i=0;i<3;i++) {
        HGFieldApp *app=hg_field_app_new(i==1?63:60,error,sizeof(error));CHECK(app && !error[0]);
        unsigned char pixels[64*64*4];DSCamera camera;hg_field_view_camera(hg_field_app_view(app),&camera,64,64);
        CHECK(hg_field_app_render(app,&camera,pixels,64,64));camera.yaw+=0.3;
        CHECK(hg_field_app_render(app,&camera,pixels,64,64));hg_field_app_free(app);
    }
    hg_field_app_free(NULL);CHECK(!hg_field_app_render(NULL,NULL,NULL,0,0));
    puts("Original overlay manager: yielded init/exec/exit, phase resets, arguments/data, scene frames, module residency and reload passed");return 0;
}
