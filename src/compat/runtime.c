#include "item.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "item_records.inc"
#include "compat/resources.h"
static void unavailable(const char *name) { fprintf(stderr, "Unsupported native boundary: %s\n", name); abort(); }
void *Heap_Alloc(enum HeapID heap, u32 size) { (void)heap; void *p = malloc(size); if (!p) abort(); return p; }
void Heap_Free(void *p) { free(p); }
void Heap_FreeExplicit(enum HeapID heap, void *p) { (void)heap; free(p); }
void MI_CpuClear16(void *p, u32 n) { memset(p, 0, n); }
void MI_CpuCopy8(const void *p, void *q, u32 n) { memcpy(q, p, n); }
void *AllocAndReadWholeNarcMemberByIdPair(int archive, int member, enum HeapID heap) {
    if(archive==NARC_itemtool_itemdata_item_icon) {
        const uint8_t *bytes;size_t size;
        if(!native_resource_member(archive,0,member,&bytes,&size))unavailable("icon member");
        void *p=Heap_Alloc(heap,(u32)size);memcpy(p,bytes,size);return p;
    }
    if (archive != NARC_itemtool_itemdata_item_data || member < 0 || (size_t)member >= NELEMS(native_items)) unavailable("archive/member");
    ItemData *p = Heap_Alloc(heap, sizeof(*p)); *p = native_items[member]; return p;
}
void *AllocAndReadFromNarcMemberByIdPair(int a, int m, enum HeapID h, u32 offset, u32 size) {
    if (a != NARC_itemtool_itemdata_item_data || m != 0 || offset > sizeof(native_items) || size > sizeof(native_items)-offset) unavailable("archive slice");
    void *p = Heap_Alloc(h, size); memcpy(p, (const u8 *)native_items+offset, size); return p;
}
/* Unported entry points fail explicitly. */
void *SaveArray_Get(SaveData *s, int i) { (void)s; (void)i; unavailable("SaveArray_Get"); return NULL; }
BagView *BagView_New(u8 h) { (void)h; unavailable("BagView_New"); return NULL; }
void BagView_SetItem(BagView *b, struct ItemSlot *s, u8 p, u8 i) { (void)b; (void)s; (void)p; (void)i; unavailable("BagView_SetItem"); }
/* UTF-8 boundary adapter over the upstream JSON messages, not a DS String ABI. */
struct String { char *utf8; };
struct MsgData { int archive,group; };
String *native_string_new(void) { String *s=Heap_Alloc(HEAP_ID_DEFAULT,sizeof(*s));s->utf8=NULL;return s; }
void native_string_free(String *s) { if(s) { free(s->utf8);free(s); } }
const char *native_string_utf8(const String *s) { return s->utf8?s->utf8:""; }
MsgData *NewMsgDataFromNarc(int type,int archive,int group,enum HeapID heap) {
    if(type!=MSGDATA_LOAD_LAZY)unavailable("message load mode");
    MsgData *m=Heap_Alloc(heap,sizeof(*m));m->archive=archive;m->group=group;return m;
}
void ReadMsgDataIntoString(MsgData *m,int index,String *s) {
    const uint8_t *bytes;size_t size;
    if(!native_resource_member(m->archive,m->group,index,&bytes,&size) || !size || bytes[size-1]!=0)unavailable("message member");
    char *text=Heap_Alloc(HEAP_ID_DEFAULT,(u32)size);memcpy(text,bytes,size);free(s->utf8);s->utf8=text;
}
void DestroyMsgData(MsgData *m) { free(m); }
String *NewString_ReadMsgData(MsgData *m,int index) { String *s=native_string_new();ReadMsgDataIntoString(m,index,s);return s; }
