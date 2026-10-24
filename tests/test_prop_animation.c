#include "compat/story.h"
#include "compat/prop_animation.h"
#include "compat/resources.h"
#include "field_view.h"
#include "field_data.h"
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static void frame(HGFieldView *view,const DSCamera *camera,const char *name,uint8_t *pixels) {
    CHECK(hg_field_view_render(view,camera,pixels,512,384));const char *dir=getenv("HG_ACCEPT_EVIDENCE");if(!dir)return;
    char path[4096];CHECK(snprintf(path,sizeof(path),"%s/door-%s.ppm",dir,name)>0);FILE *f=fopen(path,"wb");CHECK(f);fprintf(f,"P6\n512 384\n255\n");
    for(unsigned i=0;i<512*384;i++)CHECK(fwrite(pixels+4*i,1,3,f)==3);CHECK(!fclose(f));
}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void srt_checks(void) {
    unsigned count=0;
    for(unsigned id=0;id<4096;id++) {
        const uint8_t *p;size_t n;if(!native_resource_member(NARC_a_1_0_6,0,id,&p,&n))break;
        if(n<4||memcmp(p,"BTA0",4))continue;DSTextureAnimation a;CHECK(ds_texture_animation_open(p,n,&a)&&a.kind);count++;
        for(unsigned m=0;m<a.materials;m++) {
            char name[17];memcpy(name,a.bytes+a.names+16*m,16);name[16]=0;double first[6],second[6],half[6];
            CHECK(ds_texture_animation_matrix(&a,0,name,64,32,first)==1);
            CHECK(ds_texture_animation_matrix(&a,1,name,64,32,second)==1);
            CHECK(ds_texture_animation_matrix(&a,.5,name,64,32,half)==1);
            for(unsigned k=0;k<6;k++)CHECK(isfinite(half[k])&&fabs(half[k]-(first[k]+second[k])*.5)<1e-8);
            CHECK(ds_texture_animation_matrix(&a,a.frames-1,name,64,32,first)==1);
            CHECK(ds_texture_animation_matrix(&a,a.frames,name,64,32,first)==-1);
        }
    }
    CHECK(count==99);
    const uint8_t *p;size_t n;DSTextureAnimation a;CHECK(native_resource_member(NARC_a_1_0_6,0,32,&p,&n)&&ds_texture_animation_open(p,n,&a));
    for(size_t i=0;i<n;i++){DSTextureAnimation bad;CHECK(!ds_texture_animation_open(p,i,&bad));}
    char name[17];memcpy(name,a.bytes+a.names,16);name[16]=0;double q[6];
    CHECK(ds_texture_animation_matrix(&a,1,name,64,32,q)==1&&fabs(q[4]+64*137/4096.0)<1e-9);
    uint8_t *copy=malloc(n);CHECK(copy);memcpy(copy,p,n);size_t base=(size_t)(a.bytes-p),table=base+a.table;
    uint32_t values[]={8192,2048,4096,1024,(uint32_t)-1024};
    for(unsigned k=0;k<5;k++){put32(copy+table+8*k,(k==2?0x20000000u:0x30000000u)|a.frames);put32(copy+table+8*k+4,values[k]);}
    CHECK(ds_texture_animation_open(copy,n,&a)&&ds_texture_animation_matrix(&a,0,name,64,32,q)==1);
    CHECK(q[0]==0&&q[1]==-.25&&q[2]==4&&q[3]==0&&q[4]==-32&&q[5]==28);
    put32(copy+table,0x70000000u|a.frames);CHECK(!ds_texture_animation_open(copy,n,&a));free(copy);
}
static void initialization_checks(void){
    const uint8_t *p;size_t n;DSAnimationResource a;
    const unsigned members[]={168,166};const unsigned kinds[]={DS_ANIMATION_COLOR,DS_ANIMATION_JOINT};
    for(unsigned k=0;k<2;k++){
        CHECK(native_resource_member(NARC_a_1_0_3,0,members[k],&p,&n));
        CHECK(ds_animation_resource_open(p,n,0,&a)&&a.kind==kinds[k]&&a.frames);
        CHECK(a.bytes==p&&a.record>=p&&a.record+a.record_size==p+n);
        CHECK(!ds_animation_resource_open(p,n,1,&a));
        for(size_t truncated=0;truncated<n;truncated++)CHECK(!ds_animation_resource_open(p,truncated,0,&a));
        uint8_t *copy=malloc(n);CHECK(copy);memcpy(copy,p,n);copy[4]=0;
        CHECK(!ds_animation_resource_open(copy,n,0,&a));free(copy);
    }
    /* An SDK init validates mapping metadata, not sample curve encodings. */
    CHECK(native_resource_member(NARC_a_1_0_6,0,32,&p,&n)&&ds_animation_resource_open(p,n,0,&a));
    uint8_t *copy=malloc(n);CHECK(copy);memcpy(copy,p,n);
    put32(copy+(a.record-p)+a.table,0x70000000u|a.frames);
    CHECK(ds_animation_resource_open(copy,n,0,&a));
    DSTextureAnimation sampled;CHECK(!ds_texture_animation_open(copy,n,&sampled));free(copy);
}
int main(void) {
    initialization_checks();srt_checks();
    HGStoryState *story=hg_story_new();HGFieldResources *resources=hg_field_resources_new(60);HGFieldView *view=hg_field_view_new(60);HGPropAnimations *s=hg_prop_animations_new(60);CHECK(story&&resources&&view&&s);
    FieldSystem field={.native_prop_animations=s};hg_story_bind_field(story,&field);
    const uint8_t init[]={21,0,12,0,12,0,9,0,77};ScriptContext ctx={.fieldSystem=&field,.script_ptr=init};
    CHECK(hg_prop_animations_validate(s,resources,&ctx,307));ov01_021E9AE8(&field,684,393,77);
    CHECK(!hg_prop_animations_validate(s,resources,&ctx,307)); /* Duplicate tag. */
    HGPropPose poses[16];CHECK(!hg_prop_animations_poses(s,poses));
    const uint8_t tag[]={77};ctx.script_ptr=tag;CHECK(!hg_prop_animations_validate(s,resources,&ctx,308));
    CHECK(hg_prop_animations_validate(s,resources,&ctx,310));ov01_021E9BB8(&field,77);CHECK(hg_prop_animations_poses(s,poses)==1);
    CHECK(poses[0].model==26&&poses[0].pose.mask==1&&poses[0].pose.nodes[0].m[0]==1);
    DSCamera camera;hg_field_view_focus(view,poses[0].world,&camera,512,384);camera.scale=5;camera.y=12;
    uint8_t *closed=malloc(512*384*4),*opened=malloc(512*384*4),*reset=malloc(512*384*4);CHECK(closed&&opened&&reset);
    CHECK(hg_field_view_prop_poses(view,poses,1));frame(view,&camera,"closed",closed);
    ov01_021E9C00(&field,77);unsigned ticks=0;
    while(hg_prop_animations_busy(s)) {
        CHECK(hg_prop_animations_tick(s));ticks++;CHECK(ticks<=8);
        CHECK(hg_prop_animations_poses(s,poses)==1&&hg_field_view_prop_poses(view,poses,1));
        if(ticks==4) {
            HGPropAnimations *copy=hg_prop_animations_clone(s);HGPropPose cloned[16];CHECK(copy&&hg_prop_animations_busy(copy));
            CHECK(hg_prop_animations_poses(copy,cloned)==1&&!memcmp(cloned[0].pose.nodes,poses[0].pose.nodes,sizeof(poses[0].pose.nodes)));
            CHECK(hg_prop_animations_tick(copy));hg_prop_animations_free(copy); /* Never advances source. */
            CHECK(hg_prop_animations_poses(s,cloned)==1&&!memcmp(cloned[0].pose.nodes,poses[0].pose.nodes,sizeof(poses[0].pose.nodes)));
            frame(view,&camera,"middle",opened);
        }
    }
    CHECK(ticks==7);frame(view,&camera,"open",opened);CHECK(memcmp(closed,opened,512*384*4));
    CHECK(fabs(poses[0].pose.nodes[0].m[0])<0.001);
    HGPropPose rejected[2]={poses[0],poses[0]};rejected[0].pose.nodes[0]=ds_transform_identity();rejected[1].world[0]+=5000;
    CHECK(!hg_field_view_prop_poses(view,rejected,2));frame(view,&camera,"rejected",reset);CHECK(!memcmp(opened,reset,512*384*4));
    ov01_021E9BDC(&field,77);ov01_021E9C00(&field,77);ticks=0;
    while(hg_prop_animations_busy(s)){CHECK(hg_prop_animations_tick(s));CHECK(++ticks<9);}
    CHECK(ticks==7&&hg_prop_animations_poses(s,poses)==1&&hg_field_view_prop_poses(view,poses,1));
    frame(view,&camera,"closed-again",reset);CHECK(!memcmp(closed,reset,512*384*4));
    ov01_021E9C20(&field,77);CHECK(!hg_prop_animations_poses(s,poses)&&hg_field_view_prop_poses(view,NULL,0));
    frame(view,&camera,"restored",reset);CHECK(!memcmp(closed,reset,512*384*4));
    const uint8_t *bytes;size_t size;DSJointAnimation animation;CHECK(native_resource_member(NARC_a_1_0_6,0,1,&bytes,&size));
    CHECK(ds_joint_animation_open(bytes,size,&animation));DSJointPose untouched={.mask=123};
    CHECK(!ds_joint_animation_pose(&animation,animation.frames,&untouched)&&untouched.mask==123);
    for(size_t n=0;n<size;n++)CHECK(!ds_joint_animation_open(bytes,n,&animation));
    uint8_t *mutated=malloc(size);CHECK(mutated);memcpy(mutated,bytes,size);mutated[0x64]=0xff;mutated[0x65]=0xff;CHECK(!ds_joint_animation_open(mutated,size,&animation));free(mutated);
    free(closed);free(opened);free(reset);hg_prop_animations_free(s);hg_field_view_free(view);hg_field_resources_free(resources);hg_story_free(story);
    puts("Original one-shot prop playback: door frames, loop waits, private clone, rendered opening/closing and restoration passed");return 0;
}
