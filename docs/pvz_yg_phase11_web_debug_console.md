# PvZ YG Phase 11 — Web Debug Console Bridge

## Scope

Added a browser developer-console bridge for inspecting and invoking runtime logic introduced in Phases 0-10 without changing gameplay authority flow.

Implemented files:

- `src/web_debug_console.cpp`
- `src/LawnApp.cpp`

## Runtime behavior

1. Web-only bridge installation:
   - During `LawnApp::Init` on Emscripten builds, the bridge installs once and exposes `window.PvzDebug`.

2. Console invocation style:
   - Example required call pattern is now supported:
     - `window.PvzDebug.TestFunction("abc", 1)`
   - Alias also exists:
     - `window.PvzDebug.testFunction("abc", 1)`

3. Exposed debug methods (selected):
   - Session/runtime:
     - `getClientSessionState()`
     - `getLatestSnapshot()`
     - `getSessionEvents(limit)`
     - `clearSessionEvents()`
   - Authoritative commands:
     - `submitPlantCommand(x, y, seedType, imitaterSeedType)`
     - `submitRemovePlantCommand(x, y)`
   - Progression/Yandex:
     - `getAuthoritativeProfile()`
     - `getYandexStatus()`
   - Protocol and bots:
     - `validatePlaceCommand(...)`
     - `assignBotTracks(lobbyId, mmrHint, botCount)`
     - `listBotTracks(limit)`
     - `getBotTrack(trackId)`
   - Telemetry:
     - `getRuntimeTelemetry()`

4. Safety constraints:
   - Bridge does not bypass server-authoritative checks.
   - All command submissions still flow through existing validation/runtime paths.
   - Desktop/native builds are unaffected.
