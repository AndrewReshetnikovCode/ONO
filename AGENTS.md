# AGENTS.md

## Cursor Cloud specific instructions

### Overview

PvZ-Portable is a C++ (C++20) reimplementation of Plants vs. Zombies GOTY Edition. The primary development target is the **Emscripten/WebAssembly build** which runs in a browser. There are no external services, databases, or Docker containers.

### Web Build (Emscripten) — primary target

```bash
source /opt/emsdk/emsdk_env.sh
emcmake cmake -G Ninja -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

- Output: `build-web/index.html`, `build-web/index.js`, `build-web/index.wasm`.
- Serve with `python3 -m http.server 8080` from inside `build-web/`.
- Open `http://localhost:8080/index.html` in Chrome.
- Emscripten ports (zlib, libpng, libjpeg, SDL2) are fetched automatically; no system dev libraries needed for the web build.
- The Emscripten build does **not** use SDL-Mixer-X (uses WebAudio instead), so the `_Mix_Chunk` workaround is not needed.

### Game Assets

Game assets live in `GOTY_Res/` (committed for local educational use only). Before serving, copy them into `build-web/`:

```bash
cp GOTY_Res/main.pak build-web/
cp -r GOTY_Res/properties/* build-web/properties/
mkdir -p build-web/Properties
cp GOTY_Res/properties/LawnStrings.txt build-web/Properties/LawnStrings.txt
```

The WASM module fetches `main.pak` and `properties/resources.xml` via HTTP at runtime (see `src/emscripten_fetch.cpp`).

### Native Build (optional, not primary)

```bash
CC=gcc CXX=g++ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-D_Mix_Chunk=Mix_Chunk"
cmake --build build
```

- Must use GCC (`CC=gcc CXX=g++`) because the default Clang on this VM lacks `libstdc++`.
- The `-D_Mix_Chunk=Mix_Chunk` preprocessor flag works around a type mismatch in `src/Sexy.TodLib/TodFoley.h`.
- Requires system dev libraries: `ninja-build libsdl2-dev libjpeg-turbo8-dev libogg-dev libvorbis-dev libopenmpt-dev libmpg123-dev`.

### Yandex Games Builds

Two variants, same `-DYANDEX_GAMES=ON` flag, differ by `-DYANDEX_GAMES_STUB`:

**Stub (local testing, no SDK network calls):**
```bash
source /opt/emsdk/emsdk_env.sh
emcmake cmake -G Ninja -B build-yg-stub -DCMAKE_BUILD_TYPE=Release -DYANDEX_GAMES=ON -DYANDEX_GAMES_STUB=ON
cmake --build build-yg-stub
```
- Shell: `yandex_shell_stub.html` — no real SDK loaded, `window.YG` stubs use `localStorage`.
- Console shows `[YG Stub]` messages instead of SDK errors. Clean for development.

**Platform (real YG SDK for deployment):**
```bash
emcmake cmake -G Ninja -B build-yg -DCMAKE_BUILD_TYPE=Release -DYANDEX_GAMES=ON
cmake --build build-yg
```
- Shell: `yandex_shell.html` — loads `yandex.ru/games/sdk/v2`, calls `YaGames.init()`, `LoadingAPI.ready()`.
- `YandexSaveProvider` syncs saves to YG cloud via `player.setData()`/`player.getData()`.
- Audio auto-mutes on `visibilitychange` and during ads.
- Copy game assets to the build directory the same way as `build-web/`.

### Lint / Tests

There is no linter configuration or automated test suite in this codebase. Verification is done by building successfully and running the application.

### Emscripten SDK

Installed at `/opt/emsdk`. Activate with `source /opt/emsdk/emsdk_env.sh` before building. The update script handles installation/activation automatically.
