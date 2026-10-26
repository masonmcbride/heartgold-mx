/* Reuse the complete contract suites, with fresh mutable owners on each pass. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "compat/save_manager.h"
#include "compat/stats.h"
int hg_micro_platform(void),hg_micro_lifetimes(void),hg_micro_scheduler(void);
u16 MATH_CalcCRC16CCITT(const MATHCRC16Table *,const void *,u32);
void MATH_CRC16CCITTInitTable(MATHCRC16Table *);
static u16 reference(const u8 *p,unsigned n){u16 c=0xffff;while(n--){c^=(u16)*p++<<8;for(unsigned b=0;b<8;b++)c=(u16)((c<<1)^((c&0x8000)?0x1021:0));}return c;}
static int crc_check(void){
    MATHCRC16Table table;MATH_CRC16CCITTInitTable(&table);u8 data[4096];u32 state=123;
    for(unsigned i=0;i<sizeof(data);i++){state=state*1664525u+1013904223u;data[i]=(u8)(state>>24);}
    for(unsigned n=0;n<=sizeof(data);n=n<256?n+1:n+127){
        u16 expected=reference(data,n);if(MATH_CalcCRC16CCITT(&table,data,n)!=expected||MATH_CalcCRC16CCITT(NULL,data,n)!=expected)return 1;
    }
    for(unsigned byte=0;byte<256;byte++){u8 one=(u8)byte;u16 expected=reference(&one,1);if(MATH_CalcCRC16CCITT(&table,&one,1)!=expected||MATH_CalcCRC16CCITT(NULL,&one,1)!=expected)return 1;}
    if(MATH_CalcCRC16CCITT(&table,"123456789",9)!=0x29b1)return 1;
    return 0;
}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){
    const struct{const char *name;int (*run)(void);} checks[]={{"memory",hg_micro_platform},{"allocator_keys_teardown",hg_micro_lifetimes},{"original_callbacks",hg_micro_scheduler},{"crc_equivalence",crc_check}};
    for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<sizeof(checks)/sizeof(*checks);i++){
        double start=now();if(checks[i].run())return 1;
        fprintf(stderr,"HG_TIMING {\"phase\":\"%s\",\"pass\":%u,\"seconds\":%.9f}\n",checks[i].name,pass,now()-start);
    }
    return 0;
}
