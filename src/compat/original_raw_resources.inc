/* Original native raw manager's SDK transport. Views/keys borrow the existing
 * archive cache, mapped heaps and native texture budgets. No second allocator. */
static _Thread_local HGOriginalLoader *raw_context;
static _Thread_local HGRawTex *raw_current;
static _Thread_local jmp_buf raw_rejected;
static void raw_fail(const char *name){raw_context->error=name;longjmp(raw_rejected,1);}
static u32 raw_u32(const u8 *p){return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
static u32 raw_guest_address(const void *p){
    HGOriginalLoader *l=raw_context;
    for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes==p)return l->regions[i].address;
    raw_fail("raw resource: unmapped buffer identity");return 0;
}
static HGRawTex *raw_tex(const void *header){
    HGOriginalLoader *l=raw_context;
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(l->raw_views[i].owner==header){raw_current=l->raw_views+i;return raw_current;}
    unsigned texture;if(!resource_texture(l,raw_guest_address(header),&texture))raw_fail("raw resource: invalid TEX0 resource");
    unsigned region=l->textures[texture].region;DSTextureBank bank;
    if(!l->archive_bytes[region]||!ds_texture_bank(l->archive_bytes[region],l->archive_sizes[region],&bank))raw_fail("raw resource: missing immutable texture backing");
    size_t size;if(!hg_heap_allocation_size(header,&size)||size!=l->archive_sizes[region]||memcmp(header,l->archive_bytes[region],size))raw_fail("raw resource: mutated upload storage");
    /* Unused compressed records can retain a shared texture dictionary offset.
     * Require zero size/flags/key; permit that validated dictionary reference. */
    for(unsigned i=24;i<30;i++)if(bank.bytes[i])raw_fail("raw resource: compressed texture size/flags");
    if(raw_u32(bank.bytes+32)||((bank.bytes[30]||bank.bytes[31])&&(bank.bytes[30]!=bank.bytes[14]||bank.bytes[31]!=bank.bytes[15])))raw_fail("raw resource: compressed texture key/dictionary");
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(!l->raw_views[i].owner){
        HGRawTex *v=l->raw_views+i;*v=(HGRawTex){.header={.size=(u32)bank.size},.bytes=bank.bytes,.owner=(void *)header,.texture=texture,.source_guest=raw_guest_address(header)};
        l->textures[texture].bank=bank;raw_current=v;return v;
    }raw_fail("raw resource: texture view capacity");return NULL;
}
static void *raw_tex_data(HGRawTex *t){
    HGOriginalLoader *l=raw_context;unsigned region=l->textures[t->texture].region;
    size_t offset=(size_t)(t->bytes-l->archive_bytes[region])+raw_u32(t->bytes+20),size;
    if(!hg_heap_allocation_size(t->owner,&size)||offset>size)raw_fail("raw resource: texture payload boundary");
    return (u8 *)t->owner+offset;
}
static u32 raw_size(HGRawTex *t,unsigned kind){raw_current=t;unsigned at=kind==0?12:kind==1?48:24;return (t->bytes[at]|(u32)t->bytes[at+1]<<8)*8;}
static u32 raw_alloc_key(u32 bytes,BOOL special,u32 options,unsigned kind){
    static u32 serial=0x07000000;HGOriginalLoader *l=raw_context;
    if(!raw_current||special||options||!hg_native_texture_pool_active(kind))raw_fail("raw resource: unsupported key allocation mode");
    unsigned texture=raw_current->texture;if(l->textures[texture].allocation[kind])raw_fail("raw resource: duplicate key allocation");
    unsigned i=0;while(i<NELEMS(l->sdk_keys)&&l->sdk_keys[i].token)i++;
    if(i==NELEMS(l->sdk_keys)||serial==UINT32_MAX)raw_fail("raw resource: key identity capacity");
    uint64_t identity=hg_native_texture_reserve(kind,bytes);if(!identity)return 0;
    l->textures[texture].allocation[kind]=identity;l->sdk_keys[i].token=++serial;l->sdk_keys[i].texture=texture;l->sdk_keys[i].kind=kind;return serial;
}
static int raw_key_matches(HGRawTex *t,u32 key,unsigned kind){
    if(!key)return !raw_context->textures[t->texture].allocation[kind];
    HGOriginalLoader *l=raw_context;
    for(unsigned i=0;i<NELEMS(l->sdk_keys);i++)if(l->sdk_keys[i].token==key)return l->sdk_keys[i].kind==kind&&l->sdk_keys[i].texture==t->texture&&hg_native_texture_valid(kind,l->textures[t->texture].allocation[kind]);
    return 0;
}
static int raw_free_key(u32 key,unsigned kind){
    if(!key)return 0;HGOriginalLoader *l=raw_context;
    for(unsigned i=0;i<NELEMS(l->sdk_keys);i++)if(l->sdk_keys[i].token==key&&l->sdk_keys[i].kind==kind){
        unsigned texture=l->sdk_keys[i].texture;
        if(!hg_native_texture_release(kind,l->textures[texture].allocation[kind]))raw_fail("raw resource: expired key");
        l->textures[texture].allocation[kind]=0;l->textures[texture].resident&=~(1u<<kind);l->sdk_keys[i].token=0;return 0;
    }raw_fail("raw resource: invalid key owner");return -1;
}
static void raw_set_keys(HGRawTex *t,u32 key,u32 compressed){if(compressed||!raw_key_matches(t,key,0))raw_fail("raw resource: texture key binding");t->keys[0]=key;}
static void raw_set_palette(HGRawTex *t,u32 key){if(!raw_key_matches(t,key,1))raw_fail("raw resource: palette key binding");t->keys[1]=key;}
static void raw_release_keys(HGRawTex *t,u32 *key,u32 *compressed){*key=t->keys[0];*compressed=0;t->keys[0]=0;}
static u32 raw_release_palette(HGRawTex *t){u32 key=t->keys[1];t->keys[1]=0;return key;}
static void raw_upload(HGRawTex *t,BOOL synchronous,unsigned kind){
    if(!synchronous||!raw_key_matches(t,t->keys[kind],kind))raw_fail("raw resource: upload key ownership");
    HGOriginalLoader *l=raw_context;unsigned region=l->textures[t->texture].region;size_t size;
    if(!hg_heap_allocation_size(t->owner,&size)||size!=l->archive_sizes[region]||memcmp(t->owner,l->archive_bytes[region],size))raw_fail("raw resource: changed upload backing");
    l->textures[t->texture].resident|=1u<<kind;
}
static void raw_flush(const void *tex,u32 bytes){
    HGOriginalLoader *l=raw_context;
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(tex==l->raw_views+i&&bytes==l->raw_views[i].header.size){
        HGRawTex *t=l->raw_views+i;unsigned region=l->textures[t->texture].region;size_t size,offset=(size_t)(t->bytes-l->archive_bytes[region]);
        if(!hg_heap_allocation_size(t->owner,&size)||offset>size||bytes>size-offset)raw_fail("raw resource: flush extent");
        hg_native_cache_publish((u8 *)t->owner+offset,bytes);return;
    }
    raw_fail("raw resource: flush identity");
}
static void raw_map_prefix(GF_3DGfxRawResObj *obj,void *source){
    HGOriginalLoader *l=raw_context;void *prefix=GF2dGfxRawResObj_GetData(obj->obj);if(prefix==source)return;
    HGRawTex *full=raw_tex(source);size_t size;
    if(!hg_heap_allocation_size(prefix,&size)||size!=(size_t)((u8 *)raw_tex_data(full)-(u8 *)source)||memcmp(source,prefix,size)||l->cpu.memory.region_count==NELEMS(l->regions))raw_fail("raw resource: prefix ownership");
    uint32_t guest=reserve_guest(l,size,32);if(!guest)raw_fail("raw resource: guest address exhaustion");
    unsigned region=l->cpu.memory.region_count++,old=l->textures[full->texture].region;
    l->regions[region]=(DSNativeRegion){guest,prefix,size};GF_ASSERT(observe_native(l,prefix,size));l->region_heaps[region]=l->region_heaps[old];
    l->archive_bytes[region]=l->archive_bytes[old];l->archive_sizes[region]=l->archive_sizes[old];l->archive_ids[region]=l->archive_ids[old];l->archive_members[region]=l->archive_members[old];
    for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(!l->raw_views[i].owner){
        l->raw_views[i]=*full;l->raw_views[i].owner=prefix;l->raw_views[i].keys[0]=l->raw_views[i].keys[1]=0;
        l->textures[full->texture].region=region;l->textures[full->texture].guest=l->regions[region].address+(u32)(full->bytes-l->archive_bytes[region]);return;
    }raw_fail("raw resource: prefix view capacity");
}
static void raw_heap_free(void *p){
    HGOriginalLoader *l=raw_context;
    if(l)for(unsigned i=0;i<l->cpu.memory.region_count;i++)if(l->regions[i].bytes==p){
        if(l->borrowed_regions[i])raw_fail("raw resource: borrowed allocation free");
        retire_land(l,l->regions[i].address,l->regions[i].size);retire_region(l,i);
    }
    if(l)for(unsigned i=0;i<NELEMS(l->raw_views);i++)if(l->raw_views[i].owner==p)memset(l->raw_views+i,0,sizeof(l->raw_views[i]));
    Heap_Free(p);
}

/* 0: unuploaded fixture; 1: VWait transferred; 2: IRQ uploaded or pending,
 * with full source still owned until the original VWait callback transfers it. */
unsigned hg_original_effects_raw_probe(HGOriginalLoader *l,int uploaded){
    raw_context=l;if(setjmp(raw_rejected))return UINT32_MAX;
    if(!l->effect_raw)return UINT32_MAX;unsigned count=0;
    for(int i=0;i<l->effect_raw->man->max;i++){
        GF_3DGfxRawResObj *obj=l->effect_raw->objs+i;if(!obj->obj)continue;count++;
        HGRawTex *tex=GF3dGfxRawResObj_GetTex(obj);size_t size;
        if(!obj->headerNeedsLoad||!hg_heap_allocation_size(tex->owner,&size)||
           !raw_key_matches(tex,obj->texKey,0)||obj->tex4x4Key||!raw_key_matches(tex,obj->plttKey,1)||
           !ds_translate(&l->cpu.memory,raw_guest_address(tex->owner),size))return UINT32_MAX;
        if(uploaded==1){
            if(!obj->hasLoadedFromHeader||obj->resFileHeader||texture_resident(l,tex->texture)!=3||
               tex->keys[0]!=obj->texKey||tex->keys[1]!=obj->plttKey||ds_translate(&l->cpu.memory,tex->source_guest,1))return UINT32_MAX;
        }else if(obj->hasLoadedFromHeader||!obj->resFileHeader||(uploaded==0&&texture_resident(l,tex->texture))||
                 (uploaded==2&&texture_resident(l,tex->texture)!=0&&texture_resident(l,tex->texture)!=3)||
                 tex->keys[0]||tex->keys[1]||!ds_translate(&l->cpu.memory,tex->source_guest,1))return UINT32_MAX;
    }
    return count==(unsigned)l->effect_raw->man->num?count:UINT32_MAX;
}
