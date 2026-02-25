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

The game requires legally-purchased PvZ GOTY game assets. These are **gitignored** (`.gitignore` lists `main.pak` and `build-*/`) and must be provided separately:

- `main.pak` — place in `build-web/`
- `properties/resources.xml` — place in `build-web/properties/`
- `Properties/LawnStrings.txt` — place in `build-web/Properties/` (optional)

The WASM module fetches these via HTTP at runtime (see `src/emscripten_fetch.cpp`). Without them, the module initializes successfully but reports missing resource files.

### Native Build (optional, not primary)

```bash
CC=gcc CXX=g++ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-D_Mix_Chunk=Mix_Chunk"
cmake --build build
```

- Must use GCC (`CC=gcc CXX=g++`) because the default Clang on this VM lacks `libstdc++`.
- The `-D_Mix_Chunk=Mix_Chunk` preprocessor flag works around a type mismatch in `src/Sexy.TodLib/TodFoley.h`.
- Requires system dev libraries: `ninja-build libsdl2-dev libjpeg-turbo8-dev libogg-dev libvorbis-dev libopenmpt-dev libmpg123-dev`.

### Lint / Tests

There is no linter configuration or automated test suite in this codebase. Verification is done by building successfully and running the application.

### Emscripten SDK

Installed at `/opt/emsdk`. Activate with `source /opt/emsdk/emsdk_env.sh` before building. The update script handles installation/activation automatically.
