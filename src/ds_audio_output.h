#pragma once
#include "ds_audio.h"
typedef struct DSAudioOutput DSAudioOutput;
DSAudioOutput *ds_audio_output_new(char *error,size_t size);
void ds_audio_output_free(DSAudioOutput *);
void ds_audio_output_push(DSAudioOutput *,const int16_t *,unsigned frames);
uint64_t ds_audio_output_frames(DSAudioOutput *);
