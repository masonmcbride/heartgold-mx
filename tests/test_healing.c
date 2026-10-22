#include "compat/motion.h"
#include "compat/healing.h"
#include "compat/story.h"
#include "compat/resources.h"
#include "field_data.h"
#include "ds_audio.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static unsigned hash(const u8 *p,size_t n){unsigned h=2166136261u;for(size_t i=0;i<n;i++)h=(h^p[i])*16777619u;return h;}
int main(void){
    const u8 *bytes;size_t n;DSTextureAnimation animation;
    CHECK(native_resource_member(NARC_a_1_0_6,0,7,&bytes,&n));CHECK(ds_texture_animation_open(bytes,n,&animation));
    CHECK(animation.frames==68&&animation.materials==1);
    char tex[17],pal[17];CHECK(ds_texture_animation_frame(&animation,0,"pc_mb",tex,pal)==1&&!strcmp(tex,"pc_mb.1"));
    CHECK(ds_texture_animation_frame(&animation,4,"pc_mb",tex,pal)==1&&!strcmp(tex,"pc_mb.2"));
    CHECK(ds_texture_animation_frame(&animation,68,"pc_mb",tex,pal)==-1);
    DSTextureAnimation untouched={.frames=123};u8 *bad=malloc(n);CHECK(bad);memcpy(bad,bytes,n);bad[0]=0;CHECK(!ds_texture_animation_open(bad,n,&untouched)&&untouched.frames==123);
    memcpy(bad,bytes,n);bad[0x7e]=255;CHECK(!ds_texture_animation_open(bad,n,&untouched)&&untouched.frames==123);free(bad);
    HGFieldResources *resources=hg_field_resources_new(69);HGFieldView *view=hg_field_view_new(69);CHECK(resources&&view);
    LocalMapObject object={0};PlayerAvatar avatar={.mapObject=&object};MapObject_SetCurrentX(&object,8);MapObject_SetCurrentZ(&object,13);
    Location location={.mapId=69};FieldSystem field={.playerAvatar=&avatar,.location=&location,.native_scene_resources=resources};
    DSCamera camera;hg_field_view_focus(view,(double[]){128,0,144},&camera,256,192);
    u8 *pixels=malloc(256*192*4),*baseline=malloc(256*192*4);CHECK(pixels&&baseline);CHECK(hg_field_view_render(view,&camera,baseline,256,192));
    for(unsigned count=1;count<=6;count++){
        field.native_audio=ds_audio_new();field.native_prop_animations=hg_prop_animations_new(69);CHECK(field.native_audio&&field.native_prop_animations);
        CHECK(!hg_healing_new(&field,0)&&!hg_healing_new(&field,7));
        HGHealing *healing=hg_healing_new(&field,count);CHECK(healing);unsigned ticks=0,max_balls=0;int changed=0;
        while(hg_healing_busy(healing)&&ticks++<1200){
            CHECK(hg_prop_animations_tick(field.native_prop_animations)&&hg_healing_tick(healing,&field)&&ds_audio_tick(field.native_audio));
            CHECK(hg_healing_present(healing,field.native_prop_animations,view));
            CHECK(hg_field_view_render(view,&camera,pixels,256,192));changed|=hash(pixels,256*192*4)!=hash(baseline,256*192*4);
            const HGFieldView *balls=hg_healing_view(healing,field.native_prop_animations);
            if(balls){CHECK(hg_field_view_triangles(balls));unsigned triangles=(unsigned)hg_field_view_triangles(balls);if(triangles>max_balls)max_balls=triangles;}
            if(ticks==20){
                HGHealing *copy=hg_healing_clone(healing);HGPropAnimations *animations=hg_prop_animations_clone(field.native_prop_animations);DSAudio *audio=ds_audio_clone(field.native_audio);CHECK(copy&&animations&&audio);
                FieldSystem cloned=field;cloned.native_prop_animations=animations;cloned.native_audio=audio;
                CHECK(hg_healing_tick(copy,&cloned));hg_healing_free(copy);hg_prop_animations_free(animations);ds_audio_free(audio);
                CHECK(hg_healing_busy(healing));
            }
        }
        CHECK(ticks<1200&&changed&&max_balls==count*2);
        CHECK(hg_healing_present(healing,field.native_prop_animations,view)&&!hg_healing_view(healing,field.native_prop_animations));
        CHECK(hg_field_view_render(view,&camera,pixels,256,192)&&!memcmp(pixels,baseline,256*192*4));
        hg_healing_free(healing);hg_prop_animations_free(field.native_prop_animations);ds_audio_free(field.native_audio);
    }
    free(pixels);free(baseline);hg_field_view_free(view);hg_field_resources_free(resources);
    puts("Original healing task: 1-6 balls, texture/palette keyframes, animation/fanfare waits, independent clone cancellation and restored presentation pass");
}
