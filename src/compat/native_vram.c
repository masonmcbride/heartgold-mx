#include "compat/native_heap.h"
#include "ds_graphics.h"
#include "compat/native.h"
#include "compat/native_vram.h"
static NNSGfdVramTransferTask *arena;
static uint32_t capacity;
void NNS_GfdInitVramTransferManager(NNSGfdVramTransferTask *tasks,uint32_t count){GF_ASSERT(tasks&&count&&!arena);arena=tasks;capacity=count;memset(tasks,0,count*sizeof(*tasks));}
void NNS_GfdDoVramTransfer(void){GF_ASSERT(arena&&capacity);for(uint32_t i=0;i<capacity;i++)GF_ASSERT(!arena[i].source);}
void hg_native_vram_detach(NNSGfdVramTransferTask *tasks){GF_ASSERT(arena==tasks);arena=NULL;capacity=0;}
typedef struct {uint64_t identity;uint32_t offset,size;} TextureAllocation;
typedef struct {uint32_t capacity,used,count;TextureAllocation allocations[];} TexturePool;
static TexturePool *pools[2];
static uint64_t allocation_identity;
uint32_t hg_native_texture_work_size(uint32_t count){GF_ASSERT(count&&count<(UINT32_MAX-sizeof(TexturePool))/sizeof(TextureAllocation));return sizeof(TexturePool)+count*sizeof(TextureAllocation);}
static void texture_pool_init(unsigned kind,uint32_t size,void *work,uint32_t bytes,int use_default){
    GF_ASSERT(kind<2&&size&&work&&use_default&&!pools[kind]&&bytes>=hg_native_texture_work_size(1));
    memset(work,0,bytes);TexturePool *p=work;p->capacity=size;p->count=(bytes-sizeof(*p))/sizeof(TextureAllocation);pools[kind]=p;
}
void hg_native_texture_pool_init(uint32_t size,uint32_t compressed,void *work,uint32_t bytes,int use_default){GF_ASSERT(!compressed);texture_pool_init(0,size,work,bytes,use_default);}
void hg_native_palette_pool_init(uint32_t size,void *work,uint32_t bytes,int use_default){texture_pool_init(1,size,work,bytes,use_default);}
void hg_native_texture_pool_detach(void *work){
    for(unsigned k=0;k<2;k++)if(pools[k]==work){GF_ASSERT(!pools[k]->used);pools[k]=NULL;return;}
    GF_ASSERT(!"unknown native texture allocator work");
}
int hg_native_texture_pool_active(unsigned k){GF_ASSERT(k<2);return pools[k]!=NULL;}
uint32_t hg_native_texture_pool_used(unsigned k){GF_ASSERT(k<2&&pools[k]);return pools[k]->used;}
uint64_t hg_native_texture_reserve(unsigned k,uint32_t bytes){
    GF_ASSERT(k<2&&pools[k]);if(!bytes)return 0;TexturePool *p=pools[k];
    if(bytes>UINT32_MAX-7)return 0;bytes=(bytes+7)&~7u;
    if(bytes>p->capacity-p->used)return 0;
    unsigned slot=p->count;for(unsigned i=0;i<p->count;i++)if(!p->allocations[i].identity){slot=i;break;}
    if(slot==p->count)return 0;
    uint32_t offset=0;
    for(;;){int overlap=0;
        if(offset>p->capacity||bytes>p->capacity-offset)return 0;
        for(unsigned i=0;i<p->count;i++){TextureAllocation *a=p->allocations+i;
            if(a->identity&&offset<a->offset+a->size&&a->offset<offset+bytes){offset=a->offset+a->size;overlap=1;break;}}
        if(!overlap)break;
    }
    GF_ASSERT(allocation_identity!=UINT64_MAX);uint64_t id=++allocation_identity;
    p->allocations[slot]=(TextureAllocation){id,offset,bytes};p->used+=bytes;return id;
}
int hg_native_texture_valid(unsigned k,uint64_t id){
    GF_ASSERT(k<2);if(!id||!pools[k])return 0;TexturePool *p=pools[k];
    for(unsigned i=0;i<p->count;i++)if(p->allocations[i].identity==id)return 1;return 0;
}
int hg_native_texture_release(unsigned k,uint64_t id){
    GF_ASSERT(k<2);if(!id||!pools[k])return 0;TexturePool *p=pools[k];
    for(unsigned i=0;i<p->count;i++)if(p->allocations[i].identity==id){p->used-=p->allocations[i].size;p->allocations[i]=(TextureAllocation){0};return 1;}return 0;
}
void hg_native_texture_reset(unsigned k){GF_ASSERT(k<2&&pools[k]);TexturePool *p=pools[k];memset(p->allocations,0,p->count*sizeof(TextureAllocation));p->used=0;}
void hg_native_image_proxy_init(HGNativeImageProxy *p){GF_ASSERT(p);for(unsigned i=0;i<3;i++)p->locations[i]=UINT32_MAX;}
uint32_t hg_native_image_proxy_location(const HGNativeImageProxy *p,int type){GF_ASSERT(p&&type>=0&&type<3);return p->locations[type];}

int hg_native_palette_unpack(void *bytes,NNSG2dPaletteData *view){
    size_t size,length;unsigned fmt,ext;const uint8_t *pixels;
    if(!view||!hg_heap_allocation_size(bytes,&size)||!ds_palette_open(bytes,size,&pixels,&length,&fmt,&ext)||length>UINT32_MAX)return 0;
    *view=(NNSG2dPaletteData){(void *)pixels,(uint32_t)length,fmt,ext};return 1;
}
