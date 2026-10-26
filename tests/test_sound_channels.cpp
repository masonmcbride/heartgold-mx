#include "ds_audio.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <cstring>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
namespace {
#include "original_sound_channels.inc"
void dropped(SNDExChannel *,SNDExChannelCallbackStatus status,void *user){CHECK(status==SND_EX_CHANNEL_CALLBACK_DROP);++*static_cast<unsigned *>(user);}
}
int main(){
    SoundChannels pool;for(unsigned i=0;i<16;i++)pool.channel[i].myNo=i;
    unsigned drops=0,stops=0;pool.host=&stops;pool.stop=[](void *p,unsigned){++*static_cast<unsigned *>(p);};
    const unsigned order[]={4,5,6,7,2,0,3,1,8,9,10,11,14,12,15,13};
    for(unsigned i:order){auto *c=pool.SND_AllocExChannel(0xffff,100,false,dropped,&drops);CHECK(c&&c->myNo==i);}
    CHECK(!drops&&!pool.SND_AllocExChannel(0xffff,99,false,dropped,&drops));
    pool.channel[3].volume=0x37f; // divider 16: quieter than all undivided channels
    CHECK(pool.SND_AllocExChannel(0xffff,100,false,dropped,&drops)->myNo==3&&drops==1);
    auto *c=pool.SND_AllocExChannel(1u<<15,101,false,dropped,&drops);CHECK(c&&c->myNo==15);
    pool.SND_FreeExChannel(c);CHECK(!c->callback&&c->prio==101); // detach is not stop
    pool.SND_LockChannel(1u<<4,SND_LOCK_IMPLIED_ALLOC_CHANNEL);
    CHECK(!pool.SND_AllocExChannel(1u<<4,255,false,dropped,&drops));
    CHECK(pool.SND_AllocExChannel(1u<<4,255,true,dropped,&drops));
    pool.SND_LockChannel(1u<<4,0);
    CHECK(!pool.SND_AllocExChannel(1u<<4,255,true,dropped,&drops));
    pool.SND_UnlockChannel(1u<<4,0);pool.SND_UnlockChannel(1u<<4,SND_LOCK_IMPLIED_ALLOC_CHANNEL);
    CHECK(pool.SND_AllocExChannel(1u<<4,255,false,dropped,&drops)&&stops==2);
    CHECK(!pool.SND_AllocExChannel(0,255,true,dropped,&drops));

    DSAudio *a=ds_audio_new();CHECK(a);
    DSAudioSequenceInfo info{};CHECK(ds_audio_sequence_info(a,1018,&info));
    CHECK(info.player==1&&info.channel_priority==30&&info.player_priority==64&&info.volume==80);
    CHECK(ds_audio_player_channels(a,1)==0xa7fe&&ds_audio_player_channels(a,4)==0xd800);
    CHECK(ds_audio_sequence_info(a,1500,&info)&&info.player==4&&info.channel_priority==104);
    CHECK(!ds_audio_sequence_info(a,999999,&info));
    CHECK(!ds_audio_set_player_channels(a,32,1)&&!ds_audio_set_player_channels(a,1,0x10000));
    CHECK(ds_audio_set_player_channels(a,1,1u<<4)&&ds_audio_player_channels(a,1)==(1u<<4));
    CHECK(ds_audio_play(a,DS_AUDIO_BGM,1018));unsigned used=0,energy=0;
    for(unsigned t=0;t<30;t++){
        CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));used|=ds_audio_active_channels(a,1018);
        for(unsigned i=0;i<DS_AUDIO_FRAMES*2;i++)energy+=ds_audio_pcm(a)[i]!=0;
    }
    CHECK(used==(1u<<4)&&energy);
    CHECK(ds_audio_set_player_channels(a,1,1u<<5)); // active sequence retains start mask
    for(unsigned t=0;t<10;t++){CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));CHECK(!(ds_audio_active_channels(a,1018)&~(1u<<4)));}
    CHECK(ds_audio_lock_channels(a,1u<<4,0));CHECK(!ds_audio_active_channels(a,1018));
    CHECK(!ds_audio_lock_channels(a,1u<<4,0)&&!ds_audio_unlock_channels(a,1u<<5,0));
    for(unsigned t=0;t<5;t++){CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));CHECK(!ds_audio_active_channels(a,1018));}
    CHECK(ds_audio_unlock_channels(a,1u<<4,0));CHECK(!ds_audio_unlock_channels(a,1u<<4,0));
    ds_audio_stop(a,DS_AUDIO_BGM,0);CHECK(!ds_audio_active_channels(a,1018));
    CHECK(ds_audio_play(a,DS_AUDIO_BGM,1018));used=0;
    for(unsigned t=0;t<30;t++){CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));used|=ds_audio_active_channels(a,1018);}
    CHECK(used==(1u<<5));
    DSAudio *copy=ds_audio_clone(a);CHECK(copy);
    for(unsigned t=0;t<8;t++){CHECK(ds_audio_tick(a)&&ds_audio_tick(copy));CHECK(!std::memcmp(ds_audio_pcm(a),ds_audio_pcm(copy),DS_AUDIO_FRAMES*4));}
    CHECK(ds_audio_acquire(copy));CHECK(!ds_audio_restore_snapshot(a,copy));ds_audio_release(copy);
    CHECK(!ds_audio_restore_snapshot(a,a));
    // An effect and BGM compete for the SAME physical channel, never 16 each.
    CHECK(ds_audio_set_player_channels(a,4,1u<<5));CHECK(ds_audio_play(a,DS_AUDIO_EFFECT,1500));
    unsigned effect_seen=0;
    for(unsigned t=0;t<10;t++){
        CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));unsigned bgm=ds_audio_active_channels(a,1018),effect=ds_audio_active_channels(a,1500);
        effect_seen|=effect;CHECK(!(bgm&effect)&&!((bgm|effect)&~(1u<<5)));
    }
    CHECK(effect_seen==(1u<<5));
    DSAudio *snapshot=ds_audio_clone(a),*expected=ds_audio_clone(a);CHECK(snapshot&&expected&&ds_audio_acquire(a));
    CHECK(ds_audio_set_player_channels(a,1,0xa7fe)&&ds_audio_lock_channels(a,0xffff,0));
    CHECK(ds_audio_restore_snapshot(a,snapshot));ds_audio_free(snapshot);
    for(unsigned i=0;i<8;i++){CHECK(ds_audio_tick(a)&&ds_audio_tick(expected));CHECK(!std::memcmp(ds_audio_pcm(a),ds_audio_pcm(expected),DS_AUDIO_FRAMES*4));}
    CHECK(ds_audio_player_channels(a,1)==(1u<<5));ds_audio_release(a);ds_audio_free(expected);
    ds_audio_stop(a,DS_AUDIO_EFFECT,0);ds_audio_stop(a,DS_AUDIO_BGM,0);
    CHECK(!ds_audio_active_channels(a,1018)&&!ds_audio_active_channels(a,1500));
    CHECK(ds_audio_set_player_channels(a,1,0));CHECK(ds_audio_player_channels(a,1)==0);
    CHECK(ds_audio_lock_channels(a,0xffff,1)&&ds_audio_play(a,DS_AUDIO_BGM,1018));
    for(unsigned i=0;i<8;i++){CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));CHECK(!ds_audio_active_channels(a,1018));}
    ds_audio_stop(a,DS_AUDIO_BGM,0);CHECK(ds_audio_set_player_channels(a,1,1u<<4));
    CHECK(ds_audio_play(a,DS_AUDIO_BGM,1018));used=0;
    for(unsigned i=0;i<8;i++){CHECK(ds_audio_tick(a)&&ds_audio_channel_state_valid(a));used|=ds_audio_active_channels(a,1018);}
    CHECK(used==(1u<<4));CHECK(ds_audio_unlock_channels(a,0xffff,1));
    ds_audio_free(copy);ds_audio_free(a);
    std::puts("Original shared sound allocator: order, priority/volume, weak/hard locks, drop/detach, future masks, actual PCM, conflicts, clone, stop/retry checked");
}
