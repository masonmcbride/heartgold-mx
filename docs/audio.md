# Native game audio

The field viewer now plays the original game's music, scripted effects and
fanfares through macOS AudioQueue. Launch `./build/heartgold_field_viewer` normally;
no separate audio process or converted music files are needed. Elm's phone ring
and hangup also use the same renderer. Native dialogue page advances and final
acknowledgments play SEQ_SE_DP_SELECT, matching render_text.c's
TextPrinter_Continue; this also covers the shared phone dialogue pager. Original jump cues reached by the motion
interpreter route to the current object's scene owner.

Original script sound handlers execute unchanged. Their host sound boundary now
plays SDAT sequences with the original banks and wave samples, instead of
acknowledging muted requests. WaitSE and WaitFanfare poll actual renderer state.
Fanfares pause the BGM cursor and retain the original 15-frame follow-up wait
(`asm/unk_02005D10.s`, PlayFanfare/sub_02006C04). Map music uses the original
MapHeader_GetDayMusicId; crossing into another map with the same cue preserves its
cursor. Fade commands change the rendered music volume.

## Reuse and ownership

The reusable C API in `src/ds_audio.h` wraps the existing
[SSEQPlayer synthesizer](https://github.com/kode54/SSEQPlayer), pinned and attributed
in `third_party/sseqplayer/README.md`. This provides sequence execution, PCM/ADPCM
samples, instruments, envelopes, PSG/noise and software synthesis. The original
HeartGold sound.c and Nintendo SDK sound driver are not linked or executed.

The original `gs_sound_data.sdat` is copied only into ignored build resources.
Immutable loaded sequences/banks/waves are cached. Each field player and private
script scene owns its playback cursors, voices, RNG and fade state. Child scripts
and committed scenes transfer that owner. Preflight and answer probes clone it;
they open no device, cannot emit audio and cannot advance the live cursor.
Cancellation discards their private sound state. Explore freely closes the story's
output device while preserving its playback state, then restores it on return.

Audio synthesis advances once per 60 Hz simulation frame, including dialogue.
The native output callback only drains a bounded stereo PCM ring; it performs no
asset loading, sequence interpretation or game calls. Device ownership is opt-in
at the UI boundary, so ordinary tests and render-only commands remain silent.

## Verification

- `test_audio`: original select cue 1500 completes in 49 renderer ticks; item
  fanfare 1185 completes in 161 including the 15-frame tail; phone ring 2168 in 83.
  Nonzero samples, independent bit-identical snapshot continuation, invalid IDs,
  overlapping music/effects, stops and fades are checked. These are renderer
  measurements, not a comparison against DS hardware.
- `test_audio_script`: unchanged original effect and fanfare handlers/waits
  complete in 51 and 163 script ticks; preview retains the story revision.
  Music renders for bedroom, Mom's house, New Bark, Elm's lab, Routes 29/30,
  Cherrygrove, Mr. Pokemon and Goldenrod.
- `heartgold_audio --play`: native AudioQueue consumed all 176,400 frames of a
  four-second New Bark/select/fanfare probe. `heartgold_audio build/audio.wav`
  writes the same sequence to a build-local WAV for inspection.
- `heartgold_field_viewer --audio-smoke`: the actual field application fed
  original PCM to macOS, followed by the native gameplay/UI smoke scenarios.
- All 38 normal tests pass (13.35 seconds with eight workers); the native silent
  and audible AppKit smoke scenarios pass.
- ASan/UBSan: the audio renderer test passes with halt-on-error enabled. Connected
  game scenarios retain existing signed-shift diagnostics in original pokedex.c,
  sys_vars.c and pokemon.c; they are not a clean whole-game UBSan result. A host
  population comparison's out-of-range intermediate pointer was corrected by
  grouping its array index before pointer arithmetic.

The focused renderer test takes roughly 0.1 seconds, and the script/music test
roughly 0.5 seconds on this machine. Neither waits for real-time playback.

## Remaining limits

This is usable software playback, not a fidelity-complete DS sound replacement.
Each simultaneous sequence has its own 16-channel player, rather than sharing
hardware channel allocation/priority across all sequences. The host mixes one BGM,
one fanfare and up to eight effects with conservative gain. RNG, interpolation,
sequence-end envelope timing and analog sound are not verified against hardware.
Unsupported executing sequence commands fail explicitly. Day music is selected;
night/RTC and radio policy are not connected. Cries/streamed audio and sounds in
unported applications remain separate work. Native starter/menu presentation
still needs its full original overlay sound lifecycle; this change does not claim
all menu, encounter, battle or boot sounds execute.
