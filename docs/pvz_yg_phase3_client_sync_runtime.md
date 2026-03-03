# PvZ YG Phase 3 — Client Networking and Sync Runtime

## Scope

Phase 3 introduces a client session runtime that connects gameplay flow to an authoritative match runtime.

Implemented files:

- `src/Lawn/System/ClientSessionRuntime.h`
- `src/Lawn/System/ClientSessionRuntime.cpp`
- `src/LawnApp.h`
- `src/LawnApp.cpp`
- `src/Lawn/Board.cpp`
- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`

## Implemented behavior

1. Client session state machine:
   - `CONNECTING`
   - `MATCHMAKING`
   - `LOBBY`
   - `SYNCED_PLAYING`
   - `TERMINATED`

2. Loopback authoritative integration:
   - `ClientSessionRuntime` boots a loopback `AuthoritativeServerRuntime`.
   - Enqueues matchmaking request for local player.
   - Binds to lobby once assigned.
   - Pulls authoritative snapshot for local player.

3. App lifecycle integration:
   - `LawnApp` can enable authoritative mode via `-authoritative`.
   - Runtime is initialized during `Init` and shut down during `Shutdown`/destructor.
   - Runtime updates each frame through `UpdateFrames`.

4. Authoritative snapshot application:
   - `LawnApp::ApplyAuthoritativeSnapshotToBoard()` applies authoritative sun to `Board`.
   - Eliminated authoritative state triggers `ZombiesWon()`.

5. Command send hooks:
   - Plant placements in `Board::MouseDownWithPlant` submit authoritative plant commands.
   - Shovel removals in `Board::MouseDownWithTool` submit authoritative remove commands.
   - Column-mode extra placements also submit commands.

## Phase-3 handoff

Transport plumbing can now replace loopback runtime with real server I/O while preserving the same client session state and command API.
