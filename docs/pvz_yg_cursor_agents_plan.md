# PvZ YG — Cursor Agents Implementation Plan

## 1) Purpose

This document converts the technical roadmap into an execution plan optimized for Cursor agents.

Goal: deliver **PvZ YG** as a **server-authoritative web game** for Yandex Games while preserving core PvZ gameplay quality.

Primary product constraints:

- Web platform (Yandex Games)
- Russian-only player-facing localization
- Asynchronous PvP phases with role swapping
- Server-authoritative anti-cheat controls
- Server-side diamonds/MMR/cosmetics/weekly score
- Yandex SDK monetization and leaderboards
- Automatic audio pause on tab focus loss

---

## 2) Current Baseline (Code Reality)

Use these files as implementation anchors:

- Startup/web bootstrap:
  - `src/main.cpp`
  - `src/emscripten_fetch.cpp`
- App orchestration:
  - `src/LawnApp.h`
  - `src/LawnApp.cpp`
- Gameplay simulation:
  - `src/Lawn/Board.h`
  - `src/Lawn/Board.cpp`
- Persistence:
  - `src/Lawn/System/ProfileMgr.h`
  - `src/Lawn/System/ProfileMgr.cpp`
  - `src/Lawn/System/PlayerInfo.h`
  - `src/Lawn/System/PlayerInfo.cpp`
  - `src/Lawn/System/SaveGame.h`
  - `src/Lawn/System/SaveGame.cpp`
- Localization:
  - `src/Sexy.TodLib/TodStringFile.h`
  - `src/Sexy.TodLib/TodStringFile.cpp`
  - `src/SexyAppFramework/SexyAppBase.cpp`
- Focus/audio behavior:
  - `src/SexyAppFramework/platform/pc/Input.cpp`
  - `src/SexyAppFramework/SexyAppBase.h`
  - `src/SexyAppFramework/SexyAppBase.cpp`
  - `src/Lawn/System/Music.h`
  - `src/Lawn/System/Music.cpp`
  - `src/SexyAppFramework/sound/WebAudioSoundManager.cpp`
  - `src/SexyAppFramework/sound/WebMusicInterface.h`
  - `src/SexyAppFramework/sound/WebMusicInterface.cpp`
- Build and platform config:
  - `CMakeLists.txt`
- Product requirements source:
  - `docs/pvz_yg_gdd_technical_spec_ru.md`

---

## 3) Agent Operating Rules

All agents executing this plan should follow:

1. **Authority rule:** never trust client values for competitive state (sun, damage, PvP spawn budget, diamonds, MMR).
2. **Observability rule:** every feature/task must add or reuse structured logs and metrics.
3. **Compatibility rule:** avoid breaking current local modes unless explicitly intended.
4. **Increment rule:** ship in small, testable phases with passing checks at each gate.
5. **Localization rule:** player-visible production strings must be Russian-only (no hardcoded Chinese/English literals).
6. **Evidence rule:** no claim is accepted without runtime evidence (logs, tests, or manual walkthrough artifacts).

---

## 4) Delivery Strategy

### Phase 0 — Contracts and Boundaries

**Outcome:** protocol and authority boundaries are explicit.

Tasks:

- Define command/event protocol v1 (`common/protocol/` recommended).
- Add payload versioning and compatibility policy.
- Define authoritative ownership map:
  - Server: wave progression, sun grants, damage, PvP send limits, rewards.
  - Client: input capture, rendering, UX-only prediction.
- Add idempotency keys:
  - `command_id`, `player_id`, `match_id`, `tick`.

Exit criteria:

- Protocol document committed.
- Reject/accept rules documented for every gameplay command.

---

### Phase 1 — Simulation Extraction and Determinism

**Outcome:** deterministic simulation surface exists for server authority.

Tasks:

- Isolate gameplay authority segments from `Board`:
  - `UpdateSunSpawning`
  - `UpdateZombieSpawning`
  - `SpawnZombieWave`
  - wave counters and transition logic
- Standardize RNG seeding and ordering.
- Add deterministic replay harness:
  - same seed + commands => same state hash.

Exit criteria:

- Deterministic replay test passes repeatedly.
- Critical simulation state hash snapshot tool exists.

---

### Phase 2 — Authoritative Server Runtime

**Outcome:** minimal server can run matches with authoritative simulation.

Tasks:

- Create server services:
  - Matchmaking (random + MMR window)
  - Lobby/session manager (10 slots + bots fallback)
  - Authoritative tick loop
  - PvP phase scheduler + role swapping
- Implement disconnect handling per spec (immediate elimination).
- Implement authoritative validation:
  - sun generation
  - damage
  - PvP spawn quantity limits

Exit criteria:

- Headless server simulation runs end-to-end for a full match script.
- Validation rejects malformed/cheating commands with explicit error codes.

---

### Phase 3 — Client Networking Integration

**Outcome:** client gameplay is session-bound and synced to server authority.

Tasks:

- Add client network lifecycle after Emscripten resource fetch.
- Extend `LawnApp` with network states:
  - `CONNECTING`, `MATCHMAKING`, `LOBBY`, `SYNCED_PLAYING`, `TERMINATED`.
- Convert local-authority actions into command sends.
- Apply authoritative snapshots/deltas in `Board`.
- Keep prediction visual-only; server remains source of truth.

Exit criteria:

- Two-client + bot local test session completes without fatal desync.
- Command rejection paths are user-visible and recoverable.

---

### Phase 4 — Persistence Migration

**Outcome:** competitive progression is server-side authoritative.

Tasks:

- Server-side profile fields:
  - `id`, `diamonds`, `mmr`, `cosmeticsOwned`, `weeklyScore`
- Keep local files (`users.dat`, `user*.dat`, save files) for non-authoritative/local UX only.
- Add one-time migration/import flow from local profile where applicable.
- Define conflict policy (server wins, merge, or first-login import lock).

Exit criteria:

- Diamonds/MMR/cosmetics/weekly score are never final-written by client.
- Migration is replay-safe and idempotent.

---

### Phase 5 — Yandex SDK Integration

**Outcome:** SDK-backed payments and leaderboards are production-ready.

Tasks:

- Add SDK bridge module (JS + C++ facade):
  - init/readiness
  - optional auth
  - payment callbacks
  - leaderboard submit/read APIs
- Implement purchase flow:
  - client intent -> server verify -> SDK confirm -> server grant.
- Add global + weekly leaderboard submissions.

Exit criteria:

- Successful purchase simulation with grant audit trail.
- Successful leaderboard write/read with fallback retry logic.

---

### Phase 6 — Anti-Cheat and Trust Controls

**Outcome:** actionable anti-cheat pipeline with enforcement.

Tasks:

- Event detectors:
  - resource gain anomalies
  - action rate limit exceed
  - critical desync markers
- Enforcement ladder:
  - warn -> shadow limit -> queue isolate -> review/ban.
- Add anti-cheat audit records with timestamps and correlation IDs.

Exit criteria:

- Anti-cheat events are queryable and linked to match/player IDs.
- Automatic mitigation exists for at least one high-confidence abuse class.

---

### Phase 7 — UX/Localization Compliance

**Outcome:** moderation/platform requirements are met.

Tasks:

- Set `mMuteOnLostFocus = true` in web production path.
- Replace hardcoded non-Russian UI literals with string keys.
- Enforce Russian-only production string bundle.
- Resolve case-sensitive path inconsistencies (`properties` vs `Properties`) for web hosting.

Exit criteria:

- Focus-loss audio pause validated in browser.
- No hardcoded player-visible non-Russian text in production path.

---

### Phase 8 — Web Audio + Runtime Hardening

**Outcome:** stable browser audio and scene transitions.

Tasks:

- Improve `WebMusicInterface` semantics:
  - implement `IsPlaying`
  - define practical `GetMusicOrder` behavior (or safe fallback)
  - handle async decode readiness before play
- Add telemetry around decode/play/resume failures.
- Validate behavior on tab blur/focus and scene transitions.

Exit criteria:

- No stuck/ghost music after pause/resume cycles in browser tests.
- Audio failures are observable via logs/metrics.

---

### Phase 9 — Performance, Scale, and Release Gates

**Outcome:** release candidate is stable and measurable.

Tasks:

- Validate startup and FPS targets against spec:
  - startup bundle target
  - desktop/mobile FPS targets
- Load test server matchmaking + tick throughput.
- Add protocol compatibility CI checks.

Exit criteria:

- Performance reports attached.
- CI blocks incompatible protocol/schema changes.

---

## 5) Logging & Observability Specification

## 5.1 Structured Event Contract

Every log/event should include:

- `timestamp`
- `level`
- `service` (`client`, `matchmaker`, `sim`, `economy`, `anti_cheat`, `sdk`)
- `trace_id`
- `match_id`
- `lobby_id`
- `player_id`
- `tick`
- `wave`
- `build_sha`
- `protocol_version`
- `event_name`
- `payload` (JSON object)

## 5.2 Event Families (minimum)

1. Match lifecycle:
   - `matchmaking_enter`, `match_found`, `lobby_filled`, `match_start`, `match_end`
2. Authority pipeline:
   - `command_received`, `command_accepted`, `command_rejected`, `snapshot_sent`
3. Economy:
   - `sun_grant_authoritative`, `diamond_reward_calculated`, `diamond_write`
4. PvP:
   - `pvp_phase_start`, `pvp_phase_end`, `pvp_spawn_attempt`, `pvp_spawn_rejected`
5. Anti-cheat:
   - `ac_resource_anomaly`, `ac_rate_limit`, `ac_critical_desync`
6. SDK:
   - `sdk_init`, `sdk_payment_start`, `sdk_payment_result`, `sdk_leaderboard_submit`
7. Client runtime:
   - `focus_lost`, `focus_gained`, `audio_suspend`, `audio_resume`, `fps_bucket`, `frame_stall`

## 5.3 Metrics and Alerts

Track:

- Tick latency p50/p95/p99
- Command reject ratio
- Snapshot size p95
- Desync rate
- Payment success rate
- Leaderboard submit success rate
- Crash-free sessions

Alert examples:

- sustained high tick latency
- desync spike
- payment failure spike
- anti-cheat critical event spike

---

## 6) Risks and Bug Vectors

1. Determinism drift between client/server simulation paths.
2. Legacy local save/profile assumptions leaking into ranked mode.
3. Localization regressions from hardcoded literals or broken string packs.
4. Browser audio edge cases during async decode and tab visibility transitions.
5. Protocol version mismatch during staged deployments.
6. Yandex SDK callback race conditions causing duplicate grants.
7. Case-sensitive resource path issues in different hosting environments.

Mitigate each risk with:

- reproducible tests,
- structured logs,
- kill switches,
- rollout flags,
- explicit fallback UX.

---

## 7) Definition of Done (per feature)

A feature is done only if:

1. Functional behavior works end-to-end in runtime tests.
2. Structured logs and metrics are present and validated.
3. Failure paths are handled and user-visible where needed.
4. Protocol/schema compatibility impact is documented.
5. Relevant automated tests are added/updated.
6. If UI/web behavior changed, manual browser validation evidence is captured.

---

## 8) Agent Work Breakdown (Parallelizable)

Use up to 4 Cursor agents in parallel:

1. **Simulation Agent**
   - Determinism extraction, replay harness, state hash checkpoints.
2. **Backend Agent**
   - Matchmaking/session runtime, authoritative validation, anti-cheat.
3. **Client Integration Agent**
   - Network lifecycle in `LawnApp`, snapshot application in `Board`, UX states.
4. **Platform/SDK Agent**
   - Yandex SDK bridge, payments, leaderboards, web audio/focus compliance.

Coordination:

- Shared protocol branch or schema package.
- Daily integration checkpoint.
- Merge order: protocol -> server -> client -> SDK/compliance -> stabilization.

---

## 9) Suggested Cursor Agent Prompts

### Prompt A — Simulation extraction

"Extract authoritative simulation boundaries from `Board` (`UpdateSunSpawning`, `UpdateZombieSpawning`, `SpawnZombieWave`) into a deterministic core. Add replay/state-hash tests and document any nondeterministic paths."

### Prompt B — Server runtime

"Implement a minimal authoritative server runtime for matchmaking + lobby + fixed-tick simulation loop + command validation (sun, damage, PvP spawn limits). Include structured logging and rejection reasons."

### Prompt C — Client sync integration

"Integrate client networking into `LawnApp` and `Board`, replacing local authority with server command/snapshot flow while preserving UI responsiveness. Add desync diagnostics and recovery UX."

### Prompt D — SDK and compliance

"Implement Yandex SDK bridge for init, payments, and global/weekly leaderboards. Enforce Russian-only production text and tab-focus audio pause behavior for web builds."

---

## 10) Immediate Sprint Backlog (Execution Order)

1. Protocol v1 and validation table.
2. Deterministic replay harness.
3. Minimal authoritative tick server.
4. Client command transport + snapshot apply.
5. Server-side progression storage for diamonds/MMR/cosmetics/weekly score.
6. Yandex SDK payment + leaderboard integration.
7. Anti-cheat event detectors and alerting.
8. Localization and focus-audio compliance pass.
9. Performance/load test report and release gate checklist.

---

## 11) Notes for Future Extensions

Design now for:

- ranked seasons,
- new plant/zombie tiers,
- event modifiers,
- cooperative mode.

Keep protocol/versioning and service boundaries extensible to avoid rewrite debt.
