#include <fcntl.h>
#include <unistd.h>
static u32 file_test_call(HGOriginalLoader *l,const char *name,u32 a,u32 b,u32 c){
    u32 args[]={a,b,c},result=0xdeadbeef;
    int status=hg_original_test_file_import(l,name,args,3,&result);
    if(status!=1)fprintf(stderr,"File import %s failed: %s (%08x)\n",name,hg_original_loader_error(l),a);
    GF_ASSERT(status==1);return result;
}
static void probe_original_files(void){
    int external=open("/dev/null",O_RDONLY);GF_ASSERT(external>=0);
    int available=open("/dev/null",O_RDONLY);GF_ASSERT(available>=0);close(available);
    for(unsigned repeat=0;repeat<3;repeat++){
        HGOriginalLoader *l=hg_original_resource_context_new();
        u8 *storage=Heap_Alloc(HEAP_ID_DEFAULT,2048);u32 g=hg_original_map_native(l,storage,2048);GF_ASSERT(g);
        strcpy((char *)storage+640,"data/sound/gs_sound_data.sdat");
        GF_ASSERT(file_test_call(l,"FS_ConvertPathToFileID",g+768,g+640,0));
        u32 archive,id;memcpy(&archive,storage+768,4);memcpy(&id,storage+772,4);
        GF_ASSERT(file_test_call(l,"FS_ConvertPathToFileID",g+776,g+640,0));
        GF_ASSERT(!memcmp(storage+768,storage+776,8));
        file_test_call(l,"FS_InitFile",g,0,0);file_test_call(l,"FS_InitFile",g+72,0,0);
        GF_ASSERT(!file_test_call(l,"FS_OpenFileFast",g,archive^1,id));
        GF_ASSERT(!file_test_call(l,"FS_OpenFileFast",g,archive,UINT32_MAX));
        GF_ASSERT(file_test_call(l,"FS_OpenFileFast",g,archive,id));
        GF_ASSERT(file_test_call(l,"FS_OpenFile",g+72,g+640,0));
        u32 size=file_test_call(l,"FS_GetLength",g,0,0);GF_ASSERT(size>1024*1024);
        u32 bad[]={g,g+2047,16};GF_ASSERT(hg_original_test_file_import(l,"FS_ReadFile",bad,3,NULL)==-1);
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,16)==16&&!memcmp(storage+800,"SDAT",4));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g+72,g+832,16)==16&&!memcmp(storage+800,storage+832,16));
        u32 args[]={g},untouched=0xdeadbeef;
        GF_ASSERT(hg_original_test_file_import(l,"FS_InitFile",args,1,&untouched)==-1&&untouched==0xdeadbeef);
        GF_ASSERT(file_test_call(l,"FS_SeekFile",g,(u32)-4,2));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,16)==4);
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,16)==0);
        GF_ASSERT(file_test_call(l,"FS_SeekFile",g,INT32_MAX,0));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,16)==0);
        GF_ASSERT(file_test_call(l,"FS_SeekFile",g,INT32_MAX,1)); /* ARM32 overflow then clamp */
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,4)==4&&!memcmp(storage+800,"SDAT",4));
        GF_ASSERT(file_test_call(l,"FS_SeekFile",g,(u32)INT32_MIN,1));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,4)==4&&!memcmp(storage+800,"SDAT",4));
        GF_ASSERT(!file_test_call(l,"FS_SeekFile",g,0,3));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,UINT32_MAX)==UINT32_MAX);
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,4)==4&&memcmp(storage+800,"SDAT",4));
        strcpy((char *)storage+640,"data/../gs_sound_data.sdat");memset(storage+776,0x6d,8);
        GF_ASSERT(!file_test_call(l,"FS_ConvertPathToFileID",g+776,g+640,0));
        for(unsigned i=776;i<784;i++)GF_ASSERT(storage[i]==0x6d);
        GF_ASSERT(file_test_call(l,"FS_CloseFile",g,0,0));
        GF_ASSERT(file_test_call(l,"FS_OpenFileFast",g,archive,id));
        GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,4)==4&&!memcmp(storage+800,"SDAT",4));
        for(unsigned i=2;i<8;i++){
            file_test_call(l,"FS_InitFile",g+72*i,0,0);
            GF_ASSERT(file_test_call(l,"FS_OpenFileFast",g+72*i,archive,id));
        }
        args[0]=g+72*8;GF_ASSERT(hg_original_test_file_import(l,"FS_InitFile",args,1,NULL)==-1);
        /* Check both old path families against the unchanged resource cache. */
        GF_ASSERT(file_test_call(l,"FS_CloseFile",g,0,0));
        const char *paths[]={"data/area00light.txt","fielddata/build_model/bm_field_matshp.dat"};
        const char *names[]={"area00light.txt","bm_field_matshp.dat"};
        for(unsigned i=0;i<2;i++){
            const u8 *bytes;size_t length;GF_ASSERT(native_resource_file(names[i],&bytes,&length));
            strcpy((char *)storage+640,paths[i]);GF_ASSERT(file_test_call(l,"FS_OpenFile",g,g+640,0));
            GF_ASSERT(file_test_call(l,"FS_GetLength",g,0,0)==length);
            GF_ASSERT(file_test_call(l,"FS_ReadFile",g,g+800,32)==32&&!memcmp(storage+800,bytes,32));
            GF_ASSERT(file_test_call(l,"FS_CloseFile",g,0,0));
        }
        Heap_Free(storage);u32 stale[]={g+72};
        GF_ASSERT(hg_original_test_file_import(l,"FS_GetLength",stale,1,NULL)==-1);
        hg_original_loader_delete(l); /* seven live descriptors close */
        GF_ASSERT(fcntl(external,F_GETFD)>=0);
        int next=open("/dev/null",O_RDONLY);GF_ASSERT(next==available);close(next);
    }
    close(external);fprintf(stderr,"Original loader files: IDs/fast-open, independent cursors, clamped seeks/EOF, bounds/failures, capacity, cached resource bytes and descriptor teardown checked\n");
}
