#include "field_view.h"
#include "compat/resources.h"
#include "filesystem_files_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
int main(void) {
    HGFieldView *view=hg_field_view_new(60);CHECK(view);CHECK(hg_field_view_triangles(view)==1948);
    uint8_t first[128*128*4],rotated[sizeof(first)],reset[sizeof(first)];DSCamera c,initial;
    hg_field_view_camera(view,&c,128,128);initial=c;
    CHECK(hg_field_view_render(view,&c,first,128,128));unsigned colored=0;
    for(unsigned i=0;i<128*128;i++) { CHECK(first[4*i+3]==255);if(first[4*i+1]>first[4*i] && first[4*i+1]>first[4*i+2])colored++; }
    CHECK(colored>500 && colored<10000);
    c.yaw+=0.7;CHECK(hg_field_view_render(view,&c,rotated,128,128));CHECK(memcmp(first,rotated,sizeof(first)));
    CHECK(hg_field_view_render(view,&initial,reset,128,128));CHECK(!memcmp(first,reset,sizeof(first)));
    c.scale=NAN;CHECK(!hg_field_view_render(view,&c,reset,128,128));hg_field_view_free(view);
    const uint8_t *prop;size_t prop_size;CHECK(native_resource_member(NARC_a_0_4_0,0,28,&prop,&prop_size));
    DSModel prop_model;DSTextureBank embedded;DSModelMaterial prop_material;
    CHECK(ds_model_open(prop,prop_size,0,&prop_model) && ds_texture_bank(prop,prop_size,&embedded));
    CHECK(ds_model_material(&prop_model,0,&prop_material));CHECK(prop_material.translation_uv && prop_material.uv_u==40 && prop_material.uv_v==64);
    for(size_t n=0;n<prop_size;n++) { CHECK(!ds_model_open(prop,n,0,&prop_model));CHECK(!ds_texture_bank(prop,n,&embedded)); }
    char error[256];CHECK(!hg_field_view_load(540,error,sizeof(error)) && strstr(error,"Invalid map"));
    CHECK(!hg_field_view_load(10,error,sizeof(error)) && strstr(error,"prop"));
    CHECK(!hg_field_view_load(0,error,sizeof(error)) && strstr(error,"598 chunks"));
    CHECK(hg_field_map_count()==540 && !strcmp(hg_field_map_name(60),"MAP_NEW_BARK") && !hg_field_map_name(540));
    view=hg_field_view_load(33,error,sizeof(error));CHECK(view && !error[0]);CHECK(hg_field_view_triangles(view)==2179);
    hg_field_view_camera(view,&c,128,128);CHECK(hg_field_view_render(view,&c,first,128,128));
    CHECK(memcmp(first,reset,sizeof(first)));hg_field_view_free(view);
    view=hg_field_view_new(63);CHECK(view && hg_field_view_triangles(view)==707);
    hg_field_view_camera(view,&c,128,128);CHECK(hg_field_view_render(view,&c,first,128,128));
    unsigned occupied=0;for(unsigned i=0;i<128*128;i++)if(first[4*i]!=24 || first[4*i+1]!=32 || first[4*i+2]!=36)occupied++;
    CHECK(occupied>500);hg_field_view_free(view);
    view=hg_field_view_new(539);CHECK(view);hg_field_view_camera(view,&c,128,128);CHECK(hg_field_view_render(view,&c,first,128,128));hg_field_view_free(view);
    view=hg_field_view_new(60);CHECK(view && hg_field_view_triangles(view)==1948);hg_field_view_free(view);
    const uint8_t *bytes;size_t size;CHECK(native_resource_member(NARC_a_0_4_4,0,2,&bytes,&size));DSTextureBank bank;
    CHECK(ds_texture_bank(bytes,size,&bank));CHECK(bank.textures==67 && bank.palettes==66);
    DSTexture texture;CHECK(ds_texture_find(&bank,"road01","road01",&texture));CHECK(texture.format==3);
    CHECK(!ds_texture_find(&bank,"missing","road01",&texture));CHECK(!ds_texture_find(&bank,"road01","missing",&texture));
    for(size_t n=0;n<size;n++)CHECK(!ds_texture_bank(bytes,n,&bank));
    uint8_t *bad=malloc(size);CHECK(bad);
    for(size_t i=0;i<size;i+=37) {
        memcpy(bad,bytes,size);bad[i]^=255;
        if(ds_texture_bank(bad,size,&bank) && ds_texture_find(&bank,"road01","road01",&texture)) {
            uint32_t color;CHECK(ds_texture_sample(&texture,-1.5,1024.25,(1<<16)|(1<<19),&color));
        }
    }
    free(bad);puts("Original New Bark textured scene: binding, frame content, camera/reset and malformed texture resources passed");return 0;
}
