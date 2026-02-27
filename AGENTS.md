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

### Canvas & Aspect Ratio

Strict 4:3 aspect ratio is enforced by `fitCanvas()` in both HTML shells. It calculates the largest 4:3 rectangle that fits the viewport, centres it, and applies CSS via `setProperty('important')` to override SDL's inline styles. Game rendering code is untouched — the GL viewport reads actual canvas pixel dimensions via `emscripten_get_canvas_element_size` and fills the canvas exactly.

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

### Serving the WASM Build

After building and copying assets, serve from the build directory:

```bash
cd build-yg-stub && python3 -m http.server 8080
```

Open `http://localhost:8080/index.html` in Chrome. The game runs entirely client-side (no backend).

### Emscripten SDK

Installed at `/opt/emsdk`. Activate with `source /opt/emsdk/emsdk_env.sh` before building. The update script handles installation/activation automatically.
