#include "compat/story.h"
#include "compat/apricorn.h"
#include "motion_guest.h"
#include "field_view.h"
#include "constants/apricorns.h"
#include "constants/sprites.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static void preview(unsigned sprite,const char *name) {
    char error[128];HGFieldView *v=hg_character_sprite_load(sprite,0,error,sizeof(error));if(!v)fprintf(stderr,"%u: %s\n",sprite,error);CHECK(v);
    uint8_t pixels[192*192*4];DSCamera c;hg_field_view_camera(v,&c,192,192);c.yaw=0;c.pitch=0;
    CHECK(hg_field_view_render(v,&c,pixels,192,192));
    const char *dir=getenv("HG_ACCEPT_EVIDENCE");if(dir){char path[4096];CHECK(snprintf(path,sizeof(path),"%s/%s.ppm",dir,name)>0);FILE *f=fopen(path,"wb");CHECK(f);fprintf(f,"P6\n192 192\n255\n");for(unsigned i=0;i<192*192;i++)CHECK(fwrite(pixels+4*i,1,3,f)==3);CHECK(!fclose(f));}
    hg_field_view_free(v);
}
int main(void) {
    HGStoryState *live=hg_story_new(),*copy=hg_story_clone(live);CHECK(live&&copy);
    unsigned kind=99,sprite=0;int available=-1;
    CHECK(hg_story_apricorn_tree(live,5,&kind,&available)&&kind==APRICORN_GRN&&available);
    CHECK(hg_motion_apricorn_sprite(kind,available,&sprite)&&sprite==SPRITE_BONGURI_G);
    preview(sprite,"apricorn-green");preview(SPRITE_TREE,"cuttable-tree");preview(SPRITE_MONSTARBALL,"item-ball");
    FieldSystem private_field={0};hg_story_bind_field(copy,&private_field);
    sub_0202AE68(hg_story_save_apricorn_trees(private_field.saveData),5);
    CHECK(hg_story_population_changed(live,copy));
    CHECK(hg_story_apricorn_tree(copy,5,&kind,&available)&&!available);
    CHECK(hg_motion_apricorn_sprite(kind,available,&sprite)&&sprite==SPRITE_BONGURI);preview(sprite,"apricorn-empty");
    CHECK(hg_story_apricorn_tree(live,5,&kind,&available)&&available);
    CHECK(hg_story_commit(live,copy,hg_story_revision(live)));
    CHECK(hg_story_apricorn_tree(live,5,&kind,&available)&&!available);
    char error[128];CHECK(hg_story_enter_map(live,33,error,sizeof(error)));
    CHECK(hg_story_apricorn_tree(live,5,&kind,&available)&&!available); /* No refill on travel. */
    kind=99;available=-1;CHECK(!hg_story_apricorn_tree(live,NUM_APRICORN_TREE,&kind,&available)&&kind==99&&available==-1);
    CHECK(!hg_motion_apricorn_sprite(8,1,&sprite));
    for(unsigned i=0;i<NUM_APRICORN_TREE;i++){CHECK(hg_story_apricorn_tree(copy,i,&kind,&available)&&kind>=1&&kind<=7);CHECK(hg_motion_apricorn_sprite(kind,1,&sprite));char name[32];snprintf(name,sizeof(name),"apricorn-kind-%u",kind);preview(sprite,name);}
    hg_story_free(copy);hg_story_free(live);puts("Original Apricorn state, sprite selector, typed templates, private ownership and travel retention passed");return 0;
}
