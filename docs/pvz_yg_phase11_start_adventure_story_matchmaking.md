# PvZ YG Phase 11 — Start Adventure Uses Story Matchmaking

## 1) Goal

When the player clicks **Start Adventure**, the game should launch the regular story mode and begin opponent search.

Behavior requirements:

1. Opponent count must match the server-configured value.
2. If not enough human opponents are found, fill remaining slots with bots from the bot track pool.
3. Each bot must replay actions from a track (for example: `place plant: x, y, plant type`).
4. If there are not enough tracks in the pool to fill missing opponents, start the game with no opponents and disable PvP waves.

---

## 2) Current baseline

- `GameSelector::ClickedAdventure()` currently starts the local adventure launch animation and later calls `PreNewGame(GameMode::GAMEMODE_ADVENTURE, ...)`.
- Authoritative matchmaking and bot track replay exist in:
  - `src/Lawn/System/AuthoritativeRuntime.h`
  - `src/Lawn/System/AuthoritativeRuntime.cpp`
  - `src/Lawn/System/BotActionTrack.h`
  - `src/Lawn/System/BotActionTrack.cpp`
- Bot tracks already contain replayable actions and are assigned at match creation.

This phase defines how story launch should consume that runtime path.

---

## 3) New product behavior

### 3.1 Start Adventure flow

Clicking **Start Adventure** must no longer directly imply a pure solo start.  
It must execute this flow:

1. Enter story matchmaking state.
2. Search for human opponents.
3. Resolve final opponent roster according to server config and fallback rules.
4. Start story board with one of:
   - full opponent roster (humans and/or bots),
   - or solo story mode with PvP waves disabled (degraded fallback).

### 3.2 Opponent target count

Add an explicit server config field:

- `mStoryOpponentCount` (opponents only, does not include local player).

Derived values:

- `targetPlayersInStoryLobby = 1 + mStoryOpponentCount`
- `targetOpponents = mStoryOpponentCount`

### 3.3 Opponent resolution policy

At story matchmaking resolution:

1. Use all found human opponents up to `targetOpponents`.
2. Compute `missing = targetOpponents - foundHumans`.
3. If `missing == 0`: start with humans only.
4. If `missing > 0`:
   - query bot track pool size,
   - if pool has at least `missing` available tracks:
     - create `missing` bots,
     - assign one unique track per bot,
     - start story with mixed roster,
   - else:
     - discard all opponents for this run,
     - start solo story mode,
     - force PvP waves disabled for the full match.

### 3.4 PvP gating rule

PvP waves are allowed only if there is at least one opponent in the final roster.

If final roster has zero opponents, runtime must:

- skip PvP phase scheduling,
- reject any PvP-send commands as unsupported in this match mode,
- keep regular PvE/story progression active.

---

## 4) Runtime and API changes

## 4.1 Config surface

`AuthoritativeServerConfig` additions:

- `uint32_t mStoryOpponentCount`
- optional: `uint64_t mStorySearchTimeoutTicks` (if separate from existing timeout is needed)

`mPlayersPerLobby` remains available for generic matchmaking, but story launch should derive expected player count from `mStoryOpponentCount`.

### 4.2 Matchmaking additions

Add a story-specific entry point (name can vary):

- `TryBuildStoryLobby(...)`:
  - binds one local player as anchor,
  - searches human queue for up to `mStoryOpponentCount`,
  - applies fallback logic described in section 3.3,
  - returns resolved roster and metadata:
    - `opponentCount`,
    - `hasBots`,
    - `pvpEnabled`.

### 4.3 Bot track assignment policy update

For story fallback, assignment must be **strict unique**:

- no track duplication for this fallback path,
- if unique tracks are insufficient, fallback to solo/no-PvP mode.

This differs from current generic behavior where duplicates may be cycled if pool is smaller than required count.

### 4.4 Runtime flags

Add a per-match runtime flag:

- `mPvpEnabled` (default true).

At match init:

- set `mPvpEnabled = (resolvedOpponentCount > 0)`.

At tick:

- `BeginPvpPhase()` and schedule checks must early-return when `mPvpEnabled == false`.

---

## 5) Client integration changes

## 5.1 Game selector

`GameSelector::ClickedAdventure()` should keep current UX animation/audio, but hand off to story matchmaking startup rather than direct solo start.

Expected handoff shape:

- `LawnApp::StartStoryModeWithOpponentSearch()` (new orchestration method).

### 5.2 LawnApp orchestration

`LawnApp` should own story launch state transitions:

1. `STORY_MATCH_SEARCHING`
2. `STORY_MATCH_RESOLVED`
3. `STORY_LOADING_BOARD`
4. `STORY_RUNNING`

Resolution payload passed to board/session setup:

- final opponent roster (IDs + bot track IDs),
- `pvpEnabled` boolean.

### 5.3 Board/session setup

Board startup must read `pvpEnabled` and avoid scheduling PvP waves when false.

---

## 6) Telemetry and logs

Add structured events for observability:

1. `story_matchmaking_started`
2. `story_matchmaking_resolved`
   - fields: `target_opponents`, `human_found`, `bots_added`, `pvp_enabled`, `fallback_reason`
3. `story_matchmaking_fallback_solo`
   - fields: `required_tracks`, `available_tracks`
4. `story_match_started`

This ensures the fallback behavior can be audited in production.

---

## 7) Edge cases

1. **Server config sets `mStoryOpponentCount = 0`**
   - start solo story immediately with `pvpEnabled = false`.

2. **Search timeout with partial humans**
   - fill missing with bots only if enough unique tracks exist.
   - otherwise use solo/no-PvP fallback.

3. **Bot track referenced but missing at runtime**
   - treat as failed bot assignment for that launch and apply solo/no-PvP fallback before match start.

4. **Reconnect / resume**
   - persisted session metadata must include `pvpEnabled` so resumed matches keep consistent behavior.

---

## 8) Acceptance criteria

1. Clicking Start Adventure always enters story matchmaking path.
2. Final opponent count equals `mStoryOpponentCount` when humans+bots can satisfy it.
3. Every fallback bot has a valid assigned action track.
4. If track pool cannot satisfy missing opponents, story starts with zero opponents.
5. Zero-opponent story matches run with PvP waves fully disabled.
6. Structured logs clearly report which resolution path was used.

---

## 9) Suggested implementation touchpoints

- `src/Lawn/Widget/GameSelector.cpp`
  - update Start Adventure handoff target.
- `src/LawnApp.h` / `src/LawnApp.cpp`
  - add story matchmaking orchestration API/state.
- `src/Lawn/System/AuthoritativeRuntime.h` / `.cpp`
  - add story opponent config + strict fallback resolution + `mPvpEnabled` gating.
- `src/Lawn/System/BotActionTrack.h` / `.cpp`
  - expose unique-only assignment helper for story fallback path.

