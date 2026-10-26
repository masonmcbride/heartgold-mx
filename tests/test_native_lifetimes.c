#include "compat/native_heap.h"
#include "ds_mapping.h"
#include "compat/native_vram.h"
#include "compat/native_video.h"
#include "compat/overlays.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static void must_reject(void (*operation)(void)) {
    pid_t pid=fork();GF_ASSERT(pid>=0);
    if(!pid) { operation();_exit(0); }
    int status;GF_ASSERT(waitpid(pid,&status,0)==pid);
    GF_ASSERT(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT);
}
static void overflow(void) { (void)Heap_Alloc(HEAP_ID_FIELD2,257); }
static void wrong_heap(void) { Heap_FreeExplicit(HEAP_ID_FIELD3,Heap_Alloc(HEAP_ID_FIELD2,1)); }
static void grow_block(void) { void *p=Heap_Alloc(HEAP_ID_FIELD2,8);Heap_Realloc(p,16); }
static void live_child(void) { Heap_Destroy(HEAP_ID_3); }
static void expired_heap(void) { Heap_Destroy(HEAP_ID_FIELD2);(void)Heap_Alloc(HEAP_ID_FIELD2,1); }
typedef struct { int fail_load,fail_unload,live;unsigned loads,unloads;void *resource; } Module;
static BOOL module_load(void *context) {
    Module *m=context;m->loads++;GF_ASSERT(!m->live);
    GF_ASSERT(!FS_LoadOverlay(HG_PROCESSOR_ARM9,124)&&!hg_overlay_unregister(124));
    if(m->fail_load)return FALSE;
    m->resource=Heap_Alloc(HEAP_ID_FIELD2,17);m->live=1;return TRUE;
}
static BOOL module_unload(void *context) {
    Module *m=context;m->unloads++;GF_ASSERT(m->live);
    GF_ASSERT(!FS_UnloadOverlay(HG_PROCESSOR_ARM9,124)&&!hg_overlay_unregister(124));
    if(m->fail_unload)return FALSE;
    Heap_Free(m->resource);m->resource=NULL;m->live=0;return TRUE;
}
static void *texture_work;
static void live_texture_work(void){hg_native_texture_pool_detach(texture_work);}
static void texture_lifetimes(void){
    unsigned bytes=hg_native_texture_work_size(4);texture_work=malloc(bytes);GF_ASSERT(texture_work);
    hg_native_texture_pool_init(64,0,texture_work,bytes,TRUE);
    uint64_t first=hg_native_texture_reserve(0,9),second=hg_native_texture_reserve(0,40);
    GF_ASSERT(first&&second&&first!=second&&hg_native_texture_pool_used(0)==56);
    GF_ASSERT(!hg_native_texture_reserve(0,16)&&hg_native_texture_pool_used(0)==56);
    GF_ASSERT(!hg_native_texture_release(1,first));must_reject(live_texture_work);
    GF_ASSERT(hg_native_texture_release(0,first)&&!hg_native_texture_release(0,first));
    uint64_t replacement=hg_native_texture_reserve(0,16);
    GF_ASSERT(replacement&&replacement!=first&&!hg_native_texture_valid(0,first));
    hg_native_texture_reset(0);GF_ASSERT(!hg_native_texture_valid(0,second)&&!hg_native_texture_valid(0,replacement));
    GF_ASSERT(!hg_native_texture_pool_used(0));hg_native_texture_pool_detach(texture_work);
    hg_native_texture_pool_init(64,0,texture_work,bytes,TRUE);
    uint64_t next=hg_native_texture_reserve(0,64);GF_ASSERT(next&&next!=second&&!hg_native_texture_valid(0,second));
    GF_ASSERT(hg_native_texture_release(0,next));hg_native_texture_pool_detach(texture_work);free(texture_work);texture_work=NULL;
}
typedef struct { DSMapping mapping; unsigned releases,shrinks; } ObserverFixture;
static void mapping_event(void *context,const void *base,size_t old_size,
                           size_t new_size,BOOL release) {
    ObserverFixture *f=context;HGHeapAllocation info;
    GF_ASSERT(hg_heap_allocation_info(base,&info)&&info.size==old_size);
    if(release) {
        f->releases++;GF_ASSERT(ds_mapping_retire_native(&f->mapping,base,old_size?old_size:1));
    } else {
        f->shrinks++;ds_mapping_native_shrink(&f->mapping,base,old_size,new_size);
    }
}
static void observed_mapping_lifetimes(void) {
    u8 ram[16]={0};DSNativeRegion regions[8]={0};unsigned borrowed[8]={0};
    DSMainRAM memory={ram,sizeof(ram),regions,0};u32 next=0x02800000;
    ObserverFixture f={.mapping={&memory,regions,borrowed,8,&next,0x03000000}};
    u8 *p=Heap_Alloc(HEAP_ID_FIELD2,64);HGHeapAllocation owner;
    GF_ASSERT(hg_heap_observe(p+4,12,mapping_event,&f,&owner));
    GF_ASSERT(owner.base==p&&owner.size==64&&owner.heap==HEAP_ID_FIELD2&&owner.identity);
    GF_ASSERT(hg_heap_observe(p,64,mapping_event,&f,NULL));
    GF_ASSERT(!hg_heap_observe(p,65,mapping_event,&f,NULL));
    GF_ASSERT(!hg_heap_observe(p,4,mapping_event,&memory,NULL));
    u32 g=ds_mapping_bind(&f.mapping,0x03100000,p,64);
    DSMappingRef old=ds_mapping_ref(&f.mapping,g);
    regions[1]=(DSNativeRegion){0x03200000,p+40,8};borrowed[1]=1;memory.region_count=2;
    Heap_Realloc(p,32);
    GF_ASSERT(f.shrinks==1&&ds_translate(&memory,g,32)==p);
    GF_ASSERT(!ds_translate(&memory,g+32,1)&&!ds_translate(&memory,0x03200000,1));
    GF_ASSERT(!ds_mapping_bind(&f.mapping,g+40,p+40,4));
    Heap_Free(p);GF_ASSERT(f.releases==1&&!ds_mapping_resolve(&f.mapping,old,0,1));
    u8 *q=Heap_Alloc(HEAP_ID_FIELD2,64);HGHeapAllocation replacement;
    GF_ASSERT(hg_heap_observe(q,64,mapping_event,&f,&replacement));
    GF_ASSERT(replacement.identity!=owner.identity);
    GF_ASSERT(ds_mapping_rebind(&f.mapping,old,q,64)==g);
    GF_ASSERT(ds_translate(&memory,g,64)==q&&!ds_mapping_resolve(&f.mapping,old,0,1));
    Heap_FreeExplicit(HEAP_ID_FIELD2,q);GF_ASSERT(f.releases==2);
    /* Arena destruction must notify borrowers before freeing each block. */
    GF_ASSERT(Heap_Create(HEAP_ID_FIELD2,70,32));
    p=Heap_Alloc(70,16);GF_ASSERT(hg_heap_observe(p,16,mapping_event,&f,NULL));
    GF_ASSERT(ds_mapping_bind(&f.mapping,0x03300000,p,16));
    Heap_Destroy(70);GF_ASSERT(f.releases==3&&!ds_translate(&memory,0x03300000,1));
    p=Heap_Alloc(HEAP_ID_FIELD2,8);
    GF_ASSERT(hg_heap_observe(p,8,mapping_event,&f,NULL));
    GF_ASSERT(ds_mapping_bind(&f.mapping,0x03300100,p,8));
    GF_ASSERT(ds_mapping_retire_native(&f.mapping,p,8));
    hg_heap_unobserve(mapping_event,&f); /* Context dies before native owner. */
    Heap_Free(p);GF_ASSERT(f.releases==3);
}

static void nested_release(void *context,const void *base,size_t old_size,
                            size_t new_size,BOOL release) {
    (void)base;(void)old_size;(void)new_size;GF_ASSERT(release);
    void **dependent=context;void *p=*dependent;*dependent=NULL;Heap_Free(p);
}
static void dependent_owner_release(void) {
    /* Retiring a model/source can free a sidecar that precedes its owner in
     * the allocation list. Exercise both free and arena destruction. */
    void *owner=Heap_Alloc(HEAP_ID_FIELD2,8);
    void *sidecar=Heap_Alloc(HEAP_ID_FIELD2,8);
    GF_ASSERT(hg_heap_observe(owner,8,nested_release,&sidecar,NULL));
    Heap_Free(owner);GF_ASSERT(!sidecar);
    GF_ASSERT(Heap_Create(HEAP_ID_FIELD2,70,32));
    owner=Heap_Alloc(70,8);sidecar=Heap_Alloc(HEAP_ID_FIELD2,8);
    GF_ASSERT(hg_heap_observe(owner,8,nested_release,&sidecar,NULL));
    Heap_Destroy(70);GF_ASSERT(!sidecar);
}

static void acquisition_capture_lifetimes(void) {
    unsigned attempt,other_owner;
    u8 ram[8]={0};DSNativeRegion regions[4]={0};unsigned borrowed[4]={0};
    DSMainRAM memory={ram,sizeof(ram),regions,0};u32 next=0x02800000;
    ObserverFixture f={.mapping={&memory,regions,borrowed,4,&next,0x03000000}};
    u8 *parent=Heap_Alloc(HEAP_ID_FIELD2,8);memset(parent,0x5a,8);
    GF_ASSERT(!hg_heap_capture(&attempt));
    u8 *owned=Heap_Alloc(HEAP_ID_FIELD2,16);
    GF_ASSERT(hg_heap_observe(owned,16,mapping_event,&f,NULL));
    GF_ASSERT(ds_mapping_bind(&f.mapping,0x03100000,owned,16));
    regions[1]=(DSNativeRegion){0x03200000,owned+8,8};borrowed[1]=1;memory.region_count=2;
    DSMappingRef ref=ds_mapping_ref(&f.mapping,0x03100000);
    GF_ASSERT(hg_heap_captured(owned+7,&attempt)&&!hg_heap_captured(parent,&attempt));
    GF_ASSERT(hg_heap_capture(NULL)==&attempt);
    /* An IRQ/other import allocates between the attempt's acquisitions. */
    u8 *irq=Heap_Alloc(HEAP_ID_FIELD2,8);memset(irq,0xa5,8);
    GF_ASSERT(!hg_heap_capture(&other_owner));
    u8 *other=Heap_Alloc(HEAP_ID_FIELD2,8);memset(other,0x33,8);
    GF_ASSERT(hg_heap_capture(&attempt)==&other_owner);
    (void)Heap_Alloc(HEAP_ID_FIELD2,8);
    GF_ASSERT(hg_heap_capture(NULL)==&attempt);
    hg_heap_finish_capture(&attempt,FALSE);
    GF_ASSERT(f.releases==1&&!ds_mapping_resolve(&f.mapping,ref,0,1));
    GF_ASSERT(!ds_translate(&memory,0x03100000,1)&&!ds_translate(&memory,0x03200000,1));
    for(unsigned i=0;i<8;i++)GF_ASSERT(parent[i]==0x5a&&irq[i]==0xa5&&other[i]==0x33);
    GF_ASSERT(hg_heap_captured(other,&other_owner));
    hg_heap_finish_capture(&other_owner,TRUE);
    GF_ASSERT(!hg_heap_captured(other,&other_owner));
    hg_heap_finish_capture(&attempt,FALSE); /* No effect after finish. */
    hg_heap_finish_capture(&other_owner,FALSE); /* Committed block survives. */
    GF_ASSERT(other[0]==0x33);
    Heap_Free(parent);Heap_Free(irq);Heap_Free(other);
}

int main(void) {
    texture_lifetimes();
    /* Existing malloc callers keep the same pointer/free ABI. */
    void *legacy=Heap_Alloc(HEAP_ID_DEFAULT,23);Heap_Free(legacy);
    GF_ASSERT(hg_heap_register_root(HEAP_ID_3,1024));
    GF_ASSERT(!hg_heap_register_root(HEAP_ID_3,1024));
    for(unsigned pass=0;pass<3;pass++) {
        GF_ASSERT(Heap_Create(HEAP_ID_3,HEAP_ID_FIELD2,256));
        GF_ASSERT(Heap_CreateAtEnd(HEAP_ID_FIELD2,HEAP_ID_FIELD3,64));
        observed_mapping_lifetimes();
        acquisition_capture_lifetimes();
        dependent_owner_release();
        HGHeapUsage root,child;
        GF_ASSERT(hg_heap_usage(HEAP_ID_3,&root)&&root.reserved==256&&root.children==1);
        void *p=Heap_Alloc(HEAP_ID_FIELD2,32);memset(p,0x6a,32);
        GF_ASSERT(HeapExp_FndGetTotalFreeSize(HEAP_ID_FIELD2)==160);
        must_reject(overflow);must_reject(wrong_heap);must_reject(live_child);must_reject(grow_block);
        Heap_Realloc(p,16);GF_ASSERT(HeapExp_FndGetTotalFreeSize(HEAP_ID_FIELD2)==176);
        for(unsigned k=0;k<16;k++)GF_ASSERT(((u8 *)p)[k]==0x6a);
        Heap_FreeExplicit(HEAP_ID_FIELD2,p);
        /* Heap destruction releases allocations that original owners leave to
         * the arena, including constructor-owned unk108 in the later probe. */
        memset(Heap_AllocAtEnd(HEAP_ID_FIELD3,11),0xa5,11);
        Heap_Destroy(HEAP_ID_FIELD3);
        GF_ASSERT(hg_heap_usage(HEAP_ID_FIELD2,&child)&&!child.allocated&&!child.reserved&&!child.blocks);
        must_reject(expired_heap);
        Module m={.fail_load=1};GF_ASSERT(hg_overlay_register_lifecycle(124,module_load,module_unload,&m));
        GF_ASSERT(!FS_LoadOverlay(HG_PROCESSOR_ARM9,124)&&!hg_overlay_live());
        GF_ASSERT(!FS_LoadOverlay(1,124));m.fail_load=0;
        GF_ASSERT(HandleLoadOverlay(124,OVY_LOAD_NORMAL)&&m.live&&hg_overlay_live()==1);
        GF_ASSERT(!FS_LoadOverlay(HG_PROCESSOR_ARM9,124)&&!hg_overlay_unregister(124));
        m.fail_unload=1;GF_ASSERT(!FS_UnloadOverlay(HG_PROCESSOR_ARM9,124)&&m.live&&hg_overlay_live()==1);
        m.fail_unload=0;GF_ASSERT(FS_UnloadOverlay(HG_PROCESSOR_ARM9,124));
        GF_ASSERT(HandleLoadOverlay(124,OVY_LOAD_ASYNC));UnloadOverlayByID(124);
        GF_ASSERT(hg_overlay_unregister(124)&&!m.live&&m.loads==3&&m.unloads==3);
        Heap_Destroy(HEAP_ID_FIELD2);
        GF_ASSERT(hg_heap_usage(HEAP_ID_3,&root)&&!root.reserved&&!root.allocated&&!root.children);
    }
    Heap_Destroy(HEAP_ID_3);
    puts("native heap ownership/budgets/destruction and static module failure/retry/reload pass");
    return 0;
}
