#include "ds_audio.h"
#include "Player.h"
#include "SDAT.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <cstdio>

namespace {
thread_local uint32_t *sound_rng;
thread_local const uint8_t *sequence_begin,*sequence_end;
uint64_t epoch;
struct Archive {
    std::vector<uint8_t> bytes;
    std::map<uint64_t,std::shared_ptr<SDAT>> sequences;
    Archive() {
        std::ifstream input(HG_SOUND_ARCHIVE,std::ios::binary);
        if(!input)throw std::runtime_error("Cannot open original sound archive");
        bytes.assign(std::istreambuf_iterator<char>(input),{});
        if(bytes.size()<64||bytes.size()>32*1024*1024)throw std::runtime_error("Invalid sound archive size");
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
    std::shared_ptr<SDAT> asset;Player player;unsigned id=0;uint32_t rng=1;
    Voice()=default;
    Voice(const Voice &other):asset(other.asset),player(other.player),id(other.id),rng(other.rng) {
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
};
extern "C" DSAudio *ds_audio_new(void) {
    try {auto result=std::make_unique<DSAudio>();result->archive=archive();return result.release();}catch(...){return nullptr;}
}
extern "C" DSAudio *ds_audio_clone(const DSAudio *source) {
    if(!source)return ds_audio_new();
    try {
        auto result=std::make_unique<DSAudio>();result->archive=source->archive;result->pcm=source->pcm;result->last_epoch=source->last_epoch;
        if(source->saved_bgm)result->saved_bgm=std::make_unique<Voice>(*source->saved_bgm);
        result->saved_volume=source->saved_volume;
        result->volume=source->volume;result->target=source->target;result->fade=source->fade;result->next=source->next;result->fanfare_tail=source->fanfare_tail;
        for(size_t i=0;i<source->voices.size();i++)if(source->voices[i])result->voices[i]=std::make_unique<Voice>(*source->voices[i]);
        return result.release();
    }catch(...){return nullptr;}
}
extern "C" void ds_audio_free(DSAudio *audio){delete audio;}
static int audio_play_bank(DSAudio *audio,unsigned kind,unsigned id,unsigned bank) {
    if(!audio||kind>DS_AUDIO_EFFECT)return 0;
    try {
        if(kind==DS_AUDIO_BGM&&audio->voices[0]&&audio->voices[0]->id==id&&(bank==UINT32_MAX||audio->voices[0]->asset->sseq->info.bank==bank))return 1;
        auto voice=std::make_unique<Voice>();voice->asset=audio->archive->get(id,bank);voice->id=id;voice->rng=id+1;
        voice->player.sampleRate=DS_AUDIO_RATE;voice->player.interpolation=INTERPOLATION_LINEAR;
        if(!voice->player.Setup(voice->asset->sseq.get()))throw std::runtime_error("No SSEQ track available");
        voice->player.sseqVol=Cnv_Sust(voice->asset->sseq->info.vol);
        unsigned slot=kind;
        if(kind==DS_AUDIO_EFFECT){
            slot=audio->next;
            for(unsigned i=2;i<audio->voices.size();i++)if(!audio->voices[i]||!audio->voices[i]->active()||audio->voices[i]->id==id){slot=i;break;}
            audio->next=slot+1;if(audio->next>=audio->voices.size())audio->next=2;
        }
        audio->voices[slot]=std::move(voice);
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
    audio->voices[0]=std::move(audio->saved_bgm);audio->volume=audio->target=audio->saved_volume;audio->fade=0;return 1;
}
extern "C" void ds_audio_discard_saved_bgm(DSAudio *audio) { if(audio)audio->saved_bgm.reset(); }
extern "C" void ds_audio_stop(DSAudio *audio,unsigned kind,unsigned id) {
    if(!audio)return;
    if(kind==DS_AUDIO_FANFARE)audio->fanfare_tail=0;
    for(unsigned i=0;i<audio->voices.size();i++)if((kind==DS_AUDIO_EFFECT?i>=2:i==kind)&&audio->voices[i]&&(!id||audio->voices[i]->id==id))audio->voices[i].reset();
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
        for(unsigned i=0;i<audio->voices.size();i++) {
            auto &voice=audio->voices[i];if(!voice)continue;
            if(i==0&&fanfare)continue; // Pause the music cursor for item fanfares.
            sequence_begin=voice->asset->sseq->data.data();sequence_end=sequence_begin+voice->asset->sseq->data.size();
            sound_rng=&voice->rng;voice->player.GenerateSamples(audio->scratch,0,DS_AUDIO_FRAMES);sound_rng=nullptr;
            float gain=(i==0?audio->volume:1.f)*0.5f;
            for(unsigned j=0;j<mix.size();j++)mix[j]+=int16_t(unsigned(audio->scratch[j*2])|(unsigned(audio->scratch[j*2+1])<<8))*gain;
            if(!voice->active())voice.reset();
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
