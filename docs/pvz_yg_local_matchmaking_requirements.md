# PvZ YG — Local Matchmaking Test Analysis and Requirements

## 1) Goal

Define a concrete local-run spec for matchmaking that supports:

1. one playable local user,
2. bot opponents from bot action tracks,
3. synthetic players for deterministic local load/testing.

This document first records what is already implemented, then lists missing pieces, then defines requirements for a complete local setup.

---

## 2) What is already implemented

## 2.1 Authoritative matchmaking/runtime core

Implemented in `AuthoritativeRuntime`:

- random + MMR queues,
- lobby build with bot fill,
- fixed authoritative tick loop,
- PvP phase scheduling and validation,
- match start/end and runtime events.

Primary files:

- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`

## 2.2 Story matchmaking entry path from Start Adventure

Start Adventure now routes through story-matchmaking startup:

- `GameSelector` calls `LawnApp::StartStoryModeWithOpponentSearch(...)`.
- `LawnApp` triggers `ClientSessionRuntime::StartStoryMatchmaking()`.
- client then starts adventure board flow with `PreNewGame(...)`.

Primary files:

- `src/Lawn/Widget/GameSelector.cpp`
- `src/LawnApp.cpp`
- `src/Lawn/System/ClientSessionRuntime.h`
- `src/Lawn/System/ClientSessionRuntime.cpp`

## 2.3 Story opponent resolution and PvP fallback behavior

Implemented in story lobby build:

- target opponent count from `mStoryOpponentCount`,
- use available humans first,
- fill missing opponents with strict-unique bot tracks,
- if strict-unique track assignment fails, fallback to local-only (no opponents) and disable PvP.

Additionally:

- match runtime carries `mPvpEnabled`,
- PvP commands are rejected when PvP is disabled,
- PvP phase scheduler is gated by `mPvpEnabled`.

Primary files:

- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`
- `src/Lawn/System/BotActionTrack.h`
- `src/Lawn/System/BotActionTrack.cpp`

## 2.4 Local test execution surfaces

Two local testing surfaces already exist:

1. **Loopback in playable client** (enable with `-authoritative`) through `ClientSessionRuntime`.
2. **Headless dedicated runtime runner** via:
   - `pvz-authoritative-server` (`src/authoritative_server_main.cpp`)
   - launcher script `scripts/run_authoritative_server.sh`
   - synthetic player enqueue (`--players`).

---

## 3) What is missing for full local implementation

For the specific target ("solo player with bots and synthetic players"), the following gaps remain:

## M-01: No real client-to-dedicated-server transport

Current client path is loopback runtime only.  
Headless server path is separate and does not accept real client connections yet.

Impact: cannot currently run one playable client against a dedicated local runtime process with synthetic players in the same match.

## M-02: No hybrid local mode (playable user + synthetic players in same runtime instance)

`ClientSessionRuntime::StartStoryMatchmaking()` recreates a local loopback server and only starts local user story matchmaking.  
There is no built-in prefill/injection of additional synthetic humans in that same runtime before story resolution.

Impact: local user path effectively resolves to local user + bots (or local user only fallback), but not local user + synthetic humans + bots in one integrated match.

## M-03: Story search timing is immediate, not a true timed search phase

Story lobby resolution currently happens directly in `BuildStoryLobby(...)` from current queue state.  
There is no dedicated wait/search timeout state for collecting humans over time before fallback.

Impact: behavior is deterministic but does not emulate realistic matchmaking wait windows for local validation.

## M-04: Limited local config surface for story-specific testing

`mStoryOpponentCount` exists, but local operator-facing controls are limited (no dedicated server CLI argument for story-opponent count and no clear user-facing local test profile switching in client).

Impact: scenario-driven local tests require source edits or indirect config coupling (`playersPerLobby - 1`).

## M-05: Missing story-specific structured observability contract in runtime output

Existing events/logging are present, but dedicated structured events for:

- story matchmaking started,
- story matchmaking resolved (humans/bots/fallback reason),
- fallback solo reason,

are not formalized in runtime output contract.

Impact: hard to assert local scenario correctness quickly from logs.

## M-06: Missing single documented "one-command" local test matrix

There is no single source doc that defines exact local scenarios and pass/fail checks for:

- solo + bots,
- synthetic-only stress,
- hybrid playable + synthetic + bots.

Impact: setup is possible but not standardized/repeatable for contributors.

---

## 4) Requirements for a complete local machine run

## 4.1 Functional requirements

- **FR-01**: system MUST support local playable user starting story mode through matchmaking path.
- **FR-02**: system MUST support local bot fill from track pool with strict-unique story assignment.
- **FR-03**: system MUST support local synthetic player generation configurable by count/MMR/mode.
- **FR-04**: system MUST support hybrid local run where one playable user and synthetic players participate in the same authoritative runtime instance.
- **FR-05**: if missing opponents cannot be filled with strict-unique tracks, match MUST start with zero opponents and PvP disabled.
- **FR-06**: if final opponent count is zero, runtime MUST skip PvP phase scheduling and reject PvP send commands.

## 4.2 Configuration requirements

- **CR-01**: expose story-opponent count as explicit runtime option for local runs.
- **CR-02**: expose synthetic player count, MMR base, matchmaking mode, and optional enqueue cadence.
- **CR-03**: expose profile presets for:
  - `solo_bots`,
  - `synthetic_stress`,
  - `hybrid_local`.
- **CR-04**: support deterministic seed override for bot track assignment and synthetic player generation.

## 4.3 Local orchestration/tooling requirements

- **TR-01**: provide one local launcher entry that can run:
  - playable client loopback mode,
  - dedicated runtime mode,
  - hybrid mode.
- **TR-02**: launcher MUST print exact resolved config at start.
- **TR-03**: launcher MUST write separate logs for build output and runtime events.
- **TR-04**: launcher MUST allow bounded-duration runs for CI/local smoke.

## 4.4 Observability requirements

- **OR-01**: emit story matchmaking lifecycle events:
  - `story_matchmaking_started`,
  - `story_matchmaking_resolved`,
  - `story_matchmaking_fallback_solo`,
  - `story_match_started`.
- **OR-02**: `story_matchmaking_resolved` MUST include:
  - target opponents,
  - human found,
  - bots added,
  - `pvp_enabled`,
  - fallback reason (if any).
- **OR-03**: log per-bot assigned track ID for every match start.
- **OR-04**: include local run mode tag (`solo_bots`, `synthetic_stress`, `hybrid_local`) in logs.

## 4.5 Local acceptance criteria

- **AC-01 (solo+bots)**: Start Adventure on local client in authoritative mode results in one local user + expected bot count, with valid bot track assignments and PvP enabled when opponents > 0.
- **AC-02 (fallback)**: when configured required opponents exceed strict-unique track availability, system starts solo and PvP remains disabled for entire run.
- **AC-03 (synthetic)**: dedicated runtime with synthetic players forms lobbies and emits stable match lifecycle logs.
- **AC-04 (hybrid)**: one playable local user plus configured synthetic players can co-exist in same runtime/lobby and progress through at least one complete match lifecycle.
- **AC-05 (determinism)**: same seed + same config yields stable bot track assignment order and consistent lobby composition.

---

## 5) Implementation checklist (recommended touchpoints)

1. **Hybrid local orchestration**
   - Extend `ClientSessionRuntime` / `LawnApp` startup path to optionally pre-enqueue synthetic players before story resolution.
   - Files:
     - `src/Lawn/System/ClientSessionRuntime.h`
     - `src/Lawn/System/ClientSessionRuntime.cpp`
     - `src/LawnApp.h`
     - `src/LawnApp.cpp`

2. **Story local config surface**
   - Add explicit story-opponent and synthetic controls to local startup config path and/or dedicated server CLI.
   - Files:
     - `src/authoritative_server_main.cpp`
     - `scripts/run_authoritative_server.sh`
     - optionally client startup param parsing in `src/LawnApp.cpp`

3. **Story observability**
   - Add structured story lifecycle events in authoritative runtime output.
   - Files:
     - `src/Lawn/System/AuthoritativeRuntime.h`
     - `src/Lawn/System/AuthoritativeRuntime.cpp`

4. **Local run playbook**
   - Add one operator-focused doc with exact commands and expected log markers for AC-01..AC-05.
   - Suggested file:
     - `docs/pvz_yg_local_matchmaking_playbook.md`

