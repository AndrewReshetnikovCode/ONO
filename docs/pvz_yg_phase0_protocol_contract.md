# PvZ YG Phase 0 — Protocol and Authority Contract (v1)

## 1) Scope

This document defines the implementation contract for Phase 0:

1. command/event protocol v1,
2. version compatibility policy,
3. authoritative ownership matrix,
4. idempotency-key requirements,
5. command validation/rejection rules.

Related implementation module:

- `src/Lawn/System/NetProtocol.h`
- `src/Lawn/System/NetProtocol.cpp`

---

## 2) Protocol Version

Current server/client protocol target:

- `major = 1`
- `minor = 0`

Compatibility policy:

- If `client.major == server.major` and `client.minor <= server.minor`: compatible.
- If `client.major < server.major`: client upgrade required.
- If `client.major > server.major`: server upgrade required.
- If `client.major == server.major` and `client.minor > server.minor`: server upgrade required.

---

## 3) Command Envelope (required for all commands)

Fields:

- `version` (`major`, `minor`)
- `matchId` (`uint64`, non-zero)
- `playerId` (`uint64`, non-zero)
- `commandId` (`uint64`, non-zero)
- `clientTick` (`uint32`)
- `commandType` (enum)

Validation:

- unsupported version => reject
- zero `matchId/playerId/commandId` => reject
- invalid command type => reject

---

## 4) Client Command Types (v1)

1. `NET_COMMAND_PLACE_PLANT`
   - payload: `gridX`, `gridY`, `seedType`, `imitaterSeedType`
2. `NET_COMMAND_REMOVE_PLANT`
   - payload: `gridX`, `gridY`
3. `NET_COMMAND_SEND_PVP_ZOMBIES`
   - payload: `targetPlayerId`, `zombieType`, `zombieCount`
4. `NET_COMMAND_STORE_PURCHASE_INTENT`
   - payload: `sku`, `quantity`
5. `NET_COMMAND_HEARTBEAT`
   - payload: `clientFrame`

Payload mismatch rule:

- command type and payload type must match exactly, otherwise reject.

Payload validation rules in v1:

- place/remove: non-negative grid coordinates.
- place: non-negative `seedType`; `imitaterSeedType >= -1`.
- send PvP zombies:
  - non-zero target player,
  - target player must not equal sender,
  - `zombieType >= 0`,
  - `1 <= zombieCount <= 100`.
- store purchase intent:
  - non-empty sku,
  - `sku.length <= 64`,
  - `1 <= quantity <= 100`.

---

## 5) Server Event Types (v1)

Defined event names:

- `NET_EVENT_COMMAND_ACCEPTED`
- `NET_EVENT_COMMAND_REJECTED`
- `NET_EVENT_SNAPSHOT_APPLY`
- `NET_EVENT_PVP_PHASE_START`
- `NET_EVENT_PVP_PHASE_END`
- `NET_EVENT_MATCH_START`
- `NET_EVENT_MATCH_END`
- `NET_EVENT_DIAMONDS_UPDATED`

Note: event transport schema will be finalized in later phases; this phase establishes canonical names and ownership semantics.

---

## 6) Idempotency Key Contract

Idempotency key fields:

- `matchId`
- `playerId`
- `commandId`
- `clientTick`

Generation:

- generated from command envelope.

Validation:

- valid only when `matchId`, `playerId`, `commandId` are non-zero.

Server expectation:

- repeated command with same key must be treated as duplicate-safe (same effective outcome or no-op with prior result reference).

---

## 7) Authority Ownership Matrix

Server-authoritative domains:

- wave progression
- sun generation
- damage resolution
- PvP spawn limits
- reward calculation
- MMR calculation
- diamonds storage
- cosmetics ownership
- weekly score

Client-owned domains:

- input capture
- rendering
- local visual prediction

Rule:

- client can propose intents only for server-authoritative domains; it must not finalize outcomes.

---

## 8) Phase-0 Exit Criteria Mapping

Phase-0 requirements are satisfied when:

1. protocol v1 command/event enums and envelopes are implemented,
2. compatibility rules are implemented,
3. authority ownership mapping is implemented,
4. idempotency key generation/validation is implemented,
5. command/payload validation and reject reasons are implemented,
6. all of the above are documented in this contract.
