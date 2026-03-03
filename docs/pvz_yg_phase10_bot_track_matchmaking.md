# PvZ YG Phase 10 — Bot Track Matchmaking and Replay

## Scope

Phase 10 implements bot logic driven by recorded action tracks and integrates track assignment into matchmaking.

Implemented files:

- `src/Lawn/System/BotActionTrack.h`
- `src/Lawn/System/BotActionTrack.cpp`
- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`

## Implemented behavior

1. Bot track pool:
   - Introduced `BotActionTrackPool` with a default pool of 20 recorded tracks.
   - Track actions currently replay plant placement commands (`x`, `y`, `plantType`, `imitaterType`).

2. Matchmaking track assignment:
   - `AuthoritativeLobbyMember` now carries `mBotTrackId`.
   - During bot fill, track assignment runs with a unique-first policy:
     - if track pool size >= bot count in lobby, all bots get unique tracks,
     - otherwise duplicates are allowed only after unique tracks are exhausted.
   - Assignment is deterministic per lobby.

3. Match runtime replay:
   - Bot replay cursors are initialized for each bot at match start.
   - On each authoritative tick, due track actions are converted into `NetClientCommand` and queued through the existing validation path.
   - Replay completion and replay failures are surfaced through runtime events.

4. Realism constraint update:
   - No runtime track jitter is applied.
   - Replay timing follows recorded `tickOffset` values directly.

## Handoff notes

This implementation provides deterministic bot replay with unique-first track assignment and no-jitter behavior, ready for future external track import and advanced bot policies.
