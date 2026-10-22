#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct DSAudio DSAudio;
enum { DS_AUDIO_BGM, DS_AUDIO_FANFARE, DS_AUDIO_EFFECT };
enum { DS_AUDIO_RATE=44100, DS_AUDIO_FRAMES=735 };
/* No audio device is opened by this deterministic sequence renderer. */
DSAudio *ds_audio_new(void);
DSAudio *ds_audio_clone(const DSAudio *);
void ds_audio_free(DSAudio *);
int ds_audio_play(DSAudio *,unsigned kind,unsigned sequence);
/* Original StartSeqEx bank selection, used by normal Pokémon cries. */
int ds_audio_play_bank(DSAudio *,unsigned kind,unsigned sequence,unsigned bank);
void ds_audio_stop(DSAudio *,unsigned kind,unsigned sequence);
void ds_audio_fade(DSAudio *,unsigned volume,unsigned ticks);
unsigned ds_audio_bgm(const DSAudio *);
int ds_audio_playing(const DSAudio *,unsigned kind,unsigned sequence);
int ds_audio_tick(DSAudio *);
const int16_t *ds_audio_pcm(const DSAudio *);
const char *ds_audio_error(const DSAudio *);
uint64_t ds_audio_epoch(void);
uint64_t ds_audio_last_epoch(const DSAudio *);
#ifdef __cplusplus
}
#endif
