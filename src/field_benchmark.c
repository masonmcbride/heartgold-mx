/* Diagnostic measurement, never a timing-sensitive pass/fail test. */
#include "field_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double seconds(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;
}
int main(void) {
    char error[256];HGFieldApp *app=hg_field_app_new(60,error,sizeof(error));
    if(!app){fprintf(stderr,"%s\n",error);return 1;}
    unsigned char *pixels=malloc(800*600*4);if(!pixels){hg_field_app_free(app);return 1;}
    DSCamera camera;hg_field_app_camera(app,&camera,800,600);
    double start=seconds();
    for(unsigned i=0;i<30;i++)if(!hg_field_app_render(app,&camera,pixels,800,600)){free(pixels);hg_field_app_free(app);return 1;}
    double render_ms=(seconds()-start)*1000/30;
    hg_field_app_input(app,0x40);start=seconds();
    for(unsigned i=0;i<240;i++)hg_field_app_step(app);
    double simulation_ms=(seconds()-start)*1000;
    printf("{\"width\":800,\"height\":600,\"render_frames\":30,\"render_ms_per_frame\":%.3f,\"simulation_ticks\":240,\"simulation_ms\":%.3f}\n",render_ms,simulation_ms);
    free(pixels);hg_field_app_free(app);return 0;
}
