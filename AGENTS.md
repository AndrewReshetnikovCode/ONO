# AGENTS.md

## Cursor Cloud specific instructions

### Overview

PvZ-Portable is a C++ (C++20) reimplementation of Plants vs. Zombies GOTY Edition. The primary development targets are the **Yandex Games builds** (Emscripten/WebAssembly). There are no external services, databases, or Docker containers.

### Yandex Games Builds (primary)

Two variants, same `-DYANDEX_GAMES=ON` flag, differ by `-DYANDEX_GAMES_STUB`:

**Stub (local testing, no SDK network calls):**
```bash
source /opt/emsdk/emsdk_env.sh
emcmake cmake -G Ninja -B build-yg-stub -DCMAKE_BUILD_TYPE=Release \
  -DYANDEX_GAMES=ON -DYANDEX_GAMES_STUB=ON \
  -DCANVAS_WIDTH=960 -DCANVAS_HEIGHT=720
cmake --build build-yg-stub
```
- Shell: `yandex_shell_stub.html` — no real SDK, `window.YG` stubs use `localStorage`.
- Console shows `[YG Stub]` messages. Clean for development.

**Platform (real YG SDK for deployment):**
```bash
emcmake cmake -G Ninja -B build-yg -DCMAKE_BUILD_TYPE=Release \
  -DYANDEX_GAMES=ON \
  -DCANVAS_WIDTH=960 -DCANVAS_HEIGHT=720
cmake --build build-yg
```
- Shell: `yandex_shell.html` — loads `yandex.ru/games/sdk/v2`, calls `YaGames.init()`, `LoadingAPI.ready()`.
- `YandexSaveProvider` syncs saves to YG cloud via `player.setData()`/`player.getData()`.
- Audio auto-mutes on `visibilitychange` and during ads.

### Canvas Resolution

`CANVAS_WIDTH` / `CANVAS_HEIGHT` CMake options control the CSS display size of the canvas (default 0 = auto-fill viewport). The game renders at its native resolution internally; the browser CSS stretches the canvas to the configured display size. Game rendering code is never touched — this is CSS-only.

### Game Assets

Assets live in `GOTY_Res/` (committed for local educational use only). Copy into any build directory before serving:

```bash
cp GOTY_Res/main.pak build-yg-stub/
cp -r GOTY_Res/properties/* build-yg-stub/properties/
mkdir -p build-yg-stub/Properties
cp GOTY_Res/properties/LawnStrings.txt build-yg-stub/Properties/LawnStrings.txt
```

The WASM module fetches `main.pak` and `properties/resources.xml` via HTTP at runtime (see `src/emscripten_fetch.cpp`).

### Native Build (optional)

```bash
CC=gcc CXX=g++ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-D_Mix_Chunk=Mix_Chunk"
cmake --build build
```

- Must use GCC — default Clang lacks `libstdc++`.
- Requires system dev libraries: `ninja-build libsdl2-dev libjpeg-turbo8-dev libogg-dev libvorbis-dev libopenmpt-dev libmpg123-dev`.

### Lint / Tests

No linter or test suite. Verification is done by building and running the application.

### Emscripten SDK

Installed at `/opt/emsdk`. Activate with `source /opt/emsdk/emsdk_env.sh` before building. The update script handles installation/activation automatically.
