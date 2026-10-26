#include <unistd.h>
/* Original ARM9/ARM7 protocol tests: delivery, execution and recycling differ. */
static u32 command_word(HGOriginalLoader *l,u32 at){
    const u8 *p=hg_original_test_bytes(l,at,4);GF_ASSERT(p);
    return (u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;
}
static u32 command_call(HGOriginalLoader *l,const char *name,const u32 *args,unsigned count){
    u32 result=0;int ok=hg_original_test_call(l,name,args,count,&result);
    if(!ok)fprintf(stderr,"Sound command %s rejected: %s\n",name,hg_original_loader_error(l));
    GF_ASSERT(ok);return result;
}
#define COMMAND_CALL(l,n,...) command_call(l,n,(u32[]){__VA_ARGS__},sizeof((u32[]){__VA_ARGS__})/4)
/* Independently enumerate the assembly mode switch, including its unsigned
 * bounds behavior. Populated owners are ABI fixtures, not a networking backend. */
static void probe_communication_queries(HGOriginalLoader *l){
    u32 *owner=hg_original_test_bytes(l,hg_original_test_symbol("_021D4150"),4);
    u32 *overlay=hg_original_test_bytes(l,hg_original_test_symbol("_0221A680"),4);
    GF_ASSERT(owner&&overlay&&!*owner&&!*overlay);
    GF_ASSERT(command_call(l,"sub_0203993C",NULL,0)==1);
    GF_ASSERT(!command_call(l,"sub_020378CC",NULL,0));
    u8 *bytes=Heap_Alloc(HEAP_ID_DEFAULT,0x10a0);GF_ASSERT(bytes);
    u32 context=hg_original_map_native(l,bytes,0x10a0);GF_ASSERT(context);
    GF_ASSERT(hg_original_test_bytes(l,context,0x10a0)==bytes);
    memset(bytes,0,0x10a0);*owner=context;
    for(unsigned mode=0;mode<256;mode++){
        unsigned selected=(mode>=19&&mode<=25)||mode==29||(mode>=33&&mode<=36);
        bytes[0x4e]=mode;
        GF_ASSERT(command_call(l,"sub_0203993C",NULL,0)==mode);
        GF_ASSERT(COMMAND_CALL(l,"sub_02034044",mode)==selected);
        *overlay=0;GF_ASSERT(!command_call(l,"sub_020378CC",NULL,0));
        *overlay=context;
        for(unsigned flag=0;flag<3;flag++){
            u32 value=flag==2?0xdeadbeef:flag;memcpy(bytes+0x109c,&value,4);
            GF_ASSERT(command_call(l,"ov00_021E7080",NULL,0)==value);
            GF_ASSERT(command_call(l,"sub_020378CC",NULL,0)==(selected?value:0));
            GF_ASSERT(bytes[0x4e]==mode&&*owner==context&&*overlay==context);
        }
    }
    GF_ASSERT(!COMMAND_CALL(l,"sub_02034044",UINT32_MAX));
    /* Neither inactive mode nor absent overlay may dereference an owner that
     * is not needed. Selected mode must reject an invalid guest backing. */
    bytes[0x4e]=1;*overlay=0xdead0000;
    GF_ASSERT(!command_call(l,"sub_020378CC",NULL,0));
    bytes[0x4e]=19;u32 result=0xdeadbeef;
    GF_ASSERT(!hg_original_test_call(l,"sub_020378CC",NULL,0,&result)&&result==0xdeadbeef);
    GF_ASSERT(hg_scheduler_irq()->cpu_enabled);
    *owner=0;*overlay=0;
    GF_ASSERT(!command_call(l,"sub_020378CC",NULL,0));
    Heap_Free(bytes);GF_ASSERT(!hg_original_test_mapped(l,context));
    fprintf(stderr,"Original communication queries: all 256 modes, overlay flags, absent/invalid owners verified\n");
}
static void command_store(HGOriginalLoader *l,u32 at,u32 value){
    u8 *p=hg_original_test_bytes(l,at,4);GF_ASSERT(p);
    for(unsigned i=0;i<4;i++)p[i]=value>>(8*i);
}
static u32 push_channel_command(HGOriginalLoader *l,u32 mask){
    u32 node=COMMAND_CALL(l,"SND_AllocCommand",0);GF_ASSERT(node);
    command_store(l,node+4,9);
    command_store(l,node+8,0);
    command_store(l,node+12,0xffff);
    command_store(l,node+16,mask);
    command_store(l,node+20,0);
    COMMAND_CALL(l,"SND_PushCommand",node);
    return node;
}
static void probe_original_sound_commands(void){
    GF_ASSERT(hg_scheduler_init());
    DSAudio *audio=ds_audio_new();GF_ASSERT(audio);
    HGOriginalLoader *l=hg_original_resource_context_new();GF_ASSERT(hg_original_sound_bind(l,audio));
    probe_communication_queries(l);
    GF_ASSERT(COMMAND_CALL(l,"arm7_SND_CalcTimer",0x300,0)==0x300);
    GF_ASSERT(COMMAND_CALL(l,"arm7_SND_CalcTimer",0x300,768)==0x180);
    GF_ASSERT(COMMAND_CALL(l,"arm7_SND_CalcTimer",0x300,(u32)-768)==0x600);
    GF_ASSERT(COMMAND_CALL(l,"arm7_SND_CalcTimer",0x300,7*768)==0x10);
    GF_ASSERT(COMMAND_CALL(l,"arm7_SND_CalcTimer",0x300,(u32)(-7*768))==0xffff);
    u32 work=command_call(l,"GetSoundDataPointer",NULL,0);
    u32 shared=command_word(l,hg_original_test_symbol("SNDi_SharedWork"));
    GF_ASSERT(shared&&shared==command_word(l,hg_original_test_symbol("arm7_SNDi_SharedWork")));
    /* The original bootstrap SHARED_WORK list was executed and recycled. */
    GF_ASSERT(command_word(l,shared)==1&&!command_word(l,shared+4));
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    GF_ASSERT(!command_call(l,"SND_CountWaitingCommand",NULL,0));
    u32 handle=COMMAND_CALL(l,"GF_GetSoundHandle",0);
    COMMAND_CALL(l,"Sound_SetFieldBGM",1018);
    COMMAND_CALL(l,"sub_020051A4",1018,0);
    u32 request[]={handle,UINT32_MAX,UINT32_MAX,UINT32_MAX,1018};
    GF_ASSERT(command_call(l,"NNS_SndArcPlayerStartSeqEx",request,5)==1);
    u32 head_at=hg_original_test_symbol("hg_sound_snd_command_fence_sReserveList");
    u32 first=command_word(l,head_at),second=command_word(l,first),third=command_word(l,second);
    GF_ASSERT(first&&second&&third&&!command_word(l,third));
    u32 seq_info=COMMAND_CALL(l,"NNS_SndArcGetSeqInfo",1018);
    u32 param=COMMAND_CALL(l,"NNS_SndArcGetSeqParam",1018);
    const u8 *params=hg_original_test_bytes(l,param,8);GF_ASSERT(params);
    u32 bank_no=params[0]|(u32)params[1]<<8;GF_ASSERT(bank_no==702);
    u32 bank_info=COMMAND_CALL(l,"NNS_SndArcGetBankInfo",bank_no);
    u32 bank=COMMAND_CALL(l,"NNS_SndArcGetFileAddress",command_word(l,bank_info));
    u32 seq_id=command_word(l,seq_info),length=COMMAND_CALL(l,"NNS_SndArcGetFileSize",seq_id);
    u32 offset=COMMAND_CALL(l,"NNS_SndArcGetFileOffset",seq_id);
    int fd=native_resource_file_open(native_resource_file_id("sound/gs_sound_data.sdat"));GF_ASSERT(fd>=0);
    u8 *original=malloc(length);GF_ASSERT(original&&pread(fd,original,length,offset)==length);close(fd);
    /* Resolve the unique live full SSEQ independently of the command payload.
     * Guest addresses belong to this fixture, not production constants. */
    u32 seq_file=0;
    const u8 *work_bytes=hg_original_test_bytes(l,work,0xBEC88);GF_ASSERT(work_bytes);
    for(u32 at=0;at+length<=0xBEC88;at+=4){
        if(!memcmp(work_bytes+at,original,length)){
            GF_ASSERT(!seq_file);seq_file=work+at;
        }
    }
    GF_ASSERT(seq_file&&!memcmp(original,"SSEQ",4));free(original);
    u32 seq_base=seq_file+command_word(l,seq_file+24);
    GF_ASSERT(seq_base==seq_file+28&&!memcmp(hg_original_test_bytes(l,bank,4),"SBNK",4));
    u32 sdk_player=command_word(l,handle),player_heap=command_word(l,sdk_player+8);
    u32 nested_heap=command_word(l,player_heap+8);
    GF_ASSERT(COMMAND_CALL(l,"NNS_FndFindContainHeap",seq_file)==command_word(l,nested_heap));
    fprintf(stderr,"Live original sequence 1018: base=%08x bank=%08x\n",seq_base,bank);
    GF_ASSERT(command_word(l,first+4)==2&&command_word(l,first+8)==0);
    GF_ASSERT(command_word(l,first+12)==seq_base&&command_word(l,first+16)==0&&command_word(l,first+20)==bank);
    GF_ASSERT(command_word(l,second+4)==9&&command_word(l,second+8)==0);
    GF_ASSERT(command_word(l,second+12)==0xffff&&command_word(l,second+16)==0xa7fe);
    GF_ASSERT(command_word(l,third+4)==6&&command_word(l,third+8)==0);
    GF_ASSERT(command_word(l,third+12)==4&&command_word(l,third+16)==30&&command_word(l,third+20)==1);
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==253);
    GF_ASSERT(command_call(l,"SND_CountReservedCommand",NULL,0)==3);
    GF_ASSERT(!command_call(l,"SND_CountWaitingCommand",NULL,0));
    u32 tag=command_call(l,"SND_GetCurrentCommandTag",NULL,0);GF_ASSERT(tag==2);
    GF_ASSERT(!COMMAND_CALL(l,"SND_IsFinishedCommandTag",tag));
    /* Failure injected after genuine PREPARE and channel-mask execution.
     * An unsupported hardware operation cannot publish completion. */
    command_store(l,third+4,23); /* MASTER_VOLUME -> unimplemented ARM7 device import */
    size_t image_size=hg_original_test_image_size();
    u8 *before_image=malloc(image_size),*before_work=malloc(0xBEC88);GF_ASSERT(before_image&&before_work);
    memcpy(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size);
    memcpy(before_work,work_bytes,0xBEC88);
    GF_ASSERT(ds_audio_play(audio,DS_AUDIO_BGM,1018)&&ds_audio_tick(audio));
    DSAudio *external=ds_audio_clone(audio);GF_ASSERT(external);
    for(unsigned retry=0;retry<3;retry++){
        u32 untouched=0xdeadbeef;
        GF_ASSERT(!hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&untouched));
        GF_ASSERT(!strcmp(hg_original_loader_error(l),"arm7_SND_SetMasterVolume"));
        GF_ASSERT(untouched==0xdeadbeef&&hg_scheduler_irq()->cpu_enabled&&hg_original_test_arm7_irq(l));
        GF_ASSERT(!memcmp(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size));
        GF_ASSERT(!memcmp(before_work,work_bytes,0xBEC88));
        GF_ASSERT(command_word(l,shared)==1&&!command_word(l,shared+4));
        GF_ASSERT(command_call(l,"SND_CountReservedCommand",NULL,0)==3);
        GF_ASSERT(!command_call(l,"SND_CountWaitingCommand",NULL,0));
        GF_ASSERT(ds_audio_tick(audio)&&ds_audio_tick(external));
        GF_ASSERT(!memcmp(ds_audio_pcm(audio),ds_audio_pcm(external),DS_AUDIO_FRAMES*4));
    }
    free(before_work);free(before_image);
    command_store(l,third+4,6); /* restore the injected fixture command */
    /* Submission is distinct from dispatch and from ARM9 recycling. */
    GF_ASSERT(COMMAND_CALL(l,"SND_FlushCommand",0));
    GF_ASSERT(!command_call(l,"SND_CountReservedCommand",NULL,0));
    GF_ASSERT(command_call(l,"SND_CountWaitingCommand",NULL,0)==3);
    GF_ASSERT(command_word(l,shared)==1&&!command_word(l,shared+4));
    GF_ASSERT(!COMMAND_CALL(l,"SND_RecvCommandReply",0));
    command_call(l,"arm7_SND_CommandProc",NULL,0);
    GF_ASSERT(command_word(l,shared)==tag&&command_word(l,shared+4)==1);
    GF_ASSERT(!COMMAND_CALL(l,"SND_IsFinishedCommandTag",tag));
    GF_ASSERT(command_call(l,"SND_CountWaitingCommand",NULL,0)==3);
    GF_ASSERT(COMMAND_CALL(l,"SND_RecvCommandReply",0)==first);
    GF_ASSERT(COMMAND_CALL(l,"SND_IsFinishedCommandTag",tag));
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    GF_ASSERT(!command_call(l,"SND_CountWaitingCommand",NULL,0));
    u32 driver=hg_original_test_symbol("arm7_SNDi_Work"),player=driver+1344;
    const u8 *p=hg_original_test_bytes(l,player,36);GF_ASSERT(p);
    GF_ASSERT((p[0]&7)==1&&p[1]==0&&p[4]==30&&p[5]==127);
    GF_ASSERT(command_word(l,player+32)==bank);
    u32 track=driver+1920+64*p[8];
    GF_ASSERT(command_word(l,track+36)==seq_base);
    const u8 *seq=hg_original_test_bytes(l,seq_base,16);GF_ASSERT(seq);
    unsigned expected=seq[0]==0xfe?(seq[1]|(unsigned)seq[2]<<8):1,tracks=0;
    for(unsigned i=0;i<16;i++){
        GF_ASSERT((p[8+i]!=0xff)==(i==0||((expected>>i)&1)));
        if(p[8+i]==0xff)continue;
        tracks++;
        const u8 *t=hg_original_test_bytes(l,driver+1920+64*p[8+i],64);GF_ASSERT(t);
        GF_ASSERT((t[0]&0xc3)==0xc3&&t[4]==127&&t[5]==127&&t[18]==64);
        GF_ASSERT(t[30]==0xfe&&t[31]==0xa7);
    }
    GF_ASSERT(tracks&&command_word(l,track+40)==seq_base+(seq[0]==0xfe?3:0));
    for(unsigned i=0;i<16;i++){
        const u8 *v=hg_original_test_bytes(l,shared+32+2*i,2);GF_ASSERT(v&&v[0]==255&&v[1]==255);
    }
    GF_ASSERT(!command_word(l,shared+64));
    /* Reprepare an active player through the original teardown path. */
    for(unsigned repeat=0;repeat<3;repeat++){
        COMMAND_CALL(l,"SND_PrepareSeq",0,seq_base,0,bank);
        u32 out=0;GF_ASSERT(hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&out));
        GF_ASSERT((p[0]&7)==1&&p[4]==64&&p[8]==0);
        GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    }
    /* All 256 nodes are owned once. Blocking allocation must execute, reply,
     * and recycle before handing a node back to the producer. */
    for(unsigned i=0;i<256;i++)push_channel_command(l,i==255?0xa7fe:0x3fff);
    GF_ASSERT(!COMMAND_CALL(l,"SND_AllocCommand",0));
    GF_ASSERT(command_call(l,"SND_CountReservedCommand",NULL,0)==256);
    u32 allocated=COMMAND_CALL(l,"SND_AllocCommand",1);GF_ASSERT(allocated);
    GF_ASSERT(!command_call(l,"SND_CountReservedCommand",NULL,0));
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==255);
    GF_ASSERT(((const u8 *)hg_original_test_bytes(l,track+30,2))[0]==0xfe);
    command_store(l,allocated+4,9);command_store(l,allocated+8,0);
    command_store(l,allocated+12,0xffff);command_store(l,allocated+16,0xa7fe);
    COMMAND_CALL(l,"SND_PushCommand",allocated);
    u32 out;GF_ASSERT(hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&out));
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    /* Eight outstanding linked lists apply actual SDK backpressure. */
    u32 epoch=command_word(l,shared);
    for(unsigned i=0;i<8;i++){
        push_channel_command(l,0xa7fe);
        GF_ASSERT(COMMAND_CALL(l,"SND_FlushCommand",0));
    }
    push_channel_command(l,0x3fff);
    GF_ASSERT(!COMMAND_CALL(l,"SND_FlushCommand",0));
    GF_ASSERT(command_word(l,shared)==epoch);
    GF_ASSERT(command_call(l,"SND_CountWaitingCommand",NULL,0)==8);
    GF_ASSERT(command_call(l,"SND_CountReservedCommand",NULL,0)==1);
    GF_ASSERT(!COMMAND_CALL(l,"SND_RecvCommandReply",0));
    command_call(l,"arm7_SND_CommandProc",NULL,0);
    GF_ASSERT(command_word(l,shared)==epoch+8);
    for(unsigned i=0;i<8;i++)GF_ASSERT(COMMAND_CALL(l,"SND_RecvCommandReply",0));
    GF_ASSERT(!COMMAND_CALL(l,"SND_RecvCommandReply",0));
    tag=command_call(l,"SND_GetCurrentCommandTag",NULL,0);
    GF_ASSERT(COMMAND_CALL(l,"SND_FlushCommand",0));
    COMMAND_CALL(l,"SND_WaitForCommandProc",tag);
    GF_ASSERT(COMMAND_CALL(l,"SND_IsFinishedCommandTag",tag));
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    /* Transactional admission must execute a ninth reserved list, rather
     * than commit an unflushed request after draining the first eight. */
    for(unsigned i=0;i<8;i++){
        push_channel_command(l,0xa7fe);
        GF_ASSERT(COMMAND_CALL(l,"SND_FlushCommand",0));
    }
    push_channel_command(l,0x3fff);
    epoch=command_word(l,shared);
    GF_ASSERT(hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&out));
    GF_ASSERT(command_word(l,shared)==epoch+9);
    GF_ASSERT(command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    GF_ASSERT(!command_call(l,"SND_CountReservedCommand",NULL,0));
    const u8 *mask=hg_original_test_bytes(l,track+30,2);GF_ASSERT(mask&&mask[0]==0xff&&mask[1]==0x3f);
    COMMAND_CALL(l,"SND_StartPreparedSeq",0);
    GF_ASSERT(hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&out));
    GF_ASSERT((p[0]&3)==3); /* original START_PREPARED changes the prepared flag */
    GF_ASSERT(ds_audio_tick(audio)&&ds_audio_tick(external));
    GF_ASSERT(!memcmp(ds_audio_pcm(audio),ds_audio_pcm(external),DS_AUDIO_FRAMES*4));
    /* A future epoch with no list cannot invent a completion or busy-wait. */
    u32 future=command_word(l,shared)+1,untouched=0xdeadbeef;
    GF_ASSERT(!hg_original_test_sound_call(l,"SND_WaitForCommandProc",&future,1,&untouched));
    GF_ASSERT(!strcmp(hg_original_loader_error(l),"sound wait has no pending ARM7 commands"));
    GF_ASSERT(untouched==0xdeadbeef&&hg_scheduler_irq()->cpu_enabled&&hg_original_test_arm7_irq(l));
    GF_ASSERT(!COMMAND_CALL(l,"SND_IsFinishedCommandTag",future));
    /* Original periodic parsing/instrument lookup can create a real software
     * voice. Hardware publication remains a separate, rejecting operation. */
    int voiced=0;
    for(unsigned tick=0;tick<32&&!voiced;tick++){
        u32 periodic=1;
        GF_ASSERT(hg_original_test_sound_call(l,"arm7_SND_SeqMain",&periodic,1,NULL));
        for(unsigned channel=0;channel<16;channel++){
            const u8 *ch=hg_original_test_bytes(l,driver+84*channel,84);GF_ASSERT(ch);
            if(!(ch[3]&1))continue;
            voiced=1;
            GF_ASSERT(ch[3]&2); /* original note start, not hardware playback */
            u32 data=command_word(l,driver+84*channel+68);
            GF_ASSERT(data>=work&&data<work+0xBEC88&&hg_original_test_bytes(l,data,4));
        }
    }
    GF_ASSERT(voiced&&command_word(l,shared+64)>0);
    u32 periodic=1;
    GF_ASSERT(hg_original_test_sound_call(l,"arm7_SND_ExChannelMain",&periodic,1,NULL));
    before_image=malloc(image_size);before_work=malloc(0xBEC88);GF_ASSERT(before_image&&before_work);
    memcpy(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size);
    memcpy(before_work,work_bytes,0xBEC88);
    for(unsigned retry=0;retry<3;retry++){
        GF_ASSERT(!hg_original_test_sound_call(l,"arm7_SND_UpdateExChannel",NULL,0,NULL));
        GF_ASSERT(!strcmp(hg_original_loader_error(l),"arm7_SND_StopChannel"));
        GF_ASSERT(!memcmp(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size));
        GF_ASSERT(!memcmp(before_work,work_bytes,0xBEC88));
        GF_ASSERT(hg_scheduler_irq()->cpu_enabled&&hg_original_test_arm7_irq(l));
        GF_ASSERT(ds_audio_tick(audio)&&ds_audio_tick(external));
        GF_ASSERT(!memcmp(ds_audio_pcm(audio),ds_audio_pcm(external),DS_AUDIO_FRAMES*4));
    }
    free(before_image);free(before_work);
    /* Original track-allocation parsing: fill all 32 tracks, observe original
     * allocation failure, free/reuse tracks and clear shared player status. */
    COMMAND_CALL(l,"arm7_SND_StopSeq",0);
    GF_ASSERT(!command_word(l,shared+4));
    u8 *fixture=Heap_Alloc(HEAP_ID_DEFAULT,32);GF_ASSERT(fixture);
    memset(fixture,0xff,32);fixture[0]=0xfe;
    u32 fixture_address=hg_original_map_native(l,fixture,32);GF_ASSERT(fixture_address);
    COMMAND_CALL(l,"arm7_SND_PrepareSeq",0,fixture_address,0,bank);
    COMMAND_CALL(l,"arm7_SND_PrepareSeq",1,fixture_address,0,bank);
    COMMAND_CALL(l,"arm7_SND_PrepareSeq",2,fixture_address,0,bank);
    GF_ASSERT(command_word(l,shared+4)==3);
    const u8 *players=hg_original_test_bytes(l,driver+1344,16*36);GF_ASSERT(players);
    GF_ASSERT(!(players[2*36]&1));
    for(unsigned i=0;i<16;i++){
        GF_ASSERT(players[8+i]==i&&players[36+8+i]==i+16);
    }
    COMMAND_CALL(l,"arm7_SND_StopSeq",0);
    COMMAND_CALL(l,"arm7_SND_PrepareSeq",2,fixture_address,0,bank);
    GF_ASSERT(command_word(l,shared+4)==6&&players[2*36+8]==0);
    COMMAND_CALL(l,"arm7_SND_StopSeq",1);COMMAND_CALL(l,"arm7_SND_StopSeq",2);
    GF_ASSERT(!command_word(l,shared+4));
    Heap_Free(fixture);GF_ASSERT(!hg_original_test_mapped(l,fixture_address));
    GF_ASSERT(ds_audio_tick(audio)&&ds_audio_tick(external));
    GF_ASSERT(!memcmp(ds_audio_pcm(audio),ds_audio_pcm(external),DS_AUDIO_FRAMES*4));
    /* Full RTOS driver startup is not silently replaced by software staging.
     * Even its init guard and reset queues must roll back on an OS boundary. */
    before_image=malloc(image_size);before_work=malloc(0xBEC88);GF_ASSERT(before_image&&before_work);
    memcpy(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size);
    memcpy(before_work,work_bytes,0xBEC88);
    for(unsigned retry=0;retry<3;retry++){
        u32 priority=16;
        GF_ASSERT(!hg_original_test_sound_call(l,"arm7_SND_Init",&priority,1,NULL));
        GF_ASSERT(!strcmp(hg_original_loader_error(l),"OS_CreateThread"));
        GF_ASSERT(!memcmp(before_image,hg_original_test_bytes(l,0x02000000,image_size),image_size));
        GF_ASSERT(!memcmp(before_work,work_bytes,0xBEC88));
        GF_ASSERT(hg_scheduler_irq()->cpu_enabled&&hg_original_test_arm7_irq(l));
    }
    free(before_image);free(before_work);
    COMMAND_CALL(l,"SND_PrepareSeq",0,seq_base,0,bank);
    GF_ASSERT(hg_original_test_sound_call(l,"SND_GetCurrentCommandTag",NULL,0,&out));
    GF_ASSERT(command_word(l,shared+4)==1&&command_call(l,"SND_CountFreeCommand",NULL,0)==256);
    COMMAND_CALL(l,"arm7_SND_StopSeq",0);
    ds_audio_free(external);
    hg_original_loader_delete(l);ds_audio_free(audio);GF_ASSERT(hg_scheduler_shutdown());
    fprintf(stderr,"Original ARM7: live PREPARE/START_PREPARED, ordered masks/priority, shared work, 256 nodes, eight-list backpressure, completion/recycling, full failure rollback and retry verified\n");
}
#undef COMMAND_CALL
