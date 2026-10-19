#include "motion_guest.h"
#include "ds_brightness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
int main(void) {
    HGMotionFade fade={0};CHECK(hg_motion_fade_begin(&fade,0,0,6,1));CHECK(fade.active&&!fade.brightness);
    const int values[]={-2,-5,-7,-10,-13,-16};
    for(unsigned i=0;i<6;i++){CHECK(hg_motion_fade_tick(&fade));CHECK(fade.brightness==values[i]&&fade.active);}
    HGMotionFade copy=fade;CHECK(hg_motion_fade_tick(&fade)&&!fade.active&&fade.brightness==-16);CHECK(copy.active);
    CHECK(hg_motion_fade_tick(&copy)&&!copy.active);CHECK(hg_motion_fade_begin(&copy,1,0,6,2));
    for(unsigned i=0;i<12;i++){CHECK(hg_motion_fade_tick(&copy));CHECK(copy.active);}
    CHECK(!copy.brightness&&hg_motion_fade_tick(&copy)&&!copy.active);CHECK(fade.brightness==-16);
    CHECK(hg_motion_fade_begin(&fade,0,32767,1,1));CHECK(!fade.brightness);
    CHECK(hg_motion_fade_tick(&fade)&&fade.brightness==16);CHECK(hg_motion_fade_tick(&fade)&&!fade.active);
    copy=fade;CHECK(!hg_motion_fade_begin(&fade,2,0,6,1));CHECK(!hg_motion_fade_begin(&fade,0,31,6,1));CHECK(!hg_motion_fade_begin(&fade,0,0,0,1));CHECK(!memcmp(&fade,&copy,sizeof(fade)));
    uint8_t pixel[]={64,128,192,7};CHECK(ds_brightness_apply(pixel,1,-8));CHECK(pixel[0]==32&&pixel[1]==64&&pixel[2]==96&&pixel[3]==7);
    CHECK(ds_brightness_apply(pixel,1,16));CHECK(pixel[0]==255&&pixel[1]==255&&pixel[2]==255&&pixel[3]==7);
    CHECK(ds_brightness_apply(pixel,1,-16));CHECK(!pixel[0]&&!pixel[1]&&!pixel[2]&&pixel[3]==7);
    CHECK(!ds_brightness_apply(pixel,1,17)&&pixel[3]==7);
    puts("Original brightness fade timing, private continuations and native RGBA presentation passed");return 0;
}
