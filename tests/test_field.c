#include "field_data.h"
#include "ds_model.h"
#include "ds_texture.h"
#include "map_header.h"
#include "map_matrix.h"
#include "metatile_behavior.h"
#include "constants/maps.h"
#include "constants/metatile_behavior.h"
#include "compat/resources.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static u16 read16(const u8 *p) { return p[0]|(u16)p[1]<<8; }
int main(void) {
    unsigned members=0;const u8 *bytes;size_t size;
    while(native_resource_member(NARC_fielddata_mapmatrix_map_matrix,0,(int)members,&bytes,&size)) { CHECK(hg_matrix_validate(bytes,size,NULL));members++; }
    CHECK(members==288);
    CHECK(native_resource_member(NARC_fielddata_mapmatrix_map_matrix,0,0,&bytes,&size));
    HGMatrixLayout layout;CHECK(hg_matrix_validate(bytes,size,&layout));CHECK(layout.width==47 && layout.height==17 && layout.cells==799);
    for(size_t n=0;n<size;n++)CHECK(!hg_matrix_validate(bytes,n,NULL));
    u8 *bad=malloc(size);CHECK(bad);memcpy(bad,bytes,size);bad[0]=255;bad[1]=255;CHECK(!hg_matrix_validate(bad,size,NULL));
    memcpy(bad,bytes,size);bad[4]=17;CHECK(!hg_matrix_validate(bad,size,NULL));memcpy(bad,bytes,size);bad[2]=2;CHECK(!hg_matrix_validate(bad,size,NULL));free(bad);
    unsigned lands=0,metadata_mismatches=0;
    while(native_resource_member(NARC_fielddata_landdata_land_data,0,(int)lands,&bytes,&size)) {
        HGLandChunk chunk;CHECK(hg_land_decode(bytes,size,&chunk));
        HGCollision collision;CHECK(hg_collision_decode(chunk.collision,chunk.collision_size,&collision));
        DSModel model;CHECK(ds_model_open(chunk.model,chunk.model_size,0,&model));
        size_t mesh_vertices=0,mesh_triangles=0;
        for(unsigned shape=0;shape<model.shapes;shape++) {
            const u8 *dl;size_t dl_size;DSModelStats stats;
            CHECK(ds_model_shape(&model,shape,&dl,&dl_size));
            if(!ds_model_geometry(dl,dl_size,NULL,NULL,&stats)) { fprintf(stderr,"land=%u shape=%u unsupported geometry\n",lands,shape);return 1; }
            mesh_vertices+=stats.vertices;mesh_triangles+=stats.triangles;
        }
        if(mesh_vertices!=model.declared_vertices || mesh_triangles!=model.declared_triangles+2*model.declared_quads) {
            CHECK(lands==468 || lands==473 || lands==475 || lands==476);metadata_mismatches++;
        }
        DSModelStats scene;
        if(!ds_model_scene(&model,NULL,NULL,&scene)) { fprintf(stderr,"Scene failed land %u\n",lands);return 1; }
        CHECK(scene.vertices==mesh_vertices && scene.triangles==mesh_triangles);
        for(unsigned z=0;z<32;z++)for(unsigned x=0;x<32;x++) {
            u16 attr;CHECK(hg_land_tile(&chunk,x,z,&attr));
            CHECK(attr==read16(chunk.attributes+2*(z*32+x)));
            (void)MetatileBehavior_IsSurfableWater((u8)attr);
            (void)MetatileBehavior_IsTallGrass((u8)attr);
        }
        u16 attr;CHECK(!hg_land_tile(&chunk,32,0,&attr));CHECK(!hg_land_tile(&chunk,0,32,&attr));
        lands++;
    }
    CHECK(lands==676 && metadata_mismatches==4);
    CHECK(native_resource_member(NARC_fielddata_landdata_land_data,0,0,&bytes,&size));
    HGLandChunk bark;CHECK(hg_land_decode(bytes,size,&bark));CHECK(bark.extra_size==88);
    HGCollision collision;CHECK(hg_collision_decode(bark.collision,bark.collision_size,&collision));
    CHECK(collision.point_count==37 && collision.normal_count==2 && collision.constant_count==3 && collision.plane_count==20 && collision.strip_count==7 && collision.index_count==47);
    for(size_t n=0;n<bark.collision_size;n++)CHECK(!hg_collision_decode(bark.collision,n,&collision));
    u8 collision_bad[658];memcpy(collision_bad,bark.collision,sizeof(collision_bad));
    collision_bad[4]=255;collision_bad[5]=255;CHECK(!hg_collision_decode(collision_bad,sizeof(collision_bad),&collision));
    CHECK(hg_collision_decode(bark.collision,bark.collision_size,&collision));
    const u8 *collision_tables[]={collision.planes,collision.strips+4,collision.indices};
    for(unsigned i=0;i<3;i++) {
        memcpy(collision_bad,bark.collision,sizeof(collision_bad));size_t off=(size_t)(collision_tables[i]-bark.collision);
        collision_bad[off]=255;collision_bad[off+1]=255;CHECK(!hg_collision_decode(collision_bad,sizeof(collision_bad),&collision));
    }
    DSModel new_bark_model;CHECK(ds_model_open(bark.model,bark.model_size,0,&new_bark_model));
    CHECK(!strcmp(new_bark_model.name,"map21_12c") && new_bark_model.shapes==18);
    size_t vertex_count=0,triangle_count=0;
    for(unsigned i=0;i<new_bark_model.shapes;i++) {
        const u8 *dl;size_t n;DSModelStats stats;CHECK(ds_model_shape(&new_bark_model,i,&dl,&n));CHECK(ds_model_geometry(dl,n,NULL,NULL,&stats));
        vertex_count+=stats.vertices;triangle_count+=stats.triangles;
    }
    CHECK(vertex_count==1252 && triangle_count==654);
    for(size_t n=0;n<bark.model_size;n++)CHECK(!ds_model_open(bark.model,n,0,&new_bark_model));
    u8 *model_bad=malloc(bark.model_size);CHECK(model_bad);
    /* Deterministic mutations probe nested dictionary/mesh boundaries under ASan. */
    for(size_t i=0;i<bark.model_size;i+=7) {
        memcpy(model_bad,bark.model,bark.model_size);model_bad[i]^=255;
        DSModel mutated;if(ds_model_open(model_bad,bark.model_size,0,&mutated)) {
            DSModelStats scene;(void)ds_model_scene(&mutated,NULL,NULL,&scene);
            for(unsigned j=0;j<mutated.shapes;j++) {
                const u8 *dl;size_t n;DSModelStats stats;
                CHECK(ds_model_shape(&mutated,j,&dl,&n));(void)ds_model_geometry(dl,n,NULL,NULL,&stats);
            }
        }
    }
    free(model_bad);
    FILE *preview=tmpfile();CHECK(preview);CHECK(hg_field_wireframe(MAP_NEW_BARK,preview));
    CHECK(ftell(preview)==15+640*640*3);rewind(preview);char ppm_header[16]={0};CHECK(fread(ppm_header,1,15,preview)==15);
    CHECK(!strcmp(ppm_header,"P6\n640 640\n255\n"));fclose(preview);
    preview=tmpfile();CHECK(preview);CHECK(hg_field_solid(MAP_NEW_BARK,preview));CHECK(ftell(preview)==15+640*640*3);
    CHECK(fseek(preview,15,SEEK_SET)==0);unsigned shaded_pixels=0;
    for(unsigned i=0;i<640*640;i++) { unsigned char pixel[3];CHECK(fread(pixel,1,3,preview)==3);if(pixel[1]>pixel[0])shaded_pixels++; }
    CHECK(shaded_pixels>50000 && shaded_pixels<200000);fclose(preview);
    const u8 *tex_bytes;size_t tex_size;DSTextureBank bank;
    CHECK(native_resource_member(NARC_a_0_4_4,0,2,&tex_bytes,&tex_size));CHECK(ds_texture_bank(tex_bytes,tex_size,&bank));
    for(unsigned i=0;i<new_bark_model.materials;i++) {
        DSModelMaterial mat;CHECK(ds_model_material(&new_bark_model,i,&mat));
        if(mat.texture[0]) { DSTexture texture;if(!ds_texture_find(&bank,mat.texture,mat.palette,&texture)) { fprintf(stderr,"Failed binding material %u: %s/%s\n",i,mat.texture,mat.palette);return 1; } }
    }
    CHECK(bark.model_size==16604 && bark.props_size==816 && bark.collision_size==658);
    unsigned grass=0,water=0;
    for(unsigned z=0;z<32;z++)for(unsigned x=0;x<32;x++) {
        u16 attr;CHECK(hg_land_tile(&bark,x,z,&attr));
        grass+=!!MetatileBehavior_IsTallGrass((u8)attr);water+=!!MetatileBehavior_IsSurfableWater((u8)attr);
    }
    CHECK(grass==0 && water==15);
    CHECK(hg_land_decode(bytes,size,&bark));
    for(unsigned z=0;z<32;z++)for(unsigned x=0;x<32;x++) {
        u16 direct,world;CHECK(hg_land_tile(&bark,x,z,&direct));
        CHECK(hg_field_attribute(MAP_NEW_BARK,21*32+x,12*32+z,&world));CHECK(world==direct);
    }
    HGFieldResources *persistent=hg_field_resources_new(MAP_NEW_BARK);CHECK(persistent);
    CHECK(hg_field_resources_loaded(persistent)==0);
    HGFieldTile query;
    for(unsigned pass=0;pass<3;pass++)for(int z=0;z<32;z++)for(int x=0;x<32;x++) {
        u16 direct;CHECK(hg_land_tile(&bark,(unsigned)x,(unsigned)z,&direct));
        CHECK(hg_field_resources_tile(persistent,21*32+x,12*32+z,&query));
        CHECK(query.attribute==direct && query.map_header==MAP_NEW_BARK && query.land_member==0);
        CHECK(query.matrix_x==21 && query.matrix_z==12 && query.land.model==bark.model);
        CHECK(query.collision.plane_count==20 && query.collision.strip_count==7);
    }
    CHECK(hg_field_resources_loaded(persistent)==1);
    HGFieldTile unchanged=query;
    CHECK(!hg_field_resources_tile(persistent,-1,0,&query));
    CHECK(!hg_field_resources_tile(persistent,0,-1,&query));
    CHECK(!hg_field_resources_tile(persistent,47*32,0,&query));
    CHECK(!hg_field_resources_tile(persistent,0,17*32,&query));
    CHECK(!memcmp(&query,&unchanged,sizeof(query)));
    CHECK(!hg_field_resources_tile(NULL,0,0,&query));
    CHECK(!hg_field_resources_tile(persistent,0,0,NULL));
    CHECK(hg_field_resources_tile(persistent,20*32+31,12*32,&query));
    CHECK(hg_field_resources_tile(persistent,22*32,12*32,&query));
    CHECK(hg_field_resources_loaded(persistent)==3);
    hg_field_resources_free(persistent);
    persistent=hg_field_resources_new(MAP_NEW_BARK_PLAYER_HOUSE_1F);CHECK(persistent);
    CHECK(hg_field_resources_tile(persistent,0,0,&query));
    CHECK(query.map_header==MAP_NEW_BARK_PLAYER_HOUSE_1F);
    CHECK(!hg_field_resources_tile(persistent,32,0,&query));
    hg_field_resources_free(persistent);hg_field_resources_free(NULL);
    CHECK(!hg_field_resources_new(MAP_ID_MAX));
    u16 world_attr;
    CHECK(!hg_field_attribute(MAP_NEW_BARK,47*32,0,&world_attr));
    CHECK(!hg_field_attribute(MAP_NEW_BARK,0,17*32,&world_attr));
    CHECK(!hg_field_attribute(MAP_ID_MAX,0,0,&world_attr));
    CHECK(hg_field_attribute(MAP_NEW_BARK,20*32+31,12*32,&world_attr));
    CHECK(hg_field_attribute(MAP_NEW_BARK,22*32,12*32,&world_attr));
    printf("New Bark actual attributes: grass=%u water=%u; %u land chunks validated\n",grass,water,lands);
    for(size_t n=0;n<size;n++)CHECK(!hg_land_decode(bytes,n,&bark));
    u8 *land_bad=malloc(size+1);CHECK(land_bad);memcpy(land_bad,bytes,size);
    land_bad[0]=1;CHECK(!hg_land_decode(land_bad,size,&bark));memcpy(land_bad,bytes,size);
    land_bad[18]=255;land_bad[19]=255;CHECK(!hg_land_decode(land_bad,size,&bark));
    memcpy(land_bad,bytes,size);land_bad[8]=255;land_bad[9]=255;land_bad[10]=255;land_bad[11]=255;
    CHECK(!hg_land_decode(land_bad,size,&bark));memcpy(land_bad,bytes,size);land_bad[size]=0;
    CHECK(!hg_land_decode(land_bad,size+1,&bark));
    CHECK(hg_land_decode(bytes,size,&bark));size_t model_offset=(size_t)(bark.model-bytes);
    const unsigned corrupt_offsets[]={4,6,12,14,16,20,24};
    for(unsigned i=0;i<sizeof(corrupt_offsets)/sizeof(corrupt_offsets[0]);i++) {
        memcpy(land_bad,bytes,size);land_bad[model_offset+corrupt_offsets[i]]^=0xff;
        CHECK(!hg_land_decode(land_bad,size,&bark));
    }
    CHECK(!hg_land_decode(NULL,size,&bark));CHECK(!hg_land_decode(bytes,size,NULL));
    free(land_bad);
    unsigned used[288]={0},unique=0;
    for(unsigned map=0;map<MAP_ID_MAX;map++) {
        u16 id=MapHeader_GetMatrixId(map);CHECK(id<members);used[id]=1;
        CHECK(native_resource_member(NARC_fielddata_mapmatrix_map_matrix,0,id,&bytes,&size));CHECK(hg_matrix_validate(bytes,size,&layout));
        MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);
        CHECK(MapMatrix_GetWidth(matrix)==layout.width && MapMatrix_GetHeight(matrix)==layout.height);
        CHECK(MapMatrix_GetMatrixId(matrix)==(u8)id); /* Original field stores one byte. */
        size_t headers=5+layout.name_length,altitudes=headers+(layout.has_headers?layout.cells*2:0),models=altitudes+(layout.has_altitudes?layout.cells:0);
        for(unsigned y=0;y<layout.height;y++)for(unsigned x=0;x<layout.width;x++) {
            size_t i=y*layout.width+x;
            CHECK(MapMatrix_GetMapHeader(matrix,(s32)x,(s32)y)==(layout.has_headers?read16(bytes+headers+i*2):map));
            CHECK(MapMatrix_GetMapAltitude(matrix,(u8)id,(u16)x,(u16)y,(int)layout.width)==(layout.has_altitudes?bytes[altitudes+i]:0));
            CHECK(MapMatrix_GetMapModelNo((int)i,matrix)==read16(bytes+models+i*2));
        }
        MapMatrix_Free(matrix);
    }
    for(unsigned i=0;i<288;i++)unique+=used[i];
    CHECK(MapHeader_IsOutdoors(MAP_NEW_BARK));CHECK(MapHeader_IsBikeAllowed(MAP_NEW_BARK));CHECK(MapHeader_MapIsOnMainMatrix(MAP_NEW_BARK));
    CHECK(!MapHeader_IsOutdoors(MAP_NEW_BARK_PLAYER_HOUSE_1F));CHECK(MapHeader_IsInBuilding(MAP_NEW_BARK_PLAYER_HOUSE_1F));
    MapMatrix *world=MapMatrix_New();MapMatrix_Load(MAP_NEW_BARK,world);
    CHECK(MapMatrix_GetMapHeader(world,21,12)==MAP_NEW_BARK);CHECK(MapMatrix_GetMapModelNo(12*47+21,world)==0);
    SetLakeOfRageWaterLevel(world,TRUE);CHECK(MapMatrix_GetMapModelNo(47+15,world)==95);CHECK(MapMatrix_GetMapModelNo(2*47+17,world)==100);
    SetLakeOfRageWaterLevel(world,FALSE);CHECK(MapMatrix_GetMapModelNo(47+15,world)==89);CHECK(MapMatrix_GetMapModelNo(2*47+17,world)==94);
    RemoveMahoganyTownAntennaTree(world);CHECK(MapMatrix_GetMapModelNo(5*47+16,world)==86);MapMatrix_Free(world);
    MapMatrix *house=MapMatrix_New();MapMatrix_Load(MAP_NEW_BARK_PLAYER_HOUSE_1F,house);u16 model=MapMatrix_GetMapModelNo(0,house);
    SetLakeOfRageWaterLevel(house,TRUE);RemoveMahoganyTownAntennaTree(house);CHECK(MapMatrix_GetMapModelNo(0,house)==model);MapMatrix_Free(house);
    CHECK(MetatileBehavior_IsTallGrass(TILE_BEHAVIOR_TALL_GRASS));CHECK(!MetatileBehavior_IsTallGrass(TILE_BEHAVIOR_WATER_RIVER));
    CHECK(MetatileBehavior_CanGenerateWalkingEncounters(TILE_BEHAVIOR_TALL_GRASS));CHECK(MetatileBehavior_IsSurfableWater(TILE_BEHAVIOR_WATER_RIVER));
    CHECK(!MetatileBehavior_IsSurfableWater(TILE_BEHAVIOR_TALL_GRASS));CHECK(MetatileBehavior_IsDoor(TILE_BEHAVIOR_DOOR));
    CHECK(MetatileBehavior_IsIce(TILE_BEHAVIOR_ICE));CHECK(MetatileBehavior_IsWaterfall(TILE_BEHAVIOR_WATERFALL));CHECK(MetatileBehavior_IsMagma(TILE_BEHAVIOR_MAGMA));
    CHECK(MetatileBehavior_IsJumpNorth(TILE_BEHAVIOR_JUMP_NORTH));CHECK(!MetatileBehavior_IsJumpSouth(TILE_BEHAVIOR_JUMP_NORTH));
    CHECK(MetatileBehavior_IsNone(GetMetatileBehavior_None()));
    for(unsigned i=0;i<256;i++) { CHECK(MetatileBehavior_IsSurfableWater((u8)i)==MetatileBehavior_IsSurfableWater_thunk((u8)i));(void)MetatileBehavior_CanGenerateWalkingEncounters((u8)i); }
    printf("Original field tests passed: %u map headers loaded, %u distinct matrices, %u resources validated, original matrix mutations and tile predicates\n",MAP_ID_MAX,unique,members);return 0;
}
