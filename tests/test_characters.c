#include "field_view.h"
#include "compat/resources.h"
#include "filesystem_files_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
int main(void) {
    CHECK(hg_character_count()==266);CHECK(!strcmp(hg_character_name(146),"GSLEADER1"));
    char error[256];unsigned successes=0,unsupported=0;unsigned char front[96*96*4],turned[sizeof(front)],first[sizeof(front)];
    for(unsigned i=0;i<hg_character_count();i++) {
        HGFieldView *view=hg_character_view_load(i,error,sizeof(error));
        if(!view) { CHECK(error[0]);unsupported++;continue; }
        CHECK(!error[0] && hg_field_view_triangles(view)==2);DSCamera camera;
        hg_field_view_camera(view,&camera,96,96);camera.pitch=0;camera.yaw=0;
        CHECK(hg_field_view_render(view,&camera,front,96,96));successes++;
        if(i==146) { memcpy(first,front,sizeof(first));camera.yaw=0.8;CHECK(hg_field_view_render(view,&camera,turned,96,96));CHECK(memcmp(front,turned,sizeof(front))); }
        if(i==148)CHECK(memcmp(first,front,sizeof(first)));
        hg_field_view_free(view);
    }
    for(unsigned i=0;i<16;i++) { unsigned id=i<8?146+i:162+i-8;HGFieldView *view=hg_character_view_load(id,error,sizeof(error));CHECK(view);hg_field_view_free(view); }
    CHECK(!hg_character_view_load(266,error,sizeof(error)));CHECK(successes>=16 && successes+unsupported==266);
    const uint8_t *bytes;size_t size;DSModel model;DSModelStats stats;
    CHECK(native_resource_member(NARC_data_mmodel_mmodel,0,266,&bytes,&size));CHECK(ds_model_open(bytes,size,0,&model));
    CHECK(!ds_model_scene(&model,NULL,NULL,&stats));CHECK(ds_model_inspection_scene(&model,NULL,NULL,&stats) && stats.triangles==2);
    CHECK(native_resource_member(NARC_data_mmodel_mmodel,0,146,&bytes,&size));DSTextureBank bank;CHECK(ds_texture_bank(bytes,size,&bank));
    char name[17];CHECK(ds_texture_name(&bank,0,0,name));CHECK(!ds_texture_name(&bank,bank.textures,0,name));CHECK(!ds_texture_name(&bank,bank.palettes,1,name));
    HGFieldView *hero=hg_player_sprite_load(error,sizeof(error));CHECK(hero);DSCamera hero_camera;
    hg_field_view_camera(hero,&hero_camera,96,96);hero_camera.pitch=0;hero_camera.yaw=0;
    const unsigned standing[]={1,5,9,13},stride[]={2,6,10,14},other_stride[]={4,8,12,16};
    for(unsigned i=0;i<4;i++) {
        CHECK(hg_player_sprite_frame(hero,standing[i]));CHECK(hg_field_view_render(hero,&hero_camera,front,96,96));
        CHECK(hg_player_sprite_frame(hero,stride[i]));CHECK(hg_field_view_render(hero,&hero_camera,turned,96,96));
        CHECK(memcmp(front,turned,sizeof(front)));
        CHECK(hg_player_sprite_frame(hero,other_stride[i]));CHECK(hg_field_view_render(hero,&hero_camera,first,96,96));
        CHECK(memcmp(front,first,sizeof(front))&&memcmp(turned,first,sizeof(front)));
    }
    for(unsigned i=1;i<=32;i++)CHECK(hg_player_sprite_frame(hero,i));
    CHECK(!hg_player_sprite_frame(hero,0)&&!hg_player_sprite_frame(hero,33));hg_field_view_free(hero);
    printf("Character catalog: %u rendered, %u explicitly unsupported; all 16 gym leaders, orbit, texture identity and billboard isolation passed\n",successes,unsupported);return 0;
}
