# PvZ YG Phase 5 — Yandex SDK Bridge

## Scope

Phase 5 introduces a dedicated SDK bridge for Yandex Games integration points.

Implemented files:

- `src/Lawn/System/YandexSdkBridge.h`
- `src/Lawn/System/YandexSdkBridge.cpp`
- `src/LawnApp.cpp`

## Implemented behavior

1. Bridge abstraction:
   - initialize/poll lifecycle
   - readiness state
   - leaderboard submit API
   - purchase request API
   - event queue

2. Emscripten JS bridge:
   - async `YaGames.init()` bootstrap
   - SDK readiness propagation
   - leaderboard submit wiring
   - purchase request wiring
   - JS-to-C++ event queue polling

3. Event types:
   - SDK ready
   - payment success/failure
   - leaderboard submit success/failure

4. App integration:
   - `LawnApp::Init()` initializes bridge.
   - `LawnApp::UpdateClientSessionRuntime()` polls SDK events.
   - payment success event credits authoritative diamonds.
   - on terminated session state, submits global/weekly leaderboard scores from authoritative progression.

## Phase-5 handoff

Production payment SKU mapping and secure server receipt verification can be layered on top of the same bridge events and APIs.
