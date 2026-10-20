#include "field_view.h"
#include "prop_runtime.h"
#include "field_data.h"
#include "map_header.h"
#include "map_matrix.h"
#include "constants/maps.h"
#include "compat/resources.h"
#include <math.h>
#include "field_names.inc"
#include "character_names.inc"
#include "character_supported.inc"
#include "character_sprites.inc"
#include <stdarg.h>
typedef struct {
    DSModel model;unsigned id,material_base;int material,shape,posed;
    DSTransform transform;double chunk[3];size_t first,count;
} PropGeometry;
struct HGFieldView {
    PropGeometry *props;unsigned prop_count,prop_capacity;
    DSRasterTriangle *triangles;size_t count,capacity;
    DSRasterMaterial materials[4096];unsigned material_count;int failed;
    double min[3],max[3],world_origin[3];
    DSTexture player_frames[32];
    char failure[256];
    double chunk_offset[3];
    unsigned material_base;int prop_materials[550];DSTransform transform;
};
static void collect(void *user,const DSSceneVertex tri[3]) {
    HGFieldView *v=user;if(v->failed)return;
    if(v->count==v->capacity) {
        if(v->capacity>=100000) { v->failed=1;return; }
        size_t cap=v->capacity?v->capacity*2:1024;void *p=realloc(v->triangles,cap*sizeof(*v->triangles));
        if(!p) { v->failed=1;return; }v->triangles=p;v->capacity=cap;
    }
    for(unsigned i=0;i<3;i++) {
        unsigned material=v->material_base+tri[i].material;
        if(material>=v->material_count) { v->failed=1;snprintf(v->failure,sizeof(v->failure),"Scene references missing material %u",material);return; }
        DSSceneVertex vertex=tri[i];vertex.material=material;ds_transform_point(&v->transform,tri[i].x,tri[i].y,tri[i].z,&vertex.x,&vertex.y,&vertex.z);
        vertex.x+=v->chunk_offset[0];vertex.y+=v->chunk_offset[1];vertex.z+=v->chunk_offset[2];
        v->triangles[v->count].v[i]=vertex;double xyz[]={vertex.x,vertex.y,vertex.z};
        for(unsigned k=0;k<3;k++) { if(!v->count && !i)v->min[k]=v->max[k]=xyz[k];if(xyz[k]<v->min[k])v->min[k]=xyz[k];if(xyz[k]>v->max[k])v->max[k]=xyz[k]; }
    }
    v->count++;
}
static int bind_materials(HGFieldView *view,const DSModel *model,const DSTextureBank *bank,unsigned base) {
    if(base+model->materials>4096)return 0;
    for(unsigned i=0;i<model->materials;i++) {
        DSModelMaterial mat;if(!ds_model_material(model,i,&mat)) { snprintf(view->failure,sizeof(view->failure),"Invalid material %u",i);return 0; }
        /* A material without a texture is a valid vertex-color surface. Its
         * polygon alpha must still be initialized before the texture branch. */
        view->materials[base+i].polygon=mat.polygon_attrs;
        view->materials[base+i].params=mat.texture_params;
        if(!mat.texture[0])continue;
        if((!mat.identity_uv && !mat.translation_uv) || !ds_texture_find(bank,mat.texture,mat.palette,&view->materials[base+i].texture)) { snprintf(view->failure,sizeof(view->failure),"Material %u: unsupported UV transform or texture %s / palette %s",i,mat.texture,mat.palette);return 0; }
        view->materials[base+i].uv_u=mat.uv_u;view->materials[base+i].uv_v=mat.uv_v;view->materials[base+i].params=mat.texture_params;view->materials[base+i].polygon=mat.polygon_attrs;view->materials[base+i].textured=1;
    }return 1;
}
static int prop_draw(void *user,unsigned id,const uint8_t *bytes,size_t size,const DSTransform *transform,int material,int shape) {
    HGFieldView *view=user;DSModel model;DSTextureBank bank;DSModelStats stats;
    if(id>=550 || !ds_model_open(bytes,size,0,&model))return 0;
    if(view->prop_materials[id]<0) {
        if(!ds_texture_bank(bytes,size,&bank) || !bind_materials(view,&model,&bank,view->material_count)) { fprintf(stderr,"Unsupported prop material: %u\n",id);return 0; }
        view->prop_materials[id]=(int)view->material_count;view->material_count+=model.materials;
    }
    view->material_base=(unsigned)view->prop_materials[id];view->transform=*transform;
    size_t first=view->count;
    int ok=shape<0?ds_model_scene(&model,collect,view,&stats):ds_model_scene_shape(&model,(unsigned)shape,(unsigned)material,collect,view,&stats);
    if(ok&&!view->failed) {
        if(view->prop_count==view->prop_capacity) {
            unsigned cap=view->prop_capacity?view->prop_capacity*2:32;if(cap>2048)return 0;
            void *memory=realloc(view->props,cap*sizeof(*view->props));if(!memory)return 0;view->props=memory;view->prop_capacity=cap;
        }
        PropGeometry *record=view->props+view->prop_count++;
        *record=(PropGeometry){.model=model,.id=id,.material_base=view->material_base,.material=material,.shape=shape,.transform=*transform,.first=first,.count=view->count-first};
        memcpy(record->chunk,view->chunk_offset,sizeof(record->chunk));
    }
    return ok && !view->failed;
}
static int append_props(HGFieldView *view,const HGLandChunk *land,unsigned list_id,unsigned field_type) {
    const uint8_t *bytes;size_t size;Field3dRenderObjManager *resources=hg_prop_resources_new();MapPropManager *props=NULL;AreaDataManager *area=NULL;int ok=0;
    if(!resources || !hg_props_validate(land->props,land->props_size))goto cleanup;
    if(!native_resource_member(NARC_a_0_4_3,0,(int)list_id,&bytes,&size) || size<2)goto cleanup;
    unsigned count=bytes[0]|(unsigned)bytes[1]<<8;if(size!=2+2*count || count>=550)goto cleanup;
    int archive=field_type?NARC_a_0_4_0:NARC_a_1_4_8;
    unsigned ids[550];for(unsigned i=0;i<count;i++)ids[i]=bytes[2+2*i]|(unsigned)bytes[3+2*i]<<8;
    for(unsigned i=0;i<=count;i++) { unsigned id=i<count?ids[i]:0;if(!native_resource_member(archive,0,(int)id,&bytes,&size) || !hg_prop_resources_load(resources,id,bytes,size))goto cleanup; }
    props=MapPropManager_New(HEAP_ID_FIELD1,resources);if(!props)goto cleanup;
    NARC *stream=native_resource_stream(land->props,land->props_size);if(!stream)goto cleanup;
    MapPropManager_LoadFromNARC(stream,(u32)land->props_size,props,0);NARC_Delete(stream);
    if(!native_resource_file(field_type?"bm_field_matshp.dat":"bm_room_matshp.dat",&bytes,&size) || !(area=hg_prop_area_new(bytes,size)))goto cleanup;
    hg_prop_draw_begin(prop_draw,view);ov01_021F3C9C(props,area);ok=hg_prop_draw_end(NULL) && !view->failed;
cleanup:if(props) { MapPropManager_Reset(props);MapPropManager_Free(props); }hg_prop_area_free(area);hg_prop_resources_free(resources);return ok;
}
struct HGFieldProps {
    unsigned map,archive;Field3dRenderObjManager *resources;MapPropManager *manager;AreaDataManager *area;
};
void hg_field_props_free(HGFieldProps *p) {
    if(!p)return;if(p->manager){MapPropManager_Reset(p->manager);MapPropManager_Free(p->manager);}
    hg_prop_area_free(p->area);hg_prop_resources_free(p->resources);free(p);
}
HGFieldProps *hg_field_props_new(unsigned map) {
    const uint8_t *bytes;size_t size;
    if(map>=MAP_ID_MAX||!native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(map),&bytes,&size)||size!=8||bytes[6]>1)return NULL;
    unsigned field=bytes[6];HGFieldProps *p=calloc(1,sizeof(*p));if(!p)return NULL;
    p->map=map;p->archive=field?NARC_a_0_4_0:NARC_a_1_4_8;p->resources=hg_prop_resources_new();
    if(!p->resources||!(p->manager=MapPropManager_New(HEAP_ID_FIELD1,p->resources))||
       !native_resource_file(field?"bm_field_matshp.dat":"bm_room_matshp.dat",&bytes,&size)||!(p->area=hg_prop_area_new(bytes,size))) {
        hg_field_props_free(p);return NULL;
    }
    return p;
}
MapPropManager *hg_field_props_manager(HGFieldProps *p) { return p?p->manager:NULL; }
unsigned hg_field_props_count(const HGFieldProps *p) {
    unsigned count=0;if(p)for(unsigned i=0;i<MAP_PROP_MAX;i++)count+=MapProp_IsActive(MapPropManager_GetMapPropByIndex(p->manager,(u8)i))!=0;
    return count;
}
int hg_field_props_prepare(HGFieldProps *p,unsigned model,unsigned slots) {
    if(!p||model>=550||!hg_prop_area_has_model(p->area,model)||slots>MAP_PROP_MAX-hg_field_props_count(p))return 0;
    if(Field3dRenderObjManager_IsResFileHeaderLoadedByIndex(p->resources,(int)model))return 1;
    const uint8_t *bytes;size_t size;
    return native_resource_member((int)p->archive,0,(int)model,&bytes,&size)&&hg_prop_resources_load(p->resources,model,bytes,size);
}
HGFieldProps *hg_field_props_clone(const HGFieldProps *source) {
    if(!source)return NULL;HGFieldProps *p=hg_field_props_new(source->map);if(!p)return NULL;
    for(unsigned i=0;i<MAP_PROP_MAX;i++) {
        MapProp *original=MapPropManager_GetMapPropByIndex(source->manager,(u8)i);if(!MapProp_IsActive(original))continue;
        unsigned model=(unsigned)MapProp_GetBuildModel(original);VecFx32 position;MapProp_GetTranslation(&position,original);
        if(!hg_field_props_prepare(p,model,1)){hg_field_props_free(p);return NULL;}
        u8 slot=MapPropManager_LoadOne(p->manager,(int)model,&position,MapProp_GetRotation(original),NULL);
        *MapProp_GetScale(MapPropManager_GetMapPropByIndex(p->manager,slot))=*MapProp_GetScale(original);
    }
    return p;
}
HGFieldView *hg_field_props_view(HGFieldProps *p) {
    if(!p||!hg_field_props_count(p))return NULL;
    HGFieldView *view=calloc(1,sizeof(*view));if(!view)return NULL;
    for(unsigned i=0;i<550;i++)view->prop_materials[i]=-1;
    hg_prop_draw_begin(prop_draw,view);ov01_021F3C9C(p->manager,p->area);
    if(!hg_prop_draw_end(NULL)||view->failed||!view->count){hg_field_view_free(view);return NULL;}
    return view;
}
unsigned hg_field_map_count(void) { return MAP_ID_MAX; }
const char *hg_field_map_name(unsigned map) { return map<MAP_ID_MAX?map_names[map]:NULL; }
static HGFieldView *load_error(char *error,size_t size,const char *format,...) {
    if(error && size) { va_list args;va_start(args,format);vsnprintf(error,size,format,args);va_end(args); }return NULL;
}
HGFieldView *hg_field_view_new(unsigned map) { return hg_field_view_load(map,NULL,0); }
HGFieldView *hg_field_view_load(unsigned map,char *error,size_t error_size) {
    if(error && error_size)error[0]=0;
    if(map>=MAP_ID_MAX)return load_error(error,error_size,"Invalid map ID %u",map);
    const uint8_t *p;size_t n;
    if(!native_resource_member(NARC_a_0_4_2,0,MapHeader_GetAreaDataBank(map),&p,&n) || n!=8)
        return load_error(error,error_size,"Missing or invalid area metadata");
    if(p[6]>1)return load_error(error,error_size,"Area type %u is not supported yet",p[6]);
    unsigned field_type=p[6];unsigned list_id=p[0]|(unsigned)p[1]<<8;
    unsigned tex_id=p[2]|(unsigned)p[3]<<8;DSTextureBank bank;
    if(!native_resource_member(NARC_a_0_4_4,0,(int)tex_id,&p,&n) || !ds_texture_bank(p,n,&bank))
        return load_error(error,error_size,"Unsupported texture bank %u",tex_id);
    MapMatrix *matrix=MapMatrix_New();MapMatrix_Load(map,matrix);
    unsigned matches=0,min_x=UINT32_MAX,min_z=UINT32_MAX;
    for(unsigned z=0;z<MapMatrix_GetHeight(matrix);z++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,x,z)==map) {
        matches++;if(x<min_x)min_x=x;if(z<min_z)min_z=z;
    }
    if(!matches || matches>64) { MapMatrix_Free(matrix);return load_error(error,error_size,"Map has %u chunks; supported scene range is 1..64",matches); }
    HGFieldView *view=calloc(1,sizeof(*view));if(!view) { MapMatrix_Free(matrix);return load_error(error,error_size,"Out of memory"); }
    for(unsigned i=0;i<550;i++)view->prop_materials[i]=-1;
    view->world_origin[0]=min_x*512.0+256;view->world_origin[2]=min_z*512.0+256;
    const char *reason="Invalid land/model resource";
    for(unsigned z=0;z<MapMatrix_GetHeight(matrix);z++)for(unsigned x=0;x<MapMatrix_GetWidth(matrix);x++)if(MapMatrix_GetMapHeader(matrix,x,z)==map) {
        unsigned member=MapMatrix_GetMapModelNo((int)(z*matrix->width+x),matrix);HGLandChunk land;DSModel model;
        reason="Invalid land/model resource";
        if(!native_resource_member(NARC_fielddata_landdata_land_data,0,(int)member,&p,&n) || !hg_land_decode(p,n,&land) || !ds_model_open(land.model,land.model_size,0,&model))goto fail;
        /* Original ov01_021F5FB8 uses x/z <<21 and altitude <<15 in Q12:
         * 512 model units per chunk and 8 per matrix altitude unit. */
        view->chunk_offset[0]=(x-min_x)*512.0;view->chunk_offset[2]=(z-min_z)*512.0;
        view->chunk_offset[1]=8.0*MapMatrix_GetMapAltitude(matrix,MapMatrix_GetMatrixId(matrix),(u16)x,(u16)z,(int)matrix->width);
        view->transform=ds_transform_identity();view->material_base=view->material_count;
        reason="Unsupported land material or texture / material capacity exceeded";
        if(!bind_materials(view,&model,&bank,view->material_base))goto fail;
        view->material_count+=model.materials;
        reason="Unsupported land geometry or scene commands / triangle capacity exceeded";
        DSModelStats stats;if(!ds_model_scene(&model,collect,view,&stats) || view->failed)goto fail;
        reason="Unsupported prop resources, materials or scene commands";
        if(!append_props(view,&land,list_id,field_type))goto fail;
    }
    MapMatrix_Free(matrix);if(view->count)return view;
    hg_field_view_free(view);return load_error(error,error_size,"Map contains no visible triangles");
fail:{ char detail[256];snprintf(detail,sizeof(detail),"%s",view->failure[0]?view->failure:reason);
    MapMatrix_Free(matrix);hg_field_view_free(view);return load_error(error,error_size,"%s",detail); }
}

void hg_field_view_free(HGFieldView *v) { if(v) { free(v->props);free(v->triangles);free(v); } }
void hg_field_view_camera(const HGFieldView *v,DSCamera *c,unsigned w,unsigned h) {
    if(!v || !c)return;double radius=0;
    for(unsigned i=0;i<3;i++)radius+=(v->max[i]-v->min[i])*(v->max[i]-v->min[i]);radius=sqrt(radius);
    /* A gentle 12-degree orbit gives rooms depth without hiding their layout.
     * Inspection controls can still orbit from this shared entry/reset view. */
    *c=(DSCamera){.yaw=0.2094395102,.pitch=0.85,.scale=(w<h?w:h)*0.85/(radius>0?radius:1),.x=(v->min[0]+v->max[0])/2,.y=(v->min[1]+v->max[1])/2,.z=(v->min[2]+v->max[2])/2};
}
int hg_field_view_render(const HGFieldView *v,const DSCamera *c,uint8_t *out,unsigned w,unsigned h) { return v && ds_raster_draw(v->triangles,v->count,v->materials,v->material_count,c,out,w,h); }
size_t hg_field_view_triangles(const HGFieldView *v) { return v?v->count:0; }

unsigned hg_character_count(void) { return 266; }
const char *hg_character_name(unsigned id) { return id<266 && character_names[id]?character_names[id]:"Unnamed resource"; }
static HGFieldView *character_load(unsigned id,char *error,size_t size,int player,unsigned frame,unsigned mesh_id) {
    if(error && size)error[0]=0;
    if(id>=266&&player!=3)return load_error(error,size,"Invalid character ID");
    if(player<3&&!character_supported[id] && !(player && character_type7[id]))return load_error(error,size,"Specialized sprite template is not supported yet");
    const uint8_t *mesh,*texture;size_t mesh_size,texture_size;DSModel model;DSTextureBank bank;
    if(!native_resource_member(NARC_data_mmodel_mmodel,0,(int)mesh_id,&mesh,&mesh_size) || !ds_model_open(mesh,mesh_size,0,&model) ||
       !native_resource_member(NARC_data_mmodel_mmodel,0,(int)id,&texture,&texture_size) || !ds_texture_bank(texture,texture_size,&bank))
       return load_error(error,size,"Unsupported character resources");
    HGFieldView *view=calloc(1,sizeof(*view));if(!view)return load_error(error,size,"Out of memory");
    view->transform=ds_transform_identity();view->material_count=model.materials;
    DSModelStats stats;
    char texture_name[17],palette_name[17];int found=0;
    for(unsigned i=0;i<bank.textures;i++)if(ds_texture_name(&bank,i,0,texture_name)) {
        const char *suffix=strrchr(texture_name,'.');if(suffix && !strcmp(suffix,".5")) { found=1;break; }
    }
    if(!found)found=ds_texture_name(&bank,0,0,texture_name);
    if(player==3&&frame) { found=ds_texture_name(&bank,2*((frame-1)/4),0,texture_name); }
    else if(found&&frame) {
        char *suffix=strrchr(texture_name,'.');
        if(!suffix)found=0;
        else { size_t offset=(size_t)(suffix-texture_name);int written=snprintf(suffix,sizeof(texture_name)-offset,".%u",frame);
            if(written<0||(size_t)written>=sizeof(texture_name)-offset)found=0; }
    }
    if(!found || !ds_texture_name(&bank,0,1,palette_name)) { hg_field_view_free(view);return load_error(error,size,"Missing character frame/palette"); }
    for(unsigned i=0;i<model.materials;i++) {
        DSModelMaterial mat;if(!ds_model_material(&model,i,&mat) || !ds_texture_find(&bank,texture_name,palette_name,&view->materials[i].texture)) {
            hg_field_view_free(view);return load_error(error,size,"Unsupported character texture");
        }
        view->materials[i].polygon=mat.polygon_attrs;view->materials[i].params=mat.texture_params;view->materials[i].textured=1;
    }
    if(player==3)for(unsigned i=0;i<16;i++) {
        char name[17];if(!ds_texture_name(&bank,2*(i/4)+(i%2),0,name)||!ds_texture_find(&bank,name,palette_name,&view->player_frames[i])||view->player_frames[i].width!=view->materials[0].texture.width||view->player_frames[i].height!=view->materials[0].texture.height) {
            hg_field_view_free(view);return load_error(error,size,"Missing follower frame %u",i);
        }
    }
    if(player==1)for(unsigned i=1;i<=32;i++) {
        char name[17];snprintf(name,sizeof(name),"hero.%u",i);
        if(!ds_texture_find(&bank,name,palette_name,&view->player_frames[i-1])||
           view->player_frames[i-1].width!=view->materials[0].texture.width||
           view->player_frames[i-1].height!=view->materials[0].texture.height) {
            hg_field_view_free(view);return load_error(error,size,"Missing hero frame %u",i);
        }
    }
    if(!ds_model_inspection_scene(&model,collect,view,&stats) || view->failed || !view->count) {
        load_error(error,size,"Character mesh/material unsupported: %s",view->failure);hg_field_view_free(view);return NULL;
    }
    if(player&&player<4) {
        /* The native billboard used to crop type-7 textures to their center
         * 16 columns. Sideways hats/arms extend outside that strip. Expand the
         * horizontal canvas and UV span together: retain texel scale, original
         * height and centered pivot, including transparent frame padding. */
        double lo=view->triangles[0].v[0].u,hi=lo;
        for(size_t i=0;i<view->count;i++)for(unsigned j=0;j<3;j++) {
            double u=view->triangles[i].v[j].u;if(u<lo)lo=u;if(u>hi)hi=u;
        }
        if(hi<=lo){hg_field_view_free(view);return load_error(error,size,"Invalid sprite UV span");}
        double scale=view->materials[0].texture.width/(hi-lo),center=(view->min[0]+view->max[0])/2;
        for(size_t i=0;i<view->count;i++)for(unsigned j=0;j<3;j++) {
            DSSceneVertex *vertex=&view->triangles[i].v[j];
            vertex->x=center+(vertex->x-center)*scale;vertex->u=(vertex->u-lo)*scale;
        }
        view->min[0]=center+(view->min[0]-center)*scale;view->max[0]=center+(view->max[0]-center)*scale;
    }
    return view;
}

HGFieldView *hg_character_view_load(unsigned id,char *error,size_t size) { return character_load(id,error,size,id<266&&character_type7[id]?2:0,0,id<266&&character_type7[id]?272:266); }
HGFieldView *hg_player_sprite_load(char *error,size_t size) { return character_load(69,error,size,1,0,272); }
HGFieldView *hg_character_sprite_load(unsigned sprite,unsigned direction,char *error,size_t size) {
    if(direction>3)return load_error(error,size,"Invalid sprite direction %u",direction);
    for(unsigned i=0;i<NELEMS(character_sprites);i++)if(character_sprites[i].sprite==sprite)
    {
        unsigned kind=character_sprites[i].kind,flags=character_sprites[i].flags;
        int mode=kind==21?5:kind==19?3:kind==7?2:flags==0x20?4:0;
        unsigned frame=kind==21?1:flags==0x20?0:1+4*direction;
        return character_load(character_sprites[i].model,error,size,mode,frame,character_sprites[i].mesh);
    }
    return load_error(error,size,"Event sprite %u requires an unsupported template or graphics variable",sprite);
}
int hg_player_sprite_frame(HGFieldView *sprite,unsigned frame) {
    if(!sprite||frame<1||frame>32||!sprite->player_frames[frame-1].texels)return 0;
    for(unsigned i=0;i<sprite->material_count;i++)sprite->materials[i].texture=sprite->player_frames[frame-1];
    return 1;
}
int hg_field_view_render_player(const HGFieldView *v,const HGFieldView *sprite,const double world[3],
                               const DSCamera *camera,uint8_t *out,unsigned w,unsigned h) {
    if(!world)return 0;HGFieldBillboard actor={.sprite=sprite};memcpy(actor.world,world,sizeof(actor.world));
    return hg_field_view_render_actors(v,&actor,1,camera,out,w,h);
}
int hg_field_view_render_actors(const HGFieldView *v,const HGFieldBillboard *actors,size_t actor_count,
                               const DSCamera *camera,uint8_t *out,unsigned w,unsigned h) {
    if(!v||(!actors&&actor_count)||actor_count>65||!camera)return 0;
    size_t count=v->count;unsigned materials=v->material_count;
    for(size_t a=0;a<actor_count;a++) {
        const HGFieldView *sprite=actors[a].sprite;
        if(!sprite||!sprite->count||sprite->max[0]<=sprite->min[0]||sprite->max[1]<=sprite->min[1])return 0;
        for(unsigned j=0;j<3;j++)if(!isfinite(actors[a].world[j]))return 0;
        count+=sprite->count;materials+=sprite->material_count;
    }
    DSRasterTriangle *tri=malloc(count*sizeof(*tri));DSRasterMaterial *mat=malloc(materials*sizeof(*mat));
    if(!tri||!mat){free(tri);free(mat);return 0;}
    memcpy(tri,v->triangles,v->count*sizeof(*tri));memcpy(mat,v->materials,v->material_count*sizeof(*mat));
    size_t triangle_base=v->count;unsigned material_base=v->material_count;
    for(size_t a=0;a<actor_count;a++) {
        const HGFieldView *sprite=actors[a].sprite;const double *world=actors[a].world;
        memcpy(mat+material_base,sprite->materials,sprite->material_count*sizeof(*mat));
        for(size_t i=0;i<sprite->count;i++)for(unsigned j=0;j<3;j++) {
            DSSceneVertex vertex=sprite->triangles[i].v[j];
            if(actors[a].fixed) {
                vertex.x+=world[0]-v->world_origin[0];vertex.y+=world[1];vertex.z+=world[2]-v->world_origin[2];
            } else {
            double x=vertex.x-(sprite->min[0]+sprite->max[0])/2;
            vertex.x=world[0]-v->world_origin[0]+cos(camera->yaw)*x;
            vertex.z=world[2]-v->world_origin[2]-sin(camera->yaw)*x;
            vertex.y=world[1]+vertex.y-sprite->min[1]+0.1;
            }
            vertex.material+=material_base;tri[triangle_base+i].v[j]=vertex;
        }
        triangle_base+=sprite->count;material_base+=sprite->material_count;
    }
    int ok=ds_raster_draw(tri,count,mat,materials,camera,out,w,h);free(tri);free(mat);return ok;
}
void hg_field_view_focus(const HGFieldView *v,const double world[3],DSCamera *camera,unsigned w,unsigned h) {
    hg_field_view_camera(v,camera,w,h);camera->x=world[0]-v->world_origin[0];
    camera->z=world[2]-v->world_origin[2];camera->y=world[1]+8;camera->scale=(w<h?w:h)/320.0;
}

int hg_field_view_project(const HGFieldView *view,const double world[3],const DSCamera *camera,unsigned width,unsigned height,double screen[3]) {
    if(!view||!world)return 0;
    double local[]={world[0]-view->world_origin[0],world[1],world[2]-view->world_origin[2]};
    return ds_raster_project(camera,local,width,height,screen);
}

typedef struct { DSRasterTriangle *triangles;const PropGeometry *record;size_t count;int failed; } PoseWriter;
static void write_pose_triangle(void *opaque,const DSSceneVertex tri[3]) {
    PoseWriter *w=opaque;const PropGeometry *r=w->record;
    if(w->count>=r->count){w->failed=1;return;}
    for(unsigned i=0;i<3;i++) {
        DSSceneVertex v=tri[i];ds_transform_point(&r->transform,tri[i].x,tri[i].y,tri[i].z,&v.x,&v.y,&v.z);
        v.x+=r->chunk[0];v.y+=r->chunk[1];v.z+=r->chunk[2];v.material+=r->material_base;
        w->triangles[w->count].v[i]=v;
    }
    w->count++;
}
int hg_field_view_prop_poses(HGFieldView *v,const HGPropPose *poses,unsigned count) {
    if(!v||count>16||(!poses&&count))return 0;unsigned matched[16]={0};
    typedef struct { PropGeometry *record;DSRasterTriangle *triangles;int posed; } Update;
    Update updates[2048];unsigned pending=0;int ok=0;
    for(unsigned i=0;i<v->prop_count;i++) {
        PropGeometry *r=v->props+i;const DSJointPose *pose=NULL;
        for(unsigned j=0;j<count;j++)if(r->id==poses[j].model &&
            fabs(r->transform.m[3]+r->chunk[0]+v->world_origin[0]-poses[j].world[0])<0.001 &&
            fabs(r->transform.m[7]+r->chunk[1]-poses[j].world[1])<0.001 &&
            fabs(r->transform.m[11]+r->chunk[2]+v->world_origin[2]-poses[j].world[2])<0.001) { pose=&poses[j].pose;matched[j]=1;break; }
        if(!pose&&!r->posed)continue;
        /* Stage every changed mesh before touching the retained scene. A
         * missing instance or rejected pose cannot leave a half-updated map. */
        DSRasterTriangle *triangles=malloc((r->count?r->count:1)*sizeof(*triangles));
        if(!triangles)goto cleanup;
        updates[pending++]=(Update){r,triangles,pose!=NULL};
        PoseWriter writer={triangles,r,0,0};DSModelStats stats;
        if(!ds_model_scene_pose(&r->model,r->shape,r->material,pose,write_pose_triangle,&writer,&stats)||writer.failed||writer.count!=r->count)goto cleanup;
    }
    for(unsigned i=0;i<count;i++)if(!matched[i])goto cleanup;
    for(unsigned i=0;i<pending;i++) {
        Update *u=updates+i;memcpy(v->triangles+u->record->first,u->triangles,u->record->count*sizeof(*u->triangles));u->record->posed=u->posed;
    }
    ok=1;
cleanup:
    for(unsigned i=0;i<pending;i++)free(updates[i].triangles);return ok;
}
