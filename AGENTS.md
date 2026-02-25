# AGENTS.md

## Cursor Cloud specific instructions

### Overview

PvZ-Portable is a C++ (C++20) reimplementation of Plants vs. Zombies GOTY Edition. It builds into a single native binary using CMake + Ninja. There are no external services, databases, or Docker containers.

### Build

```bash
CC=gcc CXX=g++ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-D_Mix_Chunk=Mix_Chunk"
cmake --build build
```

- You **must** use GCC (`CC=gcc CXX=g++`) because the default Clang on this VM lacks `libstdc++`.
- The `-D_Mix_Chunk=Mix_Chunk` preprocessor flag is required to work around a type mismatch in `src/Sexy.TodLib/TodFoley.h` (uses `struct _Mix_Chunk*` but the bundled SDL-Mixer-X defines `struct Mix_Chunk`). The upstream repo has this fixed; this local copy still needs the workaround.
- The binary is output to `build/pvz-portable`.

### Running

The game requires legally-purchased PvZ GOTY game assets (`main.pak` and `properties/` directory) placed alongside the binary. Without them, the binary will start but immediately report missing resource files — this is expected.

### Lint / Tests

There is no linter configuration or automated test suite in this codebase. Verification is done by building successfully and running the binary.

### System dependencies (Ubuntu 24.04)

These are installed via `apt-get` and are **not** part of the update script (they persist across VM sessions):

`ninja-build libsdl2-dev libjpeg-turbo8-dev libogg-dev libvorbis-dev libopenmpt-dev libmpg123-dev`

Pre-installed: `gcc/g++ 13`, `cmake 3.28`, `pkg-config`, `libpng-dev`, `zlib1g-dev`.
