/* Independent byte-level observations of the original SDK OAM owner. The
 * explicit fixture manager keeps attributes4..127 and affine slots1..31. */
#include "compat/native_video.h"
static u16 test_oam_half(const void *bytes,unsigned at){const u8 *p=(const u8 *)bytes+at;return p[0]|(u16)p[1]<<8;}
static void test_oam_put_half(void *bytes,unsigned at,u16 value){u8 *p=(u8 *)bytes+at;p[0]=value;p[1]=value>>8;}
static void test_oam_put_word(void *bytes,unsigned at,u32 value){u8 *p=(u8 *)bytes+at;for(unsigned i=0;i<4;i++)p[i]=value>>(8*i);}
static u32 test_oam_call(HGOriginalLoader *l,const char *name,const u32 *args,unsigned count){
    u32 result=0xdeadbeef;GF_ASSERT(hg_original_test_call(l,name,args,count,&result));return result;
}
static void probe_original_oam(HGOriginalLoader *l){
    struct OamTestScratch {u8 managers[2][0x1c],collision[3][0x1c],attributes[32],matrix[16];};
    struct OamTestScratch *scratch=Heap_Alloc(HEAP_ID_FIELD1,sizeof(*scratch));GF_ASSERT(scratch);
    memset(scratch,0xa7,sizeof(*scratch));
    u32 allocation=hg_original_map_native(l,scratch,sizeof(*scratch));GF_ASSERT(allocation);
    u32 attributes=hg_original_map_native(l,scratch->attributes,32),matrix=hg_original_map_native(l,scratch->matrix,16);
    GF_ASSERT(attributes==allocation+__builtin_offsetof(struct OamTestScratch,attributes));
    GF_ASSERT(hg_original_map_native(l,scratch->attributes+8,8)==attributes+8);
    /* Fixed numerical oracle: Q12 1, just below -1, 31/4096, and -1/4096
     * become Q8.8 256, -257, 1, -1. This distinguishes truncation/rounding. */
    const u32 inputs[]={4096,0xffffefff,31,0xffffffff};
    const u16 affine[]={0x0100,0xfeff,0x0001,0xffff};
    for(unsigned i=0;i<4;i++)test_oam_put_word(scratch->matrix,4*i,inputs[i]);
    const u16 objects[4][3]={{0x0111,0x7e12,0x9234},{0x0023,0x2a78,0x4567},
                               {0x3034,0x9156,0x8abc},{0x8045,0xcaff,0xd012}};
    for(unsigned i=0;i<4;i++){
        for(unsigned k=0;k<3;k++)test_oam_put_half(scratch->attributes,8*i+2*k,objects[i][k]);
        test_oam_put_half(scratch->attributes,8*i+6,0xbeef); /* Must not replace affine storage. */
    }
    DSVideo *video=hg_native_video();GF_ASSERT(video);
    for(unsigned engine=0;engine<2;engine++){
        u32 manager=hg_original_map_native(l,scratch->managers[engine],0x1c);
        GF_ASSERT(test_oam_call(l,"NNS_G2dGetNewOamManagerInstance",(u32[]){manager,0,4,0,1,engine},6)==1);
        const u8 *record=hg_original_test_bytes(l,manager,0x1c);GF_ASSERT(record);
        GF_ASSERT(arm_word(record,0)==engine&&test_oam_half(record,4)==0&&test_oam_half(record,6)==3&&test_oam_half(record,8)==0);
        GF_ASSERT(!test_oam_half(record,10)&&!test_oam_half(record,12)&&!test_oam_half(record,14));
        GF_ASSERT(test_oam_half(record,18)==0xa7a7&&arm_word(record,20)==0&&arm_word(record,24)==0xa7a7a7a7);
        GF_ASSERT(test_oam_call(l,"NNS_G2dGetOamManagerOamCapacity",&manager,1)==4);
        GF_ASSERT(test_oam_call(l,"NNS_G2dGetOamManagerAffineCapacity",&manager,1)==1);
        u32 buffer=test_oam_call(l,"NNS_G2dGetOamBuffer",&engine,1);
        u8 *bytes=hg_original_test_bytes(l,buffer,0x400);GF_ASSERT(bytes);
        for(unsigned i=0;i<32;i+=2)GF_ASSERT(test_oam_half(bytes,i)==0x00c0);
        u8 external[0x400-32],other_hardware[32];memcpy(external,bytes+32,sizeof(external));
        memcpy(other_hardware,video->oam[1-engine],sizeof(other_hardware));
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerAffine",(u32[]){manager,matrix},2)==0);
        for(unsigned i=0;i<4;i++)GF_ASSERT(test_oam_half(bytes,8*i+6)==affine[i]);
        u8 saved_record[0x1c],saved_buffer[32];memcpy(saved_record,record,sizeof(saved_record));memcpy(saved_buffer,bytes,32);
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerAffine",(u32[]){manager,matrix},2)==0xfffe);
        GF_ASSERT(!memcmp(saved_record,record,sizeof(saved_record))&&!memcmp(saved_buffer,bytes,32));
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerOamWithAffineIdx",(u32[]){manager,attributes,0},3)==1);
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerOamWithAffineIdx",(u32[]){manager,attributes+8,7},3)==1);
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerOam",(u32[]){manager,attributes+16,2},3)==1);
        /* The affine object's slot becomes0; a nonaffine object keeps all its
         * original attr1 bits even when supplied affine index7. */
        const u16 expected_attr1[]={0x4012,0x2a78,0x9156,0xcaff};u8 expected[32];
        for(unsigned i=0;i<4;i++){
            test_oam_put_half(expected,8*i,objects[i][0]);test_oam_put_half(expected,8*i+2,expected_attr1[i]);
            test_oam_put_half(expected,8*i+4,objects[i][2]);test_oam_put_half(expected,8*i+6,affine[i]);
        }
        GF_ASSERT(!memcmp(bytes,expected,32));
        GF_ASSERT(test_oam_call(l,"NNS_G2dGetOamManagerOamCapacity",&manager,1)==0);
        memcpy(saved_record,record,sizeof(saved_record));memcpy(saved_buffer,bytes,32);
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerOam",(u32[]){manager,attributes,1},3)==0);
        GF_ASSERT(!memcmp(saved_record,record,sizeof(saved_record))&&!memcmp(saved_buffer,bytes,32));
        test_oam_call(l,"NNS_G2dApplyAndResetOamManagerBuffer",&manager,1);
        GF_ASSERT(!memcmp(video->oam[engine],expected,32));
        for(unsigned i=0;i<4;i++)test_oam_put_half(expected,8*i,0x00c0);
        GF_ASSERT(!memcmp(bytes,expected,32)&&!test_oam_half(record,8)&&!test_oam_half(record,14));
        GF_ASSERT(!memcmp(external,bytes+32,sizeof(external))&&!memcmp(other_hardware,video->oam[1-engine],32));
        /* Reuse the same live manager after reset. Unfilled entries stay
         * invisible; old affine halves remain interleaved in their slots. */
        GF_ASSERT(test_oam_call(l,"NNS_G2dEntryOamManagerOam",(u32[]){manager,attributes+8,1},3)==1);
        memcpy(expected,scratch->attributes+8,6);
        test_oam_call(l,"NNS_G2dApplyAndResetOamManagerBuffer",&manager,1);
        GF_ASSERT(!memcmp(video->oam[engine],expected,32));
        test_oam_put_half(expected,0,0x00c0);GF_ASSERT(!memcmp(bytes,expected,32));
        GF_ASSERT(!memcmp(external,bytes+32,sizeof(external))&&!memcmp(other_hardware,video->oam[1-engine],32));
        /* An exact overlapping registration must report the existing owner. */
        u32 alias=hg_original_map_native(l,scratch->collision[0],0x1c);memset(scratch->collision[0],0x5a,0x1c);
        GF_ASSERT(test_oam_call(l,"NNS_G2dGetNewOamManagerInstance",(u32[]){alias,0,4,0,1,engine},6)==0);
        for(unsigned i=0;i<0x1c;i++)GF_ASSERT(scratch->collision[0][i]==0x5a);
        GF_ASSERT(!memcmp(external,bytes+32,sizeof(external)));
    }
    /* The original SDK commits OBJ reservations before checking affine
     * overlap. Exercise that observable failure on the independent software
     * reservation table, without admitting its unsupported 3D presentation. */
    u32 first=hg_original_map_native(l,scratch->collision[0],0x1c),partial=hg_original_map_native(l,scratch->collision[1],0x1c),retry=hg_original_map_native(l,scratch->collision[2],0x1c);
    GF_ASSERT(test_oam_call(l,"NNS_G2dGetNewOamManagerInstance",(u32[]){first,0,4,0,1,2},6)==1);
    GF_ASSERT(test_oam_call(l,"NNS_G2dGetNewOamManagerInstance",(u32[]){partial,4,4,0,1,2},6)==0);
    GF_ASSERT(test_oam_half(scratch->collision[1],4)==4&&test_oam_half(scratch->collision[1],6)==7&&test_oam_half(scratch->collision[1],8)==4);
    GF_ASSERT(test_oam_call(l,"NNS_G2dGetNewOamManagerInstance",(u32[]){retry,4,4,0,0,2},6)==0);
    Heap_Free(scratch);GF_ASSERT(!hg_original_test_bytes(l,allocation,1));
    u32 stale=allocation,result=0;GF_ASSERT(!hg_original_test_call(l,"NNS_G2dGetOamManagerOamCapacity",&stale,1,&result));
    fprintf(stderr,"Original OAM: ARM32 owners, bounded reservations, entry/exhaustion, Q8.8 affine publication/reset, reuse, alias failure, and retired storage checked\n");
}
