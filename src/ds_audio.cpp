#include "ds_audio.h"
#include "Player.h"
#include "SDAT.h"
#include "INFOSection.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <cstdio>
#include <cassert>
#include <functional>
#include <cstdlib>
#include <type_traits>

namespace {
thread_local uint32_t *sound_rng;
thread_local const uint8_t *sequence_begin,*sequence_end;
uint64_t epoch;
#include "original_sound_channels.inc"
struct Archive {
    std::vector<uint8_t> bytes;
    std::array<uint16_t,32> player_masks{};
    std::map<uint64_t,std::shared_ptr<SDAT>> sequences;
    Archive() {
        std::ifstream input(HG_SOUND_ARCHIVE,std::ios::binary);
        if(!input)throw std::runtime_error("Cannot open original sound archive");
        bytes.assign(std::istreambuf_iterator<char>(input),{});
        if(bytes.size()<64||bytes.size()>32*1024*1024)throw std::runtime_error("Invalid sound archive size");
        PseudoFile file;file.data=&bytes;file.pos=0x18;unsigned infoOffset=file.ReadLE<uint32_t>();
        file.pos=infoOffset;INFOSection info;info.Read(file);
        for(const auto &entry:info.PLAYERrecord.entries){
            if(entry.first>=32||entry.second.seqMax>16)throw std::runtime_error("Invalid SDAT player record");
            player_masks[entry.first]=entry.second.allocChBitFlag;
        }
    }
    std::shared_ptr<SDAT> get(unsigned id,unsigned bank=UINT32_MAX) {
        uint64_t key=(uint64_t(bank)<<32)|id;
        auto found=sequences.find(key);if(found!=sequences.end())return found->second;
        PseudoFile file;file.data=&bytes;auto result=std::make_shared<SDAT>(file,id,bank);
        sequences.emplace(key,result);return result;
    }
};
std::shared_ptr<Archive> archive() { static auto cache=std::make_shared<Archive>();return cache; }
struct Voice {
    DSAudio *audio=nullptr;std::shared_ptr<SDAT> asset;Player player;unsigned id=0,player_no=0;uint32_t rng=1;
    Voice()=default;
    Voice(const Voice &other):asset(other.asset),player(other.player),id(other.id),player_no(other.player_no),rng(other.rng) {
        for(auto &track:player.tracks)track.ply=&player;
        for(auto &channel:player.channels)channel.ply=&player;
    }
    bool active()const {
        if(!asset)return false;
        for(unsigned i=0;i<player.nTracks;i++)if(!player.tracks[player.trackIds[i]].state[TS_END])return true;
        for(const auto &channel:player.channels)if(channel.state!=CS_NONE)return true;
        return false;
    }
};
}
void sseq_check(const uint8_t *bytes,size_t count) {
    uintptr_t at=(uintptr_t)bytes,begin=(uintptr_t)sequence_begin,end=(uintptr_t)sequence_end;
    if(at<begin || at>end || count>end-at)throw std::runtime_error("SSEQ read outside sequence");
}
uint32_t sseq_random() { if(!sound_rng)return 0;*sound_rng=*sound_rng*1664525u+1013904223u;return (*sound_rng>>1)&0x7fffffffu; }
struct DSAudio {
    std::shared_ptr<Archive> archive;
    std::array<std::unique_ptr<Voice>,10> voices;
    std::unique_ptr<Voice> saved_bgm;float saved_volume=1;
    std::array<int16_t,DS_AUDIO_FRAMES*2> pcm{};
    std::vector<uint8_t> scratch=std::vector<uint8_t>(DS_AUDIO_FRAMES*4);
    uint64_t last_epoch=0;float volume=1,target=1;unsigned fade=0,next=2,fanfare_tail=0;
    char error[192]={0};
    SoundChannels channels;
    std::array<Channel *,16> channel_owners{};
    unsigned borrowers=0;
    DSAudio(){
        for(unsigned i=0;i<16;i++)channels.channel[i].myNo=i;
        channels.host=this;channels.stop=[](void *host,unsigned i){auto *a=static_cast<DSAudio *>(host);if(a->channel_owners[i])a->channel_owners[i]->Kill();a->channel_owners[i]=nullptr;};
    }
};
static void channel_drop(SNDExChannel *,SNDExChannelCallbackStatus status,void *data){
    assert(status==SND_EX_CHANNEL_CALLBACK_DROP);static_cast<Channel *>(data)->Kill();
}
static void sync_channels(DSAudio *audio){
    for(unsigned i=0;i<16;i++){
        Channel *owner=audio->channel_owners[i];auto &sdk=audio->channels.channel[i];
        if(owner&&owner->state==CS_NONE)audio->channel_owners[i]=owner=nullptr;
        sdk.prio=owner?owner->prio:0;
        sdk.volume=owner?(owner->state==CS_START?127:owner->reg.volumeMul|(owner->reg.volumeDiv<<8)):0;
        sdk.active_flag=owner!=nullptr;sdk.callback=owner?channel_drop:nullptr;sdk.callback_data=owner;
    }
}
static int allocate_channel(DSAudio *audio,Player *player,int type,int priority,uint16_t mask,bool strong){
    if(type<0||type>2||priority<0||priority>255)return -1;
    static const uint16_t types[]={0xffff,0x3f00,0xc000};
    sync_channels(audio);
    SNDExChannel *channel=audio->channels.SND_AllocExChannel(mask&types[type],priority,strong,channel_drop,nullptr);
    if(!channel)return -1;
    unsigned id=channel->myNo;Channel *owner=&player->channels[id];
    audio->channel_owners[id]=owner;channel->callback_data=owner;
    owner->noteLength=-1;owner->vol=0x7ff;owner->clearHistory();return id;
}
static void bind_voice(DSAudio *audio,Voice *voice){
    voice->audio=audio;voice->player.channelAllocator=[voice](Player *player,int type,int priority,uint16_t mask,bool strong){return allocate_channel(voice->audio,player,type,priority,mask,strong);};
}
static void release_channels(DSAudio *audio,Voice *voice){
    if(!voice)return;
    for(unsigned i=0;i<16;i++)if(audio->channel_owners[i]&&audio->channel_owners[i]->ply==&voice->player){
        audio->channel_owners[i]=nullptr;audio->channels.channel[i].prio=0;audio->channels.SND_FreeExChannel(&audio->channels.channel[i]);
    }
}
static void claim_restored_channels(DSAudio *audio,Voice *voice){
    // Preview snapshot restoration is not an SDK pause/resume adapter. Restored
    // notes must still honor current locks and other voices' channel priority.
    for(unsigned i=0;i<16;i++)if(voice->player.channels[i].state!=CS_NONE){
        Channel *ch=&voice->player.channels[i];bool strong=false;
        if(ch->trackId>=0)strong=voice->player.tracks[ch->trackId].channelMaskEnabled;
        sync_channels(audio);
        auto *allocated=audio->channels.SND_AllocExChannel(1u<<i,ch->prio,strong,channel_drop,ch);
        if(allocated)audio->channel_owners[i]=ch;else ch->Kill();
    }
    bind_voice(audio,voice);sync_channels(audio);
}
extern "C" DSAudio *ds_audio_new(void) {
    try {auto result=std::make_unique<DSAudio>();result->archive=archive();for(unsigned p=0;p<32;p++)result->channels.NNS_SndPlayerSetAllocatableChannel(p,result->archive->player_masks[p]);return result.release();}catch(...){return nullptr;}
}
extern "C" DSAudio *ds_audio_clone(const DSAudio *source) {
    if(!source)return ds_audio_new();
    try {
        auto result=std::make_unique<DSAudio>();result->archive=source->archive;result->pcm=source->pcm;result->last_epoch=source->last_epoch;
        if(source->saved_bgm)result->saved_bgm=std::make_unique<Voice>(*source->saved_bgm);
        result->saved_volume=source->saved_volume;
        result->volume=source->volume;result->target=source->target;result->fade=source->fade;result->next=source->next;result->fanfare_tail=source->fanfare_tail;
        for(size_t i=0;i<source->voices.size();i++)if(source->voices[i])result->voices[i]=std::make_unique<Voice>(*source->voices[i]);
        for(unsigned i=0;i<32;i++)result->channels.sPlayer[i]=source->channels.sPlayer[i];
        result->channels.sLockChannel=source->channels.sLockChannel;result->channels.sWeakLockChannel=source->channels.sWeakLockChannel;
        for(unsigned i=0;i<source->voices.size();i++)if(result->voices[i]){
            bind_voice(result.get(),result->voices[i].get());
            for(unsigned ch=0;ch<16;ch++)if(source->channel_owners[ch]&&source->channel_owners[ch]->ply==&source->voices[i]->player)
                result->channel_owners[ch]=&result->voices[i]->player.channels[ch];
        }
        sync_channels(result.get());return result.release();
    }catch(...){return nullptr;}
}
extern "C" void ds_audio_free(DSAudio *audio){if(audio&&audio->borrowers)std::abort();delete audio;}
static int audio_play_bank(DSAudio *audio,unsigned kind,unsigned id,unsigned bank) {
    if(!audio||kind>DS_AUDIO_EFFECT)return 0;
    try {
        if(kind==DS_AUDIO_BGM&&audio->voices[0]&&audio->voices[0]->id==id&&(bank==UINT32_MAX||audio->voices[0]->asset->sseq->info.bank==bank))return 1;
        auto voice=std::make_unique<Voice>();voice->asset=audio->archive->get(id,bank);voice->id=id;voice->rng=id+1;voice->player_no=voice->asset->sseq->info.playerNo;
        if(voice->player_no>=32||voice->asset->sseq->info.channelPrio>127||voice->asset->sseq->info.playerPrio>127)throw std::runtime_error("Invalid SDAT player policy");
        voice->player.prio=voice->asset->sseq->info.channelPrio;
        voice->player.startChannelMaskEnabled=audio->channels.sPlayer[voice->player_no].allocChBitFlag!=0;
        voice->player.startChannelMask=audio->channels.sPlayer[voice->player_no].allocChBitFlag?audio->channels.sPlayer[voice->player_no].allocChBitFlag:0xffff;
        voice->player.sampleRate=DS_AUDIO_RATE;voice->player.interpolation=INTERPOLATION_LINEAR;
        if(!voice->player.Setup(voice->asset->sseq.get()))throw std::runtime_error("No SSEQ track available");
        voice->player.sseqVol=Cnv_Sust(voice->asset->sseq->info.vol);
        unsigned slot=kind;
        if(kind==DS_AUDIO_EFFECT){
            slot=audio->next;
            for(unsigned i=2;i<audio->voices.size();i++)if(!audio->voices[i]||!audio->voices[i]->active()||audio->voices[i]->id==id){slot=i;break;}
            audio->next=slot+1;if(audio->next>=audio->voices.size())audio->next=2;
        }
        bind_voice(audio,voice.get());release_channels(audio,audio->voices[slot].get());audio->voices[slot]=std::move(voice);
        if(kind==DS_AUDIO_FANFARE)audio->fanfare_tail=15; // Original sub_02006C04.
        if(kind==DS_AUDIO_BGM){audio->volume=audio->target=1;audio->fade=0;}
        audio->error[0]=0;return 1;
    }catch(const std::exception &error){snprintf(audio->error,sizeof(audio->error),"Sequence %u: %s",id,error.what());return 0;}
}
extern "C" int ds_audio_play(DSAudio *audio,unsigned kind,unsigned id){return audio_play_bank(audio,kind,id,UINT32_MAX);}
extern "C" int ds_audio_play_bank(DSAudio *audio,unsigned kind,unsigned id,unsigned bank){return audio_play_bank(audio,kind,id,bank);}
extern "C" int ds_audio_play_temporary_bgm(DSAudio *audio,unsigned id) {
    if(!audio)return 0;
    try {
        // Original field sound stops instead of preserving a fading/no-sequence
        // player. Native voices own the same paused sequence/track state.
        auto saved=(!audio->fade&&audio->voices[0]&&audio->voices[0]->active())?std::make_unique<Voice>(*audio->voices[0]):nullptr;
        float volume=audio->volume;
        if(!audio_play_bank(audio,DS_AUDIO_BGM,id,UINT32_MAX))return 0;
        audio->saved_bgm=std::move(saved);audio->saved_volume=volume;return 1;
    }catch(const std::exception &error){snprintf(audio->error,sizeof(audio->error),"Temporary BGM: %s",error.what());return 0;}
}
extern "C" int ds_audio_restore_bgm(DSAudio *audio) {
    if(!audio||!audio->saved_bgm)return 0;
    release_channels(audio,audio->voices[0].get());audio->voices[0]=std::move(audio->saved_bgm);claim_restored_channels(audio,audio->voices[0].get());audio->volume=audio->target=audio->saved_volume;audio->fade=0;return 1;
}
extern "C" void ds_audio_discard_saved_bgm(DSAudio *audio) { if(audio)audio->saved_bgm.reset(); }
extern "C" void ds_audio_stop(DSAudio *audio,unsigned kind,unsigned id) {
    if(!audio)return;
    if(kind==DS_AUDIO_FANFARE)audio->fanfare_tail=0;
    for(unsigned i=0;i<audio->voices.size();i++)if((kind==DS_AUDIO_EFFECT?i>=2:i==kind)&&audio->voices[i]&&(!id||audio->voices[i]->id==id)){release_channels(audio,audio->voices[i].get());audio->voices[i].reset();}
}
extern "C" void ds_audio_fade(DSAudio *audio,unsigned volume,unsigned ticks) {
    if(!audio)return;audio->target=std::min(volume,127u)/127.f;audio->fade=ticks;if(!ticks)audio->volume=audio->target;
}
extern "C" int ds_audio_playing(const DSAudio *audio,unsigned kind,unsigned id) {
    if(!audio)return 0;
    if(kind==DS_AUDIO_FANFARE&&audio->fanfare_tail)return 1;
    for(unsigned i=0;i<audio->voices.size();i++)if((kind==DS_AUDIO_EFFECT?i>=2:i==kind)&&audio->voices[i]&&(!id||audio->voices[i]->id==id)&&audio->voices[i]->active())return 1;
    return 0;
}
extern "C" int ds_audio_tick(DSAudio *audio) {
    if(!audio)return 0;
    std::array<int32_t,DS_AUDIO_FRAMES*2> mix{};
    bool fanfare=ds_audio_playing(audio,DS_AUDIO_FANFARE,0);
    if(audio->fanfare_tail&&!audio->voices[1])--audio->fanfare_tail;
    if(audio->fade){audio->volume+=(audio->target-audio->volume)/audio->fade;--audio->fade;}
    try {
        for(unsigned frame=0;frame<DS_AUDIO_FRAMES;frame++)for(unsigned i=0;i<audio->voices.size();i++) {
            auto &voice=audio->voices[i];if(!voice)continue;
            if(i==0&&fanfare)continue; // Pause the music cursor for item fanfares.
            sequence_begin=voice->asset->sseq->data.data();sequence_end=sequence_begin+voice->asset->sseq->data.size();
            sound_rng=&voice->rng;voice->player.GenerateSamples(audio->scratch,0,1);sound_rng=nullptr;
            float gain=(i==0?audio->volume:1.f)*0.5f;
            for(unsigned j=0;j<2;j++)mix[frame*2+j]+=int16_t(unsigned(audio->scratch[j*2])|(unsigned(audio->scratch[j*2+1])<<8))*gain;
            if(!voice->active()){release_channels(audio,voice.get());voice.reset();}
        }
        for(unsigned j=0;j<mix.size();j++)audio->pcm[j]=std::max(-32768,std::min(32767,mix[j]));
        audio->last_epoch=++epoch;return 1;
    }catch(const std::exception &error){sound_rng=nullptr;snprintf(audio->error,sizeof(audio->error),"%s",error.what());audio->pcm.fill(0);return 0;}
}
extern "C" const int16_t *ds_audio_pcm(const DSAudio *audio){return audio?audio->pcm.data():nullptr;}
extern "C" const char *ds_audio_error(const DSAudio *audio){return audio?audio->error:"Cannot initialize original audio";}
extern "C" uint64_t ds_audio_epoch(void){return epoch;}
extern "C" uint64_t ds_audio_last_epoch(const DSAudio *audio){return audio?audio->last_epoch:0;}

extern "C" unsigned ds_audio_bgm(const DSAudio *audio){return audio&&audio->voices[0]?audio->voices[0]->id:0;}

extern "C" int ds_audio_set_player_channels(DSAudio *a,unsigned player,unsigned mask){
    if(!a||player>=32||mask>0xffff)return 0;
    // Original NNS player default: 0 means no per-player override. This changes
    // future starts only; it is neither a channel lock nor an active track edit.
    a->channels.NNS_SndPlayerSetAllocatableChannel(player,mask);return 1;
}
extern "C" unsigned ds_audio_player_channels(const DSAudio *a,unsigned player){return a&&player<32?a->channels.sPlayer[player].allocChBitFlag:0;}
extern "C" int ds_audio_lock_channels(DSAudio *a,unsigned mask,int weak){
    if(!a||mask>0xffff||(weak!=0&&weak!=1))return 0;
    sync_channels(a);
    // NNS resource-manager locks are exclusive. ARM7 weak locks are a distinct
    // SDK operation; only tracks with explicit channel masks may override them.
    if(!weak&&(a->channels.sLockChannel&mask))return 0;
    a->channels.SND_LockChannel(mask,weak?SND_LOCK_IMPLIED_ALLOC_CHANNEL:0);return 1;
}
extern "C" int ds_audio_unlock_channels(DSAudio *a,unsigned mask,int weak){
    if(!a||mask>0xffff||(weak!=0&&weak!=1))return 0;
    unsigned locked=a->channels.SND_GetLockedChannel(weak?SND_LOCK_IMPLIED_ALLOC_CHANNEL:0);
    if((mask&locked)!=mask)return 0;
    a->channels.SND_UnlockChannel(mask,weak?SND_LOCK_IMPLIED_ALLOC_CHANNEL:0);return 1;
}
extern "C" unsigned ds_audio_locked_channels(const DSAudio *a,int weak){return a?(weak?a->channels.sWeakLockChannel:a->channels.sLockChannel):0;}
extern "C" unsigned ds_audio_active_channels(const DSAudio *a,unsigned sequence){
    if(!a)return 0;unsigned mask=0;
    for(unsigned i=0;i<16;i++)if(a->channel_owners[i]&&a->channel_owners[i]->state!=CS_NONE){
        for(const auto &voice:a->voices)if(voice&&voice->id==sequence&&a->channel_owners[i]->ply==&voice->player)mask|=1u<<i;
    }return mask;
}

extern "C" int ds_audio_acquire(DSAudio *a){if(!a||a->borrowers==UINT32_MAX)return 0;a->borrowers++;return 1;}
extern "C" void ds_audio_release(DSAudio *a){if(!a||!a->borrowers)std::abort();a->borrowers--;}
extern "C" int ds_audio_restore_snapshot(DSAudio *a,DSAudio *snapshot){
    if(!a||!snapshot||a==snapshot||snapshot->borrowers)return 0;
    static_assert(std::is_nothrow_move_assignable<DSAudio>::value,"rollback must not allocate");
    unsigned borrowers=a->borrowers;*a=std::move(*snapshot);a->borrowers=borrowers;a->channels.host=a;
    for(auto &voice:a->voices)if(voice)voice->audio=a;
    return 1;
}

extern "C" int ds_audio_sequence_info(DSAudio *a,unsigned id,DSAudioSequenceInfo *out){
    if(!a||!out)return 0;
    try{const auto &i=a->archive->get(id)->sseq->info;
        *out={i.bank,i.vol,i.channelPrio,i.playerPrio,i.playerNo};return 1;
    }catch(...){return 0;}
}

extern "C" int ds_audio_channel_state_valid(const DSAudio *a){
    if(!a)return 0;unsigned used=0;
    for(const auto &voice:a->voices)if(voice)for(const auto &ch:voice->player.channels)if(ch.state!=CS_NONE){
        if(ch.chnId<0||ch.chnId>=16)return 0;unsigned bit=1u<<ch.chnId;
        if((used&bit)||(a->channels.sLockChannel&bit)||a->channel_owners[ch.chnId]!=&ch)return 0;
        used|=bit;
    }return 1;
}
