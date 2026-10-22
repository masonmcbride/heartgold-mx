#include "ds_resources.h"
#include "compat/resources.h"
#include "message_bank_count.inc"
/* Process-lifetime cache. All callers run on the main thread. */
typedef struct { uint8_t *bytes; DSNarc narc; } Cache;
static Cache cache[45];
static Cache message_cache[HG_MESSAGE_BANK_COUNT];
static Cache *resource_archive(int archive,int group) {
    const char *name;int slot;char message_name[32];Cache *c;
    if(archive==NARC_msgdata_msg) {
        if(group<0||group>=HG_MESSAGE_BANK_COUNT)return 0;
        snprintf(message_name,sizeof(message_name),"msg_%04d.narc",group);name=message_name;c=&message_cache[group];
    } else {
        if(archive==NARC_a_2_3_0) { slot=44;name="safari_enc.narc"; }
        else if(archive==NARC_itemtool_itemdata_item_icon) { slot=0;name="item_icon.narc"; }
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
        else if(archive==NARC_fielddata_eventdata_zone_event) { slot=14;name="zone_event.narc"; }
        else if(archive==NARC_fielddata_script_scr_seq) { slot=23;name="field_scripts.narc"; }
        else if(archive==NARC_poketool_personal_personal) {slot=24;name="personal.narc";}
        else if(archive==NARC_poketool_personal_growtbl) {slot=25;name="growth.narc";}
        else if(archive==NARC_poketool_personal_wotbl) {slot=26;name="learnsets.narc";}
        else if(archive==NARC_poketool_waza_waza_tbl) {slot=27;name="moves.narc";}
        else if(archive==NARC_fielddata_tsurepoke_tp_param) {slot=28;name="follow_params.narc";}
        else if(archive==NARC_a_1_0_6) {slot=29;name="prop_animations.narc";}
        else if(archive==NARC_a_1_0_7) {slot=30;name="field_prop_animations.narc";}
        else if(archive==NARC_a_1_0_8) {slot=31;name="room_prop_animations.narc";}
        else if(archive==NARC_a_0_0_0) {slot=32;name="battle_move_script.narc";}
        else if(archive==NARC_a_0_0_1) {slot=33;name="battle_subscript.narc";}
        else if(archive==NARC_a_0_3_0) {slot=34;name="battle_effect_script.narc";}
        else if(archive==NARC_application_zukanlist_zkn_data_zukan_data) {slot=35;name="dex_measures.narc";}
        else if(archive==NARC_application_zukanlist_zkn_data_zukan_data_gira) {slot=36;name="dex_measures_gira.narc";}
        else if(archive==NARC_poketool_trainer_trdata) {slot=37;name="trainer_data.narc";}
        else if(archive==NARC_poketool_trainer_trpoke) {slot=38;name="trainer_parties.narc";}
        else if(archive==NARC_application_custom_ball_edit_gs_cb_data) {slot=39;name="trainer_capsules.narc";}
        else if(archive==NARC_poketool_trmsg_trtbl) {slot=40;name="trainer_message_table.narc";}
        else if(archive==NARC_poketool_trmsg_trtblofs) {slot=41;name="trainer_message_offsets.narc";}
        else if(archive==NARC_fielddata_encountdata_g_enc_data) {slot=42;name="wild_encounters.narc";}
        else if(archive==HG_RESOURCE_BATTLE_PNG) {slot=43;name="battle_sprites_png.narc";}
        else return 0;
        c=&cache[slot];
    }
    if(!c->bytes) {
        char path[4096];if(snprintf(path,sizeof(path),"%s/%s",HG_PACKED_ROOT,name)>=(int)sizeof(path))return 0;
        FILE *f=fopen(path,"rb");if(!f)return 0;
        if(fseek(f,0,SEEK_END)) { fclose(f);return 0; }long length=ftell(f);
        if(length<=0 || length>(archive==NARC_fielddata_landdata_land_data?32:16)*1024*1024 || fseek(f,0,SEEK_SET)) { fclose(f);return 0; }
        uint8_t *p=malloc((size_t)length);if(!p) { fclose(f);return 0; }
        int ok=fread(p,1,(size_t)length,f)==(size_t)length;fclose(f);
        if(!ok || !ds_narc_open(&c->narc,p,(size_t)length)) { free(p);return 0; }c->bytes=p;
    }
    return c;
}
int native_resource_member(int archive,int group,int member,const uint8_t **bytes,size_t *size) {
    Cache *c=resource_archive(archive,group);
    return c && member>=0 && ds_narc_member(&c->narc,(size_t)member,bytes,size);
}
int native_resource_range(int archive,int group,int member,size_t offset,size_t length,const uint8_t **bytes) {
    Cache *c=resource_archive(archive,group);const uint8_t *start;size_t member_size;
    if(!c || member<0 || !ds_narc_member(&c->narc,(size_t)member,&start,&member_size))return 0;
    /* Original filesystem range reads may cross member boundaries (LoadMoveTbl).
     * Bound these reads by the validated archive image, preserving its padding. */
    size_t available=c->narc.image_size-(size_t)(start-c->narc.image);
    if(offset>available || length>available-offset)return 0;
    *bytes=start+offset;return 1;
}

struct NARC { const uint8_t *bytes;size_t size;DSNarc narc;const uint8_t *stream;size_t stream_size,position; };
NARC *NARC_New(NarcId id,enum HeapID heap) {
    Cache *c=resource_archive(id,0);if(!c)abort();
    NARC *n=Heap_Alloc(heap,sizeof(*n));memset(n,0,sizeof(*n));n->narc=c->narc;n->bytes=c->bytes;n->size=n->narc.size;return n;
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
    static const char *const names[]={"bm_field_matshp.dat","bm_room_matshp.dat"};
    if(!name || !out || !size)return 0;
    unsigned slot=NELEMS(names);for(unsigned i=0;i<NELEMS(names);i++)if(!strcmp(name,names[i])){slot=i;break;}
    if(slot==NELEMS(names))return 0;
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

void NARC_ReadWholeMember(NARC *n,u32 member,void *out) { const uint8_t *p;size_t size;if(!ds_narc_member(&n->narc,member,&p,&size))abort();memcpy(out,p,size); }
void *NARC_AllocAndReadWholeMember(NARC *n,u32 member,enum HeapID heap) { const uint8_t *p;size_t size;if(!ds_narc_member(&n->narc,member,&p,&size)||size>UINT32_MAX)abort();void *out=Heap_Alloc(heap,(u32)size);memcpy(out,p,size);return out; }

void *GfGfxLoader_LoadFromOpenNarc(NARC *n,s32 member,BOOL compressed,enum HeapID heap,BOOL atEnd) {
    const uint8_t *bytes;size_t size;
    if(compressed || member<0 || !ds_narc_member(&n->narc,(size_t)member,&bytes,&size))abort();
    void *out=atEnd?Heap_AllocAtEnd(heap,size):Heap_Alloc(heap,size);
    if(!out)abort();memcpy(out,bytes,size);return out;
}
