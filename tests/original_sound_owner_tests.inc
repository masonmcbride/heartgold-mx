/* Independent original ARM32 software-owner tests. Active driver rejection is
 * a negative test, never evidence of rendered capture or started sequences. */
static u32 sound_owner_call(HGOriginalLoader *l,const char *name,const u32 *args,unsigned count){
    u32 result=0xbadbad;GF_ASSERT(hg_original_test_call(l,name,args,count,&result));return result;
}
#define SOUND_OWNER_CALL(l,name,...) sound_owner_call(l,name,(u32[]){__VA_ARGS__},sizeof((u32[]){__VA_ARGS__})/sizeof(u32))
static void sound_owner_word(u8 *p,u32 value){for(unsigned i=0;i<4;i++)p[i]=value>>(8*i);}


static void probe_sound_owners(void){
    GF_ASSERT(hg_scheduler_init());
    HGHeapUsage before,after;GF_ASSERT(hg_heap_usage(HEAP_ID_DEFAULT,&before));
    DSAudio *audio=ds_audio_new();GF_ASSERT(audio);
    HGOriginalLoader *l=hg_original_resource_context_new();GF_ASSERT(l&&hg_original_sound_bind(l,audio));
    GF_ASSERT(!hg_original_sound_bind(l,audio));
    u8 *backing=Heap_Alloc(HEAP_ID_DEFAULT,16416);memset(backing,0x6d,16416);
    u32 base=hg_original_map_native(l,backing,16416);GF_ASSERT(base);
    u32 arena=(base+31)&~31u;
    for(unsigned repeat=0;repeat<4;repeat++){
        u32 heap=SOUND_OWNER_CALL(l,"NNS_SndHeapCreate",arena,8192);GF_ASSERT(heap==arena);
        u32 available=SOUND_OWNER_CALL(l,"NNS_SndHeapGetFreeSize",heap);
        GF_ASSERT(available>7800&&available<8192);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetCurrentLevel",heap)==0);
        GF_ASSERT(!SOUND_OWNER_CALL(l,"NNS_SndHeapAlloc",heap,8192,0,0,0));
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetFreeSize",heap)==available);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapSaveState",heap)==1);
        u32 first=SOUND_OWNER_CALL(l,"NNS_SndHeapAlloc",heap,63,0,0,0);GF_ASSERT(first&&!(first&31));
        u32 second=SOUND_OWNER_CALL(l,"NNS_SndHeapAlloc",heap,129,0,0,0);GF_ASSERT(second>=first+64&&!(second&31));
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapSaveState",heap)==2);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapAlloc",heap,96,0,0,0));
        SOUND_OWNER_CALL(l,"NNS_SndHeapLoadState",heap,1);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetCurrentLevel",heap)==1);
        /* Numeric address reuse is original heap behavior, not a new host
         * allocation identity. Never assert that retired payload stays zero. */
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapAlloc",heap,63,0,0,0)==first);
        SOUND_OWNER_CALL(l,"NNS_SndHeapClear",heap);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetFreeSize",heap)==available);
        SOUND_OWNER_CALL(l,"NNS_SndHeapDestroy",heap);
        GF_ASSERT(!SOUND_OWNER_CALL(l,"NNS_FndFindContainHeap",arena+128));
    }
    /* Exhaustion during SaveState must not consume the previous level or
     * allocation frontier, including failure to allocate the next section. */
    u32 tiny=SOUND_OWNER_CALL(l,"NNS_SndHeapCreate",arena,112);GF_ASSERT(tiny);
    u32 level=SOUND_OWNER_CALL(l,"NNS_SndHeapGetCurrentLevel",tiny);
    u32 free_before=SOUND_OWNER_CALL(l,"NNS_SndHeapGetFreeSize",tiny);
    GF_ASSERT((s32)SOUND_OWNER_CALL(l,"NNS_SndHeapSaveState",tiny)==-1);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetCurrentLevel",tiny)==level);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetFreeSize",tiny)==free_before);
    SOUND_OWNER_CALL(l,"NNS_SndHeapDestroy",tiny);
    tiny=SOUND_OWNER_CALL(l,"NNS_SndHeapCreate",arena,128);GF_ASSERT(tiny);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapSaveState",tiny)==1);
    GF_ASSERT((s32)SOUND_OWNER_CALL(l,"NNS_SndHeapSaveState",tiny)==-1);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndHeapGetCurrentLevel",tiny)==1);
    SOUND_OWNER_CALL(l,"NNS_SndHeapDestroy",tiny);
    u32 handles=arena+12288,players[16];
    for(unsigned i=0;i<16;i++){
        u32 handle=handles+4*i;SOUND_OWNER_CALL(l,"NNS_SndHandleInit",handle);
        GF_ASSERT((s32)SOUND_OWNER_CALL(l,"NNS_SndPlayerGetSeqNo",handle)==-1);
        players[i]=SOUND_OWNER_CALL(l,"NNSi_SndPlayerAllocSeqPlayer",handle,i,64);
        GF_ASSERT(players[i]&&hg_original_test_bytes(l,players[i],68));
        SOUND_OWNER_CALL(l,"NNS_SndPlayerSetSeqNo",handle,1018+i);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndPlayerGetSeqNo",handle)==1018+i);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndPlayerCountPlayingSeqByPlayerNo",i)==1);
    }
    SOUND_OWNER_CALL(l,"NNS_SndHandleInit",handles+64);
    GF_ASSERT(!SOUND_OWNER_CALL(l,"NNSi_SndPlayerAllocSeqPlayer",handles+64,16,63));
    SOUND_OWNER_CALL(l,"NNS_SndHandleReleaseSeq",handles);
    GF_ASSERT((s32)SOUND_OWNER_CALL(l,"NNS_SndPlayerGetSeqNo",handles)==-1);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndPlayerCountPlayingSeqByPlayerNo",0)==1); /* release != stop */
    for(unsigned i=0;i<16;i++){
        SOUND_OWNER_CALL(l,"NNSi_SndPlayerFreeSeqPlayer",players[i]);
        GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndPlayerCountPlayingSeqByPlayerNo",i)==0);
        GF_ASSERT((s32)SOUND_OWNER_CALL(l,"NNS_SndPlayerGetSeqNo",handles+4*i)==-1);
    }
    for(unsigned i=0;i<16;i++){
        u32 player=SOUND_OWNER_CALL(l,"NNSi_SndPlayerAllocSeqPlayer",handles,i,64);
        GF_ASSERT(player);SOUND_OWNER_CALL(l,"NNSi_SndPlayerFreeSeqPlayer",player);
    }
    GF_ASSERT(!sound_owner_call(l,"NNS_SndCaptureIsActive",NULL,0));
    SOUND_OWNER_CALL(l,"NNS_SndCaptureStopReverb",0);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNS_SndLockCapture",3));
    GF_ASSERT(!SOUND_OWNER_CALL(l,"NNS_SndLockCapture",1));
    memset(hg_original_test_bytes(l,arena,128),0x5a,128);
    GF_ASSERT(!SOUND_OWNER_CALL(l,"NNS_SndCaptureStartReverb",arena,128,1,32000,63));
    GF_ASSERT(!sound_owner_call(l,"NNS_SndCaptureIsActive",NULL,0));
    for(unsigned i=0;i<128;i++)GF_ASSERT(!((u8 *)hg_original_test_bytes(l,arena,128))[i]);
    SOUND_OWNER_CALL(l,"NNS_SndUnlockCapture",3);
    for(unsigned i=0;i<8;i++)GF_ASSERT(sound_owner_call(l,"NNS_SndAllocAlarm",NULL,0)==i);
    GF_ASSERT((s32)sound_owner_call(l,"NNS_SndAllocAlarm",NULL,0)==-1);
    SOUND_OWNER_CALL(l,"NNS_SndFreeAlarm",3);GF_ASSERT(sound_owner_call(l,"NNS_SndAllocAlarm",NULL,0)==3);
    for(unsigned i=0;i<8;i++)SOUND_OWNER_CALL(l,"NNS_SndFreeAlarm",i);
    /* Explicit active-state fixture: original fade math executes, while a
     * mandatory device command rejects and the scene transaction restores all
     * original state. This is not a working capture backend. */
    u32 cap_address=hg_original_test_symbol("hg_sound_snd_capture_sCapParam");
    u8 *cap=hg_original_test_bytes(l,cap_address,84);GF_ASSERT(cap);
    sound_owner_word(cap,1);sound_owner_word(cap+4,0);sound_owner_word(cap+60,100<<8);
    sound_owner_word(cap+64,100<<8);sound_owner_word(cap+72,1);
    SOUND_OWNER_CALL(l,"NNS_SndCaptureStopReverb",3);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNSi_SndFaderGet",cap_address+60)==100<<8);
    SOUND_OWNER_CALL(l,"NNSi_SndFaderUpdate",cap_address+60);
    GF_ASSERT(SOUND_OWNER_CALL(l,"NNSi_SndFaderGet",cap_address+60)==17067); /* C truncation */
    u8 cap_before[84];memcpy(cap_before,cap,84);
    for(unsigned retry=0;retry<3;retry++){
        u32 result=0xdeadbeef;
        GF_ASSERT(!hg_original_sound_policy(l,HG_SOUND_FIELD_SCENE,1018,&result));
        GF_ASSERT(!strcmp(hg_original_loader_error(l),"SND_StopTimer")&&result==0xdeadbeef);
        GF_ASSERT(!memcmp(cap,cap_before,84));
    }
    memset(cap,0,84);
    /* Match the real field request: publish the selected BGM and enter the
     * field-load guard before the scene transition. A bare scene request can
     * legitimately finish without reserving a sequence command. */
    GF_ASSERT(hg_original_sound_policy(l,HG_SOUND_FIELD_BGM,1018,NULL));
    GF_ASSERT(hg_original_sound_policy(l,HG_SOUND_FIELD_GUARD,1,NULL));
    u32 result;GF_ASSERT(hg_original_sound_policy(l,HG_SOUND_FIELD_SCENE,1018,&result)&&result==1);
    /* The original SDAT lookup must now succeed and return its real record. */
    GF_ASSERT(sound_owner_call(l,"SND_GetPlayerStatus",NULL,0)&1);
    u32 seq=1018,param=0;
    GF_ASSERT(hg_original_test_call(l,"NNS_SndArcGetSeqParam",&seq,1,&param));
    const u8 *rec=hg_original_test_bytes(l,param,8);
    GF_ASSERT(rec&&((u32)rec[0]|((u32)rec[1]<<8))==702);
    GF_ASSERT(rec[2]==80&&rec[3]==30&&rec[4]==64&&rec[5]==1);

    Heap_Free(backing);GF_ASSERT(!hg_original_test_mapped(l,base));hg_original_loader_delete(l);
    /* The caller's audio owner remains usable after guest records retire. */
    GF_ASSERT(ds_audio_play(audio,DS_AUDIO_BGM,1018)&&ds_audio_tick(audio));ds_audio_free(audio);
    GF_ASSERT(hg_scheduler_shutdown());
    GF_ASSERT(hg_heap_usage(HEAP_ID_DEFAULT,&after)&&before.blocks==after.blocks&&before.allocated==after.allocated);
    fprintf(stderr,"Original sound software owners: heap alignment/exhaustion/save/load/reuse, 16 slots, handle detach/free/retry, capture conflicts/fader and active-device rejection rollback checked\n");
}
#undef SOUND_OWNER_CALL
