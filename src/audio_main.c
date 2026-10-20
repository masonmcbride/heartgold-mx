#include "ds_audio_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void le(FILE *file,unsigned value,unsigned size){while(size--){fputc(value&255,file);value>>=8;}}
int main(int argc,char **argv) {
    int play=argc==2&&!strcmp(argv[1],"--play");
    if(!play&&argc!=2){fprintf(stderr,"Usage: heartgold_audio [--play | output.wav]\n");return 2;}
    DSAudio *audio=ds_audio_new();if(!audio)return 1;char error[192];
    DSAudioOutput *output=play?ds_audio_output_new(error,sizeof(error)):NULL;
    if(play&&!output){fprintf(stderr,"%s\n",error);ds_audio_free(audio);return 1;}
    FILE *wav=play?NULL:fopen(argv[1],"wb");if(!play&&!wav){ds_audio_free(audio);return 1;}
    const unsigned ticks=240,bytes=ticks*DS_AUDIO_FRAMES*4;
    if(wav){fwrite("RIFF",1,4,wav);le(wav,bytes+36,4);fwrite("WAVEfmt ",1,8,wav);le(wav,16,4);le(wav,1,2);le(wav,2,2);le(wav,DS_AUDIO_RATE,4);le(wav,DS_AUDIO_RATE*4,4);le(wav,4,2);le(wav,16,2);fwrite("data",1,4,wav);le(wav,bytes,4);}
    if(!ds_audio_play(audio,DS_AUDIO_BGM,1018))return 1;
    for(unsigned i=0;i<ticks;i++) {
        if(i==60&&!ds_audio_play(audio,DS_AUDIO_EFFECT,1500))return 1;
        if(i==120&&!ds_audio_play(audio,DS_AUDIO_FANFARE,1185))return 1;
        if(!ds_audio_tick(audio)){fprintf(stderr,"%s\n",ds_audio_error(audio));return 1;}
        if(wav)fwrite(ds_audio_pcm(audio),4,DS_AUDIO_FRAMES,wav);
        if(output){ds_audio_output_push(output,ds_audio_pcm(audio),DS_AUDIO_FRAMES);struct timespec delay={0,16666667};nanosleep(&delay,NULL);}
    }
    uint64_t frames=ds_audio_output_frames(output);
    if(output)printf("Native AudioQueue consumed %llu original PCM frames\n",(unsigned long long)frames);
    if(wav){fclose(wav);printf("Rendered %u original stereo frames\n",ticks*DS_AUDIO_FRAMES);}
    ds_audio_output_free(output);ds_audio_free(audio);return play&&frames<DS_AUDIO_RATE?1:0;
}
