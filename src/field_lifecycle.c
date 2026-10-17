#include "prop_runtime.h"
#include "compat/resources.h"
#include "field_data.h"
#include "map_header.h"
#include "map_matrix.h"
#include "ds_model.h"
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static unsigned r16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t r32(const uint8_t *p) { return r16(p)|(uint32_t)r16(p+2)<<16; }
static int32_t signed32(const uint8_t *p) { uint32_t v=r32(p);return (int32_t)((int64_t)v-((v&0x80000000u)?INT64_C(4294967296):0)); }
static int slots_match(MapPropManager *props,const uint8_t *bytes,unsigned count) {
    for(unsigned i=0;i<MAP_PROP_MAX;i++) {
        MapProp *prop=MapPropManager_GetMapPropByIndex(props,(u8)i);
        if(MapProp_IsActive(prop)!=(i<count))return 0;
        if(i>=count)continue;const uint8_t *record=bytes+48*i;VecFx32 t;MapProp_GetTranslation(&t,prop);
        if(MapProp_GetBuildModel(prop)!=(int)r32(record) || t.x!=signed32(record+4) || t.y!=signed32(record+8) || t.z!=signed32(record+12))return 0;
        VecFx32 *rotation=MapProp_GetRotation(prop),*scale=MapProp_GetScale(prop);
        if(rotation->x!=signed32(record+16) || rotation->y!=signed32(record+20) || rotation->z!=signed32(record+24) || scale->x!=signed32(record+28) || scale->y!=signed32(record+32) || scale->z!=signed32(record+36))return 0;
        if(!MapProp_GetResModel(prop) || !MapProp_GetRenderSurface(prop))return 0;
    }return 1;
}
typedef struct { unsigned submissions,whole,shapes,models[550],placement_mask;size_t triangles;const uint8_t *placements; } DrawCheck;
static int check_draw(void *user,unsigned id,const uint8_t *bytes,size_t size,const DSTransform *transform,int material,int shape) {
    DrawCheck *check=user;DSModel model;DSModelStats stats;
    for(unsigned i=0;i<12;i++)if(!isfinite(transform->m[i]))return 0;
    if(!ds_model_open(bytes,size,0,&model))return 0;
    int ok=shape<0?ds_model_scene(&model,NULL,NULL,&stats):ds_model_scene_shape(&model,(unsigned)shape,(unsigned)material,NULL,NULL,&stats);
    if(!ok || !stats.triangles) { fprintf(stderr,"Failed prop geometry id=%u shape=%d\n",id,shape);return 0; }
    int matched=0;
    for(unsigned i=0;i<17;i++) {
        const uint8_t *p=check->placements+48*i;
        if(r32(p)==id && transform->m[3]==signed32(p+4)/4096.0 && transform->m[7]==signed32(p+8)/4096.0 && transform->m[11]==signed32(p+12)/4096.0) { matched=1;check->placement_mask|=1u<<i; }
    }
    if(!matched)return 0;
    check->submissions++;check->whole+=shape<0;check->shapes+=shape>=0;check->models[id]++;check->triangles+=stats.triangles;return 1;
}
int main(void) {
    const uint8_t *bytes;size_t size;
    CHECK(sizeof(VecFx32)==12 && sizeof(fx32)==4);
    CHECK(native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(60),&bytes,&size) && size==8);
    unsigned area=r16(bytes);Field3dRenderObjManager *resources=hg_prop_resources_new();CHECK(resources);
    CHECK(native_resource_member(NARC_a_0_4_3,0,(int)area,&bytes,&size) && size>=2 && size==2+2*r16(bytes));
    unsigned ids[550],num=r16(bytes);CHECK(num<550);for(unsigned i=0;i<num;i++)ids[i]=r16(bytes+2+2*i);
    for(unsigned i=0;i<=num;i++) { unsigned id=i<num?ids[i]:0;CHECK(native_resource_member(NARC_a_0_4_0,0,(int)id,&bytes,&size));CHECK(hg_prop_resources_load(resources,id,bytes,size)); }
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(60,matrix);CHECK(MapMatrix_GetMapHeader(matrix,21,12)==60);
    unsigned land_id=MapMatrix_GetMapModelNo(12*47+21,matrix);MapMatrix_Free(matrix);
    CHECK(native_resource_member(NARC_fielddata_landdata_land_data,0,(int)land_id,&bytes,&size));HGLandChunk land;CHECK(hg_land_decode(bytes,size,&land));
    CHECK(hg_props_validate(land.props,land.props_size));CHECK(!hg_props_validate(land.props,land.props_size-1));CHECK(!hg_props_validate(land.props,48*33));
    uint8_t bad[48]={255,255,255,255};CHECK(!hg_props_validate(bad,sizeof(bad)));CHECK(land.props_size/48==17);
    MtxFx33 rotated;VecFx32 angle={0,16384,0};sub_02020D2C(&rotated,&angle);
    const fx32 expected_rotation[]={0,0,-4096,0,4096,0,4096,0,0};CHECK(!memcmp(rotated.m,expected_rotation,sizeof(expected_rotation)));
    angle.y+=15;sub_02020D2C(&rotated,&angle);CHECK(!memcmp(rotated.m,expected_rotation,sizeof(expected_rotation)));
    puts("{\"scenario\":\"new-bark-props-v1\",\"map\":60,\"land\":0,\"props\":17}");
    hg_prop_trace_begin();MapPropManager *props=MapPropManager_New(HEAP_ID_FIELD1,resources);CHECK(props);CHECK(slots_match(props,NULL,0));
    hg_prop_trace_mark("created",hg_prop_resources_live(resources));
    NARC *stream=native_resource_stream(land.props,land.props_size);CHECK(stream);
    MapPropManager_LoadFromNARC(stream,(u32)land.props_size,props,0);NARC_Delete(stream);
    CHECK(slots_match(props,land.props,17) && hg_prop_resources_live(resources)==17);hg_prop_trace_mark("loaded",17);
    CHECK(native_resource_file("bm_field_matshp.dat",&bytes,&size));AreaDataManager *area_data=hg_prop_area_new(bytes,size);CHECK(area_data);
    for(size_t n=0;n<size;n++)CHECK(!hg_prop_area_new(bytes,n));
    uint8_t *bad_area=malloc(size);CHECK(bad_area);memcpy(bad_area,bytes,size);bad_area[4]=255;bad_area[5]=255;
    CHECK(!hg_prop_area_new(bad_area,size));free(bad_area);
    CHECK(!hg_prop_area_new(bytes,size-1));DrawCheck drawing={.placements=land.props};unsigned submitted=0;
    hg_prop_draw_begin(check_draw,&drawing);ov01_021F3C9C(props,area_data);CHECK(hg_prop_draw_end(&submitted));
    CHECK(submitted==35 && drawing.submissions==35 && drawing.whole==9 && drawing.shapes==26 && drawing.models[27]==4 && drawing.triangles==1294 && drawing.placement_mask==((1u<<17)-1));
    hg_prop_trace_mark("drawn",17);hg_prop_area_free(area_data);
    MapProp *first=MapPropManager_GetMapPropByIndex_Checked_RequireActive(props,0);CHECK(MapPropManager_FindMapPropByBuildModel(props,21)==first);
    VecFx32 position={4096,-8192,12288},rotation={0,4096,0},actual;MapProp_SetTranslation(first,&position);MapProp_GetTranslation(&actual,first);CHECK(!memcmp(&actual,&position,sizeof(actual)));
    MapProp_SetCulled(first,TRUE);MapProp_SetCulled(first,FALSE);
    MapPropManager_RemoveMapPropByIndex(3,props);CHECK(!MapProp_IsActive(MapPropManager_GetMapPropByIndex(props,3)));CHECK(hg_prop_resources_live(resources)==16);
    CHECK(MapPropManager_LoadOne(props,27,&position,&rotation,NULL)==3);CHECK(hg_prop_resources_live(resources)==17);
    CHECK(!memcmp(MapProp_GetRotation(MapPropManager_GetMapPropByIndex(props,3)),&rotation,sizeof(rotation)));
    for(unsigned i=17;i<32;i++)CHECK(MapPropManager_LoadOne(props,20,&position,NULL,NULL)==i);
    CHECK(hg_prop_resources_live(resources)==32);hg_prop_trace_mark("capacity",32);
    MapPropManager_Reset(props);CHECK(slots_match(props,NULL,0));CHECK(hg_prop_resources_live(resources)==0);hg_prop_trace_mark("reset",0);
    stream=native_resource_stream(NULL,0);MapPropManager_LoadFromNARC(stream,0,props,0);NARC_Delete(stream);CHECK(slots_match(props,NULL,0));
    memcpy(bad,land.props,48);bad[0]=0x25;bad[1]=2;bad[2]=bad[3]=0;CHECK(hg_props_validate(bad,48)); /* model 549 unavailable: original fallback to 0 */
    stream=native_resource_stream(bad,48);MapPropManager_LoadFromNARC(stream,48,props,0);NARC_Delete(stream);
    CHECK(MapProp_IsActive(first) && MapProp_GetBuildModel(first)==0 && hg_prop_resources_live(resources)==1);hg_prop_trace_mark("fallback",1);MapPropManager_Reset(props);
    stream=native_resource_stream(land.props,land.props_size);MapPropManager_LoadFromNARC(stream,(u32)land.props_size,props,0);NARC_Delete(stream);CHECK(slots_match(props,land.props,17));hg_prop_trace_mark("reloaded",17);
    MapPropManager_Reset(props);MapPropManager_Free(props);CHECK(hg_prop_resources_live(resources)==0);hg_prop_trace_mark("destroyed",0);hg_prop_resources_free(resources);
    CHECK(hg_prop_trace_write(stdout));return 0;
}
