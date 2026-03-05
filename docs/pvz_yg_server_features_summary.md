# PvZ YG Server Features Summary (Educational)

This document summarizes the full server-side stack currently implemented in this repository, from protocol basics to runtime orchestration and platform integration.

It is written as an onboarding-style guide so you can quickly understand:

1. what the server does,
2. what is authoritative vs client-owned,
3. what is already production-shaped vs still scaffolding,
4. how to run the new local/cloud server runner script.

---

## 1) What "server" means in this project

The server model is **authoritative gameplay runtime** logic implemented in C++:

- it validates player commands,
- controls match progression and PvP rules,
- owns authoritative resources (sun, damage, elimination),
- emits runtime events (accepted/rejected/start/end),
- and can be hosted loopback (inside client runtime) or as a dedicated process.

Current dedicated entrypoint:

- `src/authoritative_server_main.cpp` (headless runner binary)
- built as `pvz-authoritative-server`

Launcher script:

- `scripts/run_authoritative_server.sh`

---

## 2) Protocol layer (Phase 0)

Core files:

- `src/Lawn/System/NetProtocol.h`
- `src/Lawn/System/NetProtocol.cpp`
- `docs/pvz_yg_phase0_protocol_contract.md`

### 2.1 Versioning and compatibility

- Protocol is `major=1, minor=0`.
- Compatibility is explicit:
  - same major + client minor <= server minor => compatible,
  - older major => client upgrade required,
  - newer major/minor than server => server upgrade required.

### 2.2 Command envelope (required fields)

Every client command has:

- `version`
- `matchId`
- `playerId`
- `commandId`
- `clientTick`
- `commandType`

### 2.3 Validation baseline

The protocol rejects:

- unsupported version,
- zero IDs in envelope (`matchId/playerId/commandId`),
- invalid command type,
- command/payload type mismatch,
- invalid payload values per command.

### 2.4 Supported command types (v1)

- place plant
- remove plant
- send PvP zombies
- store purchase intent
- heartbeat

### 2.5 Event names (v1)

- command accepted/rejected
- snapshot apply
- PvP phase start/end
- match start/end
- diamonds updated

### 2.6 Idempotency key contract

Idempotency key fields:

- matchId + playerId + commandId + clientTick

The runtime deduplicates repeated keys to make retries safe.

---

## 3) Authority model (Phase 0 contract, used by runtime)

### 3.1 Server-authoritative domains

- wave progression
- sun generation
- damage resolution
- PvP spawn limits
- reward/MMR logic domains
- diamonds, cosmetics, weekly score ownership

### 3.2 Client-owned domains

- input capture
- rendering
- local visual prediction

### 3.3 Operational rule

Client sends intents; server finalizes outcomes for authoritative domains.

---

## 4) Authoritative runtime core (Phase 2)

Core files:

- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`
- `docs/pvz_yg_phase2_authoritative_runtime.md`

### 4.1 Matchmaking queues

- random queue
- MMR queue (`|mmr - anchor| <= window`)

### 4.2 Lobby formation

- forms full lobbies when possible,
- starts timed-out partial lobbies and fills with bots,
- prevents duplicate queue entry for same player.

### 4.3 Fixed-tick orchestration

`AuthoritativeServerRuntime::AdvanceOneTick()`:

1. increments server tick,
2. creates new matches from ready lobbies,
3. advances each active match,
4. collects and exposes runtime events,
5. removes finished matches and routing entries.

### 4.4 In-match authoritative systems

Per tick:

- authoritative sun generation on interval,
- authoritative damage application with cap per tick,
- PvP phase open/close checks,
- command queue processing,
- match-end detection.

### 4.5 PvP phases and role swap

- deterministic alive-player pairing by player ID order,
- attacker/defender roles alternate between phases,
- attacker can only target authoritative paired defender,
- per-phase zombie send cap is enforced.

### 4.6 Disconnect behavior

- disconnect => immediate elimination,
- routing cleanup,
- immediate match-end reevaluation.

---

## 5) Command processing guarantees

### 5.1 Multi-stage validation

Commands are validated by:

1. protocol validation (`NetValidateClientCommand`),
2. runtime ownership checks (match/player/alive),
3. domain constraints (sun cost, PvP role/target/limit).

### 5.2 Duplicate-safe execution

- runtime stores recent idempotency keys,
- duplicate key => no harmful re-execution,
- emits accepted event with duplicate-safe reason.

### 5.3 Evented rejection reasons

Rejected commands carry structured reason text (e.g. invalid target, insufficient sun, out-of-phase PvP send).

---

## 6) Bot-track matchmaking and replay (Phase 10)

Core files:

- `src/Lawn/System/BotActionTrack.h`
- `src/Lawn/System/BotActionTrack.cpp`
- `src/Lawn/System/AuthoritativeRuntime.cpp`
- `docs/pvz_yg_phase10_bot_track_matchmaking.md`

### 6.1 Track pool

- default pool of 20 deterministic recorded tracks,
- actions currently replay place-plant commands.

### 6.2 Track assignment

- bots get unique tracks first (when possible),
- deterministic shuffle by lobby id,
- duplicates only when bot count exceeds track pool size.

### 6.3 Replay execution

- bot actions are injected as regular `NetClientCommand`,
- replay goes through the same validation path as player commands,
- replay start/fail/complete are surfaced as runtime events.

---

## 7) Anti-cheat monitoring pipeline (Phase 6)

Core files:

- `src/Lawn/System/AuthoritativeAntiCheat.h`
- `src/Lawn/System/AuthoritativeAntiCheat.cpp`
- `src/Lawn/System/AuthoritativeRuntime.cpp`
- `docs/pvz_yg_phase6_anti_cheat_monitor.md`

### 7.1 Detectors

- resource anomaly (sun delta threshold),
- action-rate limit per tick,
- critical desync reports (explicit runtime signal).

### 7.2 Runtime integration points

- command submits are counted,
- sun snapshots are tracked,
- PvP target mismatch and send-limit breach raise critical desync events,
- anti-cheat events are forwarded into runtime event stream.

---

## 8) Authoritative progression model (Phase 4)

Core files:

- `src/Lawn/System/AuthoritativeProgressionStore.h`
- `src/Lawn/System/AuthoritativeProgressionStore.cpp`
- `docs/pvz_yg_phase4_authoritative_progression.md`

### 8.1 Stored profile fields

- playerId
- diamonds
- mmr
- weeklyScore
- cosmeticsOwned
- schemaVersion
- importedFromLocal flag

### 8.2 Operations

- get/create profile
- add/subtract diamonds with non-negative guard
- set MMR
- add weekly score with non-negative guard
- grant/list cosmetics

### 8.3 Migration

One-time migration from local `PlayerInfo` seeds server profile with conservative mappings:

- coins -> starter diamonds
- adventure progress -> initial MMR
- challenge records -> weekly score

---

## 9) Client sync runtime on top of server core (Phase 3)

Core files:

- `src/Lawn/System/ClientSessionRuntime.h`
- `src/Lawn/System/ClientSessionRuntime.cpp`
- `src/LawnApp.cpp`
- `docs/pvz_yg_phase3_client_sync_runtime.md`

### 9.1 Session state machine

- CONNECTING -> MATCHMAKING -> LOBBY -> SYNCED_PLAYING -> TERMINATED

### 9.2 Loopback integration

- client boots loopback `AuthoritativeServerRuntime`,
- binds to lobby once assigned,
- reads authoritative snapshot for local player.

### 9.3 Gameplay command hooks

- plant placement/removal commands are submitted to authoritative runtime,
- authoritative sun snapshot is applied to board state,
- eliminated authoritative state drives match-loss behavior.

---

## 10) Yandex SDK bridge and server-facing progression effects (Phase 5)

Core files:

- `src/Lawn/System/YandexSdkBridge.h`
- `src/Lawn/System/YandexSdkBridge.cpp`
- `src/LawnApp.cpp`
- `docs/pvz_yg_phase5_yandex_sdk_bridge.md`

### 10.1 Bridge surface

- initialize/update lifecycle
- ready state
- leaderboard submit
- purchase request
- event queue

### 10.2 Runtime effects

- payment success event currently credits authoritative diamonds,
- on terminated session + ready SDK, submits global/weekly leaderboard scores using authoritative progression values.

---

## 11) Deterministic simulation foundation (Phase 1)

Core files:

- `src/Lawn/System/DeterministicSim.h`
- `src/Lawn/System/DeterministicSim.cpp`
- `docs/pvz_yg_phase1_deterministic_sim.md`

Why it matters for server:

- deterministic sun/wave helpers isolate authority math,
- replay recorder + hash checkpoints provide drift diagnostics,
- this is the parity foundation for long-term server/client deterministic alignment.

---

## 12) Release-gate and telemetry support (Phase 9)

Core files:

- `src/Lawn/System/RuntimeTelemetry.h`
- `src/Lawn/System/RuntimeTelemetry.cpp`
- `tools/ci/check_protocol_compat.py`
- `docs/pvz_yg_phase9_release_gates.md`

### 12.1 Runtime telemetry

- frame count and frame-time buckets (<=16ms, <=33ms, >33ms),
- average and max frame time snapshots.

### 12.2 Protocol compatibility gate

`tools/ci/check_protocol_compat.py` verifies code version and phase-0 documented protocol version stay synchronized.

---

## 13) New local/cloud server launch workflow

### 13.1 Build + run script

Use:

- `./scripts/run_authoritative_server.sh local`
- `./scripts/run_authoritative_server.sh cloud`
- `./scripts/run_authoritative_server.sh local --build-only` (build only, no run)

The script:

1. configures CMake in `build-server` by default,
2. builds `pvz-authoritative-server`,
3. launches with profile-appropriate defaults.

Profile behavior:

- `local`: runs continuously by default (stop with `Ctrl+C`).
- `cloud`: runs continuously by default.

Windows launch note:

- if launched from a shell window that auto-closes on script exit, use `PAUSE_ON_ERROR=1` (default on Windows-like shells) and check `build-server/run_authoritative_server.log` for exact failure details.

### 13.2 Override examples

- Local short run:
  - `./scripts/run_authoritative_server.sh local --duration-seconds 10 --players 8`
- Cloud long-running:
  - `./scripts/run_authoritative_server.sh cloud --mode mmr --players 100 --duration-seconds 0`

### 13.3 Environment overrides

- `BUILD_DIR` (default `build-server`)
- `CMAKE_BUILD_TYPE` (default `Release`)
- `CMAKE_GENERATOR` (default `Ninja`)
- `BUILD_PVZ_PORTABLE` (default `OFF` in launcher for server-only builds)
- `TOOLCHAIN_FILE` (optional, for cross-target builds such as MinGW Windows x64)
- `SERVER_EXTRA_ARGS` (appended to executable args)

### 13.4 Runner executable flags

`pvz-authoritative-server` supports:

- `--profile local|cloud`
- `--tick-rate N`
- `--duration-seconds N` (0 = run forever)
- `--players N`
- `--mmr-base N`
- `--mode random|mmr`
- `--players-per-lobby N`
- `--min-players-to-start N`
- `--bot-fill-after-ticks N`
- `--pvp-max-zombies N`
- `--log-every-ticks N`
- `--disable-sleep`

### 13.5 Windows x64 artifact on repository

For Windows-targeted server delivery, the repository can carry:

- `build-server/pvz-authoritative-server.exe`

This artifact can be produced with the MinGW toolchain file:

- `cmake/toolchains/mingw-w64-x86_64.cmake`

Runtime note:

- current Windows artifact is linked with static MinGW C++ runtime (`-static-libgcc -static-libstdc++`), so `libgcc_s_seh-1.dll` and `libstdc++-6.dll` are not required beside the executable.

---

## 14) Current scope boundaries (important)

What exists now:

- authoritative runtime engine with validation, matchmaking, PvP phases, anti-cheat signals, bot replay, progression scaffolding, and integration hooks.

What is not yet a full external service stack:

- no real network transport daemon is wired in this binary,
- no persistent DB backend for progression in this runner,
- no production auth/session service layer in this runner.

So today this is a **runtime-correct server core + operational runner**, ready to be embedded behind real transport/persistence layers.
