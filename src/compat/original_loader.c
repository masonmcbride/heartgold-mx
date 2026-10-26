#include <pthread.h>
#include "compat/native_irq.h"
#include <stdlib.h>
#include "compat/native_vram.h"
#include "ds_audio.h"
#include "compat/scheduler.h"
#include "system.h"
#include "compat/native_video.h"
#include "original_field_owner.h"
#include "map_matrix.h"
#include "compat/props.h"
#define MI_CpuClearFast MI_CpuClear8
#define MapPropManager_New original_MapPropManager_New
#define MapPropManager_Free original_MapPropManager_Free
#include "original_loader_props.inc"
#include "compat/native_heap.h"
#include "compat/original_loader.h"
#include <setjmp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "original_loader_raw.inc"
#include "ds_thumb.h"
#include "ds_mapping.h"
#include "ds_raster.h"
#include "original_camera_math.inc"
#include "ds_texture.h"
#include "ds_joint_animation.h"
#include "prop_runtime.h"
#include "compat/original_prop_animation.h"
#include "map_header.h"
#include "sys_task_api.h"
#include "compat/resources.h"
#include "original_loader_guest.inc"
#include "original_sound_state.inc"
#define sub_0201A430 original_ClearFileCache
#include "original_loader_system.inc"
#undef sub_0201A430
void NARC_ReadFromMember(NARC *,u32,u32,u32,void *);
u16 NARC_GetFileCount(NARC *);
void *GfGfxLoader_LoadFromNarc(NarcId,s32,BOOL,enum HeapID,BOOL);
void *GfGfxLoader_LoadFromOpenNarc(NARC *,s32,BOOL,enum HeapID,BOOL);
#define BASE 0x02000000u
#define MATRIX 0x03000000u
#define NARC_HANDLE 0x03000004u
#define LOADER_SCRATCH_OFFSET ((sizeof(hg_loader_image)+0x1fffu)&~0x1fffu)
/* One 8 KiB scratch interval, eight 8 KiB call-stack intervals, then
 * 4 KiB for top-level outgoing arguments. Stack-depth bounds remain explicit. */
#define LOADER_RAM_SIZE (LOADER_SCRATCH_OFFSET+0x13000u)
#define SOUND_WORK_SIZE 0xBEC88u
#define SOUND_POLICY_OFFSET 0xBEB78u
#define SOUND_POLICY_SIZE (SOUND_WORK_SIZE-SOUND_POLICY_OFFSET)
_Static_assert(SOUND_POLICY_SIZE == 0x110u, "original sound tail");
#define SCRATCH (BASE+LOADER_SCRATCH_OFFSET)
#define STACK (BASE+LOADER_RAM_SIZE-0x1000u)
_Static_assert(LOADER_RAM_SIZE<=0x100000u,"guest RAM/scratch/stacks must precede import identities");
/* Acquisition journal for the synchronous original graphics constructor.
 * Existing IRQ owners run outside its heap capture; Main/VWait never run here. */
typedef struct {
    unsigned depth,callback_depth,regions,heap_count,heaps[8],raw_managers,tasks[160];
    unsigned old_task_queue[160],old_task_invoked[160];
    u8 owned_regions[1024];
    uint32_t next,pool;u8 storage[0x10c],pool_bytes[0xe0];
} HGGraphicsConstruction;
/* Regions map the actual native heap allocations. No duplicate guest payloads. */
struct HGOriginalLoader {
    DSThumb cpu; DSNativeRegion regions[1024];
    unsigned borrowed_regions[1024],region_heaps[1024],child_heaps[8],child_count;
    struct {uint32_t identity,heap,alignment;} allocators[64];unsigned allocator_count;
    struct {HGOriginalLoader *context;SysTask *native;uint32_t callback,data,identity;unsigned queue,invoked;const void *capture;} tasks[160];
    struct {uint32_t resource,record;} animation_resources[128];unsigned animation_resource_count;
    struct {uint32_t identity,allocator,resource,model;unsigned region;void *native;} sdk_animations[128];
    uint32_t default_free_functions[2];
    struct {uint32_t token;unsigned texture,kind;} sdk_keys[128];
    HGRawTex raw_views[128];
    uint32_t bottom_screen_guest_type;
    uint32_t field_manager_identity,field_effect_identity;
    DSMappingRef hblank_owner;
    void *canonical_field;uint32_t effects;GF_3DGfxRawResMan *effect_raw,*raw_managers[4];unsigned object_graphics_live;HGGraphicsConstruction *construction;
#ifdef HG_LOADER_TEST_HOOKS
    unsigned graphics_fail_import,graphics_imports,owner_fail_import,owner_imports;
    uint32_t sound_submission[6];unsigned sound_submission_count;
#endif
    uint32_t object_model_narc;
    const u8 *archive_bytes[1024];size_t archive_sizes[1024];
    struct { uint32_t guest;unsigned region,resident;uint64_t allocation[2];DSTextureBank bank; } textures[128];
    u16 edge_colors[8];
    struct {uint32_t identity,size,position;int fd;const void *capture;} files[8];
    uint32_t guest_hblank_cb, guest_hblank_arg;
    unsigned call_depth;
    DSAudio *audio;
    /* HG_NATIVE_RECURSIVE_SOUND_MUTEX */
    pthread_mutex_t snd_mutex;
    unsigned snd_mutex_ready, snd_mutex_depth;
    uint32_t sound_receiver[2];
    unsigned sound_cpu, arm7_irq_enabled;
    u8 *sound_work; /* complete original ARM32 SND_WORK backing */
    uint32_t rtc_time[3],light_material[2][3];int16_t light_vectors[12];uint16_t light_colors[4];
    uint32_t camera_fields[2];DSMappingRef camera_owner,camera_target,light_attributes;size_t camera_target_offset;
    DSCamera graphics_camera;unsigned camera_view_valid,owner_capture_depth;const void *owner_capture;
    uint32_t field_fog_identity;DSMappingRef weather_owner,field_texture_owner;unsigned field_texture_loaded;const void *narc_capture[8];
    DSMappingRef environment[3]; /* model attributes, fog: existing binding identities */
    unsigned texture_count,shrinks;HGOriginalPropAnimations *animations;
    int archive_ids[1024],archive_members[1024],narc_ids[8];
    Field3dRenderObjManager *render;Field3dRenderObj *render_objects[128];unsigned render_count,bound_models;
    void *canonical_save;
    MapMatrix *matrix; NARC *narcs[8]; MapPropManager *props[4];
    struct {uint32_t resource,model,object;size_t size;NNSG3dRenderObj native;NNSG3dRenderObj *native_object;unsigned terrain;} land[64];
    struct {uint32_t identity;unsigned resource;NNSG3dRenderObj native;} arm_objects[256];
    uint32_t next,owner,height_owner,area_owner;void *native_pools;NNSG3dResFileHeader **headers; unsigned area_loaded,initial_attempted,initialized,ending,ended,draw_triangles,billboards_live; const char *error;
};
/* One registry and address allocator for this guest address space. */
static DSMapping original_mapping(HGOriginalLoader *l) {
    return (DSMapping){
        .memory = &l->cpu.memory, .regions = l->regions,
        .borrowed = l->borrowed_regions,
        .capacity = NELEMS(l->regions), .next = &l->next,
        .allocation_limit = 0x03000000u
    };
}
static int graphics_acquiring(const HGOriginalLoader *l) {
    return l->construction&&(l->call_depth==l->construction->depth||
                             l->call_depth==l->construction->callback_depth);
}
static uint32_t reserve_guest(HGOriginalLoader *l,size_t size,uint32_t alignment) {
    DSMapping m=original_mapping(l);
    return ds_mapping_reserve(&m,size,alignment);
}
/* Retire the full backing allocation, including borrowed interior views. */
static void *retire_region(HGOriginalLoader *l,unsigned index) {
    DSNativeRegion *r=l->regions+index;void *bytes=r->bytes;size_t size=r->size;
    if(!bytes)return NULL;
    if(!hg_heap_allocation_size(bytes,&size))size=r->size;
    DSMapping m=original_mapping(l);
    GF_ASSERT(ds_mapping_retire_native(&m,bytes,size?size:1));
    return bytes;
}
static void retire_land(HGOriginalLoader *,uint32_t,size_t);
static void native_mapping_event(void *context,const void *base,size_t old_size,
                                  size_t new_size,BOOL release) {
    HGOriginalLoader *l=context;DSMapping m=original_mapping(l);
    if(release) {
        uintptr_t start=(uintptr_t)base,field=(uintptr_t)l->canonical_field;
        if(l->canonical_field&&field>=start&&(field-start<old_size||field==start)) {
            /* Original FieldSystem destruction follows FieldMap/effect exit.
             * Releasing a live callback root is an ownership violation. */
            GF_ASSERT(!l->effects&&!l->object_graphics_live);
            /* Pointer-field bytes belong to FieldSystem, not its camera.
             * Keep original dangling numeric values until this owner retires. */
            ds_mapping_unbind(&m,0x03100024);ds_mapping_unbind(&m,0x0310004c);l->camera_fields[0]=l->camera_fields[1]=0;
            l->canonical_field=NULL;
        }
        for(unsigned i=0;i<l->cpu.memory.region_count;i++) {
            DSNativeRegion *r=l->regions+i;uintptr_t p=(uintptr_t)r->bytes;
            if(r->bytes && p>=start && (p-start<old_size || p==start))
                retire_land(l,r->address,r->size);
        }
        ds_mapping_retire_native(&m,base,old_size?old_size:1);
    } else ds_mapping_native_shrink(&m,base,old_size,new_size);
}
static int observe_native(HGOriginalLoader *l,const void *bytes,size_t size) {
    uintptr_t p=(uintptr_t)bytes,base=(uintptr_t)l;
    /* Scalar projections embedded in the context have its lifetime. */
    if(p>=base && p-base<sizeof(*l) && size<=sizeof(*l)-(p-base))return 1;
    return hg_heap_observe(bytes,size,native_mapping_event,l,NULL);
}
static uint32_t borrow_native_at(HGOriginalLoader *l,uint32_t address,
                                 void *bytes,size_t size) {
    if(!l||!observe_native(l,bytes,size))return 0;
    DSMapping m=original_mapping(l);
    for(unsigned i=0;i<l->cpu.memory.region_count;i++) {
        DSNativeRegion *r=l->regions+i;
        if(r->address==address && !r->bytes && l->borrowed_regions[i])
            return ds_mapping_rebind(&m,(DSMappingRef){i,r->generation,m.memory},bytes,size);
    }
    return ds_mapping_bind(&m,address,bytes,size);
}
static void retire_fixed(HGOriginalLoader *l,uint32_t address) {
    DSMapping m=original_mapping(l);ds_mapping_unbind(&m,address);
}
static uint32_t word(HGOriginalLoader *l,uint32_t address) {
    const u8 *p=ds_translate(&l->cpu.memory,address,4);GF_ASSERT(p);
    return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;
}
static unsigned texture_resident(const HGOriginalLoader *l,unsigned i){
    unsigned resident=l->textures[i].resident;
    for(unsigned k=0;k<2;k++)if(l->textures[i].allocation[k]&&!hg_native_texture_valid(k,l->textures[i].allocation[k]))resident&=~(1u<<k);
    return resident;
}
static void texture_release(HGOriginalLoader *l,unsigned i){
    for(unsigned j=0;j<NELEMS(l->sdk_keys);j++)if(l->sdk_keys[j].token&&l->sdk_keys[j].texture==i)l->sdk_keys[j].token=0;
    for(unsigned k=0;k<2;k++)if(hg_native_texture_valid(k,l->textures[i].allocation[k]))GF_ASSERT(hg_native_texture_release(k,l->textures[i].allocation[k]));
    l->textures[i].allocation[0]=l->textures[i].allocation[1]=0;l->textures[i].resident=0;
}
static int upload_texture(HGOriginalLoader *l,unsigned j){
    unsigned i=l->textures[j].region;
                DSTextureBank bank;
                if(!l->archive_bytes[i]||!ds_texture_bank(l->archive_bytes[i],l->archive_sizes[i],&bank))return 0;
                if(hg_native_texture_pool_active(0)) {
                    GF_ASSERT(hg_native_texture_pool_active(1));
                    if(bank.bytes[24]||bank.bytes[25]){l->error="compressed texture allocation requires native format-5 support";return 0;}
                    uint32_t sizes[]={((uint32_t)bank.bytes[12]|(uint32_t)bank.bytes[13]<<8)*8,((uint32_t)bank.bytes[48]|(uint32_t)bank.bytes[49]<<8)*8};
                    if(texture_resident(l,j)!=3){
                        texture_release(l,j);
                        for(unsigned k=0;k<2;k++)if(sizes[k]){
                            l->textures[j].allocation[k]=hg_native_texture_reserve(k,sizes[k]);
                            if(!l->textures[j].allocation[k]){texture_release(l,j);return 0;}
                        }
                    }
                }
                l->textures[j].bank=bank;l->textures[j].resident=3;
    return 1;
}
/* Full upload headers and retained prefixes are distinct guest addresses of
 * one original raw texture owner. Resolve both without copying texel payloads. */
static int texture_identity(HGOriginalLoader *l,unsigned index,uint32_t guest){
    if(index>=l->texture_count||!l->regions[l->textures[index].region].bytes)return 0;
    if(l->textures[index].guest==guest)return 1;
    for(unsigned v=0;v<NELEMS(l->raw_views);v++)if(l->raw_views[v].owner&&l->raw_views[v].texture==index){
        for(unsigned r=0;r<l->cpu.memory.region_count;r++)if(l->regions[r].bytes==l->raw_views[v].owner&&l->archive_bytes[r]){
            size_t offset=(size_t)(l->raw_views[v].bytes-l->archive_bytes[r]);
            if(offset<l->regions[r].size&&l->regions[r].address+offset==guest)return 1;
        }
    }return 0;
}
static int resource_texture(HGOriginalLoader *l,uint32_t resource,unsigned *index){
    for(unsigned i=0;i<l->cpu.memory.region_count;i++){
        DSNativeRegion *r=l->regions+i;if(!r->bytes||r->address!=resource)continue;
        for(unsigned v=0;v<NELEMS(l->raw_views);v++)if(l->raw_views[v].owner==r->bytes){*index=l->raw_views[v].texture;return *index<l->texture_count;}
        DSTextureBank bank;if(!ds_texture_bank(r->bytes,r->size,&bank))return 0;
        uint32_t tex=resource+(uint32_t)(bank.bytes-r->bytes);
        for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,tex)){*index=j;return 1;}
        if(l->texture_count==NELEMS(l->textures))return 0;
        *index=l->texture_count++;l->textures[*index].guest=tex;l->textures[*index].region=i;return 1;
    }return 0;
}
static int model_resource(HGOriginalLoader *l,uint32_t resource,const DSTextureBank *bank,unsigned terrain){
    const u8 *header=ds_translate(&l->cpu.memory,resource,20);if(!header||memcmp(header,"BMD0",4))return -1;
    size_t size=word(l,resource+8);const u8 *bytes=ds_translate(&l->cpu.memory,resource,size);if(!bytes)return -1;
    for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].resource==resource)return (int)i;
    for(unsigned i=0;i<NELEMS(l->land);i++)if(!l->land[i].resource){
        NNSG3dResMdl *view=hg_prop_model_view_new(bytes,size,bank);if(!view)return -1;
        DSModel parsed;GF_ASSERT(ds_model_open(bytes,size,0,&parsed));
        l->land[i].resource=resource;l->land[i].model=resource+(uint32_t)(parsed.bytes-bytes);l->land[i].size=size;l->land[i].native.model=view;l->land[i].terrain=terrain;return (int)i;
    }return -1;
}
static int resolve_model(HGOriginalLoader *l,uint32_t reference){
    int model=-1;
        for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].resource&&l->land[i].model==reference){model=i;break;}
        if(model<0)for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].size>=20){
            DSModel parsed;DSNativeRegion *r=l->regions+i;
            if(memcmp(r->bytes,"BMD0",4)||!ds_model_open(r->bytes,r->size,0,&parsed)||r->address+(uint32_t)(parsed.bytes-r->bytes)!=reference)continue;
            DSTextureBank bank={0};if(l->archive_bytes[i])ds_texture_bank(l->archive_bytes[i],l->archive_sizes[i],&bank);
            model=model_resource(l,r->address,&bank,0);break;
        }
    return model;
}
static int setup_model_resource(HGOriginalLoader *l,uint32_t resource){
    unsigned j;if(!resource_texture(l,resource,&j)||!upload_texture(l,j))return -1;
    return model_resource(l,resource,&l->textures[j].bank,0);
}
static void retire_land(HGOriginalLoader *l,uint32_t address,size_t size) {
    for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].resource&&address<l->land[i].resource+l->land[i].size&&l->land[i].resource<address+size) {
        if(l->land[i].native_object)memset(l->land[i].native_object,0,sizeof(*l->land[i].native_object));
        for(unsigned j=0;j<NELEMS(l->arm_objects);j++)if(l->arm_objects[j].identity&&l->arm_objects[j].resource==i){hg_native_field_animation_detach_object(&l->arm_objects[j].native);memset(l->arm_objects+j,0,sizeof(l->arm_objects[j]));}
        hg_prop_model_view_delete(l->land[i].native.model);memset(&l->land[i],0,sizeof(l->land[i]));
    }
}
#include "compat/original_raw_resources.inc"
static void draw_triangle(void *user,const DSSceneVertex v[3]){(void)v;((HGOriginalLoader *)user)->draw_triangles++;}
static int draw_land(void *user,unsigned id,const u8 *bytes,size_t size,const DSTransform *transform,int material,int shape) {
    (void)id;(void)transform;HGOriginalLoader *l=user;DSModel model;DSModelStats stats;
    if(material!=-1||shape!=-1||!ds_model_open(bytes,size,0,&model))return 0;
    return ds_model_scene(&model,draw_triangle,l,&stats);
}
static int call(HGOriginalLoader *,uint32_t,const uint32_t *,unsigned);

/* HG_GUEST_HBLANK_BRIDGE */
static HGOriginalLoader *guest_hblank_owner;

static void guest_hblank_clear(HGOriginalLoader *l) {
    if(!l || guest_hblank_owner!=l)return;
    DSIrq *irq=hg_scheduler_irq();
    if(irq){
        ds_irq_disable_mask(irq,DS_IRQ_HBLANK);
        ds_irq_hblank_request(irq,0);
        irq->pending &= ~DS_IRQ_HBLANK;
        ds_irq_set_handler(irq,DS_IRQ_HBLANK,NULL);
    }
    l->guest_hblank_cb=l->guest_hblank_arg=0;
    guest_hblank_owner=NULL;
}

static void guest_hblank_dispatch(void) {
    HGOriginalLoader *l=guest_hblank_owner;
    if(!l || !l->guest_hblank_cb)return;
    uint32_t arg=l->guest_hblank_arg;
    if(!call(l,l->guest_hblank_cb&~1u,&arg,1)){
        fprintf(stderr,"Original HBlank callback failed: %s\n",
                l->error?l->error:"unknown guest failure");
        GF_ASSERT(FALSE);
    }
}

static int call_arm7(HGOriginalLoader *,uint32_t,const uint32_t *,unsigned,uint32_t *);
u32 canonical_effect_manager_priority(void *);
u32 canonical_manager_operation(void *,unsigned,u32);
void *canonical_manager_storage(void *,size_t *);
void canonical_manager_narc(void *,NARC *);
int canonical_loader_object_import(void *,const char *,DSThumb *);
static void original_resource_task(SysTask *task,void *data){
    HGOriginalLoader *l=*(HGOriginalLoader **)data;
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].native==task){
        uint32_t callback=l->tasks[i].callback,args[]={l->tasks[i].identity,l->tasks[i].data};l->tasks[i].invoked++;
        if(getenv("HG_TRACE_ORIGINAL"))fprintf(stderr,"HG_ORIGINAL_TRACE {\"event\":\"task_callback\",\"entry\":%u,\"queue\":%u,\"data\":%u}\n",callback&~1u,l->tasks[i].queue,l->tasks[i].data);
        unsigned callback_depth=l->construction?l->construction->callback_depth:0;
        if(l->construction&&l->construction->tasks[i])l->construction->callback_depth=l->call_depth+1;
        int ok=call(l,callback&~1u,args,2);
        if(l->construction)l->construction->callback_depth=callback_depth;
        if(!ok){fprintf(stderr,"Original resource task rejected: %s\n",l->error);GF_ASSERT(FALSE);}return;
    }GF_ASSERT(!"unregistered original resource task");
}
void ClearMainOAM(enum HeapID);void ClearSubOAM(enum HeapID);
static int boundary_import(DSThumb *,HGOriginalLoader *);
static int boundary(DSThumb *c,void *opaque) {
    HGOriginalLoader *l=opaque;
    const void *previous=hg_heap_capture(NULL);
    hg_scheduler_service_irqs();
    int constructing=l->construction&&l->call_depth==l->construction->depth;
#ifdef HG_LOADER_TEST_HOOKS
    if(constructing&&++l->graphics_imports==l->graphics_fail_import) {
        l->error="injected graphics construction failure";
        hg_heap_capture(previous);return -1;
    }
#endif
    #ifdef HG_LOADER_TEST_HOOKS
    if(l->owner_capture&&l->call_depth==l->owner_capture_depth&&++l->owner_imports==l->owner_fail_import){
        l->error="injected owner construction failure";hg_heap_capture(previous);return -1;
    }
#endif
    int acquiring=graphics_acquiring(l);
    hg_heap_capture(acquiring?(const void *)l->construction:
        l->owner_capture&&l->call_depth==l->owner_capture_depth?l->owner_capture:NULL);
    int result=boundary_import(c,l);
    if(acquiring)for(unsigned i=l->construction->regions;i<l->cpu.memory.region_count;i++)
        if(hg_heap_captured(l->regions[i].bytes,l->construction))l->construction->owned_regions[i]=1;
    hg_heap_capture(previous);return result;
}
/* ARM32 FSFileID is {archive identity, file index}; no host pointer crosses.
 * Only initialized/open FSFile identities occupy this bounded descriptor table. */
#define FILE_ARCHIVE 0x033e0000u
static void finish_files(HGOriginalLoader *l,const void *capture,int commit){
    for(unsigned i=0;i<NELEMS(l->files);i++)if(l->files[i].identity&&
        (!capture||l->files[i].capture==capture)){
        if(commit){l->files[i].capture=NULL;continue;}
        if(l->files[i].fd>=0)close(l->files[i].fd);
        memset(l->files+i,0,sizeof(l->files[i]));
    }
}
static int file_import(DSThumb *c,HGOriginalLoader *l,const char *name){
    if(strncmp(name,"FS_",3))return 0;
    if(!strcmp(name,"FS_ConvertPathToFileID")||!strcmp(name,"FS_OpenFile")){
        const char *path=ds_translate(&c->memory,c->r[1],1);if(!path)goto failed;
        size_t n=0;while(n<128&&ds_translate(&c->memory,c->r[1]+n,1)&&path[n])n++;
        if(n==128||!ds_translate(&c->memory,c->r[1]+n,1))goto failed;
        const char *prefix="fielddata/build_model/",*relative=NULL;
        if(!strncmp(path,prefix,strlen(prefix)))relative=path+strlen(prefix);
        else if(!strncmp(path,"data/",5))relative=path+5;
        int id=native_resource_file_id(relative);
        if(!strcmp(name,"FS_ConvertPathToFileID")){
            u8 *out=ds_translate(&c->memory,c->r[0],8);if(!out)goto failed;
            int fd=id<0?-1:native_resource_file_open(id);if(fd<0){c->r[0]=0;return 1;}close(fd);
            for(unsigned k=0;k<4;k++){out[k]=FILE_ARCHIVE>>(8*k);out[k+4]=(u32)id>>(8*k);}
            c->r[0]=1;return 1;
        }
        c->r[1]=FILE_ARCHIVE;c->r[2]=(u32)id;name="FS_OpenFileFast";
    }
    unsigned i;for(i=0;i<NELEMS(l->files);i++)if(l->files[i].identity==c->r[0])break;
    if(!strcmp(name,"FS_InitFile")){
        if(!c->r[0]||!ds_translate(&c->memory,c->r[0],0x48))goto failed;
        if(i<NELEMS(l->files)&&l->files[i].fd>=0)goto failed;
        if(i==NELEMS(l->files))for(i=0;i<NELEMS(l->files);i++)if(!l->files[i].identity)break;
        if(i==NELEMS(l->files))for(i=0;i<NELEMS(l->files);i++)if(l->files[i].fd<0)break;
        if(i==NELEMS(l->files))goto failed;
        memset(l->files+i,0,sizeof(l->files[i]));l->files[i].identity=c->r[0];l->files[i].fd=-1;return 1;
    }
    if(!strcmp(name,"FS_OpenFileFast")){
        if(i==NELEMS(l->files)||l->files[i].fd>=0)goto failed;
        int fd=c->r[1]==FILE_ARCHIVE?native_resource_file_open(c->r[2]):-1;
        struct stat st;if(fd<0){c->r[0]=0;return 1;}
        if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>INT32_MAX){close(fd);c->r[0]=0;return 1;}
        l->files[i].fd=fd;l->files[i].size=st.st_size;l->files[i].position=0;
        l->files[i].capture=l->call_depth==l->owner_capture_depth?l->owner_capture:NULL;
        c->r[0]=1;return 1;
    }
    if(strcmp(name,"FS_GetLength")&&strcmp(name,"FS_ReadFile")&&strcmp(name,"FS_SeekFile")&&strcmp(name,"FS_CloseFile"))return 0;
    if(i==NELEMS(l->files)||l->files[i].fd<0||!ds_translate(&c->memory,c->r[0],0x48))goto failed;
    if(!strcmp(name,"FS_GetLength")){c->r[0]=l->files[i].size;return 1;}
    if(!strcmp(name,"FS_SeekFile")){
        if(c->r[2]>2){c->r[0]=0;return 1;}
        /* SDK adds in ARM32 unsigned arithmetic before its signed clamp. */
        u32 sum=c->r[1];if(c->r[2]==1)sum+=l->files[i].position;else if(c->r[2]==2)sum+=l->files[i].size;
        s32 pos=(s32)sum;l->files[i].position=pos<0?0:(u32)pos>l->files[i].size?l->files[i].size:(u32)pos;c->r[0]=1;return 1;
    }
    if(!strcmp(name,"FS_ReadFile")){
        if((s32)c->r[2]<0){c->r[0]=UINT32_MAX;return 1;}
        u32 count=c->r[2],rest=l->files[i].size-l->files[i].position;if(count>rest)count=rest;
        void *out=ds_translate(&c->memory,c->r[1],count);if(!out)goto failed;
        ssize_t n;do {n=pread(l->files[i].fd,out,count,l->files[i].position);}while(n<0&&errno==EINTR);
        if(n>=0)l->files[i].position+=n;c->r[0]=(u32)n;return 1;
    }
    int ok=close(l->files[i].fd)==0;l->files[i].fd=-1;l->files[i].capture=NULL;c->r[0]=ok;return 1;
failed:l->error=name;return -1;
}
static int boundary_import(DSThumb *c,HGOriginalLoader *l) {
    const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_loader_imports);i++)if(c->r[15]==hg_loader_imports[i].address)name=hg_loader_imports[i].name;
    if(c->r[15]==0x033f0000)name="native_default_free_texture";
    if(c->r[15]==0x033f0004)name="native_default_free_palette";
    if(!name){fprintf(stderr,"Unmapped resource target pc=%08x lr=%08x\n",c->r[15],c->r[14]);l->error="unmapped guest target";return -1;}
    if(getenv("HG_TRACE_ORIGINAL"))fprintf(stderr,"HG_ORIGINAL_TRACE {\"event\":\"import\",\"symbol\":\"%s\",\"pc\":%u,\"lr\":%u,\"a0\":%u}\n",name,c->r[15],c->r[14],c->r[0]);

    /* Retail DS profile: the SDK's Ensata-only allocation probe is absent.
     * This is not a report that an ARM7 consumer is ready. */

    if(!strcmp(name,"Main_SetHBlankIntrCB")){
        if(!c->r[0]){
            if(guest_hblank_owner && guest_hblank_owner!=l){
                c->r[0]=0;return 1;
            }
            guest_hblank_clear(l);
            c->r[0]=1;return 1;
        }

        DSIrq *irq=hg_scheduler_irq();
        unsigned slot=0;
        while(slot<32 && !(DS_IRQ_HBLANK&(1u<<slot)))slot++;

        if(guest_hblank_owner || !irq || irq->dispatching ||
           slot==32 || irq->handlers[slot] ||
           !(c->r[0]&1u) ||
           !ds_translate(&c->memory,c->r[0]&~1u,2)){
            c->r[0]=0;return 1;
        }

        l->guest_hblank_cb=c->r[0];
        l->guest_hblank_arg=c->r[1];
        guest_hblank_owner=l;

        ds_irq_set_handler(irq,DS_IRQ_HBLANK,guest_hblank_dispatch);
        ds_irq_enable_mask(irq,DS_IRQ_HBLANK);
        ds_irq_hblank_request(irq,1);
        ds_irq_cpu_enable(irq,1);

        c->r[0]=1;return 1;
    }
    if(!strcmp(name,"OS_IsRunOnEmulator")){c->r[0]=0;return 1;}
    if(!strcmp(name,"PXI_SendWordByFifo")){
#ifdef HG_LOADER_TEST_HOOKS
        l->sound_submission_count=0;memset(l->sound_submission,0,sizeof(l->sound_submission));
        if(c->r[0]==7&&c->r[1]){
            u32 node=c->r[1],pool=HG_LOADER_hg_sound_snd_command_fence_sCommandArray;
            while(node&&l->sound_submission_count<256){
                if(node<pool||node-pool>=256*24||(node-pool)%24)break;
                if(!l->sound_submission_count)for(unsigned k=0;k<6;k++)l->sound_submission[k]=word(l,node+k*4);
                l->sound_submission_count++;node=word(l,node);
            }
        }
#endif
        if(c->r[0]!=7 || c->r[2] || !l->sound_receiver[1]) goto failed;
        if(!c->r[1]){
            if(!call_arm7(l,HG_LOADER_arm7_SND_CommandProc,NULL,0,NULL)) return -1;
            c->r[0]=0;
            return 1;
        }
        u32 queue=HG_LOADER_hg_sound_arm7_command_sCommandMesgQueue;
        if(word(l,queue+28)>=word(l,queue+20)){
            c->r[0]=UINT32_MAX;
            return 1;
        }
        u32 node=c->r[1],pool=HG_LOADER_hg_sound_snd_command_fence_sCommandArray;
        unsigned count=0;
        while(node){
            if(node<pool || node-pool>=256*24 || (node-pool)%24 || ++count>256 || word(l,node+4)>33){
                l->error="invalid ARM9 sound command list";
                return -1;
            }
            if(word(l,node+4)==29 && word(l,node+8)!=HG_LOADER_hg_sound_snd_command_fence_sSharedWork){
                l->error="sound shared-work owner mismatch";
                return -1;
            }
            node=word(l,node);
        }
        u32 args[]={7,c->r[1],0};
        if(!call_arm7(l,l->sound_receiver[1]&~1u,args,3,NULL)) return -1;
        c->r[0]=0;
        return 1;
    }
    if(!strcmp(name,"PXI_SetFifoRecvCallback")){
        if(c->r[0]!=7) goto failed;
        unsigned cpu=c->r[1]==(HG_LOADER_hg_sound_arm7_command_PxiFifoCallback|1u);
        if(!cpu && c->r[1]!=(HG_LOADER_hg_sound_snd_command_fence_PxiFifoCallback|1u)) goto failed;
        l->sound_receiver[cpu]=c->r[1];
        return 1;
    }
    if(!strcmp(name,"PXI_IsCallbackReady")){
        if(c->r[0]!=7 || c->r[1]!=1) goto failed;
        c->r[0]=l->sound_receiver[1]!=0;
        return 1;
    }
    if(!strcmp(name,"OS_WakeupThread")){
        /* These original message queues have no emulated thread waiters.
         * Waking an empty queue has no effect; any real waiter is unsupported. */
        if(word(l,c->r[0]) || word(l,c->r[0]+4)) goto failed;
        return 1;
    }
    if(!strcmp(name,"OS_SpinWait")){
        if(!word(l,HG_LOADER_hg_sound_arm7_command_sCommandMesgQueue+28)){
            l->error="sound wait has no pending ARM7 commands";
            return -1;
        }
        return call_arm7(l,HG_LOADER_arm7_SND_CommandProc,NULL,0,NULL)?1:-1;
    }

    if(!strcmp(name,"SNDi_LockMutex")){
        if(!l->snd_mutex_ready || pthread_mutex_lock(&l->snd_mutex))
            goto failed;
        l->snd_mutex_depth++;
        return 1;
    }
    if(!strcmp(name,"SNDi_UnlockMutex")){
        if(!l->snd_mutex_ready || !l->snd_mutex_depth)
            goto failed;
        if(pthread_mutex_unlock(&l->snd_mutex))
            goto failed;
        l->snd_mutex_depth--;
        return 1;
    }
    if(l->sound_cpu && (!strcmp(name,"OS_DisableInterrupts") ||
                       !strcmp(name,"OS_EnableInterrupts") ||
                       !strcmp(name,"OS_RestoreInterrupts"))){
        u32 before=l->arm7_irq_enabled?0:0x80;
        l->arm7_irq_enabled=!strcmp(name,"OS_RestoreInterrupts") ?
            !(c->r[0]&0x80) : strcmp(name,"OS_DisableInterrupts")!=0;
        c->r[0]=before;
        return 1;
    }
    /* IRQ_IMPORT_SCHEDULER_GUARD */
    if((!strcmp(name,"OS_DisableInterrupts")||
        !strcmp(name,"OS_EnableInterrupts")||
        !strcmp(name,"OS_RestoreInterrupts")) &&
       !hg_scheduler_irq())goto failed;
    if(!strcmp(name,"OS_DisableInterrupts")){
        c->r[0]=hg_native_irq_disable();return 1;
    }
    if(!strcmp(name,"OS_EnableInterrupts")){
        c->r[0]=hg_native_irq_enable();return 1;
    }
    if(!strcmp(name,"OS_RestoreInterrupts")){
        c->r[0]=hg_native_irq_restore(c->r[0]);return 1;
    }
    if(l->canonical_field){int object_result=canonical_loader_object_import(l->canonical_field,name,c);if(object_result){if(object_result<0)l->error=name;return object_result;}}
    int math=ds_thumb_math_boundary(c,name);if(math){if(math<0)l->error=name;return math;}
    if(!strcmp(name,"GX_LoadOAM")||!strcmp(name,"GXS_LoadOAM")){
        const void *bytes=ds_translate(&c->memory,c->r[0],c->r[2]);
        if(!bytes||c->r[1]>0x400||c->r[2]>0x400-c->r[1]||(c->r[0]|c->r[1]|c->r[2])&1)goto failed;
        if(!strcmp(name,"GX_LoadOAM"))hg_native_oam_load_main(bytes,c->r[1],c->r[2]);else hg_native_oam_load_sub(bytes,c->r[1],c->r[2]);return 1;
    }
    if(!strcmp(name,"DC_StoreRange")||!strcmp(name,"DC_FlushRange")||!strcmp(name,"DC_InvalidateRange")){
        const void *bytes=ds_translate(&c->memory,c->r[0],c->r[1]);if(!bytes)goto failed;hg_native_cache_publish(bytes,c->r[1]);return 1;
    }
    if(!strcmp(name,"MIi_CpuCopy16")||!strcmp(name,"MIi_CpuCopy32")){
        unsigned width=!strcmp(name,"MIi_CpuCopy16")?2:4;
        const void *bytes=ds_translate(&c->memory,c->r[0],c->r[2]);
        if(!bytes||(c->r[0]|c->r[1]|c->r[2])&(width-1))goto failed;
        uint32_t offset=c->r[1]-0x07000000u;
        if(offset<0x800u){unsigned e=offset/0x400;offset%=0x400;if(c->r[2]>0x400-offset)goto failed;
            if(!e)hg_native_oam_load_main(bytes,offset,c->r[2]);else hg_native_oam_load_sub(bytes,offset,c->r[2]);return 1;}
        void *out=ds_translate(&c->memory,c->r[1],c->r[2]);if(!out)goto failed;
        retire_land(l,c->r[1],c->r[2]);memmove(out,bytes,c->r[2]);return 1;
    }
    if(!strcmp(name,"GfGfx_EngineATogglePlanes")){
        extern void GfGfx_EngineATogglePlanes(u8,u8);
        if(c->r[0]>=32||c->r[1]>1)goto failed;GfGfx_EngineATogglePlanes(c->r[0],c->r[1]);return 1;
    }
    if(!strcmp(name,"G3X_SetFog")){
        if(c->r[0]>1||c->r[1]>1||c->r[2]>10||c->r[3]>=0x8000)goto failed;
        hg_native_fog(c->r[0],c->r[1],c->r[2],c->r[3]);return 1;
    }
    if(!strcmp(name,"ClearMainOAM")){ClearMainOAM(c->r[0]);return 1;}
    if(!strcmp(name,"ClearSubOAM")){ClearSubOAM(c->r[0]);return 1;}
    if(!strcmp(name,"GF3dRender_DrawModel")) {
        for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].object==c->r[0]&&l->land[i].native.model) {
            if(word(l,c->r[0]+4)!=l->land[i].model)goto failed;
            /* Only static land records are admitted; callbacks/animation pointers
             * remain explicit unsupported contracts, never native pointer casts. */
            for(unsigned k=8;k<0x3c;k+=4)if(word(l,c->r[0]+k))goto failed;
            VecFx32 t,s;MtxFx33 r;const void *tp=ds_translate(&c->memory,c->r[1],sizeof(t)),*rp=ds_translate(&c->memory,c->r[2],sizeof(r)),*sp=ds_translate(&c->memory,c->r[3],sizeof(s));
            if(!tp||!rp||!sp)goto failed;memcpy(&t,tp,sizeof(t));memcpy(&r,rp,sizeof(r));memcpy(&s,sp,sizeof(s));
            hg_prop_draw_begin(draw_land,l);GF3dRender_DrawModel(&l->land[i].native,&t,&r,&s);
            if(!hg_prop_draw_end(NULL))goto failed;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"sub_0201A430")){original_ClearFileCache();return 1;}
    if(!strcmp(name,"NNS_GfdResetLnkTexVramState")||!strcmp(name,"NNS_GfdResetLnkPlttVramState")) {
        unsigned bit=!strcmp(name,"NNS_GfdResetLnkTexVramState")?1:2;
        if(hg_native_texture_pool_active(bit==1?0:1))hg_native_texture_reset(bit==1?0:1);
        for(unsigned i=0;i<l->texture_count;i++)l->textures[i].resident&=~bit;
        retire_land(l,0x02000000,0x01000000);return 1;
    }
    if(!strcmp(name,"G3X_SetEdgeColorTable")) {const u8 *p=ds_translate(&c->memory,c->r[0],16);if(!p)goto failed;memcpy(l->edge_colors,p,16);return 1;}
    if(!strcmp(name,"sprintf")) {
        const char *p=ds_translate(&c->memory,c->r[1],1);if(!p)goto failed;size_t n=0;
        while(n<128&&ds_translate(&c->memory,c->r[1]+n,1)&&p[n])n++;
        if(n==128||!ds_translate(&c->memory,c->r[1]+n,1))goto failed;
        char formatted[160];int length;
        unsigned conversions=0;for(size_t k=0;k<n;k++)if(p[k]=='%'){if(k+1>=n||p[++k]!='d'||++conversions>1)goto failed;}
        if(conversions)length=snprintf(formatted,sizeof(formatted),p,(s32)c->r[2]);
        else {memcpy(formatted,p,n+1);length=n;}
        char *out=length<0?NULL:ds_translate(&c->memory,c->r[0],(size_t)length+1);
        if(!out||(size_t)length>=sizeof(formatted))goto failed;retire_land(l,c->r[0],length+1);memcpy(out,formatted,length+1);c->r[0]=length;return 1;
    }
    int fs=file_import(c,l,name);if(fs){if(fs<0)goto failed;return fs;}

    if(!strcmp(name,"ov01_02204698")) {if(c->r[0]!=0x03000204||!l->animations)goto failed;c->r[0]=hg_original_prop_land_animation_count(l->animations);return 1;}
    if(!strcmp(name,"MapPropAnimationManager_GetAnimListNARCFileCount")) {
        if(c->r[0]!=0x03000200||!l->animations)goto failed;
        c->r[0]=hg_original_prop_animations_count(l->animations);return 1;
    }
    if(!strcmp(name,"ov01_021E8F3C")) {
        unsigned i=(c->r[2]-0x03010000)/4;
        if(c->r[2]<0x03010000||c->r[2]%4||i>=l->render_count||!l->animations||word(l,c->r[13]+4)!=0x03000200)goto failed;
        const u8 *list=ds_translate(&c->memory,word(l,c->r[13]),24);if(!list)goto failed;
        for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,c->r[3])&&texture_resident(l,j)==3) {
            if(!hg_original_prop_animations_preload(l->animations,c->r[0],&l->render_objects[i]->renderObj,list,&l->textures[j].bank)) { l->error=hg_original_prop_animations_error(l->animations);return -1; }
            return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"NNS_G3dBindMdlSet")){
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].size>=20&&
            !memcmp(l->regions[i].bytes,"BMD0",4)&&l->regions[i].address+word(l,l->regions[i].address+16)==c->r[0]){
            for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,c->r[1])&&texture_resident(l,j)==3){
                if(model_resource(l,l->regions[i].address,&l->textures[j].bank,0)<0)goto failed;c->r[0]=1;return 1;
            }
        }goto failed;
    }
    if(!strcmp(name,"GF3dRender_BindModelSet")) {
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address==c->r[0]) {
            for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,c->r[1])&&texture_resident(l,j)==3) {
                int id=l->archive_members[i];
                if((l->archive_ids[i]!=NARC_a_1_4_8&&l->archive_ids[i]!=NARC_a_0_4_0)||id<0||id>=550)break;
                if(!hg_prop_resources_load(l->render,id,l->archive_bytes[i],l->archive_sizes[i])||
                    !hg_prop_resources_bind(l->render,id,&l->textures[j].bank))goto failed;
                l->bound_models++;c->r[0]=1;return 1;
            }
        }
        const u8 *header=ds_translate(&c->memory,c->r[0],20);if(!header||memcmp(header,"BMD0",4))goto failed;
        size_t size=word(l,c->r[0]+8);const u8 *bytes=ds_translate(&c->memory,c->r[0],size);if(!bytes)goto failed;
        for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,c->r[1])&&texture_resident(l,j)==3) {
            unsigned streamed=1;
            for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].address==c->r[0]&&l->archive_bytes[i])streamed=0;
            if(model_resource(l,c->r[0],&l->textures[j].bank,streamed)<0)goto failed;
            c->r[0]=1;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"DC_FlushRange")){if(!ds_translate(&c->memory,c->r[0],c->r[1]))goto failed;hg_native_cache_publish(ds_translate(&c->memory,c->r[0],c->r[1]),c->r[1]);return 1;}
    if(!strcmp(name,"NNSi_G3dModifyMatFlag")){
        if(c->r[2]>UINT16_MAX)goto failed;
        for(unsigned i=0;i<c->memory.region_count;i++){
            DSNativeRegion *r=l->regions+i;if(!r->bytes||r->size<20||memcmp(r->bytes,"BMD0",4))continue;
            DSModel model;if(!ds_model_open(r->bytes,r->size,0,&model)||r->address+(uint32_t)(model.bytes-r->bytes)!=c->r[0])continue;
            if(!ds_model_modify_material_flags(&model,(u8 *)model.bytes,c->r[2],c->r[1]!=0))goto failed;
            return 1;
        }goto failed;
    }
    if(!strcmp(name,"NNS_G3dResDefaultSetup")){c->r[0]=setup_model_resource(l,c->r[0])>=0;return 1;}
    if(!strcmp(name,"NNS_G3dRenderObjInit")) {
        u8 *record=ds_translate(&c->memory,c->r[0],0x54);if(!record)goto failed;
        int model=resolve_model(l,c->r[1]);
        if(model<0)goto failed;
        unsigned slot=NELEMS(l->arm_objects);
        for(unsigned i=0;i<NELEMS(l->arm_objects);i++)if(l->arm_objects[i].identity==c->r[0]){slot=i;break;}
        if(slot==NELEMS(l->arm_objects))for(unsigned i=0;i<NELEMS(l->arm_objects);i++)if(!l->arm_objects[i].identity){slot=i;break;}
        if(slot==NELEMS(l->arm_objects)||(l->arm_objects[slot].native.anmJnt||l->arm_objects[slot].native.anmMat))goto failed;
        l->arm_objects[slot].identity=c->r[0];l->arm_objects[slot].resource=model;l->arm_objects[slot].native=l->land[model].native;
        memset(record,0,0x54);for(unsigned k=0;k<4;k++)record[4+k]=c->r[1]>>(8*k);
        if(l->land[model].terrain)l->land[model].object=c->r[0];return 1;
    }
    if(!strcmp(name,"Field3dRenderObjManager_AllocRenderObj")) {
        if(c->r[0]!=0x03000080||c->r[1]>=550||l->render_count>=128)goto failed;
        l->render_objects[l->render_count]=Field3dRenderObjManager_AllocRenderObj(l->render,c->r[1]);
        c->r[0]=0x03010000+4*l->render_count++;return 1;
    }
    if(!strcmp(name,"NNS_G3dGetMdlSet")) {
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address==c->r[0]) {
            DSModel model;if(!ds_model_open(l->regions[i].bytes,l->regions[i].size,0,&model))goto failed;
            c->r[0]+=word(l,c->r[0]+16);return 1;
        }
        for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].resource==c->r[0]){c->r[0]+=word(l,c->r[0]+16);return 1;}
        goto failed;
    }
    if(!strcmp(name,"NNS_G3dTexReleaseTexKey")||!strcmp(name,"NNS_G3dPlttReleasePlttKey")){
        static uint32_t next_token=0x06000000;unsigned kind=!strcmp(name,"NNS_G3dPlttReleasePlttKey");
        for(unsigned i=0;i<l->texture_count;i++)if(texture_identity(l,i,c->r[0])){
            uint32_t token=0;
            if(l->textures[i].allocation[kind]){
                for(unsigned j=0;j<NELEMS(l->sdk_keys);j++)if(l->sdk_keys[j].token&&l->sdk_keys[j].texture==i&&l->sdk_keys[j].kind==kind)goto failed;
                unsigned key=0;while(key<NELEMS(l->sdk_keys)&&l->sdk_keys[key].token)key++;if(key==NELEMS(l->sdk_keys)||next_token==UINT32_MAX)goto failed;
                token=++next_token;l->sdk_keys[key].token=token;l->sdk_keys[key].texture=i;l->sdk_keys[key].kind=kind;
            }
            l->textures[i].resident&=~(1u<<kind);
            if(kind)c->r[0]=token;
            else {u8 *a=ds_translate(&c->memory,c->r[1],4),*b=ds_translate(&c->memory,c->r[2],4);if(!a||!b)goto failed;for(unsigned k=0;k<4;k++){a[k]=token>>(8*k);b[k]=0;}}
            return 1;
        }goto failed;
    }
    if(!strcmp(name,"native_default_free_texture")||!strcmp(name,"native_default_free_palette")){
        unsigned kind=!strcmp(name,"native_default_free_palette");if(!c->r[0])return 1;
        for(unsigned i=0;i<NELEMS(l->sdk_keys);i++)if(l->sdk_keys[i].token==c->r[0]&&l->sdk_keys[i].kind==kind){
            unsigned texture=l->sdk_keys[i].texture;uint64_t identity=l->textures[texture].allocation[kind];
            if(!hg_native_texture_valid(kind,identity)||!hg_native_texture_release(kind,identity))goto failed;
            l->textures[texture].allocation[kind]=0;l->sdk_keys[i].token=0;c->r[0]=0;return 1;
        }goto failed;
    }
    if(!strcmp(name,"NNS_G3dGetAnmByIdx")){
        if(c->r[1])goto failed;
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address==c->r[0]){
            DSAnimationResource resource;if(!ds_animation_resource_open(l->regions[i].bytes,l->regions[i].size,c->r[1],&resource)){l->error="NNS_G3dGetAnmByIdx: invalid animation resource/index";return -1;}
            const u8 *record=resource.record;
            if(l->animation_resource_count==NELEMS(l->animation_resources))goto failed;
            unsigned n=l->animation_resource_count++;l->animation_resources[n].resource=c->r[0];l->animation_resources[n].record=c->r[0]+(uint32_t)(record-l->regions[i].bytes);c->r[0]=l->animation_resources[n].record;return 1;
        }goto failed;
    }
    if(!strcmp(name,"NNS_G3dAllocAnmObj")){
        unsigned allocator=l->allocator_count;for(unsigned i=0;i<l->allocator_count;i++)if(l->allocators[i].identity==c->r[0])allocator=i;
        if(allocator==l->allocator_count||c->memory.region_count+2>NELEMS(l->regions))goto failed;
        unsigned resource=l->animation_resource_count;for(unsigned i=0;i<l->animation_resource_count;i++)if(l->animation_resources[i].record==c->r[1])resource=i;
        if(resource==l->animation_resource_count)goto failed;
        unsigned slot=NELEMS(l->sdk_animations);for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(!l->sdk_animations[i].identity){slot=i;break;}if(slot==NELEMS(l->sdk_animations))goto failed;
        uint32_t guest=reserve_guest(l,12,32);if(!guest)goto failed;
        unsigned i=c->memory.region_count++,heap=l->allocators[allocator].heap;
        l->regions[i]=(DSNativeRegion){guest,Heap_Alloc((enum HeapID)heap,12),4};l->region_heaps[i]=heap;
        GF_ASSERT(observe_native(l,l->regions[i].bytes,12));
        unsigned resource_slot=c->memory.region_count++;l->regions[resource_slot]=(DSNativeRegion){l->regions[i].address+8,l->regions[i].bytes+8,4};l->borrowed_regions[resource_slot]=1;
        /* Original field-model callers read frame at +0 and resource at +8.
         * All remaining SDK object state lives in the typed native sidecar;
         * all other ARM fields remain unmapped and reject, including offset +4. */
        memset(l->regions[i].bytes,0,12);for(unsigned k=0;k<4;k++)l->regions[i].bytes[8+k]=c->r[1]>>(8*k);
        l->sdk_animations[slot].identity=l->regions[i].address;l->sdk_animations[slot].allocator=c->r[0];l->sdk_animations[slot].resource=l->animation_resources[resource].resource;l->sdk_animations[slot].model=c->r[2];l->sdk_animations[slot].region=i;
        c->r[0]=l->regions[i].address;return 1;
    }
    if(!strcmp(name,"NNS_G3dAnmObjInit")){
        for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].identity==c->r[0]){
            if(l->sdk_animations[i].native||l->sdk_animations[i].model!=c->r[2]||word(l,c->r[0]+8)!=c->r[1])goto failed;
            int model=resolve_model(l,c->r[2]);if(model<0)goto failed;
            for(unsigned j=0;j<c->memory.region_count;j++)if(l->regions[j].bytes&&l->regions[j].address==l->sdk_animations[i].resource){
                l->sdk_animations[i].native=hg_native_field_animation_new(l->regions[j].bytes,l->regions[j].size,l->land[model].native.model,l->region_heaps[l->sdk_animations[i].region]);
                if(!l->sdk_animations[i].native){l->error="NNS_G3dAnmObjInit: unsupported field-model animation/material contract";return -1;}hg_native_field_animation_observe_frame(l->sdk_animations[i].native,l->regions[l->sdk_animations[i].region].bytes);return 1;
            }goto failed;
        }goto failed;
    }
    if(!strcmp(name,"NNS_G3dRenderObjAddAnmObj")||!strcmp(name,"NNS_G3dRenderObjRemoveAnmObj")){
        unsigned object=NELEMS(l->arm_objects),animation=NELEMS(l->sdk_animations);
        for(unsigned i=0;i<NELEMS(l->arm_objects);i++)if(l->arm_objects[i].identity==c->r[0])object=i;
        for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].identity==c->r[1]&&l->sdk_animations[i].native)animation=i;
        if(object==NELEMS(l->arm_objects)||animation==NELEMS(l->sdk_animations)||l->sdk_animations[animation].model!=l->land[l->arm_objects[object].resource].model)goto failed;
        if(!strcmp(name,"NNS_G3dRenderObjAddAnmObj")){if(!hg_native_field_animation_bind(&l->arm_objects[object].native,l->sdk_animations[animation].native))goto failed;}
        else {if(!hg_native_field_animation_is_bound(l->sdk_animations[animation].native,&l->arm_objects[object].native))goto failed;hg_native_field_animation_detach(l->sdk_animations[animation].native);}
        return 1;
    }
    if(!strcmp(name,"NNS_G3dFreeAnmObj")){
        for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].identity==c->r[1]){
            if(l->sdk_animations[i].allocator!=c->r[0])goto failed;unsigned region=l->sdk_animations[i].region;
            hg_native_field_animation_free(l->sdk_animations[i].native);
            Heap_Free(retire_region(l,region));memset(l->sdk_animations+i,0,sizeof(l->sdk_animations[i]));return 1;
        }goto failed;
    }
    if(!strcmp(name,"sub_0205F1A0")){
        if(c->r[0]!=0x03101000||!l->canonical_field)goto failed;c->r[0]=0x03102000;return 1;
    }
    const char *manager_ops[]={"MapObjectManager_GetFlagsBitsMask","MapObjectManager_GetObjectCount","MapObjectManager_SetFlagsBits","MapObjectManager_ClearFlagsBits","sub_0205F5D4"};
    for(unsigned op=0;op<NELEMS(manager_ops);op++)if(!strcmp(name,manager_ops[op])){
        if(c->r[0]!=0x03101000||!l->canonical_field)goto failed;c->r[0]=canonical_manager_operation(l->canonical_field,op,c->r[1]);return 1;
    }
    if(!strcmp(name,"MapObjectManager_SetMapModelNarc")){
        unsigned i=(c->r[1]-NARC_HANDLE)/4;
        if(c->r[0]!=0x03101000||c->r[1]<NARC_HANDLE||c->r[1]%4||i>=NELEMS(l->narcs)||!l->narcs[i])goto failed;
        canonical_manager_narc(l->canonical_field,l->narcs[i]);l->object_model_narc=c->r[1];return 1;
    }
    if(!strcmp(name,"MapObjectManager_GetMapModelNarc")){
        if(c->r[0]!=0x03101000||!l->object_model_narc)goto failed;c->r[0]=l->object_model_narc;return 1;
    }
    if(!strcmp(name,"MapObjectManager_GetPriority")){
        if(c->r[0]!=0x03101000||!l->canonical_field)goto failed;c->r[0]=canonical_effect_manager_priority(l->canonical_field);return 1;
    }
    if(!strcmp(name,"SysTask_CreateOnMainQueue")||!strcmp(name,"SysTask_CreateOnVBlankQueue")||!strcmp(name,"SysTask_CreateOnVWaitQueue")){
        /* Fast native LOAD can outrun the display. Relinquish the engine CPU
         * to a real pending/next VBlank before allocation, never fake a frame. */
        if(!strcmp(name,"SysTask_CreateOnVBlankQueue")&&hg_scheduler_vblank_active()&&gSystem.vblankTaskQueue->activeCount==gSystem.vblankTaskQueue->limit) {
            const void *capture=hg_heap_capture(NULL);hg_scheduler_wait_vblank();hg_heap_capture(capture);
        }
        unsigned i=0;while(i<NELEMS(l->tasks)&&l->tasks[i].native)i++;if(i==NELEMS(l->tasks)||!(c->r[0]&1)||!ds_translate(&c->memory,c->r[1],1)){fprintf(stderr,"Original resource queue rejected: callback=%08x data=%08x priority=%u slots=%u\n",c->r[0],c->r[1],c->r[2],i);goto failed;}
        if(graphics_acquiring(l)) {
            l->construction->tasks[i]=1;
            l->construction->old_task_queue[i]=l->tasks[i].queue;
            l->construction->old_task_invoked[i]=l->tasks[i].invoked;
        }
        l->tasks[i].capture=l->owner_capture&&l->call_depth==l->owner_capture_depth?l->owner_capture:NULL;
        l->tasks[i].invoked=0;l->tasks[i].context=l;l->tasks[i].callback=c->r[0];l->tasks[i].data=c->r[1];l->tasks[i].identity=0x03200000+4*i;
        if(!strcmp(name,"SysTask_CreateOnMainQueue")){l->tasks[i].queue=0;l->tasks[i].native=SysTask_CreateOnMainQueue(original_resource_task,&l->tasks[i],c->r[2]);}
        else if(!strcmp(name,"SysTask_CreateOnVBlankQueue")){l->tasks[i].queue=1;l->tasks[i].native=SysTask_CreateOnVBlankQueue(original_resource_task,&l->tasks[i],c->r[2]);}
        else {l->tasks[i].queue=2;l->tasks[i].native=SysTask_CreateOnVWaitQueue(original_resource_task,&l->tasks[i],c->r[2]);}
        if(!l->tasks[i].native){unsigned counts[3]={0};for(unsigned j=0;j<NELEMS(l->tasks);j++)if(l->tasks[j].native)counts[l->tasks[j].queue]++;fprintf(stderr,"Original resource queue exhausted: callback=%08x queue=%u active=%u/%u/%u\n",c->r[0],l->tasks[i].queue,counts[0],counts[1],counts[2]);goto failed;}c->r[0]=l->tasks[i].identity;return 1;
    }

    if(!strcmp(name,"SysTask_GetData")){
        u32 handle=c->r[0];
        if(handle<0x03200000u || (handle-0x03200000u)%4u)
            goto failed;
        unsigned i=(handle-0x03200000u)/4u;
        if(i>=NELEMS(l->tasks) || !l->tasks[i].native ||
           l->tasks[i].identity!=handle)
            goto failed;
        c->r[0]=l->tasks[i].data;
        return 1;
    }
    if(!strcmp(name,"SysTask_Destroy")){
        unsigned i=(c->r[0]-0x03200000)/4;if(c->r[0]<0x03200000||c->r[0]%4||i>=NELEMS(l->tasks)||!l->tasks[i].native)goto failed;
        SysTask_Destroy(l->tasks[i].native);l->tasks[i].native=NULL;return 1;
    }
    if(!strcmp(name,"HeapExp_FndInitAllocator")){
        HGHeapUsage usage;u8 *record=ds_translate(&c->memory,c->r[0],16);
        if(!record||!hg_heap_usage(c->r[1],&usage)||!c->r[2]||(c->r[2]&(c->r[2]-1))||c->r[2]>32||l->allocator_count==NELEMS(l->allocators))goto failed;
        for(unsigned i=0;i<l->allocator_count;i++)if(l->allocators[i].identity==c->r[0])goto failed;
        unsigned i=l->allocator_count++;l->allocators[i].identity=c->r[0];l->allocators[i].heap=c->r[1];l->allocators[i].alignment=c->r[2];
        /* SDK storage is opaque to original callers. Every native SDK consumer
         * must resolve this exact lease, never a DS heap/function pointer. */
        memset(record,0,16);return 1;
    }
    if(!strncmp(name,"GF3dGfxRawRes",13)){
        raw_context=l;
        if(setjmp(raw_rejected))return -1;
        if(!strcmp(name,"GF3dGfxRawResMan_Create")){
            unsigned slot=0;while(slot<NELEMS(l->raw_managers)&&l->raw_managers[slot])slot++;
            if(slot==NELEMS(l->raw_managers)||c->r[1]!=HEAP_ID_FIELD1||!c->r[0]||c->r[0]>128)goto failed;
            l->raw_managers[slot]=GF3dGfxRawResMan_Create(c->r[0],c->r[1]);
            if(graphics_acquiring(l))l->construction->raw_managers|=1u<<slot;
            if(!slot)l->effect_raw=l->raw_managers[slot];c->r[0]=0x03000308+4*slot;return 1;
        }
        unsigned manager=(c->r[0]-0x03000308)/4;
        if(!strncmp(name,"GF3dGfxRawResObj_",17))manager=(c->r[0]-0x03400000)/0x1000;
        if(manager>=NELEMS(l->raw_managers)||!l->raw_managers[manager])goto failed;
        GF_3DGfxRawResMan *raw_manager=l->raw_managers[manager];
        if(!strcmp(name,"GF3dGfxRawResMan_AllocObjAndKeys")||!strcmp(name,"GF3dGfxRawResMan_AllocObj")){
            if(!raw_manager||c->r[0]!=0x03000308+4*manager||c->r[3]>1)goto failed;
            void *resource=ds_translate(&c->memory,c->r[1],20);size_t size;
            if(!resource||!hg_heap_allocation_size(resource,&size))goto failed;
            GF_3DGfxRawResObj *obj=!strcmp(name,"GF3dGfxRawResMan_AllocObj")?GF3dGfxRawResMan_AllocObj(raw_manager,resource,c->r[2],c->r[3],word(l,c->r[13])):GF3dGfxRawResMan_AllocObjAndKeys(raw_manager,resource,c->r[2],c->r[3],word(l,c->r[13]));
            if(!obj){c->r[0]=0;return 1;}
            raw_map_prefix(obj,resource);c->r[0]=0x03400000+0x1000*manager+4*(unsigned)(obj-raw_manager->objs);return 1;
        }
        if(!strcmp(name,"GF3dGfxRawResMan_Destroy")){
            if(!raw_manager||c->r[0]!=0x03000308+4*manager)goto failed;
            GF3dGfxRawResMan_Destroy(raw_manager);l->raw_managers[manager]=NULL;if(!manager)l->effect_raw=NULL;return 1;
        }
        if(!strcmp(name,"GF3dGfxRawResMan_DoesNotHaveObjWithId")){if(c->r[0]!=0x03000308+4*manager)goto failed;c->r[0]=GF3dGfxRawResMan_DoesNotHaveObjWithId(raw_manager,c->r[1]);return 1;}
        if(!strcmp(name,"GF3dGfxRawResMan_FreeAllObjs")){if(c->r[0]!=0x03000308+4*manager)goto failed;GF3dGfxRawResMan_FreeAllObjs(raw_manager);return 1;}
        if(!raw_manager)goto failed;
        if(!strcmp(name,"GF3dGfxRawResMan_LoadObjTexById")||!strcmp(name,"GF3dGfxRawResMan_FreeObjVramAndSecondaryHeaderById")||!strcmp(name,"GF3dGfxRawResMan_FreeObjById")||!strcmp(name,"GF3dGfxRawResMan_GetObjById")){
            if(c->r[0]!=0x03000308+4*manager)goto failed;
            if(!strcmp(name,"GF3dGfxRawResMan_LoadObjTexById"))GF3dGfxRawResMan_LoadObjTexById(raw_manager,c->r[1]);
            else if(!strcmp(name,"GF3dGfxRawResMan_FreeObjVramAndSecondaryHeaderById"))GF3dGfxRawResMan_FreeObjVramAndSecondaryHeaderById(raw_manager,c->r[1]);
            else if(!strcmp(name,"GF3dGfxRawResMan_FreeObjById"))GF3dGfxRawResMan_FreeObjById(raw_manager,c->r[1]);
            else {GF_3DGfxRawResObj *obj=GF3dGfxRawResMan_GetObjById(raw_manager,c->r[1]);c->r[0]=obj?0x03400000+0x1000*manager+4*(unsigned)(obj-raw_manager->objs):0;}
            return 1;
        }
        unsigned index=(c->r[0]-0x03400000-0x1000*manager)/4;
        if(c->r[0]<0x03400000||(c->r[0]&3)||index>=(unsigned)raw_manager->man->max||!raw_manager->objs[index].obj)goto failed;
        GF_3DGfxRawResObj *obj=raw_manager->objs+index;
        if(!strcmp(name,"GF3dGfxRawResObj_GetTex")){HGRawTex *tex=GF3dGfxRawResObj_GetTex(obj);c->r[0]=raw_guest_address(tex->owner)+(uint32_t)(tex->bytes-l->archive_bytes[l->textures[tex->texture].region]);}
        else if(!strcmp(name,"GF3dGfxRawResObj_LoadTex"))GF3dGfxRawResObj_LoadTex(obj);
        else if(!strcmp(name,"GF3dGfxRawResObj_FreeVramAndSecondaryHeader"))GF3dGfxRawResObj_FreeVramAndSecondaryHeader(obj);
        else if(!strcmp(name,"GF3dGfxRawResObj_AllocVramAndGetKeys"))GF3dGfxRawResObj_AllocVramAndGetKeys(obj);
        else if(!strcmp(name,"GF3dGfxRawResObj_GetTexKey"))c->r[0]=GF3dGfxRawResObj_GetTexKey(obj);
        else if(!strcmp(name,"GF3dGfxRawResObj_GetTex4x4Key"))c->r[0]=GF3dGfxRawResObj_GetTex4x4Key(obj);
        else if(!strcmp(name,"GF3dGfxRawResObj_GetPlttKey"))c->r[0]=GF3dGfxRawResObj_GetPlttKey(obj);
        else goto failed;
        return 1;
    }
    if(!strcmp(name,"Heap_Create")){
        if(c->r[0]!=HEAP_ID_FIELD1||(c->r[1]!=63&&c->r[1]!=64&&c->r[1]!=68&&c->r[1]!=69)||l->child_count>=NELEMS(l->child_heaps))goto failed;
        HGHeapUsage child_usage,parent_usage;
        if(hg_heap_usage(c->r[1],&child_usage)||!hg_heap_usage(c->r[0],&parent_usage)||
           c->r[2]>parent_usage.capacity-parent_usage.allocated-parent_usage.reserved)goto failed;
        if(!Heap_Create(c->r[0],c->r[1],c->r[2])){c->r[0]=0;return 1;}
        l->child_heaps[l->child_count++]=c->r[1];
        if(graphics_acquiring(l))
            l->construction->heaps[l->construction->heap_count++]=c->r[1];
        c->r[0]=1;return 1;
    }
    if(!strcmp(name,"Heap_Destroy")){
        unsigned child=c->r[0],i=0;while(i<l->child_count&&l->child_heaps[i]!=child)i++;
        if(i==l->child_count)goto failed;
        for(unsigned j=0;j<c->memory.region_count;j++)if(l->regions[j].bytes&&l->region_heaps[j]==child){retire_land(l,l->regions[j].address,l->regions[j].size);retire_region(l,j);}
        Heap_Destroy(child);l->child_heaps[i]=l->child_heaps[--l->child_count];return 1;
    }
    if(!strcmp(name,"Heap_Alloc")||!strcmp(name,"Heap_AllocAtEnd")) {
        HGHeapUsage usage;if(!hg_heap_usage(c->r[0],&usage)||c->r[1]>0x40000||
            c->r[1]>usage.capacity-usage.allocated-usage.reserved||
            c->memory.region_count>=NELEMS(l->regions))goto failed;
        uint32_t guest=reserve_guest(l,c->r[1],16);if(!guest)goto failed;
        DSNativeRegion *r=&l->regions[c->memory.region_count++];
        r->address=guest;r->size=c->r[1];l->region_heaps[c->memory.region_count-1]=c->r[0];
        r->bytes=!strcmp(name,"Heap_AllocAtEnd")?Heap_AllocAtEnd(c->r[0],r->size):Heap_Alloc(c->r[0],r->size);
        GF_ASSERT(observe_native(l,r->bytes,r->size));
        c->r[0]=r->address;return 1;
    }
    if(!strcmp(name,"Heap_Realloc")) {
        for(unsigned i=0;i<c->memory.region_count;i++) {
            DSNativeRegion *r=&l->regions[i];
            if(!r->bytes||r->address!=c->r[0])continue;
            if(l->borrowed_regions[i])goto failed;
            size_t native_size;
            if(c->r[1]>r->size||!hg_heap_allocation_size(r->bytes,&native_size)||
               native_size<r->size)goto failed;
            /* Any texture in this header must have renderer-visible storage
             * before original code discards its source texels/palettes. */
            for(unsigned j=0;j<l->texture_count;j++)if(l->textures[j].region==i&&texture_resident(l,j)!=3)goto failed;
            DSMapping mapping=original_mapping(l);
            if(!ds_mapping_shrink(&mapping,r->address,c->r[1]))goto failed;
            Heap_Realloc(r->bytes,c->r[1]);l->shrinks++;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"GF3dRender_AllocAndLoadTexResources")||!strcmp(name,"GF3dRender_ResTexIsLoaded")) {
        if(!c->r[0]&&!strcmp(name,"GF3dRender_AllocAndLoadTexResources")){c->r[0]=0;return 1;}
        for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,c->r[0])) {
            unsigned i=l->textures[j].region;
            if(!l->regions[i].bytes)goto failed;
            if(!strcmp(name,"GF3dRender_AllocAndLoadTexResources")) {
                if(!upload_texture(l,j)){c->r[0]=0;return 1;}
            }
            c->r[0]=texture_resident(l,j)==3;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"FX_SinIdx")||!strcmp(name,"FX_CosIdx")) {
        unsigned index=((u16)c->r[0]>>4)+(!strcmp(name,"FX_CosIdx")?1024:0);
        c->r[0]=(u32)(s32)camera_sine[index&4095];return 1;
    }
    if(!strcmp(name,"FX_Mul")) {
        int64_t product=(int64_t)(s32)c->r[0]*(s32)c->r[1]+2048;
        int64_t q=product/4096;if(product<0&&product%4096)q--;
        c->r[0]=(u32)q;return 1;
    }
    if(!strcmp(name,"VEC_Add")||!strcmp(name,"VEC_Subtract")) {
        const u8 *a=ds_translate(&c->memory,c->r[0],12),*b=ds_translate(&c->memory,c->r[1],12);
        u8 *out=ds_translate(&c->memory,c->r[2],12);if(!a||!b||!out||(c->r[0]|c->r[1]|c->r[2])&3)goto failed;
        retire_land(l,c->r[2],12);u32 values[3];for(unsigned i=0;i<3;i++)values[i]=!strcmp(name,"VEC_Add")?word(l,c->r[0]+4*i)+word(l,c->r[1]+4*i):word(l,c->r[0]+4*i)-word(l,c->r[1]+4*i);
        for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)out[4*i+j]=values[i]>>(8*j);return 1;
    }
    if(!strcmp(name,"__aeabi_memcpy")||!strcmp(name,"__aeabi_memcpy4")||!strcmp(name,"memcpy")||!strcmp(name,"memmove")) {
        void *out=ds_translate(&c->memory,c->r[0],c->r[2]);const void *in=ds_translate(&c->memory,c->r[1],c->r[2]);
        if(!out||!in)goto failed;retire_land(l,c->r[0],c->r[2]);memmove(out,in,c->r[2]);return 1;
    }
    if(!strcmp(name,"NNS_G3dGlbLookAt")) {
        int32_t v[3][3];for(unsigned i=0;i<3;i++){if((c->r[i]&3)||!ds_translate(&c->memory,c->r[i],12))goto failed;for(unsigned j=0;j<3;j++)v[i][j]=(s32)word(l,c->r[i]+4*j);}
        if(!ds_camera_look_at(&l->graphics_camera,v[0],v[1],v[2]))goto failed;l->camera_view_valid=1;return 1;
    }
    if(!strcmp(name,"NNS_G3dGlbPerspective")||!strcmp(name,"NNS_G3dGlbOrtho")) {
        int32_t a[6];unsigned n=!strcmp(name,"NNS_G3dGlbPerspective")?5:6;
        if(!ds_translate(&c->memory,c->r[13],4*(n-4)))goto failed;
        for(unsigned i=0;i<n;i++)a[i]=(s32)(i<4?c->r[i]:word(l,c->r[13]+4*(i-4)));
        if(!(n==5?ds_camera_perspective(&l->graphics_camera,a):ds_camera_ortho(&l->graphics_camera,a)))goto failed;return 1;
    }
    if(!strcmp(name,"hg_field_rtc_time")){
        extern void canonical_field_rtc_time(void *,uint32_t [3]);
        canonical_field_rtc_time(l->canonical_field,l->rtc_time);
        c->r[0]=hg_original_map_native(l,l->rtc_time,sizeof(l->rtc_time));return c->r[0]?1:-1;
    }
    if(!strcmp(name,"NNS_G3dGlbLightVector")){
        if(c->r[0]>=4)goto failed;for(unsigned i=0;i<3;i++)l->light_vectors[3*c->r[0]+i]=(s16)c->r[i+1];return 1;
    }
    if(!strcmp(name,"NNS_G3dGlbLightColor")){if(c->r[0]>=4||c->r[1]>=0x8000)goto failed;l->light_colors[c->r[0]]=c->r[1];return 1;}
    if(!strcmp(name,"NNS_G3dGlbMaterialColorDiffAmb")||!strcmp(name,"NNS_G3dGlbMaterialColorSpecEmi")){
        if(c->r[0]>=0x8000||c->r[1]>=0x8000||c->r[2]>1)goto failed;
        memcpy(l->light_material[!strcmp(name,"NNS_G3dGlbMaterialColorSpecEmi")],c->r,12);return 1;
    }
    if(!strcmp(name,"NNS_G3dGlbPolygonAttr")) {
        u8 *stack=ds_translate(&c->memory,c->r[13],8);if(!stack)goto failed;
        unsigned alpha=stack[0]|(u32)stack[1]<<8|(u32)stack[2]<<16|(u32)stack[3]<<24;
        unsigned misc=stack[4]|(u32)stack[5]<<8|(u32)stack[6]<<16|(u32)stack[7]<<24;
        if(c->r[0]>=16||c->r[1]>=4||c->r[2]>=4||c->r[3]>=64||alpha>=32||(misc&~0xf800u))goto failed;
        hg_native_global_polygon_attr(c->r[0],c->r[1],c->r[2],c->r[3],alpha,misc);return 1;
    }
    if(!strcmp(name,"Heap_FreeExplicit")){
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address==c->r[1]){
            if(l->borrowed_regions[i]||l->region_heaps[i]!=c->r[0])goto failed;
            retire_land(l,l->regions[i].address,l->regions[i].size);Heap_FreeExplicit(c->r[0],retire_region(l,i));return 1;
        }goto failed;
    }
    if(!strcmp(name,"Heap_Free")) {
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address==c->r[0]) {
            if(l->borrowed_regions[i])goto failed;
            retire_land(l,l->regions[i].address,l->regions[i].size);
            for(unsigned j=0;j<l->texture_count;j++)if(l->textures[j].region==i)texture_release(l,j);
            for(unsigned j=0;j<l->allocator_count;)if(l->allocators[j].identity>=l->regions[i].address&&l->allocators[j].identity-l->regions[i].address<l->regions[i].size)l->allocators[j]=l->allocators[--l->allocator_count];else j++;
            Heap_Free(retire_region(l,i));return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"MapPropManager_New")) {
        if(c->r[0]!=HEAP_ID_FIELD1||(c->r[1]&&c->r[1]!=0x03000080))goto failed;
        for(unsigned i=0;i<4;i++)if(!l->props[i]) {
            l->props[i]=MapPropManager_New(HEAP_ID_FIELD1,c->r[1]?l->render:NULL);c->r[0]=0x03000100+4*i;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"MapPropManager_LoadFromNARC")) {
        unsigned n=(c->r[0]-NARC_HANDLE)/4,i=(c->r[2]-0x03000100)/4;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||n>=NELEMS(l->narcs)||!l->narcs[n]||c->r[2]<0x03000100||c->r[2]%4||i>=4||!l->props[i]||c->r[1]%48||c->r[1]/48>MAP_PROP_MAX)goto failed;
        MapPropManager_LoadFromNARC(l->narcs[n],c->r[1],l->props[i],c->r[3]);return 1;
    }
    if(!strcmp(name,"MapPropManager_Free")) {
        unsigned i=(c->r[0]-0x03000100)/4;
        if(c->r[0]<0x03000100||c->r[0]%4||i>=4||!l->props[i])goto failed;
        MapPropManager_Free(l->props[i]);l->props[i]=NULL;return 1;
    }
    if(!strcmp(name,"MI_CpuClear32")) {
        if((c->r[0]&3)||(c->r[1]&3))goto failed;
        u8 *p=ds_translate(&c->memory,c->r[0],c->r[1]);if(!p)goto failed;
        retire_land(l,c->r[0],c->r[1]);memset(p,0,c->r[1]);return 1;
    }
    if(!strcmp(name,"MIi_CpuClearFast")||!strcmp(name,"MIi_CpuClear32")||!strcmp(name,"MIi_CpuClear16")) {
        unsigned width=!strcmp(name,"MIi_CpuClear16")?2:4;
        u8 *p=ds_translate(&c->memory,c->r[1],c->r[2]);if(!p||c->r[1]%width||c->r[2]%width)goto failed;
        retire_land(l,c->r[1],c->r[2]);for(unsigned i=0;i<c->r[2];i++)p[i]=c->r[0]>>(8*(i%width));return 1;
    }
    if(!strcmp(name,"MI_CpuFill8")||!strcmp(name,"memset")) {
        if(!c->r[2])return 1;
        void *p=ds_translate(&c->memory,c->r[0],c->r[2]);if(!p)goto failed;
        retire_land(l,c->r[0],c->r[2]);memset(p,c->r[1]&255,c->r[2]);return 1;
    }
    if(!strcmp(name,"_s32_div_f")||!strcmp(name,"__aeabi_idiv")||!strcmp(name,"__aeabi_idivmod")||!strcmp(name,"_u32_div_f")||!strcmp(name,"__aeabi_uidiv")) {
        if(!c->r[1])goto failed;
        if(strcmp(name,"_u32_div_f")&&strcmp(name,"__aeabi_uidiv")) {
            int64_t a=(int32_t)c->r[0],b=(int32_t)c->r[1];
            c->r[0]=(u32)(a/b);c->r[1]=(u32)(a%b);
        } else {u32 a=c->r[0],b=c->r[1];c->r[0]=a/b;c->r[1]=a%b;}
        return 1;
    }
    if(!strcmp(name,"MapMatrix_GetMatrixId")) {if(c->r[0]!=MATRIX)goto failed;c->r[0]=MapMatrix_GetMatrixId(l->matrix);return 1;}
    if(!strcmp(name,"MapMatrix_GetMapAltitude")) {if(c->r[0]!=MATRIX)goto failed;c->r[0]=MapMatrix_GetMapAltitude(l->matrix,c->r[1],c->r[2],c->r[3],word(l,c->r[13]));return 1;}
    if(!strcmp(name,"GetMapModelNo")) {if(c->r[1]!=MATRIX)goto failed;c->r[0]=GetMapModelNo(c->r[0],l->matrix);return 1;}
    if(!strcmp(name,"MapMatrix_GetWidth")||!strcmp(name,"MapMatrix_GetHeight")) {
        if(c->r[0]!=MATRIX)goto failed;
        c->r[0]=!strcmp(name,"MapMatrix_GetWidth")?MapMatrix_GetWidth(l->matrix):MapMatrix_GetHeight(l->matrix);return 1;
    }
    if(!strcmp(name,"NNS_G3dGetTex")) {
        for(unsigned i=0;i<c->memory.region_count;i++) {
            DSNativeRegion *r=&l->regions[i];
            if(!r->bytes||r->address!=c->r[0])continue;
            /* Original raw managers retain a validated header prefix while the
             * immutable cache continues to own the texels. The full-file parser
             * correctly rejects that truncated payload; resolve its typed view. */
            for(unsigned v=0;v<NELEMS(l->raw_views);v++)if(l->raw_views[v].owner==r->bytes){
                unsigned texture=l->raw_views[v].texture;
                size_t offset=(size_t)(l->raw_views[v].bytes-l->archive_bytes[i]);
                if(texture>=l->texture_count||offset>r->size||r->size-offset<60){fprintf(stderr,"Typed TEX trace: texture=%u/%u region=%u bytes=%zu offset=%zu tag=%.4s\n",texture,l->texture_count,i,r->size,offset,r->bytes);goto failed;}
                c->r[0]=r->address+(u32)offset;return 1;
            }
            DSTextureBank bank;
            if(!ds_texture_bank(r->bytes,r->size,&bank)){
                /* A valid MDL0-only BMD has no embedded texture resource. The
                 * original object manager binds a separate raw texture owner. */
                DSModel model;
                if(r->size>=20&&!memcmp(r->bytes,"BMD0",4)&&r->bytes[14]==1&&!r->bytes[15]&&ds_model_open(r->bytes,r->size,0,&model)){c->r[0]=0;return 1;}
                fprintf(stderr,"Texture header trace: region=%u bytes=%zu tag=%.4s blocks=%u archive=%d member=%d\n",i,r->size,r->bytes,r->size>15?r->bytes[14]:0,l->archive_ids[i],l->archive_members[i]);
                goto failed;
            }
            size_t offset=(size_t)(bank.bytes-r->bytes);
            if(offset>=r->size)goto failed;
            uint32_t tex=r->address+(u32)offset;
            for(unsigned j=0;j<l->texture_count;j++)if(texture_identity(l,j,tex)){c->r[0]=tex;return 1;}
            if(l->texture_count>=NELEMS(l->textures)){l->error="native texture identity capacity";return -1;}
            unsigned j=l->texture_count++;l->textures[j].guest=tex;l->textures[j].region=i;
            c->r[0]=tex;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"ReadWholeNarcMemberByIdPair")) {
        const uint8_t *bytes;size_t size;
        if(!native_resource_member(c->r[1],0,c->r[2],&bytes,&size))goto failed;
        void *p=ds_translate(&c->memory,c->r[0],size);if(!p)goto failed;
        memcpy(p,bytes,size);return 1;
    }
    if(!strcmp(name,"GfGfxLoader_LoadFromOpenNarc")){
        unsigned n=(c->r[0]-NARC_HANDLE)/4,at_end=word(l,c->r[13]);const u8 *bytes;size_t size,allocated;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||n>=NELEMS(l->narcs)||!l->narcs[n]||c->r[2]||at_end>1||
           !native_resource_member(l->narc_ids[n],0,c->r[1],&bytes,&size)||c->memory.region_count==NELEMS(l->regions))goto failed;
        void *result=GfGfxLoader_LoadFromOpenNarc(l->narcs[n],c->r[1],FALSE,c->r[3],at_end);
        if(!result)goto failed;
        if(!hg_heap_allocation_size(result,&allocated)){Heap_Free(result);goto failed;}
        uint32_t guest=reserve_guest(l,allocated,16);if(!guest){Heap_Free(result);goto failed;}
        unsigned i=c->memory.region_count++;l->regions[i]=(DSNativeRegion){guest,result,allocated};l->region_heaps[i]=c->r[3];
        GF_ASSERT(observe_native(l,result,allocated));
        l->archive_ids[i]=l->narc_ids[n];l->archive_members[i]=c->r[1];l->archive_bytes[i]=bytes;l->archive_sizes[i]=size;
        c->r[0]=l->regions[i].address;return 1;
    }
    if(!strcmp(name,"GfGfxLoader_LoadFromNarc")){
        unsigned at_end=word(l,c->r[13]);const u8 *bytes;size_t size,allocated;
        if(!native_resource_member(c->r[0],0,c->r[1],&bytes,&size)||c->memory.region_count==NELEMS(l->regions))goto failed;
        void *result=GfGfxLoader_LoadFromNarc(c->r[0],c->r[1],c->r[2],c->r[3],at_end);
        if(!result)goto failed;
        if(!hg_heap_allocation_size(result,&allocated)){Heap_Free(result);goto failed;}
        uint32_t guest=reserve_guest(l,allocated,16);if(!guest){Heap_Free(result);goto failed;}
        unsigned i=c->memory.region_count++;l->regions[i]=(DSNativeRegion){guest,result,allocated};l->region_heaps[i]=c->r[3];
        GF_ASSERT(observe_native(l,result,allocated));
        l->archive_ids[i]=c->r[0];l->archive_members[i]=c->r[1];
        if(!c->r[2]){l->archive_bytes[i]=bytes;l->archive_sizes[i]=size;}
        c->r[0]=l->regions[i].address;return 1;
    }
    if(!strcmp(name,"NARC_GetMemberSize")||!strcmp(name,"NARC_GetFileCount")){
        unsigned n=(c->r[0]-NARC_HANDLE)/4;const u8 *bytes;size_t size;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||n>=NELEMS(l->narcs)||!l->narcs[n])goto failed;
        if(!strcmp(name,"NARC_GetFileCount")){c->r[0]=NARC_GetFileCount(l->narcs[n]);return 1;}
        if(!native_resource_member(l->narc_ids[n],0,c->r[1],&bytes,&size)||size>UINT32_MAX)goto failed;c->r[0]=(u32)size;return 1;
    }
    if(!strcmp(name,"NARC_ReadWholeMember")) {
        unsigned n=(c->r[0]-NARC_HANDLE)/4;const uint8_t *bytes;size_t size;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||n>=NELEMS(l->narcs)||!l->narcs[n]||
            !native_resource_member(l->narc_ids[n],0,c->r[1],&bytes,&size))goto failed;
        void *p=ds_translate(&c->memory,c->r[2],size);if(!p)goto failed;memcpy(p,bytes,size);
        for(unsigned i=0;i<c->memory.region_count;i++)if(l->regions[i].address==c->r[2]){l->archive_bytes[i]=bytes;l->archive_sizes[i]=size;l->archive_ids[i]=l->narc_ids[n];l->archive_members[i]=c->r[1];}
        return 1;
    }
    if(!strcmp(name,"AllocAndReadWholeNarcMemberByIdPair")||!strcmp(name,"NARC_AllocAndReadWholeMember")) {
        int archive=c->r[0],member=c->r[1];
        if(!strcmp(name,"NARC_AllocAndReadWholeMember")) {
            unsigned n=(c->r[0]-NARC_HANDLE)/4;
            if(c->r[0]<NARC_HANDLE||c->r[0]%4||n>=NELEMS(l->narcs)||!l->narcs[n])goto failed;
            archive=l->narc_ids[n];
        }
        const uint8_t *bytes;size_t size;
        if(c->r[2]!=HEAP_ID_FIELD1||!native_resource_member(archive,0,member,&bytes,&size)||
            !size||size>0x40000||c->memory.region_count>=NELEMS(l->regions))goto failed;
        uint32_t guest=reserve_guest(l,size,16);if(!guest)goto failed;
        DSNativeRegion *r=&l->regions[c->memory.region_count++];
        r->address=guest;r->size=size;l->region_heaps[c->memory.region_count-1]=HEAP_ID_FIELD1;r->bytes=Heap_Alloc(HEAP_ID_FIELD1,size);
        GF_ASSERT(observe_native(l,r->bytes,size));
        l->archive_ids[c->memory.region_count-1]=archive;l->archive_members[c->memory.region_count-1]=member;
        l->archive_bytes[c->memory.region_count-1]=bytes;l->archive_sizes[c->memory.region_count-1]=size;
        memcpy(r->bytes,bytes,size);c->r[0]=r->address;
        if((archive==NARC_a_1_4_8||archive==NARC_a_0_4_0)&&!hg_prop_resources_load(l->render,member,bytes,size))goto failed;
        return 1;
    }
    if(!strcmp(name,"NARC_ReadFile")) {
        unsigned i=(c->r[0]-NARC_HANDLE)/4;void *p=ds_translate(&c->memory,c->r[2],c->r[1]);
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||i>=NELEMS(l->narcs)||!l->narcs[i]||!p)goto failed;
        retire_land(l,c->r[2],c->r[1]);NARC_ReadFile(l->narcs[i],c->r[1],p);return 1;
    }
    if(!strcmp(name,"NARC_ReadFromMember")) {
        unsigned i=(c->r[0]-NARC_HANDLE)/4;const u8 *bytes;size_t size;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||i>=NELEMS(l->narcs)||!l->narcs[i]||!native_resource_member(l->narc_ids[i],0,c->r[1],&bytes,&size)||c->r[2]>size||c->r[3]>size-c->r[2])goto failed;
        void *p=ds_translate(&c->memory,word(l,c->r[13]),c->r[3]);if(!p)goto failed;retire_land(l,word(l,c->r[13]),c->r[3]);NARC_ReadFromMember(l->narcs[i],c->r[1],c->r[2],c->r[3],p);return 1;
    }
    if(!strcmp(name,"NARC_New")) {
        const uint8_t *bytes;size_t size;
        if(c->r[1]!=HEAP_ID_FIELD1||!native_resource_member(c->r[0],0,0,&bytes,&size))goto failed;
        for(unsigned i=0;i<NELEMS(l->narcs);i++)if(!l->narcs[i]) {
            l->narc_capture[i]=l->owner_capture&&l->call_depth==l->owner_capture_depth?l->owner_capture:NULL;
            l->narc_ids[i]=c->r[0];l->narcs[i]=NARC_New(c->r[0],c->r[1]);c->r[0]=NARC_HANDLE+4*i;return 1;
        }
        goto failed;
    }
    if(!strcmp(name,"NARC_Delete")) {
        unsigned i=(c->r[0]-NARC_HANDLE)/4;
        if(c->r[0]<NARC_HANDLE||c->r[0]%4||i>=NELEMS(l->narcs)||!l->narcs[i])goto failed;
        NARC_Delete(l->narcs[i]);l->narcs[i]=NULL;l->narc_capture[i]=NULL;return 1;
    }
failed:fprintf(stderr,"Original import rejected: %s args=%08x/%08x/%08x/%08x lr=%08x\n",name,c->r[0],c->r[1],c->r[2],c->r[3],c->r[14]);l->error=name;return -1;
}

/* HG_ARCHIVE_POINTER_TRACE */
static u32 archive_trace_word(HGOriginalLoader *l,u32 addr){
    const u8 *p=addr?ds_translate(&l->cpu.memory,addr,4):NULL;
    if(!p)return 0;
    return (u32)p[0]|((u32)p[1]<<8)|((u32)p[2]<<16)|((u32)p[3]<<24);
}
static void archive_trace_state(HGOriginalLoader *l,u32 *arc,u32 *info){
    *arc=archive_trace_word(l,HG_LOADER_hg_sound_snd_sndarc_sCurrent);
    *info=*arc?archive_trace_word(l,*arc+0x84u):0;
}
static int call_result(HGOriginalLoader *l,uint32_t entry,const uint32_t *args,unsigned count,uint32_t *out) {
    unsigned snd_depth_before=l->snd_mutex_depth;
    DSIrq *entry_irq=hg_scheduler_irq();unsigned entry_irq_enabled=entry_irq?entry_irq->cpu_enabled:0;

    DSThumb *c=&l->cpu,saved=*c;unsigned depth=l->call_depth++;
    GF_ASSERT(depth<8);uint32_t stack=STACK-depth*0x2000;
    memset(c->r,0,sizeof(c->r));l->error=NULL;
    c->r[13]=stack;c->r[14]=0x03fffffdu;c->return_address=c->r[14];c->r[15]=entry;
    for(unsigned i=0;i<count;i++) {
        if(i<4)c->r[i]=args[i];else {
            u8 *p=ds_translate(&c->memory,stack+4*(i-4),4);GF_ASSERT(p);
            for(unsigned k=0;k<4;k++)p[k]=args[i]>>(8*k);
        }
    }
    if(getenv("HG_TRACE_ORIGINAL"))fprintf(stderr,"HG_ORIGINAL_TRACE {\"event\":\"call\",\"entry\":%u,\"depth\":%u}\n",entry,depth);
    u32 trace_arc_before=0,trace_info_before=0;
    int trace_archive=getenv("HG_TRACE_ARCHIVE_POINTERS")!=NULL;
    if(trace_archive)
        archive_trace_state(l,&trace_arc_before,&trace_info_before);

    DSThumbResult result=ds_thumb_run(c,1000000);
    if(trace_archive){
        u32 trace_arc_after=0,trace_info_after=0;
        archive_trace_state(l,&trace_arc_after,&trace_info_after);
        if(result!=DS_THUMB_RETURN||
           trace_arc_before!=trace_arc_after||
           trace_info_before!=trace_info_after)
            fprintf(stderr,
                "ARCHIVE STATE entry=%08x result=%u pc=%08x "
                "arc=%08x->%08x info=%08x->%08x fault=%08x\\n",
                entry,(unsigned)result,c->r[15],
                trace_arc_before,trace_arc_after,
                trace_info_before,trace_info_after,c->fault_address);
    }

    if(getenv("HG_TRACE_ORIGINAL"))fprintf(stderr,"HG_ORIGINAL_TRACE {\"event\":\"return\",\"entry\":%u,\"result\":%u,\"depth\":%u,\"steps\":%zu}\n",entry,result,depth,c->steps);
    if(entry==HG_LOADER_ov01_021F1450)
        fprintf(stderr,
            "LOOKUP TRACE: arg0=%08x id=%u depth=%u result=%u r0=%08x pc=%08x lr=%08x steps=%zu\\n",
            args[0],args[1],depth,result,c->r[0],c->r[15],c->r[14],c->steps);
    if(out && result==DS_THUMB_RETURN)*out=c->r[0];
    l->call_depth--;if(depth){saved.memory=c->memory;*c=saved;}
    if(result!=DS_THUMB_RETURN) {
        if(entry_irq)entry_irq->cpu_enabled=entry_irq_enabled;
        while(l->snd_mutex_depth>snd_depth_before){
            if(pthread_mutex_unlock(&l->snd_mutex))break;
            l->snd_mutex_depth--;
        }
        if(!l->error){fprintf(stderr,"Original guest failure: result=%u pc=%08x address=%08x instruction=%04x steps=%zu\n",result,c->r[15],c->fault_address,c->fault_instruction,c->steps);l->error="original loader instruction/memory/budget boundary";}
        return 0;
    }
    return 1;
}

static int call_arm7(HGOriginalLoader *l,uint32_t entry,const uint32_t *args,unsigned count,uint32_t *out){
    unsigned cpu=l->sound_cpu,irq=l->arm7_irq_enabled;
    l->sound_cpu=1;
    int ok=call_result(l,entry,args,count,out);
    l->sound_cpu=cpu;
    if(!ok)l->arm7_irq_enabled=irq;
    return ok;
}

static int call(HGOriginalLoader *l,uint32_t entry,const uint32_t *args,unsigned count) {
    return call_result(l,entry,args,count,NULL);
}
/* Original SDK software owners are local to this resource context. The staged
 * ARM7 command service owns original driver RAM; hardware and periodic sound
 * processing remain distinct boundaries from the borrowed native PCM owner. */
int hg_original_sound_bind(HGOriginalLoader *l,DSAudio *audio){
    if(!l||l->call_depth||l->audio||!audio||!ds_audio_acquire(audio))return 0;
    if(!l->snd_mutex_ready){
        pthread_mutexattr_t attr;
        int rc=pthread_mutexattr_init(&attr);
        if(!rc){
            rc=pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
            if(!rc)rc=pthread_mutex_init(&l->snd_mutex,&attr);
            pthread_mutexattr_destroy(&attr);
        }
        if(rc){
            l->error="native sound mutex initialization";
            ds_audio_release(audio);
            return 0;
        }
        l->snd_mutex_ready=1;
    }
    uint32_t work=HG_LOADER_IMPORT_sSoundWork;
    if(!call(l,HG_LOADER_NNSi_SndPlayerInit,NULL,0)||
       !call(l,HG_LOADER_NNSi_SndCaptureInit,NULL,0)||
       !call(l,HG_LOADER_NNSi_SndInitResourceMgr,NULL,0)){
        ds_audio_release(audio);return 0;
    }


    /* Initialize Nintendo's persistent sound heap and file-backed SDAT.
     * Keep the original ARM32 SDK algorithms and their INFO/FAT pointers. */
    u8 *saved_work=malloc(SOUND_WORK_SIZE);
    u8 *saved_statics=malloc(HG_SOUND_STATIC_BYTES);
    void *saved_files=malloc(sizeof(l->files));
    if(!saved_work||!saved_statics||!saved_files){
        free(saved_work);free(saved_statics);free(saved_files);
        l->error="sound archive snapshot allocation";
        ds_audio_release(audio);return 0;
    }
    unsigned saved_arm7_irq=l->arm7_irq_enabled;
    u32 saved_receivers[2];
    memcpy(saved_receivers,l->sound_receiver,sizeof(saved_receivers));
    memcpy(saved_work,l->sound_work,SOUND_WORK_SIZE);
    memcpy(saved_files,l->files,sizeof(l->files));
    int old_fds[NELEMS(l->files)];
    for(unsigned i=0;i<NELEMS(l->files);i++)old_fds[i]=l->files[i].fd;
    unsigned off=0;
    for(unsigned i=0;i<NELEMS(sound_static_regions);i++){
        const u8 *p=ds_translate(&l->cpu.memory,
            sound_static_regions[i].address,sound_static_regions[i].size);
        GF_ASSERT(p);
        memcpy(saved_statics+off,p,sound_static_regions[i].size);
        off+=sound_static_regions[i].size;
    }

    const u32 path_addr=STACK+0x100u;
    const char path[]="data/sound/gs_sound_data.sdat";
    u8 *guest_path=ds_translate(&l->cpu.memory,path_addr,sizeof(path));
    u8 *heap_slot=ds_translate(&l->cpu.memory,work+0x94u,4);
    u32 heap=0;
    int ok=guest_path&&heap_slot;
    if(ok)ok=call(l,HG_LOADER_GF_SoundDataInit,&work,1);
    if(ok){
        memcpy(guest_path,path,sizeof(path));
        const u32 heap_args[]={work+0x98u,0xBEAE0u};
        ok=call_result(l,HG_LOADER_NNS_SndHeapCreate,heap_args,2,&heap)&&heap;
        if(ok){
            for(unsigned k=0;k<4;k++)heap_slot[k]=(u8)(heap>>(8*k));
            const u32 arc_args[]={work,path_addr,heap,0};
            ok=call(l,HG_LOADER_NNS_SndArcInit,arc_args,4);
        }
        if(ok){
            u32 setup_result=0;
            ok=call_result(l,HG_LOADER_NNS_SndArcPlayerSetup,
                           &heap,1,&setup_result)&&setup_result;
            if(!ok && !l->error)
                l->error="original sound archive player setup";
        }
        if(ok){
            const u32 radio_arg=work+0x94u;
            ok=call(l,HG_LOADER_SndRadio_Init,&radio_arg,1);
        }
        if(ok)ok=call(l,HG_LOADER_GF_SndHandleInitAll,&work,1);
        if(ok)ok=call(l,HG_LOADER_sub_02004898,&work,1);
    }
    if(ok)l->arm7_irq_enabled=1;
    if(ok)ok=call_arm7(l,HG_LOADER_arm7_SND_ExChannelInit,NULL,0,NULL);
    if(ok)ok=call_arm7(l,HG_LOADER_arm7_SND_SeqInit,NULL,0,NULL);
    if(ok)ok=call_arm7(l,HG_LOADER_arm7_SND_CommandInit,NULL,0,NULL);
    if(ok)ok=call(l,HG_LOADER_SND_CommandInit,NULL,0);
    if(ok)ok=call_arm7(l,HG_LOADER_arm7_SND_CommandProc,NULL,0,NULL);
    const u32 nonblock=0;
    u32 reply=0;
    if(ok){
        do {
            ok=call_result(l,HG_LOADER_SND_RecvCommandReply,&nonblock,1,&reply);
        } while(ok && reply);
    }
    if(!ok){
        const char *failure=l->error?l->error:"original sound archive initialization";
        for(unsigned i=0;i<NELEMS(l->files);i++)
            if(l->files[i].fd>=0&&l->files[i].fd!=old_fds[i])
                close(l->files[i].fd);
        memcpy(l->files,saved_files,sizeof(l->files));
        memcpy(l->sound_work,saved_work,SOUND_WORK_SIZE);
        memcpy(l->sound_receiver,saved_receivers,sizeof(saved_receivers));
        l->arm7_irq_enabled=saved_arm7_irq;
        off=0;
        for(unsigned i=0;i<NELEMS(sound_static_regions);i++){
            u8 *p=ds_translate(&l->cpu.memory,
                sound_static_regions[i].address,sound_static_regions[i].size);
            GF_ASSERT(p);
            memcpy(p,saved_statics+off,sound_static_regions[i].size);
            off+=sound_static_regions[i].size;
        }
        l->error=failure;
    }
    free(saved_work);free(saved_statics);free(saved_files);
    if(!ok){ds_audio_release(audio);return 0;}
    l->audio=audio;return 1;
}
static int sound_transaction(HGOriginalLoader *l,uint32_t entry,const uint32_t *args,unsigned count,uint32_t *result){
    /* No sound archive or heap is synthesized. The entire original software
     * owner (including private SDK lists/capture state) must roll back, not
     * only the scene number. No active device command is admitted here. */
    u8 *work=ds_translate(&l->cpu.memory,HG_LOADER_IMPORT_sSoundWork,SOUND_WORK_SIZE);
    u8 *control=ds_translate(&l->cpu.memory,HG_LOADER__021D05E8,0x10);
    if(!work||!control)return 0;
    u8 *before=malloc(SOUND_WORK_SIZE);
    if(!before){l->error="sound work snapshot allocation";return 0;}
    u8 before_control[0x10],statics[HG_SOUND_STATIC_BYTES];
    unsigned offset=0;
    for(unsigned i=0;i<NELEMS(sound_static_regions);i++){
        const u8 *p=ds_translate(&l->cpu.memory,sound_static_regions[i].address,sound_static_regions[i].size);GF_ASSERT(p);
        memcpy(statics+offset,p,sound_static_regions[i].size);offset+=sound_static_regions[i].size;
    }
    memcpy(before,work,SOUND_WORK_SIZE);memcpy(before_control,control,sizeof(before_control));
    DSAudio *snapshot=l->audio?ds_audio_clone(l->audio):NULL;
    if(l->audio&&!snapshot){free(before);l->error="sound request snapshot allocation";return 0;}
    DSIrq *sound_irq=hg_scheduler_irq();
    unsigned saved_irq_enabled=sound_irq?sound_irq->cpu_enabled:0;
    unsigned saved_arm7_irq=l->arm7_irq_enabled;
    u32 saved_receivers[2];
    memcpy(saved_receivers,l->sound_receiver,sizeof(saved_receivers));
    uint32_t scene_result=0;
    unsigned char files_before[sizeof(l->files)];memcpy(files_before,l->files,sizeof(files_before));
    int ok=call_result(l,entry,args,count,&scene_result);
    l->sound_cpu=0;
    /* Commands stay owned by ARM9 until original ARM7 dispatch publishes its
     * shared completion epoch and original ARM9 reply handling recycles them.
     * Any unsupported operation restores both CPUs and the delivery queues. */
    if(ok){
        uint32_t flags=1,flushed=0;
        ok=call_result(l,HG_LOADER_SND_FlushCommand,&flags,1,&flushed)&&flushed;
    }
    if(ok)ok=call_arm7(l,HG_LOADER_arm7_SND_CommandProc,NULL,0,NULL);
    if(ok){
        u32 nonblock=0;
        u32 reply;
        do {
            ok=call_result(l,HG_LOADER_SND_RecvCommandReply,&nonblock,1,&reply);
        } while(ok && reply);
    }
    if(!ok){
        memcpy(l->sound_receiver,saved_receivers,sizeof(saved_receivers));
        l->arm7_irq_enabled=saved_arm7_irq;
        memcpy(work,before,SOUND_WORK_SIZE);memcpy(control,before_control,sizeof(before_control));
        offset=0;
        for(unsigned i=0;i<NELEMS(sound_static_regions);i++){
            memcpy(ds_translate(&l->cpu.memory,sound_static_regions[i].address,sound_static_regions[i].size),statics+offset,sound_static_regions[i].size);
            offset+=sound_static_regions[i].size;
        }
        memcpy(l->files,files_before,sizeof(files_before));
        if(snapshot)GF_ASSERT(ds_audio_restore_snapshot(l->audio,snapshot));
        if(sound_irq)sound_irq->cpu_enabled=saved_irq_enabled;
    }
    if(ok&&result)*result=scene_result;
    ds_audio_free(snapshot);free(before);return ok;
}
int hg_original_sound_policy(HGOriginalLoader *l,enum HGOriginalSoundPolicy operation,
                             unsigned value,uint32_t *result) {
    if(!l||l->call_depth||(unsigned)operation>HG_SOUND_FIELD_SCENE||value>0xffffu||(operation==HG_SOUND_FIELD_GUARD&&value>1u))return 0;
    if(operation==HG_SOUND_FIELD_SCENE&&!l->audio){l->error="original sound owner unbound";return 0;}
    uint32_t entries[]={HG_LOADER_Sound_SetFieldBGM,HG_LOADER_sub_02005CF4,HG_LOADER_sub_02004EB4};
    uint32_t args[]={value};
    if(operation!=HG_SOUND_FIELD_SCENE)return call_result(l,entries[operation],args,1,result);
    return sound_transaction(l,entries[operation],args,1,result);
}
int hg_original_sound_request(HGOriginalLoader *l,enum HGOriginalSoundPolicy operation,
                              const uint32_t *args,unsigned count,uint32_t *result){
    if(!l||!args||l->call_depth)return 0;
    if((unsigned)operation<=HG_SOUND_FIELD_SCENE)
        return count==1&&hg_original_sound_policy(l,operation,args[0],result);
    static const struct {uint32_t entry;unsigned count;} entries[]={
        {HG_LOADER_StopSE,2}, {HG_LOADER_GF_SndHandleMoveVolume,3},
        {HG_LOADER_PlaySE,1}, {HG_LOADER_sub_02006088,1}
    };
    unsigned index=(unsigned)operation-HG_SOUND_STOP_SE;
    if(index>=NELEMS(entries)||count!=entries[index].count)return 0;
    if(!l->audio){l->error="original sound owner unbound";return 0;}
    return sound_transaction(l,entries[index].entry,args,count,result);
}
static u8 *video_register(uint32_t address,unsigned width){
    if(width!=2||(address!=0x04000008u&&address!=0x0400000cu&&address!=0x04000050u))return NULL;
    return hg_native_video()->engine[0]+(address-0x04000000u);
}
static int video_read(void *context,uint32_t address,unsigned width,uint32_t *out){
    (void)context;const u8 *p=video_register(address,width);if(!p)return 0;*out=p[0]|(u32)p[1]<<8;return 1;
}
static int video_write(void *context,uint32_t address,unsigned width,uint32_t value){
    (void)context;u8 *p=video_register(address,width);if(!p)return 0;p[0]=value;p[1]=value>>8;return 1;
}
HGOriginalLoader *hg_original_resource_context_new(void) {
    HGOriginalLoader *l=Heap_Alloc(HEAP_ID_DEFAULT,sizeof(*l));memset(l,0,sizeof(*l));
    l->next=0x02800000;
    l->cpu.memory=(DSMainRAM){.ram=Heap_Alloc(HEAP_ID_DEFAULT,LOADER_RAM_SIZE),.size=LOADER_RAM_SIZE,.regions=l->regions};
    GF_ASSERT(sizeof(hg_loader_image)<STACK-BASE-7*0x2000);memset(l->cpu.memory.ram,0,LOADER_RAM_SIZE);
    memcpy(l->cpu.memory.ram,hg_loader_image,sizeof(hg_loader_image));
    l->cpu.code_start=BASE;l->cpu.code_size=HG_LOADER_CODE_SIZE;l->cpu.boundary=boundary;l->cpu.boundary_user=l;l->cpu.device_read=video_read;l->cpu.device_write=video_write;
    l->regions[l->cpu.memory.region_count++]=(DSNativeRegion){0x04000060,hg_native_video()->engine[0]+0x60,2};
    l->default_free_functions[0]=0x033f0001;l->default_free_functions[1]=0x033f0005;
    for(unsigned i=0;i<2;i++){
        unsigned slot=l->cpu.memory.region_count++;uint32_t address=i?HG_LOADER_IMPORT_NNS_GfdDefaultFuncFreePlttVram:HG_LOADER_IMPORT_NNS_GfdDefaultFuncFreeTexVram;
        l->regions[slot]=(DSNativeRegion){address,(u8 *)(l->default_free_functions+i),4};l->borrowed_regions[slot]=1;
    }
    /* One coherent, persistent original SND_WORK byte image.
     * SDK archive and heap initialization are separate lifecycle steps. */
    l->sound_work=calloc(1,SOUND_WORK_SIZE);
    GF_ASSERT(l->sound_work);
    l->regions[l->cpu.memory.region_count]=(DSNativeRegion){
        HG_LOADER_IMPORT_sSoundWork,l->sound_work,SOUND_WORK_SIZE
    };
    l->borrowed_regions[l->cpu.memory.region_count++]=1;
    return l;
}
HGOriginalLoader *hg_original_loader_new(MapMatrix *matrix,unsigned mode) {
    GF_ASSERT(matrix&&mode<=2);
    HGOriginalLoader *l=hg_original_resource_context_new();GF_ASSERT(hg_original_loader_attach_map(l,matrix,mode));return l;
}
int hg_original_loader_attach_map(HGOriginalLoader *l,MapMatrix *matrix,unsigned mode){
    if(!matrix||mode>2||l->owner)return 0;l->matrix=matrix;
    if(!l->render)l->render=hg_prop_resources_new();
    uint32_t args[]={MATRIX,l->area_owner,0x03000080,0x03000200,0x03000204,mode,l->canonical_save?0x0300020c:0};
    GF_ASSERT(call(l,HG_LOADER_MapLoadManager_New,args,NELEMS(args)));l->owner=l->cpu.r[0];GF_ASSERT(l->owner);
    GF_ASSERT(word(l,l->owner+0xc0)==MATRIX&&word(l,l->owner+0xc4)==MapMatrix_GetWidth(matrix));
    return 1;
}
int hg_original_loader_initial_load(HGOriginalLoader *l,int x,int z) {
    if(!l->area_loaded){l->error="AreaDataManager_Load must complete before InitialLoad";return 0;}
    if(l->initial_attempted){l->error="loader InitialLoad already attempted";return 0;}
    l->initial_attempted=1;
    if(!l->height_owner){uint32_t height_args[]={8,HEAP_ID_FIELD1};if(!call(l,HG_LOADER_DynamicTerrainHeightManager_New,height_args,2))return 0;l->height_owner=l->cpu.r[0];}
    uint32_t args[]={l->owner,(u32)x,(u32)z};l->initialized=call(l,HG_LOADER_MapLoadManager_InitialLoad,args,NELEMS(args));
    return l->initialized;
}
int hg_original_loader_prepare_area(HGOriginalLoader *l,unsigned map) {
    uint32_t args[]={MapHeader_GetAreaDataBank(map)};
    if(!call(l,HG_LOADER_AreaDataManager_Alloc,args,1))return 0;
    uint32_t area=l->cpu.r[0];l->area_owner=area;u8 *slot=ds_translate(&l->cpu.memory,l->owner+0xb8,4);GF_ASSERT(slot);
    for(unsigned k=0;k<4;k++)slot[k]=area>>(8*k);
    uint32_t list=word(l,area+0x8c0);unsigned slot_id=(list-NARC_HANDLE)/4;
    GF_ASSERT(slot_id<NELEMS(l->narcs)&&l->narcs[slot_id]&&!l->animations);
    l->animations=hg_original_prop_animations_new(l->narcs[slot_id]);
    uint32_t load_args[]={area,0x03000080,0x03000200,0,0};
    /* Dependent managers remain unprovided. This probe must stop before use. */
    l->area_loaded=call(l,HG_LOADER_AreaDataManager_Load,load_args,5);return l->area_loaded;
}
const char *hg_original_loader_error(const HGOriginalLoader *l){return l->error;}
unsigned hg_original_loader_texture_count(const HGOriginalLoader *l){return l->texture_count;}
const DSTextureBank *hg_original_loader_texture(const HGOriginalLoader *l,unsigned i) {
    return i<l->texture_count&&texture_resident(l,i)==3?&l->textures[i].bank:NULL;
}
unsigned hg_original_loader_texture_shrinks(const HGOriginalLoader *l){return l->shrinks;}
unsigned hg_original_loader_bound_models(const HGOriginalLoader *l){return l->bound_models;}
int hg_original_loader_model_material(HGOriginalLoader *l,unsigned model,unsigned index,DSModelMaterial *material,DSTexture *texture) {
    for(unsigned i=0;i<l->texture_count;i++)if(l->textures[i].resident&&texture_resident(l,i)!=3)return 0;
    return hg_prop_resources_material(l->render,model,index,material,texture);
}
void hg_original_loader_delete(HGOriginalLoader *l) {
    guest_hblank_clear(l);
    for(unsigned i=0;i<NELEMS(l->tasks);i++)GF_ASSERT(!l->tasks[i].native);
    for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].native){hg_native_field_animation_free(l->sdk_animations[i].native);l->sdk_animations[i].native=NULL;}
    GF_ASSERT(!l->effects&&!l->effect_raw&&!l->child_count);
    GF_ASSERT((!l->initialized||l->ended)&&!l->billboards_live&&!word(l,HG_LOADER__021D0EB0));
    if(l->height_owner){uint32_t h[]={l->height_owner};GF_ASSERT(call(l,HG_LOADER_DynamicTerrainHeightManager_Free,h,1));}
    retire_land(l,0x02000000,0x01000000);
    hg_original_prop_animations_free(l->animations);
    for(unsigned i=0;i<4;i++)if(l->props[i])MapPropManager_Free(l->props[i]);
    if(l->render)hg_prop_resources_free_preloaded(l->render);
    for(unsigned i=0;i<l->texture_count;i++)texture_release(l,i);
    if(l->area_loaded){uint32_t target=l->owner?l->owner+0xb8:SCRATCH+0x300;
        u8 *area_slot=ds_translate(&l->cpu.memory,target,4);GF_ASSERT(area_slot);
        for(unsigned k=0;k<4;k++)area_slot[k]=l->area_owner>>(8*k);
        uint32_t area_args[]={target};GF_ASSERT(call(l,HG_LOADER_AreaDataManager_Free,area_args,1));GF_ASSERT(!word(l,target));l->area_owner=0;}
    if(l->owner){uint32_t args[]={l->owner};GF_ASSERT(call(l,HG_LOADER_MapLoadManager_FreeNARCAndLoadedMapBuffers,args,1));}
    for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes&&l->regions[i].address!=0x04000060&&!l->borrowed_regions[i])Heap_Free(retire_region(l,i));
    for(unsigned i=0;i<NELEMS(l->narcs);i++)if(l->narcs[i])NARC_Delete(l->narcs[i]);
    finish_files(l,NULL,FALSE);
    if(l->snd_mutex_ready){
        GF_ASSERT(l->snd_mutex_depth==0);
        GF_ASSERT(pthread_mutex_destroy(&l->snd_mutex)==0);
        l->snd_mutex_ready=0;
    }
    if(l->audio){ds_audio_release(l->audio);l->audio=NULL;}
    hg_heap_unobserve(native_mapping_event,l);
    free(l->sound_work);
    Heap_Free(l->cpu.memory.ram);if(raw_context==l){raw_context=NULL;raw_current=NULL;}Heap_Free(l);
}

void hg_original_loader_tick_animations(HGOriginalLoader *l){GF_ASSERT(l->area_loaded);hg_original_prop_animations_tick(l->animations);}
int hg_original_loader_animation_sample(HGOriginalLoader *l,double matrix[6]) {
    for(unsigned i=0;i<l->render_count;i++)for(unsigned j=0;j<32;j++) {
        DSModelMaterial mat;DSTexture tex;Field3dRenderObj *r=l->render_objects[i];
        if(hg_prop_resources_material(l->render,r->index,j,&mat,&tex)&&hg_prop_render_animation_matrix(&r->renderObj,&mat,matrix)==1)return 1;
    }
    return 0;
}

unsigned hg_original_loader_active_props(HGOriginalLoader *l) {
    unsigned count=0;for(unsigned i=0;i<4;i++)if(l->props[i])for(unsigned j=0;j<MAP_PROP_MAX;j++) {
        MapProp *p=&l->props[i]->mapProps[j];if(p->active){GF_ASSERT(p->render&&p->render->renderObj.model);count++;}
    }
    return count;
}

int hg_original_loader_tick(HGOriginalLoader *l){if(!l->initialized)return 0;uint32_t args[]={l->owner};return call(l,HG_LOADER_MapLoadManager_Tick,args,1);}
static void count_land_triangle(void *opaque,const DSSceneVertex v[3]){(void)v;(*(unsigned *)opaque)++;}
unsigned hg_original_loader_land_triangles(HGOriginalLoader *l) {
    unsigned count=0;
    for(unsigned i=0;i<NELEMS(l->land);i++)if(l->land[i].resource&&l->land[i].terrain) {
        GF_ASSERT(l->land[i].object&&word(l,l->land[i].object+4)==l->land[i].model);
        GF_ASSERT(ds_translate(&l->cpu.memory,l->land[i].resource,l->land[i].size));
        DSModelStats stats;GF_ASSERT(hg_prop_model_view_scene(l->land[i].native.model,count_land_triangle,&count,&stats));
    }
    return count;
}

int hg_original_loader_end(HGOriginalLoader *l) {
    if(!l->initialized||l->ending)return 0;uint32_t args[]={l->owner};
    if(!call(l,HG_LOADER_MapLoadManager_End,args,1))return 0;
    l->ending=1;return 1;
}
int hg_original_loader_has_ended(HGOriginalLoader *l,int *ended) {
    if(!l->initialized||!ended)return 0;uint32_t args[]={l->owner};
    if(!call(l,HG_LOADER_MapLoadManager_HasEnded,args,1))return 0;
    *ended=l->cpu.r[0]!=0;if(*ended)l->ended=1;return 1;
}

int hg_original_loader_render_probe(HGOriginalLoader *l,unsigned *triangles) {
    *triangles=0;if(!l->initialized||l->ending)return 0;
    for(unsigned i=0;i<l->texture_count;i++)if(l->textures[i].resident&&texture_resident(l,i)!=3){l->error="retired native texture allocation";return 0;}
    l->draw_triangles=0;
    uint32_t args[]={l->owner,0};int complete=call(l,HG_LOADER_MapLoadManager_RenderLoadedMaps,args,2);
    *triangles=l->draw_triangles;return complete;
}

/* Native slot retains a descriptor, never a guest executable pointer. Original
 * ARM installation selects the table; queries execute its original entries. */
struct NativeTerrainDispatch {uint32_t table;};
static const struct NativeTerrainDispatch terrain_tables[]={{HG_LOADER__020FC604},{HG_LOADER__020FC614}};
void hg_original_terrain_install(void **slot,BOOL simple) {
    GF_ASSERT(slot&&(simple==0||simple==1));u8 stack[64]={0};DSNativeRegion regions[]={{BASE,(u8 *)hg_loader_image,sizeof(hg_loader_image)},{0x03000400,stack,sizeof(stack)}};
    DSThumb c={0};c.memory.ram=(u8 *)hg_loader_image;c.memory.size=sizeof(hg_loader_image);c.memory.regions=regions;c.memory.region_count=2;c.code_start=BASE;c.code_size=HG_LOADER_CODE_SIZE;
    c.r[0]=0x03000400;c.r[1]=simple;c.r[13]=0x03000440;c.return_address=0x03000501;c.r[14]=c.return_address;c.r[15]=HG_LOADER_sub_0205489C;
    DSThumbResult status=ds_thumb_run(&c,100);if(status!=DS_THUMB_RETURN)fprintf(stderr,"terrain installer status %d PC %x fault %x steps %zu\n",status,c.r[15],c.fault_address,c.steps);GF_ASSERT(status==DS_THUMB_RETURN);
    uint32_t table=stack[0]|(u32)stack[1]<<8|(u32)stack[2]<<16|(u32)stack[3]<<24;
    GF_ASSERT(table==terrain_tables[simple].table);*slot=(void *)&terrain_tables[simple];
}
static void guest_put(HGOriginalLoader *l,uint32_t address,uint32_t value) {
    u8 *p=ds_translate(&l->cpu.memory,address,4);GF_ASSERT(p);for(unsigned i=0;i<4;i++)p[i]=value>>(8*i);
}
void *hg_original_loader_soundplate(HGOriginalLoader *l,int x,int z){
    if(!l||!l->initialized||l->ending||l->call_depth)return NULL;
    const u32 field=SCRATCH;
    guest_put(l,field+0x2c,l->owner);
    u32 address=0,args[]={field,(u32)x,(u32)z};
    if(!call_result(l,HG_LOADER_sub_02054874,args,3,&address))return NULL;
    const u8 *header=ds_translate(&l->cpu.memory,address,4);
    if(header){
        unsigned size=4+(unsigned)header[2]+((unsigned)header[3]<<8);
        void *view=ds_translate(&l->cpu.memory,address,size);
        if(view)return view;
    }
    l->error="soundplate backing unavailable";
    return NULL;
}
static uint32_t prepare_terrain_field(HGOriginalLoader *l,const void *dispatch) {
    if(!l->initialized||l->ending||!l->height_owner){l->error="terrain query requires live original loader and dynamic-height owner";return 0;}
    if(dispatch!=terrain_tables&&dispatch!=terrain_tables+1){l->error="unknown native original terrain descriptor";return 0;}
    const struct NativeTerrainDispatch *d=dispatch;
    if(d==terrain_tables+1){l->error="separate native terrain-attribute owner attachment required";return 0;}
    const uint32_t field=SCRATCH;
    memset(ds_translate(&l->cpu.memory,field,0x204),0,0x204);
    guest_put(l,field+0x2c,l->owner);guest_put(l,field+0x30,MATRIX);guest_put(l,field+0x98,l->height_owner);guest_put(l,field+0x60,d->table);
    return field;
}
int hg_original_loader_terrain(HGOriginalLoader *l,const void *dispatch,int height,int x,int z,int32_t reference,int32_t *value) {
    if(!value)return 0;
    uint32_t field=prepare_terrain_field(l,dispatch);if(!field)return 0;
    const struct NativeTerrainDispatch *d=dispatch;const uint32_t out=field+0x200;
    if(height){uint32_t args[]={field,(u32)reference,(u32)x,(u32)z,out};if(!call(l,word(l,d->table)&~1u,args,5))return 0;*value=(s32)l->cpu.r[0];return (int)(word(l,out)&255); }
    uint32_t args[]={field,(u32)x,(u32)z,out};if(!call(l,word(l,d->table+4)&~1u,args,4))return 0;
    *value=(s32)(word(l,out)&65535);return l->cpu.r[0];
}

int hg_original_loader_height(HGOriginalLoader *l,const void *dispatch,int x,int z,int32_t reference,int32_t *value,unsigned *kind) {
    if(!value||!kind)return 0;
    uint32_t field=prepare_terrain_field(l,dispatch);if(!field)return 0;
    uint32_t out=field+0x200,args[]={field,(u32)reference,(u32)x,(u32)z,out};
    if(!call(l,HG_LOADER_sub_02054940,args,5))return 0;
    *value=(s32)l->cpu.r[0];*kind=word(l,out)&255;return 1;
}
int hg_original_loader_behavior(HGOriginalLoader *l,const void *dispatch,int x,int z,unsigned *value) {
    if(!value)return 0;
    uint32_t field=prepare_terrain_field(l,dispatch);if(!field)return 0;
    uint32_t args[]={field,(u32)x,(u32)z};
    if(!call(l,HG_LOADER_GetMetatileBehavior,args,3))return 0;
    *value=l->cpu.r[0];return 1;
}

int hg_original_billboards_create(HGOriginalLoader *l,unsigned count,unsigned heap){
    uint32_t args[]={count,heap};if(!call(l,HG_LOADER_BillboardLists_Create,args,2))return 0;l->billboards_live=1;return 1;
}
int hg_original_billboards_delete(HGOriginalLoader *l){if(!call(l,HG_LOADER_BillboardLists_Delete,NULL,0))return 0;l->billboards_live=0;return 1;}
int hg_original_billboards_draw(HGOriginalLoader *l){return call(l,HG_LOADER_BillboardLists_Draw,NULL,0);}

void hg_original_oam_module_init(HGOriginalLoader *l){GF_ASSERT(!word(l,HG_LOADER__021D0EB0));GF_ASSERT(call(l,HG_LOADER_NNS_G2dInitOamManagerModule,NULL,0));}
int hg_original_oam_create(HGOriginalLoader *l,const uint32_t args[9]){return call(l,HG_LOADER_OamManager_Create,args,9);}
int hg_original_oam_apply(HGOriginalLoader *l){return call(l,HG_LOADER_OamManager_ApplyAndResetBuffers,NULL,0);}
int hg_original_oam_free(HGOriginalLoader *l){return call(l,HG_LOADER_OamManager_Free,NULL,0);}

void *hg_original_aux_new(HGOriginalLoader *l,unsigned heap){uint32_t args[]={heap};if(!call(l,HG_LOADER_ov02_0224F864,args,1))return NULL;return ds_translate(&l->cpu.memory,l->cpu.r[0],0x884);}
int hg_original_aux_free(HGOriginalLoader *l,void *owner){
    for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes==owner){uint32_t args[]={l->regions[i].address};return call(l,HG_LOADER_ov02_0224F8F4,args,1);}return 0;
}

void *hg_original_area_new(HGOriginalLoader *l,unsigned bank){
    if(l->area_owner)return NULL;uint32_t args[]={bank};if(!call(l,HG_LOADER_AreaDataManager_Alloc,args,1))return NULL;
    l->area_owner=l->cpu.r[0];return ds_translate(&l->cpu.memory,l->area_owner,0x8c4);
}
static int canonical_area(HGOriginalLoader *l,const void *area){return l->area_owner&&ds_translate(&l->cpu.memory,l->area_owner,0x8c4)==area;}
void *hg_original_area_headers(HGOriginalLoader *l,void *area){
    if(!canonical_area(l,area)||l->headers)return NULL;
    uint32_t args[]={l->area_owner};if(!call(l,HG_LOADER_AreaDataManager_GetMapPropModelFile,args,1))return NULL;
    GF_ASSERT(l->cpu.r[0]==l->area_owner+4);l->headers=hg_prop_resources_headers_new();return l->headers;
}
NARC *hg_original_area_animation_list(HGOriginalLoader *l,void *area){
    if(!canonical_area(l,area))return NULL;uint32_t args[]={l->area_owner};if(!call(l,HG_LOADER_ov01_021FB904,args,1))return NULL;
    unsigned n=(l->cpu.r[0]-NARC_HANDLE)/4;if(l->cpu.r[0]<NARC_HANDLE||n>=NELEMS(l->narcs))return NULL;return l->narcs[n];
}
void hg_original_bind_pools(HGOriginalLoader *l,void *pools){GF_ASSERT(pools&&!l->native_pools);l->native_pools=pools;}
int hg_original_area_load(HGOriginalLoader *l,void *area,void *render,void *manager,void *land,void *time){
    if(!canonical_area(l,area)||l->animations||!l->native_pools||!render||!manager||!land||!time)return 0;
    l->render=render;GF_ASSERT(l->render->resFileHeaders==l->headers);
    l->animations=hg_original_prop_animations_attach(l->native_pools,manager,land);
    uint32_t args[]={l->area_owner,0x03000080,0x03000200,0x03000204,0x03000208};
    l->area_loaded=call(l,HG_LOADER_AreaDataManager_Load,args,5);return l->area_loaded;
}
void hg_original_release_pools(HGOriginalLoader *l){GF_ASSERT(l->animations&&l->native_pools);hg_original_prop_animations_release(l->animations);l->native_pools=NULL;}
/* HG_ORIGINAL_BOTTOM_SCREEN_SELECTOR */
int hg_original_bottom_screen_select(
    HGOriginalLoader *l, void *field, unsigned type, unsigned *slot
){
    if(!l || !field || !slot || l->canonical_field!=field ||
       l->call_depth || type<1 || type>6)
        return 0;

    l->bottom_screen_guest_type=type;
    void *existing=ds_translate(&l->cpu.memory,0x03100018u,4);
    if(existing){
        if(existing!=&l->bottom_screen_guest_type)return 0;
    }else if(!borrow_native_at(
        l,0x03100018u,&l->bottom_screen_guest_type,4
    ))return 0;

    uint32_t args[]={0x03100000u}, result=UINT32_MAX;
    if(!call_result(l,HG_LOADER_ov01_021EAF00,args,1,&result) ||
       result!=type-1)
        return 0;

    *slot=result;
    return 1;
}

void *hg_original_map_owner(HGOriginalLoader *l){return l->owner?ds_translate(&l->cpu.memory,l->owner,0x110):NULL;}
void *hg_original_height_new(HGOriginalLoader *l,unsigned count,unsigned heap){
    if(l->height_owner)return NULL;uint32_t args[]={count,heap};if(!call(l,HG_LOADER_DynamicTerrainHeightManager_New,args,2))return NULL;
    l->height_owner=l->cpu.r[0];return ds_translate(&l->cpu.memory,l->height_owner,4);
}

void hg_original_loader_bind_save(HGOriginalLoader *l,void *save){GF_ASSERT(save&&!l->owner&&!l->canonical_save);l->canonical_save=save;}

void *hg_original_effects_new(HGOriginalLoader *l,void *field,unsigned count,unsigned heap){
    if(!field||l->effects||l->canonical_field)return NULL;
    HGHeapUsage usage;
    if(count>0x8000 || !hg_heap_usage(heap,&usage) ||
       0x24ull+8ull*count>usage.capacity-usage.allocated-usage.reserved ||
       l->cpu.memory.region_count+4>NELEMS(l->regions))return NULL;
    if(!observe_native(l,field,1))return NULL;
    if(!borrow_native_at(l,0x0310003c,&l->field_manager_identity,4))return NULL;
    if(!borrow_native_at(l,0x03100044,&l->field_effect_identity,4)) {
        retire_fixed(l,0x0310003c);return NULL;
    }
    l->canonical_field=field;l->field_manager_identity=0x03101000;
    unsigned checkpoint=l->cpu.memory.region_count,narc_mask=0;
    for(unsigned i=0;i<NELEMS(l->narcs);i++)if(l->narcs[i])narc_mask|=1u<<i;
    uint32_t args[]={0x03100000,count,heap};
    if(!call(l,HG_LOADER_FieldEffectManager_New,args,3)) {
        /* Source closure: two private blocks and one NARC, no callbacks or
         * publication. Roll back these owners, retaining numeric reservations. */
        for(unsigned i=0;i<NELEMS(l->narcs);i++)if(l->narcs[i]&&!(narc_mask&(1u<<i))) {
            NARC_Delete(l->narcs[i]);l->narcs[i]=NULL;
        }
        for(unsigned i=checkpoint;i<l->cpu.memory.region_count;i++)
            if(l->regions[i].bytes&&!l->borrowed_regions[i])Heap_Free(retire_region(l,i));
        retire_fixed(l,0x0310003c);retire_fixed(l,0x03100044);
        l->canonical_field=NULL;l->field_manager_identity=0;return NULL;
    }
    l->effects=l->cpu.r[0];l->field_effect_identity=l->effects;
    GF_ASSERT(word(l,l->effects+0x10)==0x03100000);
    return ds_translate(&l->cpu.memory,l->effects,0x24);
}
int hg_original_effects_anim_init(HGOriginalLoader *l,unsigned count){uint32_t args[]={l->effects,count};return l->effects&&call(l,HG_LOADER_FieldEffectManager_InitAnimManagerList,args,2);}
int hg_original_effects_resources_init(HGOriginalLoader *l,const uint32_t values[8]){uint32_t args[9]={l->effects};memcpy(args+1,values,8*sizeof(uint32_t));return l->effects&&call(l,HG_LOADER_ov01_021F1390,args,9);}
int hg_original_effects_register(HGOriginalLoader *l){uint32_t args[]={l->effects,HG_LOADER_defaultFieldEffectRenderers};return l->effects&&call(l,HG_LOADER_FieldEffectManager_InitRenderers,args,2);}
int hg_original_effects_free(HGOriginalLoader *l){
    if(!l->effects||l->object_graphics_live)return 0;
    uint32_t args[]={l->effects};if(!call(l,HG_LOADER_FieldEffectManager_Free,args,1))return 0;
    retire_fixed(l,0x0310003c);retire_fixed(l,0x03100044);
    retire_fixed(l,0x031000f9);
    for(unsigned i=0;i<64;i++) {
        retire_fixed(l,0x03210000u+i*0x100u);
        retire_fixed(l,0x03220000u+i*0x100u);
    }
    l->effects=0;l->field_effect_identity=0;l->canonical_field=NULL;return 1;
}

static void rollback_graphics(HGOriginalLoader *l,HGGraphicsConstruction *tx,void *storage) {
    /* Quiesce only this attempt's tasks before releasing anything they retain. */
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(tx->tasks[i]) {
        if(l->tasks[i].native)SysTask_Destroy(l->tasks[i].native);
        memset(l->tasks+i,0,sizeof(l->tasks[i]));
        l->tasks[i].queue=tx->old_task_queue[i];l->tasks[i].invoked=tx->old_task_invoked[i];
    }
    u8 *owned=tx->owned_regions;unsigned textures[128]={0},views[128]={0};
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)views[i]=hg_heap_captured(l->raw_views[i].owner,tx);
    for(unsigned i=0;i<l->texture_count;i++)textures[i]=owned[l->textures[i].region];
    raw_context=l;raw_current=NULL;
    /* Original native destructors understand complete raw objects, including
     * transferred full headers/prefixes and keys. No complete ARM destructor
     * may run against the interrupted aggregate. */
    if(setjmp(raw_rejected))GF_ASSERT(!"graphics rollback raw ownership invariant");
    for(unsigned i=0;i<NELEMS(l->raw_managers);i++)
        if((tx->raw_managers&(1u<<i))&&l->raw_managers[i]) {
            GF3dGfxRawResMan_Destroy(l->raw_managers[i]);l->raw_managers[i]=NULL;
            if(!i)l->effect_raw=NULL;
        }
    for(unsigned i=0;i<l->texture_count;i++)if(textures[i]) {
        texture_release(l,i);memset(l->textures+i,0,sizeof(l->textures[i]));
    }
    for(unsigned i=0;i<NELEMS(l->sdk_keys);i++)
        if(l->sdk_keys[i].token&&textures[l->sdk_keys[i].texture])memset(l->sdk_keys+i,0,sizeof(l->sdk_keys[i]));
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(views[i])memset(l->raw_views+i,0,sizeof(l->raw_views[i]));
    while(l->texture_count&&!l->textures[l->texture_count-1].guest)l->texture_count--;
    for(unsigned i=0;i<l->allocator_count;)
        if(hg_heap_captured(ds_translate(&l->cpu.memory,l->allocators[i].identity,1),tx))
            l->allocators[i]=l->allocators[--l->allocator_count];else i++;
    for(unsigned i=tx->regions;i<l->cpu.memory.region_count;i++)if(owned[i]) {
        retire_land(l,l->regions[i].address,l->regions[i].size);
        if(l->regions[i].bytes)retire_region(l,i);
    }
    for(unsigned i=0;i<NELEMS(l->narcs);i++)if(l->narcs[i]&&hg_heap_captured(l->narcs[i],tx)) {
        NARC_Delete(l->narcs[i]);l->narcs[i]=NULL;
    }
    hg_heap_finish_capture(tx,FALSE);
    while(tx->heap_count) {
        unsigned heap=tx->heaps[--tx->heap_count],i=0;
        while(i<l->child_count&&l->child_heaps[i]!=heap)i++;
        GF_ASSERT(i<l->child_count);Heap_Destroy(heap);
        l->child_heaps[i]=l->child_heaps[--l->child_count];
    }
    /* The only borrowed-byte mutations in this constructor's source closure:
     * manager opaque tail and one initially inactive original model-pool slot. */
    memcpy(storage,tx->storage,sizeof(tx->storage));
    memcpy(ds_translate(&l->cpu.memory,tx->pool,sizeof(tx->pool_bytes)),tx->pool_bytes,sizeof(tx->pool_bytes));
    canonical_manager_operation(l->canonical_field,3,1);
    canonical_manager_narc(l->canonical_field,NULL);l->object_model_narc=0;
    retire_fixed(l,0x03102000);retire_fixed(l,0x03103000);
    /* These mappings never escaped: original callback queues are cancelled,
     * published parent bytes restored, and no native binding refs were issued.
     * Reclaim only our trailing unpublished entries; preserve interleaved IRQ
     * mappings/reservations. Fixed-view generations retain their usual policy. */
    unsigned external=0;
    for(unsigned i=tx->regions;i<l->cpu.memory.region_count;i++)if(!owned[i])external=1;
    while(l->cpu.memory.region_count>tx->regions&&owned[l->cpu.memory.region_count-1]) {
        unsigned i=--l->cpu.memory.region_count;
        memset(l->regions+i,0,sizeof(l->regions[i]));
        l->borrowed_regions[i]=l->region_heaps[i]=0;
        l->archive_bytes[i]=NULL;l->archive_sizes[i]=0;
        l->archive_ids[i]=l->archive_members[i]=0;
    }
    if(!external)l->next=tx->next;
}
void *hg_original_objects_pool(HGOriginalLoader *l,const void *storage){
    if(!l||!l->object_graphics_live||storage!=ds_translate(&l->cpu.memory,0x03102000,0x10c))return NULL;
    uint32_t args[]={0x03102000},pool;
    if(!call_result(l,HG_LOADER_ov01_021FA1D0,args,1,&pool))return NULL;
    return ds_translate(&l->cpu.memory,pool,0xe0);
}
int hg_original_objects_pool_publish(HGOriginalLoader *l,void *pool){
    if(!l||!l->object_graphics_live||!pool)return 0;
    uint32_t guest=word(l,0x031020e0);
    if(!guest||pool!=ds_translate(&l->cpu.memory,guest,0xe0))return 0;
    return call(l,HG_LOADER_sub_02023910,&guest,1);
}
int hg_original_objects_graphics_new(HGOriginalLoader *l,unsigned slots,unsigned count,const int *ids,unsigned mode){
    if(!l->canonical_field||l->object_graphics_live||l->construction||count>32||(!ids&&count)||
       l->object_model_narc||canonical_manager_operation(l->canonical_field,0,1)||
       canonical_manager_operation(l->canonical_field,5,0))return 0;
    size_t size;void *storage=canonical_manager_storage(l->canonical_field,&size);
    if(!storage||size!=0x10c||l->cpu.memory.region_count+2>NELEMS(l->regions))return 0;
    if(!borrow_native_at(l,0x03102000,storage,size))return 0;
    if(count&&!borrow_native_at(l,0x03103000,(void *)ids,count*sizeof(int))) {
        retire_fixed(l,0x03102000);return 0;
    }
    HGGraphicsConstruction tx={.depth=l->call_depth+1,.regions=l->cpu.memory.region_count,
        .next=l->next};
    /* Ask the unchanged original selector, rather than clone its pool search. */
    if(!call_result(l,HG_LOADER_sub_0202391C,NULL,0,&tx.pool)||!tx.pool) {
        retire_fixed(l,0x03102000);retire_fixed(l,0x03103000);return 0;
    }
    void *pool=ds_translate(&l->cpu.memory,tx.pool,sizeof(tx.pool_bytes));
    if(!pool){retire_fixed(l,0x03102000);retire_fixed(l,0x03103000);return 0;}
    memcpy(tx.storage,storage,sizeof(tx.storage));memcpy(tx.pool_bytes,pool,sizeof(tx.pool_bytes));
    l->construction=&tx;
#ifdef HG_LOADER_TEST_HOOKS
    l->graphics_imports=0;
#endif
    uint32_t args[]={0x03101000,slots,count,0x03103000,mode};
    int ok=call(l,HG_LOADER_ov01_021F91F8,args,5);
#ifdef HG_LOADER_TEST_HOOKS
    if(ok&&++l->graphics_imports==l->graphics_fail_import) {
        l->error="injected graphics construction failure";ok=0;
    }
#endif
    l->construction=NULL;
    if(!ok) {const char *error=l->error;rollback_graphics(l,&tx,storage);l->error=error;return 0;}
    hg_heap_finish_capture(&tx,TRUE);l->object_graphics_live=1;return 1;
}
#ifdef HG_LOADER_TEST_HOOKS
void hg_original_graphics_fail_import(HGOriginalLoader *l,unsigned index){l->graphics_fail_import=index;}
unsigned hg_original_graphics_imports(HGOriginalLoader *l){return l->graphics_imports;}
unsigned hg_original_test_graphics_raw_objects(HGOriginalLoader *l){
    unsigned count=0;for(unsigned i=1;i<NELEMS(l->raw_managers);i++)if(l->raw_managers[i])
        for(int j=0;j<l->raw_managers[i]->man->max;j++)count+=l->raw_managers[i]->objs[j].obj!=NULL;
    return count;
}
size_t hg_original_test_region_count(HGOriginalLoader *l){return l->cpu.memory.region_count;}
uint32_t hg_original_test_next(HGOriginalLoader *l){return l->next;}
int hg_original_test_mapped(HGOriginalLoader *l,uint32_t address){return ds_translate(&l->cpu.memory,address,1)!=NULL;}
#endif

uint32_t hg_original_map_native(
    HGOriginalLoader *l, const void *bytes, size_t size
) {
    if (!l) return 0;
    DSMapping m = original_mapping(l);
    uint32_t existing=ds_mapping_address_of(&m,bytes,size);
    if(existing)return existing;
    if(!observe_native(l,bytes,size))return 0;
    return ds_mapping_borrow(&m, (void *)bytes, size);
}

uint32_t hg_original_object_work(
    HGOriginalLoader *l, unsigned index, void *bytes, size_t size
) {
    if (!l || !l->canonical_field || index >= 64 || size != 80)
        return 0;
    return borrow_native_at(
        l, 0x03210000u + index * 0x100u, bytes, size
    );
}

uint32_t hg_original_object_facing_vector(
    HGOriginalLoader *l, unsigned index, void *bytes, size_t size
) {
    if (!l || !l->canonical_field || index >= 64 || size != 12)
        return 0;
    return borrow_native_at(
        l, 0x03220000u + index * 0x100u, bytes, size
    );
}

int hg_original_map_track_target(
    HGOriginalLoader *l, void *target, size_t size
) {
    if (!l || !l->canonical_field || !l->initialized ||
        !l->owner || size != 12)
        return 0;

    uint32_t ptr = hg_original_map_native(l, target, size);
    if (!ptr) {
        l->error = "MapLoadManager_TrackTarget: native mapping failed";
        return 0;
    }

    uint32_t args[] = {ptr, l->owner};
    if (!call(l, HG_LOADER_MapLoadManager_TrackTarget, args, 2))
        return 0;

    void *copy = ds_translate(&l->cpu.memory, l->owner + 0xd0, size);

    if (!copy || memcmp(copy, target, size) ||
        word(l, l->owner + 0xdc) != ptr) {
        l->error = "MapLoadManager_TrackTarget: ARM result mismatch";
        return 0;
    }

    return 1;
}

int hg_original_followmon_pending(
    HGOriginalLoader *l, void *field, uint8_t *pending
) {
    if (!l || l->canonical_field != field || !pending)
        return 0;

    if (*pending) {
        l->error = "ov01_022059AC: nonzero pending flag needs follower ABI";
        return 0;
    }

    if (!borrow_native_at(l, 0x031000f9u, pending, 1)) return 0;

    const uint32_t args[] = {0x03100000};

    if (!call(l, HG_LOADER_ov01_022059AC, args, 1))
        return 0;

    return *pending == 0;
}

int hg_original_comm_force_position(HGOriginalLoader *l) {
    if (!l || !l->canonical_field)
        return 0;

    // Refuse active sessions until their native/guest ABI is implemented.
    if (word(l, HG_LOADER__021D41C4) != 0) {
        l->error = "CommPlayerManager: active communication ABI pending";
        return 0;
    }

    // The original Thumb function checks the global and returns.
    return call(l, HG_LOADER_CommPlayerManager_ForcePosition, NULL, 0);
}

void *hg_original_object_graphics_info(HGOriginalLoader *l,uint32_t sprite){
    if(!l->canonical_field||!call(l,HG_LOADER_ObjectEvent_GetGraphicsInfo,&sprite,1))return NULL;
    return ds_translate(&l->cpu.memory,l->cpu.r[0],6);
}
int hg_original_object_callback(HGOriginalLoader *l,unsigned index,uint32_t callback){
    if(!l->canonical_field||!l->object_graphics_live||index>=64)return 0;
    uint32_t object=0x03110000+4*index;return call(l,callback&~1u,&object,1);
}
int hg_original_object_graphics_mode(HGOriginalLoader *l,unsigned index,unsigned mode){
    if(!l->object_graphics_live||index>=64||mode>1)return 0;
    uint32_t args[]={0x03110000+4*index,mode};return call(l,HG_LOADER_ov01_021FA2B8,args,2);
}
int hg_original_avatar_features_start(HGOriginalLoader *l){
    if(!l->canonical_field||!l->object_graphics_live)return 0;
    uint32_t arg=0x03104000;return call(l,HG_LOADER_ov01_022008B4,&arg,1);
}
int hg_original_objects_graphics_prepare(HGOriginalLoader *l,unsigned operation){
    if(!l->object_graphics_live||operation>1)return 0;
    uint32_t arg=operation?l->effects:0x03101000;
    return call(l,operation?HG_LOADER_FieldEffect_InitRenderObject:HG_LOADER_ov01_022057DC,&arg,1);
}
int hg_original_objects_graphics_free(HGOriginalLoader *l){
    if(!l->object_graphics_live)return 0;uint32_t args[]={0x03101000};
    if(!call(l,HG_LOADER_ov01_021F9250,args,1))return 0;
    canonical_manager_narc(l->canonical_field,NULL);l->object_model_narc=0;l->object_graphics_live=0;
    retire_fixed(l,0x03102000);retire_fixed(l,0x03103000);return 1;
}
unsigned hg_original_effect_member_size(HGOriginalLoader *l,unsigned member){uint32_t args[]={l->effects,member};if(!call(l,HG_LOADER_ov01_021F149C,args,2))return 0;return l->cpu.r[0];}
int hg_original_effect_member_read(HGOriginalLoader *l,unsigned member,void *bytes){
    size_t size;if(!hg_heap_allocation_size(bytes,&size)||size!=hg_original_effect_member_size(l,member)||l->cpu.memory.region_count==NELEMS(l->regions))return 0;
    uint32_t guest=hg_original_map_native(l,bytes,size);if(!guest)return 0;
    uint32_t args[]={l->effects,member,guest};return call(l,HG_LOADER_ov01_021F14A8,args,3);
}
int hg_original_effect_native_model(HGOriginalLoader *l,void *bytes,void *object,void **model){
    for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes==bytes&&l->borrowed_regions[i]){
        int n=setup_model_resource(l,l->regions[i].address);if(n<0)return 0;
        l->land[n].native_object=object;*(NNSG3dRenderObj *)object=l->land[n].native;*model=l->land[n].native.model;return 1;
    }return 0;
}
int hg_original_effect_native_free(HGOriginalLoader *l,void *bytes){
    for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes==bytes&&l->borrowed_regions[i]){
        retire_land(l,l->regions[i].address,l->regions[i].size);
        for(unsigned j=0;j<l->texture_count;j++)if(l->textures[j].region==i)texture_release(l,j);
        retire_region(l,i);Heap_Free(bytes);return 1;
    }return 0;
}

void hg_original_effects_cancel_pending(HGOriginalLoader *l){
    /* Aborted-session cleanup only: original model construction can enqueue
     * VWait work before returning. A rejected constructor cannot own Exit. */
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].native&&l->tasks[i].queue!=0){SysTask_Destroy(l->tasks[i].native);l->tasks[i].native=NULL;}
}

unsigned hg_original_effects_registered(HGOriginalLoader *l){
    GF_ASSERT(l->effects&&word(l,l->effects+4)==23);unsigned mask=0;
    uint32_t entries=word(l,l->effects+0x14);
    for(unsigned i=0;i<23;i++){unsigned id=word(l,entries+8*i);if(id==23){GF_ASSERT(!word(l,entries+8*i+4));continue;}GF_ASSERT(id<23&&word(l,entries+8*i+4)&&!(mask&(1u<<id)));mask|=1u<<id;}
    return mask;
}
int hg_original_effects_animation_probe(HGOriginalLoader *l){
    /* Renderer 22 initializes two categories before any draw/evaluation call. */
    uint32_t entries=word(l,l->effects+0x14),owner14=0;
    for(unsigned i=0;i<23;i++)if(word(l,entries+8*i)==14)owner14=word(l,entries+8*i+4);
    if(!owner14||word(l,owner14)!=l->effects||!ds_translate(&l->cpu.memory,owner14,0x1a4))return 0;
    for(unsigned i=0;i<4;i++){
        uint32_t holder=owner14+4+20*i,model=word(l,holder+12),object=owner14+0x54+84*i;
        if(!model||word(l,object+4)!=model)return 0;
        unsigned j=0;while(j<NELEMS(l->arm_objects)&&l->arm_objects[j].identity!=object)j++;
        if(j==NELEMS(l->arm_objects)||l->land[l->arm_objects[j].resource].model!=model)return 0;
        uint32_t resource=word(l,holder+4);unsigned region=0;
        while(region<l->cpu.memory.region_count&&l->regions[region].address!=resource)region++;
        if(region==l->cpu.memory.region_count||!l->regions[region].bytes||l->archive_members[region]!=(int)(100+i))return 0;
    }
    for(unsigned i=0;i<23;i++)if(word(l,entries+8*i)==22)fprintf(stderr,"Effect 22: manager=%08x entry=%08x value=%08x\n",l->effects,entries+8*i,word(l,entries+8*i+4));
    uint32_t expected22=0,outer_r0=l->cpu.r[0];
    int nested22=l->call_depth!=0;
    for(unsigned i=0;i<23;i++)
        if(word(l,entries+8*i)==22)
            expected22=word(l,entries+8*i+4);
    GF_ASSERT(expected22);
    uint32_t renderer=0,color_args[]={l->effects,22};
    if(!call_result(l,HG_LOADER_ov01_021F1450,color_args,2,&renderer))return 0;
    GF_ASSERT(renderer==expected22);
    GF_ASSERT(l->cpu.r[0]==(nested22?outer_r0:renderer));
    uint32_t ids[]={word(l,renderer+0x24+8),word(l,renderer+0x38+8)};
    if(word(l,renderer+0x4c+0x6c))return 0; /* Original inactive flag. */
    unsigned categories=0;NNSG3dRenderObj *bound=NULL;
    for(unsigned k=0;k<2;k++){
        for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].identity==ids[k]){
            unsigned region=0;while(region<l->cpu.memory.region_count&&l->regions[region].address!=l->sdk_animations[i].resource)region++;
            if(region==l->cpu.memory.region_count)return 0;
            DSAnimationResource a;if(!ds_animation_resource_open(l->regions[region].bytes,l->regions[region].size,0,&a))return 0;
            if(a.kind!=(k?DS_ANIMATION_JOINT:DS_ANIMATION_COLOR)||word(l,ids[k])||!hg_native_field_animation_frame(l->sdk_animations[i].native,0))return 0;
            for(unsigned j=0;j<NELEMS(l->arm_objects);j++)if(l->arm_objects[j].identity&&hg_native_field_animation_is_bound(l->sdk_animations[i].native,&l->arm_objects[j].native)){
                if(bound&&bound!=&l->arm_objects[j].native)return 0;bound=&l->arm_objects[j].native;categories|=1u<<k;
            }
        }
    }
    if(categories!=3||!bound||!bound->anmJnt||!bound->anmMat)return 0;
    uint32_t expected17=0;
    for(unsigned i=0;i<23;i++)
        if(word(l,entries+8*i)==17)
            expected17=word(l,entries+8*i+4);
    GF_ASSERT(expected17);
    int nested17=l->call_depth!=0;
    outer_r0=l->cpu.r[0];
    uint32_t renderer17=0,args[]={l->effects,17};
    if(!call_result(l,HG_LOADER_ov01_021F1450,args,2,&renderer17))return 0;
    GF_ASSERT(renderer17==expected17);
    GF_ASSERT(l->cpu.r[0]==(nested17?outer_r0:renderer17));
    uint32_t holder=renderer17+0x12c,identity=word(l,holder+8),resource=word(l,identity+8);
    unsigned frames=word(l,resource+4)&65535;GF_ASSERT(frames>1);
    GF_ASSERT(!ds_translate(&l->cpu.memory,identity+4,4)&&!ds_translate(&l->cpu.memory,identity,12));
    uint32_t before=word(l,holder+0xc),step[]={holder,4096};
    if(!call(l,HG_LOADER_Field3dModelAnimation_FrameAdvanceAndLoop,step,2))return 0;
    uint32_t after=word(l,holder+0xc);GF_ASSERT(after==(before+4096)%(frames*4096)&&word(l,identity)==after);
    for(unsigned i=0;i<NELEMS(l->sdk_animations);i++)if(l->sdk_animations[i].identity==identity)return hg_native_field_animation_frame(l->sdk_animations[i].native,after);
    return 0;
}
unsigned hg_original_effects_deferred(HGOriginalLoader *l){
    unsigned count=0;
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].native){
        GF_ASSERT((l->tasks[i].callback&1)&&l->tasks[i].callback>=BASE&&l->tasks[i].callback-BASE<HG_LOADER_CODE_SIZE);
        if(l->tasks[i].queue)count++;

    }return count;
}

void *hg_original_effects_owner(HGOriginalLoader *l){return l->effects?ds_translate(&l->cpu.memory,l->effects,0x24):NULL;}
unsigned hg_original_effects_uploads(HGOriginalLoader *l){unsigned count=0;for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].queue==2)count+=l->tasks[i].invoked;return count;}

int hg_original_effects_reject_alias_free(HGOriginalLoader *l){uint32_t args[]={0x0310003c};return !call(l,HG_LOADER_ov01_021F1448,args,1)&&l->error&&!strcmp(l->error,"Heap_Free");}

unsigned hg_original_effects_pending_vwait(HGOriginalLoader *l){unsigned count=0;for(unsigned i=0;i<NELEMS(l->tasks);i++)count+=l->tasks[i].native&&l->tasks[i].queue==2;return count;}

/* Exact live allocation bases only: native callers receive shared bytes, never
 * an ARM32 struct containing widened native pointers. */
static uint32_t environment_address(HGOriginalLoader *l,const void *owner,unsigned kind){
    if(!l||!owner||kind>2)return 0;DSMapping m=original_mapping(l);
    DSMappingRef ref=l->environment[kind];size_t size=(size_t[]){0x48,0x38,0x14}[kind];
    if(ds_mapping_resolve(&m,ref,0,size)!=owner)return 0;
    DSNativeRegion *r=l->regions+ref.slot;
    return !l->borrowed_regions[ref.slot]&&l->region_heaps[ref.slot]==HEAP_ID_FIELD1&&r->size==size?r->address:0;
}
static void *environment_new(HGOriginalLoader *l,unsigned kind,uint32_t entry){
    if(!l)return NULL;DSMapping m=original_mapping(l);size_t size=(size_t[]){0x48,0x38,0x14}[kind];
    if(ds_mapping_resolve(&m,l->environment[kind],0,0)||!call(l,entry,NULL,0))return NULL;
    l->environment[kind]=ds_mapping_ref(&m,l->cpu.r[0]);
    return ds_mapping_resolve(&m,l->environment[kind],0,size);
}
void *hg_original_model_attributes_new(HGOriginalLoader *l){return environment_new(l,0,HG_LOADER_ModelAttributes_Init);}
int hg_original_model_attributes_configure(HGOriginalLoader *l,void *owner){
    uint32_t address=environment_address(l,owner,0);
    return address&&call(l,HG_LOADER_FieldMap_InitModelAttributes,&address,1);
}
void *hg_original_fog_new(HGOriginalLoader *l){return environment_new(l,1,HG_LOADER_Fog_New);}
static int environment_free(HGOriginalLoader *l,void **owner,unsigned kind,uint32_t entry){
    if(!owner)return 0;uint32_t address=environment_address(l,*owner,kind);
    if(!address||l->call_depth)return 0;
    /* Scoped guest pointer cell above the interpreter stack, never a native
     * pointer-to-pointer ABI cast. No callback in these original frees. */
    uint32_t slot=STACK+0x800;u8 *bytes=ds_translate(&l->cpu.memory,slot,4);
    if(!bytes)return 0;for(unsigned i=0;i<4;i++)bytes[i]=address>>(8*i);
    if(!call(l,entry,&slot,1))return 0;
    if(word(l,slot))return 0;*owner=NULL;l->environment[kind]=(DSMappingRef){0};return 1;
}
int hg_original_model_attributes_free(HGOriginalLoader *l,void **owner){return environment_free(l,owner,0,HG_LOADER_ModelAttributes_Free);}
int hg_original_fog_free(HGOriginalLoader *l,void **owner){return environment_free(l,owner,1,HG_LOADER_Fog_Free);}

static int camera_target_valid(HGOriginalLoader *l){DSMapping m=original_mapping(l);return ds_mapping_resolve(&m,l->camera_target,l->camera_target_offset,12)!=NULL;}
static int camera_set_target_ref(HGOriginalLoader *l,uint32_t g){
    DSMapping m=original_mapping(l);
    for(unsigned i=0;i<l->cpu.memory.region_count;i++){
        DSNativeRegion *r=l->regions+i;if(r->bytes&&g>=r->address&&g-r->address<=r->size&&12<=r->size-(g-r->address)){
            l->camera_target=ds_mapping_ref(&m,r->address);l->camera_target_offset=g-r->address;return 1;
        }
    }return 0;
}
void *hg_original_camera_owner(HGOriginalLoader *l){if(!l)return NULL;DSMapping m=original_mapping(l);return ds_mapping_resolve(&m,l->camera_owner,0,0x68);}
void *hg_original_camera_transition(HGOriginalLoader *l){return l?ds_translate(&l->cpu.memory,l->camera_fields[1],0x34):NULL;}
int hg_original_camera_new(HGOriginalLoader *l,void *target,unsigned type,unsigned history){
    if(!l||!l->canonical_field||type>=17||history>1||l->call_depth||hg_original_camera_owner(l))return 0;
    uint32_t guest=hg_original_map_native(l,target,12);if(!guest||!camera_set_target_ref(l,guest))return 0;
    void *fields=ds_translate(&l->cpu.memory,0x03100024,8);
    if(fields?fields!=l->camera_fields:!borrow_native_at(l,0x03100024,l->camera_fields,8))return 0;
    uint32_t fields_before[2];memcpy(fields_before,l->camera_fields,sizeof(fields_before));
    DSCamera old=l->graphics_camera;unsigned old_view=l->camera_view_valid;
    u8 depth_before[4];memcpy(depth_before,ds_translate(&l->cpu.memory,HG_LOADER_gG3dDepthBufferingMode,4),4);
    unsigned scope=0;l->owner_capture=&scope;l->owner_capture_depth=l->call_depth+1;
    uint32_t args[]={guest,0x03100000,type,history};
    int ok=call(l,HG_LOADER_FieldCamera_Create,args,4);l->owner_capture=NULL;
    if(!ok||!camera_target_valid(l)){
        const char *error=l->error;call(l,HG_LOADER_Camera_UnsetStaticPtr,NULL,0);
        for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes&&hg_heap_captured(l->regions[i].bytes,&scope))retire_region(l,i);
        hg_heap_finish_capture(&scope,FALSE);memcpy(l->camera_fields,fields_before,sizeof(fields_before));
        memcpy(ds_translate(&l->cpu.memory,HG_LOADER_gG3dDepthBufferingMode,4),depth_before,4);l->camera_target=(DSMappingRef){0};
        l->graphics_camera=old;l->camera_view_valid=old_view;l->error=error?error:"retired camera target";return 0;
    }
    hg_heap_finish_capture(&scope,TRUE);DSMapping m=original_mapping(l);l->camera_owner=ds_mapping_ref(&m,l->camera_fields[0]);return hg_original_camera_owner(l)!=NULL;
}
int hg_original_camera_delete(HGOriginalLoader *l){
    if(!hg_original_camera_owner(l)||l->call_depth)return 0;
    uint32_t field=0x03100000;if(!call(l,HG_LOADER_FieldCamera_Delete,&field,1))return 0;
    l->camera_owner=(DSMappingRef){0};l->camera_target=(DSMappingRef){0};return 1;
}
int hg_original_camera_update(HGOriginalLoader *l){
    if(!hg_original_camera_owner(l)||!camera_target_valid(l))return 0;
    return call(l,HG_LOADER_ov01_021EAD8C,l->camera_fields+1,1);
}
int hg_original_camera_push(HGOriginalLoader *l){
    if(!hg_original_camera_owner(l)||!camera_target_valid(l)){if(l)l->error="retired camera/target";return 0;}
    return call(l,HG_LOADER_Camera_PushLookAtToNNSGlb,NULL,0);
}
int hg_original_camera_retarget(HGOriginalLoader *l,void *target){
    if(!hg_original_camera_owner(l))return 0;uint32_t guest=hg_original_map_native(l,target,12);if(!guest)return 0;
    uint32_t args[]={guest,l->camera_fields[0]};if(!call(l,HG_LOADER_Camera_SetFixedTarget,args,2))return 0;return camera_set_target_ref(l,guest);
}
const DSCamera *hg_original_camera_render_state(HGOriginalLoader *l){return l&&l->camera_view_valid&&l->graphics_camera.projection_kind?&l->graphics_camera:NULL;}
unsigned hg_original_camera_depth(HGOriginalLoader *l){return word(l,HG_LOADER_gG3dDepthBufferingMode);}

#ifdef HG_LOADER_TEST_HOOKS
void *hg_original_test_bytes(HGOriginalLoader *l,uint32_t g,size_t size){return ds_translate(&l->cpu.memory,g,size);}
int hg_original_test_camera_angles(HGOriginalLoader *l,int32_t distance,const uint16_t angle[4],unsigned perspective){
    if(!hg_original_camera_owner(l)||l->call_depth)return 0;
    uint32_t scratch=STACK+0x810;u8 *bytes=ds_translate(&l->cpu.memory,scratch,8);
    for(unsigned i=0;i<4;i++){bytes[2*i]=angle[i];bytes[2*i+1]=angle[i]>>8;}
    uint32_t dist[]={(u32)distance,l->camera_fields[0]},args[]={scratch,l->camera_fields[0]},proj[]={perspective,l->camera_fields[0]};
    return call(l,HG_LOADER_Camera_SetDistance,dist,2)&&call(l,HG_LOADER_Camera_SetAnglePos,args,2)&&call(l,HG_LOADER_Camera_ApplyPerspectiveType,proj,2);
}
#endif

unsigned hg_original_area_light_archive(HGOriginalLoader *l,void *owner){
    if(!canonical_area(l,owner))return UINT32_MAX;
    if(!call(l,HG_LOADER_AreaDataManager_GetAreaLightArchiveID,&l->area_owner,1))return UINT32_MAX;return l->cpu.r[0];
}
void *hg_original_light_new(HGOriginalLoader *l,void *attributes,unsigned archive){
    if(!l||l->call_depth||archive>=5)return NULL;DSMapping m=original_mapping(l);
    uint32_t attr=environment_address(l,attributes,0);if(!attr||ds_mapping_resolve(&m,l->environment[2],0,0))return NULL;
    DSMappingRef attributes_ref=l->environment[0];
    u8 before[0x48];memcpy(before,attributes,sizeof(before));
    s16 vectors[12];u16 colors[4];u32 material[2][3];memcpy(vectors,l->light_vectors,sizeof(vectors));memcpy(colors,l->light_colors,sizeof(colors));memcpy(material,l->light_material,sizeof(material));
    unsigned scope=0;l->owner_capture=&scope;l->owner_capture_depth=l->call_depth+1;
    uint32_t args[]={attr,archive};int ok=call(l,HG_LOADER_AreaLightManager_New,args,2);uint32_t owner=l->cpu.r[0];l->owner_capture=NULL;
    int input_live=ds_mapping_resolve(&m,attributes_ref,0,0x48)==attributes;
    if(!ok||!input_live){
        for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes&&hg_heap_captured(l->regions[i].bytes,&scope))retire_region(l,i);
        hg_heap_finish_capture(&scope,FALSE);if(input_live)memcpy(attributes,before,sizeof(before));memcpy(l->light_vectors,vectors,sizeof(vectors));memcpy(l->light_colors,colors,sizeof(colors));memcpy(l->light_material,material,sizeof(material));
        finish_files(l,&scope,FALSE);if(!input_live)l->error="retired area-light attributes";return NULL;
    }
    finish_files(l,&scope,TRUE);hg_heap_finish_capture(&scope,TRUE);l->environment[2]=ds_mapping_ref(&m,owner);l->light_attributes=attributes_ref;return ds_mapping_resolve(&m,l->environment[2],0,0x14);
}
int hg_original_light_free(HGOriginalLoader *l,void **owner){if(!environment_free(l,owner,2,HG_LOADER_AreaLightManager_Free))return 0;l->light_attributes=(DSMappingRef){0};return 1;}
int hg_original_light_update(HGOriginalLoader *l,void *owner){
    uint32_t g=environment_address(l,owner,2);if(!g)return 0;DSMapping m=original_mapping(l);
    if(!ds_mapping_resolve(&m,l->light_attributes,0,0x48)){l->error="retired area-light attributes";return 0;}
    return call(l,HG_LOADER_AreaLightManager_UpdateActiveTemplate,&g,1);
}

#ifdef HG_LOADER_TEST_HOOKS
const void *hg_original_test_camera_template(HGOriginalLoader *l,unsigned type){return type<17?ds_translate(&l->cpu.memory,HG_LOADER_ov01_02206478+36*type,36):NULL;}
#endif

/* The existing capture tag is also attached to non-heap publications. IRQ
 * tasks run outside this synchronous tag. Quiesce owned tasks before backing. */
static void finish_owner_capture(HGOriginalLoader *l,const void *scope,int commit){
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].capture==scope){
        if(!commit&&l->tasks[i].native){SysTask_Destroy(l->tasks[i].native);l->tasks[i].native=NULL;}
        l->tasks[i].capture=NULL;
    }
    for(unsigned i=0;i<NELEMS(l->narcs);i++)if(l->narc_capture[i]==scope){
        if(!commit&&l->narcs[i]){NARC_Delete(l->narcs[i]);l->narcs[i]=NULL;}
        l->narc_capture[i]=NULL;
    }
    if(!commit){
        for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes&&hg_heap_captured(l->regions[i].bytes,scope))retire_region(l,i);
    }
    finish_files(l,scope,commit);hg_heap_finish_capture(scope,commit);
}
static void *construct_field_owner(HGOriginalLoader *l,uint32_t entry,const u32 *args,unsigned count,unsigned size){
    if(!l||l->call_depth||l->owner_capture)return NULL;
    unsigned scope=0;
    l->owner_capture=&scope;l->owner_capture_depth=1;
    u32 address=0;
    int ok=call_result(l,entry,args,count,&address);
    l->owner_capture=NULL;
    finish_owner_capture(l,&scope,ok);
    return ok?ds_translate(&l->cpu.memory,address,size):NULL;
}
void *hg_original_hblank_new(HGOriginalLoader *l,unsigned heap){
    DSMapping m=original_mapping(l);
    if(ds_mapping_resolve(&m,l->hblank_owner,0,28))return NULL;
    void *owner=construct_field_owner(l,HG_LOADER_HBlankSystem_New,&heap,1,28);
    if(owner)l->hblank_owner=ds_mapping_ref(&m,hg_original_map_native(l,owner,28));
    return owner;
}
static int hblank_call(HGOriginalLoader *l,void *owner,uint32_t entry){
    DSMapping m=original_mapping(l);
    if(!owner||ds_mapping_resolve(&m,l->hblank_owner,0,28)!=owner)return 0;
    u32 address=l->regions[l->hblank_owner.slot].address;
    return call(l,entry,&address,1);
}
int hg_original_hblank_start(HGOriginalLoader *l,void *owner){
    return hblank_call(l,owner,HG_LOADER_HBlankSystem_Start);
}
int hg_original_hblank_free(HGOriginalLoader *l,void *owner){
    if(!hblank_call(l,owner,HG_LOADER_HBlankSystem_Delete))return 0;
    l->hblank_owner=(DSMappingRef){0};
    return 1;
}
int hg_original_hblank_dispatch(HGOriginalLoader *l){
    DSMapping m=original_mapping(l);
    return hblank_call(l,ds_mapping_resolve(&m,l->hblank_owner,0,28),HG_LOADER_ov01_021FB594);
}
void *hg_original_poison_new(HGOriginalLoader *l,unsigned heap,void *hblank){
    DSMapping m=original_mapping(l);
    if(!hblank||ds_mapping_resolve(&m,l->hblank_owner,0,28)!=hblank)return NULL;
    u32 args[]={heap,l->regions[l->hblank_owner.slot].address};
    return construct_field_owner(l,HG_LOADER_ov01_021FB5D4,args,2,0x328);
}
int hg_original_poison_free(HGOriginalLoader *l,void *owner){
    DSMapping m=original_mapping(l);
    u32 address=ds_mapping_address_of(&m,owner,0x328);
    if(!address)return 0;
    return call(l,HG_LOADER_ov01_021FB610,&address,1);
}

void *hg_original_area_texture(HGOriginalLoader *l,void *area){
    if(!canonical_area(l,area)||!call(l,HG_LOADER_AreaDataManager_GetMapTexture,&l->area_owner,1))return NULL;
    return ds_translate(&l->cpu.memory,l->cpu.r[0],0x3c);
}
void *hg_original_field_texture_owner(HGOriginalLoader *l){if(!l)return NULL;DSMapping m=original_mapping(l);return ds_mapping_resolve(&m,l->field_texture_owner,0,0x18c);}
void *hg_original_field_texture_new(HGOriginalLoader *l){
    if(!l||l->call_depth||l->field_texture_loaded||hg_original_field_texture_owner(l)||!call(l,HG_LOADER_FieldTextureManager_Init,NULL,0))return NULL;
    DSMapping m=original_mapping(l);l->field_texture_owner=ds_mapping_ref(&m,l->cpu.r[0]);return hg_original_field_texture_owner(l);
}
int hg_original_field_texture_load(HGOriginalLoader *l,void *owner,void *texture,unsigned *count){
    if(!owner||owner!=hg_original_field_texture_owner(l)||l->field_texture_loaded||l->call_depth||l->owner_capture)return 0;
    void *area=ds_translate(&l->cpu.memory,l->area_owner,0x8c4);
    if(!area||texture!=hg_original_area_texture(l,area)||!texture)return 0;
    u32 tex=l->cpu.r[0],args[]={l->regions[l->field_texture_owner.slot].address,tex};
    u8 before[0x18c];memcpy(before,owner,sizeof(before));unsigned scope=0;
    l->owner_capture=&scope;l->owner_capture_depth=l->call_depth+1;
#ifdef HG_LOADER_TEST_HOOKS
    l->owner_imports=0;
#endif
    int ok=call(l,HG_LOADER_FieldTextureManager_LoadTexture,args,2);unsigned result=l->cpu.r[0];l->owner_capture=NULL;
    finish_owner_capture(l,&scope,ok);if(!ok){memcpy(owner,before,sizeof(before));return 0;}
    l->field_texture_loaded=1;if(count)*count=result;return 1;
}
int hg_original_field_texture_clear(HGOriginalLoader *l,void *owner){
    if(!owner||owner!=hg_original_field_texture_owner(l)||!l->field_texture_loaded||l->call_depth)return 0;
    u32 g=l->regions[l->field_texture_owner.slot].address;
    if(!call(l,HG_LOADER_FieldTextureManager_FreeAllSlots,&g,1))return 0;l->field_texture_loaded=0;return 1;
}
int hg_original_field_texture_destroy(HGOriginalLoader *l,void *owner){
    if(!owner||owner!=hg_original_field_texture_owner(l)||l->field_texture_loaded||l->call_depth)return 0;
    u32 g=l->regions[l->field_texture_owner.slot].address;
    if(!call(l,HG_LOADER_FieldTextureManager_Destroy,&g,1))return 0;l->field_texture_owner=(DSMappingRef){0};return 1;
}
void *hg_original_weather_owner(HGOriginalLoader *l){if(!l)return NULL;DSMapping m=original_mapping(l);return ds_mapping_resolve(&m,l->weather_owner,0,0x18);}
void *hg_original_weather_new(HGOriginalLoader *l){
    if(!l||!l->canonical_field||l->call_depth||l->owner_capture||hg_original_weather_owner(l))return NULL;
    DSMapping m=original_mapping(l);
    if(!ds_mapping_resolve(&m,l->environment[1],0,0x38))return NULL;
    l->field_fog_identity=l->regions[l->environment[1].slot].address;
    void *fields=ds_translate(&l->cpu.memory,0x0310004c,4);
    if(fields?fields!=&l->field_fog_identity:!borrow_native_at(l,0x0310004c,&l->field_fog_identity,4))return NULL;
    unsigned scope=0;l->owner_capture=&scope;l->owner_capture_depth=l->call_depth+1;
#ifdef HG_LOADER_TEST_HOOKS
    l->owner_imports=0;
#endif
    uint32_t field=0x03100000;int ok=call(l,HG_LOADER_WeatherManager_New,&field,1);uint32_t owner=l->cpu.r[0];l->owner_capture=NULL;
    finish_owner_capture(l,&scope,ok);
    if(!ok)return NULL;
    l->weather_owner=ds_mapping_ref(&m,owner);return hg_original_weather_owner(l);
}
int hg_original_weather_free(HGOriginalLoader *l,void **owner){
    if(!owner||*owner!=hg_original_weather_owner(l)||!*owner||l->call_depth)return 0;
    DSMapping m=original_mapping(l);
    if(!ds_mapping_resolve(&m,l->environment[1],0,0x38)){l->error="retired weather fog";return 0;}
    uint32_t g=l->regions[l->weather_owner.slot].address;
    if(!call(l,HG_LOADER_WeatherManager_Delete,&g,1))return 0;
    l->weather_owner=(DSMappingRef){0};*owner=NULL;return 1;
}
int hg_original_weather_set(HGOriginalLoader *l,unsigned type){
    if(!hg_original_weather_owner(l)||type>=14||l->call_depth)return 0;
    uint32_t args[]={l->regions[l->weather_owner.slot].address,type};return call(l,HG_LOADER_WeatherManager_SetWeather,args,2);
}
#ifdef HG_LOADER_TEST_HOOKS
void hg_original_owner_fail_import(HGOriginalLoader *l,unsigned at){l->owner_fail_import=at;}
unsigned hg_original_owner_imports(HGOriginalLoader *l){return l->owner_imports;}
unsigned hg_original_test_main_tasks(HGOriginalLoader *l){unsigned n=0;for(unsigned i=0;i<NELEMS(l->tasks);i++)n+=l->tasks[i].native&&l->tasks[i].queue==0;return n;}
#endif

#ifdef HG_LOADER_TEST_HOOKS
unsigned hg_original_test_persistent_graphics_vblank(HGOriginalLoader *l){unsigned n=0;for(unsigned i=0;i<NELEMS(l->tasks);i++)n+=l->tasks[i].native&&l->tasks[i].callback==(HG_LOADER_ov01_021FA6E0|1u);return n;}
#endif

#ifdef HG_LOADER_TEST_HOOKS
const void *hg_original_test_camera_preset(HGOriginalLoader *l){return ds_translate(&l->cpu.memory,HG_LOADER_ov01_02206464,20);}
int hg_original_test_camera_preset_start(HGOriginalLoader *l){if(!hg_original_camera_owner(l))return 0;uint32_t args[]={l->camera_fields[1],1};return call(l,HG_LOADER_ov01_021EACBC,args,2);}
#endif

#ifdef HG_LOADER_TEST_HOOKS
int hg_original_test_sound_call(HGOriginalLoader *l,const char *name,const uint32_t *args,unsigned count,uint32_t *result){
    uint32_t entry=hg_original_test_symbol(name);
    if(!l||!l->audio||!entry||count>12||l->call_depth)return 0;
    unsigned cpu=l->sound_cpu;
    l->sound_cpu=!strncmp(name,"arm7_",5);
    int ok=sound_transaction(l,entry,args,count,result);
    l->sound_cpu=cpu;
    return ok;
}
unsigned hg_original_test_arm7_irq(HGOriginalLoader *l){
    return l->arm7_irq_enabled;
}
unsigned hg_original_test_sound_submission(HGOriginalLoader *l,uint32_t out[6]){
    if(out)memcpy(out,l->sound_submission,sizeof(l->sound_submission));return l->sound_submission_count;
}
int hg_original_test_file_import(HGOriginalLoader *l,const char *name,const uint32_t *args,unsigned count,uint32_t *out){
    if(!l||!name||count>3||l->call_depth)return 0;DSThumb c=l->cpu;
    for(unsigned i=0;i<count;i++)c.r[i]=args[i];
    int ok=file_import(&c,l,name);if(ok>0&&out)*out=c.r[0];return ok;
}
size_t hg_original_test_image_size(void){return sizeof(hg_loader_image);}
uint32_t hg_original_test_scratch_address(void){return SCRATCH;}
uint32_t hg_original_test_symbol(const char *name){
    static const struct {const char *name;uint32_t address;} symbols[]={
#include "original_loader_test_exports.inc"
    };
    if(!name)return 0;
    for(unsigned i=0;i<NELEMS(symbols);i++)if(!strcmp(name,symbols[i].name))return symbols[i].address;
    return 0;
}
int hg_original_test_call(HGOriginalLoader *l,const char *name,const uint32_t *args,unsigned count,uint32_t *result){
    uint32_t entry=hg_original_test_symbol(name);
    if(!l||!entry||count>12||l->call_depth)return 0;
    if(!strncmp(name,"arm7_",5))return call_arm7(l,entry,args,count,result);
    return call_result(l,entry,args,count,result);
}
int hg_original_test_weather_task(HGOriginalLoader *l){
    for(unsigned i=0;i<NELEMS(l->tasks);i++)if(l->tasks[i].native&&l->tasks[i].callback==(HG_LOADER_ov01_021EB56C|1u)){
        original_resource_task(l->tasks[i].native,l->tasks+i);return l->error==NULL;
    }return 0;
}
#endif
