#include "ds_audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static unsigned energy(const int16_t *pcm){unsigned n=0;for(unsigned i=0;i<DS_AUDIO_FRAMES*2;i++)if(pcm[i])n++;return n;}
int main(void) {
    DSAudio *a=ds_audio_new();CHECK(a);
    const unsigned ids[]={1500,1185,2168};
    for(unsigned k=0;k<3;k++) {
        unsigned kind=k==1?DS_AUDIO_FANFARE:DS_AUDIO_EFFECT;
        CHECK(ds_audio_play(a,kind,ids[k]));unsigned ticks=0,nonzero=0;
        do {CHECK(ds_audio_tick(a));nonzero+=energy(ds_audio_pcm(a));ticks++;}while(ds_audio_playing(a,kind,ids[k])&&ticks<1200);
        printf("Sequence %u: %u frames, %u nonzero samples\n",ids[k],ticks,nonzero);
        CHECK(ticks>1&&ticks<1200&&nonzero);
    }
    for(unsigned species=152;species<=183;species+=31){
        CHECK(ds_audio_play_bank(a,DS_AUDIO_EFFECT,2,species));unsigned ticks=0,nonzero=0;
        do{CHECK(ds_audio_tick(a));nonzero+=energy(ds_audio_pcm(a));ticks++;}while(ds_audio_playing(a,DS_AUDIO_EFFECT,2)&&ticks<600);
        CHECK(ticks>1&&ticks<600&&nonzero);printf("Cry bank %u: %u frames, %u nonzero samples\n",species,ticks,nonzero);
    }
    CHECK(!ds_audio_play_bank(a,DS_AUDIO_EFFECT,2,999999));
    CHECK(!ds_audio_play(a,DS_AUDIO_EFFECT,999999));CHECK(ds_audio_error(a)[0]);
    CHECK(ds_audio_play(a,DS_AUDIO_BGM,1018));
    for(unsigned i=0;i<60;i++)CHECK(ds_audio_tick(a));
    DSAudio *b=ds_audio_clone(a);CHECK(b);
    for(unsigned i=0;i<60;i++){CHECK(ds_audio_tick(a));CHECK(ds_audio_tick(b));CHECK(!memcmp(ds_audio_pcm(a),ds_audio_pcm(b),DS_AUDIO_FRAMES*4));}
    ds_audio_stop(b,DS_AUDIO_BGM,0);CHECK(!ds_audio_playing(b,DS_AUDIO_BGM,0));CHECK(ds_audio_playing(a,DS_AUDIO_BGM,1018));
    CHECK(ds_audio_play(a,DS_AUDIO_EFFECT,1500));CHECK(ds_audio_tick(a));CHECK(energy(ds_audio_pcm(a)));
    ds_audio_fade(a,0,12);for(unsigned i=0;i<120;i++)CHECK(ds_audio_tick(a));CHECK(!energy(ds_audio_pcm(a)));
    ds_audio_free(a);ds_audio_free(b);puts("Audio playback, real completion, independent snapshots, mixing and fade passed");
}
