#include "field_data.h"
#include "map_matrix.h"
#include "safari_zone.h"
#include "sys_vars.h"
#include "compat/resources.h"
int hg_matrix_validate(const uint8_t *p,size_t n,HGMatrixLayout *layout) {
    if(n<5)return 0;
    HGMatrixLayout v={p[0],p[1],p[2],p[3],p[4],(size_t)p[0]*p[1]};
    if(!v.width || !v.height || v.cells>MAP_MATRIX_MAX_SIZE || v.has_headers>1 || v.has_altitudes>1 || v.name_length>MAP_MATRIX_MAX_NAME_LENGTH)return 0;
    size_t expected=5+v.name_length+v.cells*(2+2*v.has_headers+v.has_altitudes);
    if(n!=expected)return 0;if(layout)*layout=v;return 1;
}
void MI_CpuFill16(void *dest,u16 value,u32 bytes) { if(bytes%2)abort();u16 *p=dest;for(u32 i=0;i<bytes/2;i++)p[i]=value; }
void *AllocAtEndAndReadWholeNarcMemberByIdPair(NarcId archive,s32 member,enum HeapID heap) {
    const uint8_t *p;size_t n;
    if(!native_resource_member(archive,0,member,&p,&n) || n>UINT32_MAX || (archive==NARC_fielddata_mapmatrix_map_matrix && !hg_matrix_validate(p,n,NULL))) { fprintf(stderr,"Invalid archive member resource\n");abort(); }
    void *copy=Heap_AllocAtEnd(heap,(u32)n);memcpy(copy,p,n);return copy;
}
static void unsupported(const char *name) { fprintf(stderr,"Unsupported field boundary: %s\n",name);abort(); }
void GF_RTC_CopyDate(RTCDate *date) { (void)date;unsupported("calendar/save-dependent map variants"); }
void sub_02066C1C(SaveVarsFlags *s,s32 f) { (void)s;(void)f;unsupported("save flags"); }
void sub_02066C4C(SaveVarsFlags *s,s32 f) { (void)s;(void)f;unsupported("save flags"); }
__attribute__((weak)) SafariZone *Save_SafariZone_Get(SaveData *s) { (void)s;unsupported("Safari Zone save");return NULL; }
__attribute__((weak)) SafariZoneAreaSet *SafariZone_GetAreaSet(SafariZone *s,s32 i) { (void)s;(void)i;unsupported("Safari Zone areas");return NULL; }
