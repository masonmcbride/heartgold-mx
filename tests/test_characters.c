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
    /* Compare every opaque source texel at its projected center. Running
     * frames have hat/arm pixels outside the old center-16 crop. Checking all
     * 32 frames also catches clipping in standing and walking silhouettes.
     * The fixed camera independently checks one model unit per horizontal
     * texel, original 34-unit height and a stable, centered canvas. */
    CHECK(native_resource_member(NARC_data_mmodel_mmodel,0,69,&bytes,&size));CHECK(ds_texture_bank(bytes,size,&bank));
    char palette[17];CHECK(ds_texture_name(&bank,0,1,palette));
    DSCamera canvas={.scale=4,.y=19},billboard_camera={.scale=4,.x=64,.y=17.1,.yaw=0.7};
    unsigned char pixels[160*160*4],composited[sizeof(pixels)];unsigned formerly_clipped=0;
    /* Offscreen character geometry supplies a small background fixture; the
     * production shared-depth/billboard path must preserve the same texels. */
    HGFieldView *background=hg_character_view_load(146,error,sizeof(error));CHECK(background);
    HGFieldBillboard actor={.sprite=hero,.world={64,0,0}};
    for(unsigned f=1;f<=32;f++) {
        char texture_name[17];snprintf(texture_name,sizeof(texture_name),"hero.%u",f);DSTexture texture;
        CHECK(ds_texture_find(&bank,texture_name,palette,&texture)&&texture.width==32&&texture.height==32);
        CHECK(hg_player_sprite_frame(hero,f)&&hg_field_view_render(hero,&canvas,pixels,160,160));
        CHECK(hg_field_view_render_actors(background,&actor,1,&billboard_camera,composited,160,160));
        for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++) {
            uint32_t expected;CHECK(ds_texture_sample(&texture,x,y,0,&expected));if(!(expected>>24))continue;
            unsigned px=18+4*x,py=(unsigned)(12+4.25*(y+0.5));const unsigned char *actual=pixels+4*(py*160+px);
            int matches=abs((int)actual[0]-(uint8_t)expected)<=1&&abs((int)actual[1]-(uint8_t)(expected>>8))<=1&&abs((int)actual[2]-(uint8_t)(expected>>16))<=1;
            if(!matches)
                fprintf(stderr,"Clipped/misplaced hero frame %u texel %u,%u at %u,%u: %u,%u,%u expected %u,%u,%u\n",f,x,y,px,py,actual[0],actual[1],actual[2],(uint8_t)expected,(uint8_t)(expected>>8),(uint8_t)(expected>>16));
            CHECK(matches); /* Allow one RGB unit of raster interpolation rounding. */
            actual=composited+4*(py*160+px);
            CHECK(abs((int)actual[0]-(uint8_t)expected)<=1&&abs((int)actual[1]-(uint8_t)(expected>>8))<=1&&abs((int)actual[2]-(uint8_t)(expected>>16))<=1);
            formerly_clipped+=x<8||x>=24;
        }
    }
    CHECK(formerly_clipped>300);hg_field_view_free(background);
    CHECK(!hg_player_sprite_frame(hero,0)&&!hg_player_sprite_frame(hero,33));hg_field_view_free(hero);
    printf("Character catalog: %u rendered, %u explicitly unsupported; all 16 gym leaders, orbit, texture identity and billboard isolation passed\n",successes,unsupported);return 0;
}
