# PvZ YG Phase 9 — Performance, Scale, and Release Gates

## Scope

Phase 9 adds release-gate scaffolding for protocol consistency and runtime telemetry.

Implemented files:

- `src/Lawn/System/RuntimeTelemetry.h`
- `src/Lawn/System/RuntimeTelemetry.cpp`
- `src/LawnApp.h`
- `src/LawnApp.cpp`
- `tools/ci/check_protocol_compat.py`

## Implemented behavior

1. Runtime frame telemetry:
   - frame count
   - <=16ms bucket
   - <=33ms bucket
   - >33ms bucket
   - average frame ms
   - max frame ms

2. App integration:
   - telemetry object lifecycle managed in `LawnApp`
   - per-update frame durations recorded in `UpdateFrames`

3. Protocol release gate script:
   - `tools/ci/check_protocol_compat.py`
   - verifies `NET_PROTOCOL_VERSION_V1` matches documented Phase-0 contract version (`major`/`minor`)
   - intended for CI integration as a fast compatibility gate

## Suggested CI gate

Run:

- `python3 tools/ci/check_protocol_compat.py`

Fail build if script exits non-zero.

## Phase-9 closure

All planned phases (3–9) now have concrete code/docs scaffolding in repository and can be iterated toward production hardening.
