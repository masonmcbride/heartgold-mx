#include "ds_resources.h"
#include "compat/resources.h"
/* Process-lifetime cache. All callers run on the main thread. */
typedef struct { uint8_t *bytes; DSNarc narc; } Cache;
static Cache cache[14];
int native_resource_member(int archive,int group,int member,const uint8_t **bytes,size_t *size) {
    const char *name;int slot;
    if(archive==NARC_itemtool_itemdata_item_icon) { slot=0;name="item_icon.narc"; }
    else if(archive==NARC_msgdata_msg && group==221) { slot=1;name="msg_0221.narc"; }
    else if(archive==NARC_msgdata_msg && group==222) { slot=2;name="msg_0222.narc"; }
    else if(archive==NARC_msgdata_msg && group==251) { slot=3;name="msg_0251.narc"; }
    else if(archive==NARC_graphic_font) { slot=4;name="font.narc"; }
    else if(archive==NARC_native_bag) { slot=5;name="bag.narc"; }
    else if(archive==NARC_fielddata_mapmatrix_map_matrix) { slot=6;name="map_matrix.narc"; }
    else if(archive==NARC_fielddata_landdata_land_data) { slot=7;name="land_data.narc"; }
    else if(archive==NARC_a_0_4_2) { slot=8;name="area_data.narc"; }
    else if(archive==NARC_a_0_4_4) { slot=9;name="map_texture.narc"; }
    else if(archive==NARC_a_0_4_0) { slot=10;name="prop_models.narc"; }
    else if(archive==NARC_a_0_4_3) { slot=11;name="area_models.narc"; }
    else if(archive==NARC_a_1_4_8) { slot=12;name="room_props.narc"; }
    else if(archive==NARC_data_mmodel_mmodel) { slot=13;name="characters.narc"; }
    else return 0;
    Cache *c=&cache[slot];
    if(!c->bytes) {
        char path[4096];if(snprintf(path,sizeof(path),"%s/%s",HG_PACKED_ROOT,name)>=(int)sizeof(path))return 0;
        FILE *f=fopen(path,"rb");if(!f)return 0;
        if(fseek(f,0,SEEK_END)) { fclose(f);return 0; }long length=ftell(f);
        if(length<=0 || length>(archive==NARC_fielddata_landdata_land_data?32:16)*1024*1024 || fseek(f,0,SEEK_SET)) { fclose(f);return 0; }
        uint8_t *p=malloc((size_t)length);if(!p) { fclose(f);return 0; }
        int ok=fread(p,1,(size_t)length,f)==(size_t)length;fclose(f);
        if(!ok || !ds_narc_open(&c->narc,p,(size_t)length)) { free(p);return 0; }c->bytes=p;
    }
    return member>=0 && ds_narc_member(&c->narc,(size_t)member,bytes,size);
}

struct NARC { const uint8_t *bytes;size_t size;DSNarc narc;const uint8_t *stream;size_t stream_size,position; };
NARC *NARC_New(NarcId id,enum HeapID heap) {
    const uint8_t *unused;size_t size;
    if(!native_resource_member(id,0,0,&unused,&size))abort();
    int slot=id==NARC_graphic_font?4:id==NARC_native_bag?5:-1;if(slot<0)abort();
    NARC *n=Heap_Alloc(heap,sizeof(*n));memset(n,0,sizeof(*n));n->narc=cache[slot].narc;n->bytes=cache[slot].bytes;n->size=n->narc.size;return n;
}
void NARC_Delete(NARC *n) { free(n); }
void NARC_ReadFromMember(NARC *n,u32 member,u32 offset,u32 length,void *out) {
    const uint8_t *p;size_t size;if(!ds_narc_member(&n->narc,member,&p,&size) || offset>size || length>size-offset)abort();memcpy(out,p+offset,length);
}
void NARC_ReadFromAbsolutePos(NARC *n,u32 offset,u32 length,void *out) { if(offset>n->size || length>n->size-offset)abort();memcpy(out,n->bytes+offset,length); }
void NARC_GetMemberImageStartOffset(NARC *n,u32 member,u32 *offset) { const uint8_t *p;size_t size;if(!ds_narc_member(&n->narc,member,&p,&size))abort();*offset=(u32)(p-n->bytes); }

NARC *native_resource_stream(const void *p,size_t size) {
    if(!p && size)return NULL;NARC *n=calloc(1,sizeof(*n));if(!n)return NULL;
    n->stream=p;n->stream_size=size;return n;
}
void NARC_ReadFile(NARC *n,u32 length,void *out) {
    if(!n || n->position>n->stream_size || length>n->stream_size-n->position || (!out && length))abort();
    if(length)memcpy(out,n->stream+n->position,length);n->position+=length;
}

int native_resource_file(const char *name,const uint8_t **out,size_t *size) {
    static struct { uint8_t *bytes;size_t length; } files[2];
    if(!name || !out || !size)return 0;
    unsigned slot;if(!strcmp(name,"bm_field_matshp.dat"))slot=0;else if(!strcmp(name,"bm_room_matshp.dat"))slot=1;else return 0;
    uint8_t *bytes=files[slot].bytes;size_t length=files[slot].length;
    if(!bytes) {
        char path[4096];if(snprintf(path,sizeof(path),"%s/%s",HG_PACKED_ROOT,name)>=(int)sizeof(path))return 0;
        FILE *f=fopen(path,"rb");if(!f)return 0;
        if(fseek(f,0,SEEK_END)) { fclose(f);return 0; }long n=ftell(f);
        if(n<4 || n>1024*1024 || fseek(f,0,SEEK_SET)) { fclose(f);return 0; }
        uint8_t *p=malloc((size_t)n);if(!p) { fclose(f);return 0; }
        int ok=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok) { free(p);return 0; }
        bytes=p;length=(size_t)n;files[slot].bytes=bytes;files[slot].length=length;
    }
    *out=bytes;*size=length;return 1;
}
