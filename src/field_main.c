#include "field_data.h"
#include "compat/resources.h"
#include "map_header.h"
#include "map_matrix.h"
#include "metatile_behavior.h"
#include "constants/maps.h"
#include "field_names.inc"
#include <errno.h>
static int parse(const char *text,unsigned max,unsigned *out) {
    if(!text || *text<'0' || *text>'9')return 0;char *end;errno=0;unsigned long value=strtoul(text,&end,10);
    if(errno || *end || value>max)return 0;*out=(unsigned)value;return 1;
}
int main(int argc,char **argv) {
    unsigned map=MAP_NEW_BARK,tile=0;int tile_mode=0,terrain_mode=0;
    if(argc!=1 && (argc!=3 || (strcmp(argv[1],"--map") && strcmp(argv[1],"--tile") && strcmp(argv[1],"--terrain") && strcmp(argv[1],"--wireframe") && strcmp(argv[1],"--solid")))) { fprintf(stderr,"Usage: heartgold_field [--map 0..539 | --tile 0..255 | --terrain 0..539 | --wireframe 0..539 | --solid 0..539]\n");return 2; }
    if(argc==3) {
        tile_mode=!strcmp(argv[1],"--tile");terrain_mode=!strcmp(argv[1],"--terrain");
        if(!parse(argv[2],tile_mode?255:MAP_ID_MAX-1,tile_mode?&tile:&map)) { fprintf(stderr,"Invalid %s ID\n",tile_mode?"tile behavior":"map");return 2; }
    }
    if(argc==3 && !strcmp(argv[1],"--wireframe") && strcmp(argv[1],"--solid"))return hg_field_wireframe(map,stdout)?0:1;
    if(argc==3 && !strcmp(argv[1],"--solid"))return hg_field_solid(map,stdout)?0:1;
    if(tile_mode) {
        printf("Original metatile behavior %u: grass=%d surfable=%d walking_encounters=%d door=%d ice=%d warp_panel=%d\n",tile,MetatileBehavior_IsTallGrass((u8)tile),MetatileBehavior_IsSurfableWater((u8)tile),MetatileBehavior_CanGenerateWalkingEncounters((u8)tile),MetatileBehavior_IsDoor((u8)tile),MetatileBehavior_IsIce((u8)tile),MetatileBehavior_IsWarpPanel((u8)tile));return 0;
    }
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);
    printf("HeartGold native field data: %s (%u)\n",map_names[map],map);
    printf("matrix=%u, chunks=%ux%u, resource=%.*s\n",MapHeader_GetMatrixId(map),MapMatrix_GetWidth(matrix),MapMatrix_GetHeight(matrix),MAP_MATRIX_MAX_NAME_LENGTH,(const char *)matrix->data.name);
    printf("outdoors=%d, bike=%d, fly=%d, map_section=%u, events=%u, scripts=%u\n",MapHeader_IsOutdoors(map),MapHeader_IsBikeAllowed(map),MapHeader_IsFlyAllowed(map),MapHeader_GetMapSec(map),MapHeader_GetEventsBank(map),MapHeader_GetScriptsBank(map));
    HGFieldResources *resources=terrain_mode?hg_field_resources_new(map):NULL;
    if(terrain_mode && !resources) {MapMatrix_Free(matrix);return 1; }
    unsigned matches=0;
    for(unsigned y=0;y<MapMatrix_GetHeight(matrix);y++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,(s32)x,(s32)y)==map) {
        printf("  chunk (%u,%u): model=%u altitude=%u\n",x,y,MapMatrix_GetMapModelNo((int)(y*matrix->width+x),matrix),MapMatrix_GetMapAltitude(matrix,matrix->matrix_id,(u16)x,(u16)y,matrix->width));matches++;
        if(terrain_mode) {
            const u8 *bytes;size_t size;HGLandChunk chunk;
            unsigned member=MapMatrix_GetMapModelNo((int)(y*matrix->width+x),matrix);
            if(!native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&bytes,&size) || !hg_land_decode(bytes,size,&chunk)) {
                fprintf(stderr,"Invalid land chunk %u\n",member);hg_field_resources_free(resources);MapMatrix_Free(matrix);return 1;
            }
            printf("  land=%u, prefix=%u bytes, props=%zu model=%zu collision=%zu bytes\n",member,chunk.extra_size,chunk.props_size,chunk.model_size,chunk.collision_size);
            HGCollision collision;
            if(!hg_collision_decode(chunk.collision,chunk.collision_size,&collision)) { fprintf(stderr,"Invalid collision tables\n");hg_field_resources_free(resources);MapMatrix_Free(matrix);return 1; }
            printf("  collision: points=%u normals=%u constants=%u planes=%u strips=%u indices=%u\n",collision.point_count,collision.normal_count,collision.constant_count,collision.plane_count,collision.strip_count,collision.index_count);
            puts("  Terrain diagnostic (local x right, z down): ~=surfable g=grass D=door .=other");
            unsigned height_hits=0,height_misses=0;int32_t min_height=INT32_MAX,max_height=INT32_MIN;
            for(unsigned z=0;z<32;z++) {
                for(unsigned tx=0;tx<32;tx++) {
                    HGFieldTile tile;if(!hg_field_resources_tile(resources,(int)(x*32+tx),(int)(y*32+z),&tile))abort();u8 behavior=(u8)tile.attribute;
                    int32_t height;int hit=hg_field_resources_height(resources,(int)(x*32+tx),(int)(y*32+z),0,0,&height);
                    if(hit<0) { fprintf(stderr,"Guest terrain execution failed\n");hg_field_resources_free(resources);MapMatrix_Free(matrix);return 1; }
                    if(hit) { height_hits++;if(height<min_height)min_height=height;if(height>max_height)max_height=height; }else height_misses++;
                    putchar(MetatileBehavior_IsSurfableWater(behavior)?'~':MetatileBehavior_IsTallGrass(behavior)?'g':MetatileBehavior_IsDoor(behavior)?'D':'.');
                }
                putchar('\n');
            }
            printf("  Original Thumb heights: hits=%u misses=%u",height_hits,height_misses);
            if(height_hits)printf(", local Q12 range=%d..%d",min_height,max_height);
            putchar('\n');
        }
    }
    if(!matches)puts("  No chunks in this matrix carry the requested map header.");
    if(terrain_mode)printf("Persistent terrain context: %zu decoded matrix cells\n",hg_field_resources_loaded(resources));
    hg_field_resources_free(resources);
    puts("Metadata inspection only; movement is pending. Use heartgold_field_viewer for rendering.");
    MapMatrix_Free(matrix);return 0;
}
