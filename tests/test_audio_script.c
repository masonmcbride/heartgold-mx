#include "compat/story.h"
#include "ds_audio.h"
#include "field_player.h"
#include "map_header.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define H(n) ((n)&255),(((n)>>8)&255)
int main(void) {
    HGStoryState *story=hg_story_new();CHECK(story);char error[256];
    const uint8_t se[]={H(73),H(1500),H(75),H(1500),H(2)};
    const uint8_t fanfare[]={H(78),H(1185),H(79),H(2)};
    for(unsigned k=0;k<2;k++) {
        uint64_t revision=hg_story_revision(story);
        HGFieldScript *script=hg_field_script_bytes(story,k?fanfare:se,k?sizeof(fanfare):sizeof(se),0,0,error,sizeof(error));CHECK(script);
        unsigned ticks=0;int status;
        do{status=hg_field_script_tick(script);CHECK(status!=HG_SCRIPT_ERROR);ticks++;}while(status!=HG_SCRIPT_DONE&&ticks<600);
        CHECK(ticks>30&&ticks<600&&hg_story_revision(story)==revision);CHECK(hg_field_script_commit(script));
        printf("Original %s wait completed after %u ticks; preflight retained story revision\n",k?"fanfare":"effect",ticks);hg_field_script_free(script);
    }
    const unsigned maps[]={64,63,60,61,384,67,385,143,76};
    DSAudio *audio=ds_audio_new();CHECK(audio);
    for(unsigned i=0;i<sizeof(maps)/sizeof(*maps);i++) {
        unsigned id=MapHeader_GetDayMusicId(maps[i]);CHECK(ds_audio_play(audio,DS_AUDIO_BGM,id));
        unsigned nonzero=0;for(unsigned tick=0;tick<120;tick++) {
            CHECK(ds_audio_tick(audio));const int16_t *pcm=ds_audio_pcm(audio);
            for(unsigned n=0;n<DS_AUDIO_FRAMES*2;n++)nonzero+=pcm[n]!=0;
        }
        CHECK(nonzero);printf("Map %u: original music %u rendered\n",maps[i],id);
    }
    HGFieldPlayer *player=hg_field_player_new_game();CHECK(player);
    CHECK(ds_audio_bgm(hg_field_player_audio(player))==MapHeader_GetDayMusicId(64));
    hg_field_player_free(player);ds_audio_free(audio);hg_story_free(story);return 0;
}
