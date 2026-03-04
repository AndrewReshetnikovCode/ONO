# PvZ YG Phase 8 — Web Audio Hardening

## Scope

Phase 8 improves browser music runtime correctness in the Emscripten audio backend.

Implemented files:

- `src/SexyAppFramework/sound/WebMusicInterface.h`
- `src/SexyAppFramework/sound/WebMusicInterface.cpp`

## Implemented behavior

1. Playback state tracking:
   - current song id
   - playing flag
   - paused flag
   - global volume as `double`

2. JS audio runtime improvements:
   - persistent gain node for music output
   - playback timing state (`startTime`, `pausedOffset`, loop mode)
   - elapsed-time query API for music order approximation
   - explicit `isPlaying` query API
   - safer stop behavior with guarded `stop()` calls

3. C++ behavior improvements:
   - `IsPlaying()` now checks actual runtime state
   - `GetMusicOrder()` returns elapsed milliseconds for current song
   - `SetVolume()` and `SetSongVolume()` apply consistent gain scaling
   - pause/resume APIs update local state flags

## Phase-8 handoff

This runtime now has enough state observability to support deeper music-sync telemetry and cross-scene audio recovery tuning.
