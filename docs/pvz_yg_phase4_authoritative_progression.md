# PvZ YG Phase 4 — Authoritative Progression Storage and Migration

## Scope

Phase 4 establishes server-authoritative progression shape and one-time migration from local `PlayerInfo`.

Implemented files:

- `src/Lawn/System/AuthoritativeProgressionStore.h`
- `src/Lawn/System/AuthoritativeProgressionStore.cpp`
- `src/LawnApp.cpp`

## Implemented behavior

1. Authoritative profile model:
   - `playerId`
   - `diamonds`
   - `mmr`
   - `weeklyScore`
   - `cosmeticsOwned`
   - `schemaVersion`
   - `importedFromLocal` marker

2. Progression store operations:
   - profile get/create and lookup
   - diamonds add/subtract with non-negative guard
   - MMR update
   - weekly score update
   - cosmetic grant/list

3. Local-to-authoritative migration:
   - One-time import path from `PlayerInfo`.
   - Conservative seed mapping:
     - coins -> starter diamonds ratio
     - adventure progress -> initial MMR bias
     - challenge records -> weekly score seed
   - migration lock avoids repeated re-import when enabled.

4. App integration:
   - `LawnApp::Init()` creates progression store.
   - Current profile migrates at startup if available.
   - `InitClientSessionRuntime()` uses authoritative MMR when present.

## Phase-4 handoff

Backend persistence can replace in-memory storage with DB-backed profile service while keeping current model and migration semantics.
