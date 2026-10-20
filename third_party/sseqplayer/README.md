# SSEQPlayer provenance

Vendored from https://github.com/kode54/SSEQPlayer at
`77222d3657adff358fb4e610d3e56bb7ada8ec24`.
The upstream `LICENSE.TXT` (WTFPL v2) and source attribution headers are retained.
No game assets are included.

The native port uses this existing SDAT/SSEQ/SBNK/SWAR synthesizer instead of
implementing a second sequence engine. `src/ds_audio.cpp` supplies the C interface,
shared immutable asset cache, independent snapshots, sequence mixing and the
per-voice deterministic RNG. `src/ds_audio_output.c` provides macOS AudioQueue
output separately.

Local source adaptations: bounded archive/sequence reads and record counts;
checked bank/wave lookup and sequence targets/variables; instruction and track
limits; explicit rejection of unsupported executing sequence commands; exception-safe
archive ownership; removal of a negative-index channel reference and undefined
signed shifts; signed ADPCM predictor initialization and index checking. Random
instructions use the host voice's private RNG so previews do not alter gameplay
or another audio owner. Track allocation and debug-print commands are consumed
explicitly. Existing source comments still identify unsupported features.

This is a software renderer derived from FSS, not the original Nintendo SDK sound
driver or cycle-exact DS sound hardware. See `docs/audio.md` for execution evidence
and remaining fidelity/integration limits.
