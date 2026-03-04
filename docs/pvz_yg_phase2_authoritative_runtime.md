# PvZ YG Phase 2 — Authoritative Server Runtime (Minimal)

## 1) Scope

Phase 2 delivers a minimal server-authoritative runtime layer in C++ with:

- matchmaking (random + MMR window),
- lobby formation with bot fill up to 10 players,
- fixed-tick match runtime,
- PvP phase scheduling with role swapping between phases,
- command queue validation via Phase-0 protocol,
- server-side limits for PvP send actions,
- immediate elimination on disconnect.

Implementation files:

- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`

---

## 2) New Core Types

### 2.1 Config and matchmaking

- `AuthoritativeServerConfig`
- `AuthoritativeMatchmakingMode`
- `AuthoritativeMatchmakingRequest`
- `AuthoritativeLobbyMember`
- `AuthoritativeLobby`

### 2.2 Match runtime state

- `AuthoritativePlayerState`
- `AuthoritativeRuntimeEvent`
- `AuthoritativeMatchRuntime`

### 2.3 Server orchestration

- `AuthoritativeMatchmaker`
- `AuthoritativeServerRuntime`

---

## 3) Implemented Runtime Behavior

### 3.1 Matchmaking

- Supports two queues:
  - random queue,
  - MMR queue (`|mmr - anchorMmr| <= mmrWindow`).
- Forms full lobbies whenever enough players are available.
- On queue timeout, starts lobby with available players and fills the rest with bots.
- Prevents duplicate queue entries for the same player ID.

### 3.2 Lobby/session lifecycle

- `AuthoritativeServerRuntime::AdvanceOneTick()`:
  1. increments server tick,
  2. pulls ready lobbies from matchmaker,
  3. creates active `AuthoritativeMatchRuntime` instances,
  4. advances each active match one tick,
  5. collects events,
  6. removes finished matches.

### 3.3 Fixed-tick authoritative match runtime

`AuthoritativeMatchRuntime::AdvanceOneTick()` handles:

- authoritative sun generation on interval,
- authoritative pending damage application with per-tick cap,
- PvP phase begin/end checks,
- queued command processing,
- match end detection.

### 3.4 PvP phase scheduling and role swap

- PvP phase starts every configured interval.
- Alive players are deterministically paired by player ID order.
- Each pair has one attacker and one defender.
- Roles swap on the next phase.
- During a phase, attacker can only target the authoritative paired defender.

### 3.5 Command validation and idempotency

- Uses Phase-0 `NetValidateClientCommand`.
- Enforces match/player ownership in runtime.
- Deduplicates commands using idempotency key string (`match/player/command/tick`).
- Emits accepted/rejected events with reasons.

### 3.6 Authoritative limits

- Plant placement checks authoritative sun before accepting.
- PvP zombie send checks:
  - phase must be active,
  - attacker role must be valid,
  - target must match authoritative pairing,
  - per-phase send cap must not be exceeded.

### 3.7 Disconnect handling

- `DisconnectPlayer` marks player disconnected and immediately eliminated.
- Player mapping is removed from routing map.
- Match end condition reevaluates after elimination.

---

## 4) Event Surface

Runtime emits `AuthoritativeRuntimeEvent` with:

- event type (`NetEventType`),
- match ID,
- tick,
- player ID,
- error code (`NetProtocolError`),
- details string.

Events include:

- match start/end,
- PvP phase start/end,
- command accepted/rejected.

---

## 5) Phase-2 Exit Criteria Mapping

Phase 2 requirements satisfied by this implementation:

1. **Matchmaking + lobby manager**: implemented with random/MMR queues and bot fill.
2. **Fixed authoritative tick loop**: implemented via `AdvanceOneTick`.
3. **PvP phase sync skeleton**: implemented with deterministic pairing and role swap.
4. **Server-side command validation**: implemented with protocol validation + runtime constraints.
5. **Spawn limit enforcement**: implemented with per-phase PvP send cap.
6. **Disconnect = immediate elimination**: implemented and integrated with match end check.

---

## 6) Intended Phase-3 Handoff

Phase 3 can wire transport and client integration on top of this runtime:

- feed network-decoded `NetClientCommand` into `SubmitCommand`,
- consume `AuthoritativeRuntimeEvent` as outbound server event stream,
- serialize authoritative player/match state snapshots for clients.
