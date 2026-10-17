#include <nitro/types.h>
#include "struct_defs/personal_narc_structs.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "DS byte width");
_Static_assert(sizeof(u8)==1 && sizeof(s8)==1, "8-bit scalars");
_Static_assert(sizeof(u16)==2 && sizeof(s16)==2, "16-bit scalars");
_Static_assert(sizeof(u32)==4 && sizeof(s32)==4, "32-bit scalars on LP64");
_Static_assert(sizeof(u64)==8 && sizeof(s64)==8, "64-bit scalars");
_Static_assert(sizeof(BOOL)==4, "Game BOOL is not Objective-C BOOL");
_Static_assert(sizeof(f32)==4 && sizeof(f64)==8, "Floating scalar widths");
_Static_assert((s8)-1<0 && (s16)-1<0 && (s32)-1<0 && (s64)-1<0, "Signed scalars");
_Static_assert((u8)-1==UINT8_MAX && (u16)-1==UINT16_MAX && (u32)-1==UINT32_MAX && (u64)-1==UINT64_MAX, "Unsigned scalars");
_Static_assert(_Generic((vu32 *)0, volatile u32 *:1, default:0), "Volatile alias qualification");
_Static_assert(_Generic((vs64 *)0, volatile s64 *:1, default:0), "Signed volatile alias qualification");
_Static_assert(sizeof(struct BaseStats)==0x2c, "Original personal-data record width");
_Static_assert(offsetof(struct BaseStats,item1)==0x0c, "Original item field offset");
_Static_assert(offsetof(struct BaseStats,tmhm_1)==0x1c, "Original TM/HM field offset");
_Static_assert(sizeof(struct Evolution)==6, "Original evolution-data record width");

#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
int main(void) {
    CHECK(sizeof(void *)==8);CHECK(TRUE==1 && FALSE==0);
    u16 endian=1;CHECK(*(const u8 *)&endian==1);
    struct BaseStats stats={0};stats.hp_yield=3;stats.atk_yield=2;stats.def_yield=1;stats.speed_yield=0;stats.spatk_yield=2;stats.spdef_yield=3;
    u16 packed=0;memcpy(&packed,(const u8 *)&stats+0x0a,sizeof(packed));
    CHECK(packed==(3u|(2u<<2)|(1u<<4)|(2u<<8)|(3u<<10)));
    vu32 word=UINT32_MAX;vs64 signed_word=INT64_MIN;
    CHECK(word==UINT32_MAX && signed_word==INT64_MIN);
    puts("Standalone DS scalar interface and original personal/evolution data ABI passed");
    return 0;
}
