#include "field_data.h"
#include <string.h>
static unsigned le16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t le32(const uint8_t *p) { return le16(p)|(uint32_t)le16(p+2)<<16; }
int hg_land_decode(const uint8_t *p,size_t n,HGLandChunk *out) {
    if(!p || n<20 || !out)return 0;
    /* Original ov01_021F49FC reads four sizes; ov01_021F4AAC then
     * consumes a four-byte prefix and its high-halfword-sized payload. */
    size_t sizes[4];for(unsigned i=0;i<4;i++)sizes[i]=le32(p+4*i);
    unsigned extra=le16(p+18);
    if(sizes[0]!=2048 || le16(p+16)!=0x1234 || extra>n-20)return 0;
    size_t offset=20+extra;const uint8_t *sections[4];
    for(unsigned i=0;i<4;i++) {
        if(sizes[i]>n-offset)return 0;
        sections[i]=p+offset;offset+=sizes[i];
    }
    if(offset!=n || sizes[2]<16 || memcmp(sections[2],"BMD0",4) || le32(sections[2]+8)!=sizes[2])return 0;
    /* Land models currently contain one MDL0 block. Validate the container
     * before exposing it; geometry commands and model dictionaries stay opaque. */
    const uint8_t *model=sections[2];
    if(sizes[2]<28 || le16(model+4)!=0xfeff || le16(model+6)!=2 ||
       le16(model+12)!=16 || le16(model+14)!=1)return 0;
    size_t block=le32(model+16);
    if(block<20 || block>sizes[2]-8 || memcmp(model+block,"MDL0",4) ||
       le32(model+block+4)!=sizes[2]-block)return 0;
    HGLandChunk v={sections[0],sections[1],sections[2],sections[3],sizes[1],sizes[2],sizes[3],extra};
    *out=v;return 1;
}
int hg_land_tile(const HGLandChunk *chunk,unsigned x,unsigned z,uint16_t *out) {
    if(!chunk || !chunk->attributes || !out || x>=32 || z>=32)return 0;
    *out=(uint16_t)le16(chunk->attributes+2*(z*32+x));return 1;
}

#include "map_header.h"
#include "map_matrix.h"
#include "constants/maps.h"
#include "compat/resources.h"
#include <stdlib.h>
typedef struct { unsigned state; HGLandChunk land; HGCollision collision; } HGFieldCell;
struct HGFieldResources {
    MapMatrix *matrix;
    unsigned width,height;
    size_t loaded;
    HGFieldCell *cells;
};
HGFieldResources *hg_field_resources_new(unsigned map) {
    if(map>=MAP_ID_MAX)return NULL;
    HGFieldResources *f=calloc(1,sizeof(*f));if(!f)return NULL;
    f->matrix=MapMatrix_New();MapMatrix_Load(map,f->matrix);
    f->width=MapMatrix_GetWidth(f->matrix);f->height=MapMatrix_GetHeight(f->matrix);
    if(!f->width || !f->height ||
       !(f->cells=calloc((size_t)f->width*f->height,sizeof(*f->cells)))) {
        hg_field_resources_free(f);return NULL;
    }
    return f;
}
void hg_field_resources_free(HGFieldResources *f) {
    if(!f)return;
    if(f->matrix)MapMatrix_Free(f->matrix);
    free(f->cells);free(f);
}
size_t hg_field_resources_loaded(const HGFieldResources *f) { return f?f->loaded:0; }
int hg_field_resources_tile(HGFieldResources *f,int x,int z,HGFieldTile *out) {
    if(!f || !out || x<0 || z<0 || (unsigned)x/32>=f->width || (unsigned)z/32>=f->height)return 0;
    unsigned mx=(unsigned)x/32,mz=(unsigned)z/32,index=mz*f->width+mx;
    HGFieldCell *cell=&f->cells[index];
    unsigned member=MapMatrix_GetMapModelNo((int)index,f->matrix);
    if(!cell->state) {
        const uint8_t *bytes;size_t size;
        cell->state=2; /* Remember missing/invalid cells without repeated I/O. */
        if(native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&bytes,&size) &&
           hg_land_decode(bytes,size,&cell->land) &&
           hg_collision_decode(cell->land.collision,cell->land.collision_size,&cell->collision)) { cell->state=1;f->loaded++; }
    }
    if(cell->state!=1)return 0;
    HGFieldTile tile={0};
    tile.map_header=MapMatrix_GetMapHeader(f->matrix,(int)mx,(int)mz);
    tile.land_member=member;tile.matrix_x=mx;tile.matrix_z=mz;
    tile.altitude=MapMatrix_GetMapAltitude(f->matrix,MapMatrix_GetMatrixId(f->matrix),(u16)mx,(u16)mz,(int)f->width);
    tile.land=cell->land;tile.collision=cell->collision;
    if(!hg_land_tile(&cell->land,(unsigned)x%32,(unsigned)z%32,&tile.attribute))return 0;
    *out=tile;return 1;
}
int hg_field_attribute(unsigned map,unsigned x,unsigned z,uint16_t *out) {
    /* Keep the one-shot API; persistent consumers should retain a context. */
    if(!out || x>INT32_MAX || z>INT32_MAX)return 0;
    HGFieldResources *f=hg_field_resources_new(map);if(!f)return 0;
    HGFieldTile tile;int ok=hg_field_resources_tile(f,(int)x,(int)z,&tile);
    if(ok)*out=tile.attribute;
    hg_field_resources_free(f);return ok;
}

int hg_collision_decode(const uint8_t *p,size_t n,HGCollision *out) {
    if(!p || !out || n<16 || memcmp(p,"BDHC",4))return 0;
    unsigned counts[6];const unsigned widths[6]={8,12,4,8,8,2};const uint8_t *tables[6];size_t offset=16;
    /* Original ov01_021FB04C/ov01_021FB0B0 establish table order/strides. */
    for(unsigned i=0;i<6;i++) {
        counts[i]=le16(p+4+2*i);size_t bytes=(size_t)counts[i]*widths[i];
        if(bytes>n-offset)return 0;tables[i]=p+offset;offset+=bytes;
    }
    if(offset!=n)return 0;
    /* Original ov01_021FAD6C/AD9C/ADBC consume these four indices. */
    for(unsigned i=0;i<counts[3];i++) {
        const uint8_t *plane=tables[3]+8*i;
        if(le16(plane)>=counts[0] || le16(plane+2)>=counts[0] || le16(plane+4)>=counts[1] || le16(plane+6)>=counts[2])return 0;
    }
    /* Original ov01_021FAE50 reads a strip's count and start at +4/+6. */
    for(unsigned i=0;i<counts[4];i++) {
        const uint8_t *strip=tables[4]+8*i;unsigned count=le16(strip+4),start=le16(strip+6);
        if(start>counts[5] || count>counts[5]-start)return 0;
    }
    for(unsigned i=0;i<counts[5];i++)if(le16(tables[5]+2*i)>=counts[3])return 0;
    HGCollision v={tables[0],tables[1],tables[2],tables[3],tables[4],tables[5],counts[0],counts[1],counts[2],counts[3],counts[4],counts[5]};*out=v;return 1;
}

int hg_field_resources_height(HGFieldResources *f,int x,int z,unsigned mode,
                              int32_t reference,int32_t *height) {
    HGFieldTile tile;
    if(!height || mode>2 || !hg_field_resources_tile(f,x,z,&tile))return -1;
    /* Original sub_02054654 subtracts the chunk center from world Q12
     * coordinates before ov01_021FAE50. One tile is 16 model units. */
    int32_t local_x=((x%32)*16-248)*4096;
    int32_t local_z=((z%32)*16-248)*4096;
    return hg_collision_height(tile.land.collision,tile.land.collision_size,mode,
                               reference,local_x,local_z,height);
}
