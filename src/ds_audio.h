#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct DSAudio DSAudio;
typedef struct {unsigned bank,volume,channel_priority,player_priority,player;} DSAudioSequenceInfo;
int ds_audio_sequence_info(DSAudio *,unsigned sequence,DSAudioSequenceInfo *);
enum { DS_AUDIO_BGM, DS_AUDIO_FANFARE, DS_AUDIO_EFFECT };
enum { DS_AUDIO_RATE=44100, DS_AUDIO_FRAMES=735 };
/* No audio device is opened by this deterministic sequence renderer. */
DSAudio *ds_audio_new(void);
DSAudio *ds_audio_clone(const DSAudio *);
void ds_audio_free(DSAudio *);
int ds_audio_play(DSAudio *,unsigned kind,unsigned sequence);
/* Original StartSeqEx bank selection, used by normal Pokémon cries. */
int ds_audio_play_bank(DSAudio *,unsigned kind,unsigned sequence,unsigned bank);
/* Preserve the current BGM playback position for a temporary encounter theme.
 * Failed preparation preserves playback; snapshots retain independent copies. */
int ds_audio_play_temporary_bgm(DSAudio *,unsigned sequence);
int ds_audio_restore_bgm(DSAudio *);
void ds_audio_discard_saved_bgm(DSAudio *);
void ds_audio_stop(DSAudio *,unsigned kind,unsigned sequence);
void ds_audio_fade(DSAudio *,unsigned volume,unsigned ticks);
unsigned ds_audio_bgm(const DSAudio *);
int ds_audio_playing(const DSAudio *,unsigned kind,unsigned sequence);
/* NNS policy has 32 player slots; transport voices share 16 physical channels.
 * This interface does not supply SDK sequence/handle arbitration. A zero
 * player mask leaves the original default unchanged at the next sequence start.
 * Mask changes do not rewrite active tracks. Locks stop their current channels;
 * failed exclusive locks and invalid releases preserve state. */
int ds_audio_set_player_channels(DSAudio *,unsigned player,unsigned mask);
unsigned ds_audio_player_channels(const DSAudio *,unsigned player);
int ds_audio_lock_channels(DSAudio *,unsigned mask,int weak);
int ds_audio_unlock_channels(DSAudio *,unsigned mask,int weak);
unsigned ds_audio_locked_channels(const DSAudio *,int weak);
unsigned ds_audio_active_channels(const DSAudio *,unsigned sequence);
int ds_audio_channel_state_valid(const DSAudio *);
/* A borrowed runtime binding never owns or frees the audio engine. Freeing a
 * leased engine is invalid. Restore consumes snapshot contents, keeps the
 * engine identity/leases, and performs no allocation. */
int ds_audio_acquire(DSAudio *);
void ds_audio_release(DSAudio *);
int ds_audio_restore_snapshot(DSAudio *,DSAudio *snapshot);
int ds_audio_tick(DSAudio *);
const int16_t *ds_audio_pcm(const DSAudio *);
const char *ds_audio_error(const DSAudio *);
uint64_t ds_audio_epoch(void);
uint64_t ds_audio_last_epoch(const DSAudio *);
#ifdef __cplusplus
}
#endif
