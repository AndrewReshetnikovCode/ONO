# PvZ YG Phase 6 — Anti-Cheat Monitoring Pipeline

## Scope

Phase 6 adds server-side anti-cheat monitoring hooks and event propagation.

Implemented files:

- `src/Lawn/System/AuthoritativeAntiCheat.h`
- `src/Lawn/System/AuthoritativeAntiCheat.cpp`
- `src/Lawn/System/AuthoritativeRuntime.h`
- `src/Lawn/System/AuthoritativeRuntime.cpp`

## Implemented behavior

1. Anti-cheat monitor:
   - tracks per-player sun snapshots
   - tracks per-player command rates per tick
   - accepts explicit critical desync reports

2. Event classes:
   - resource anomaly
   - action rate limit exceeded
   - critical desync

3. Runtime integration:
   - command submissions are recorded by anti-cheat monitor
   - sun snapshots are recorded in authoritative sun tick logic
   - PvP target mismatch / limit breach register critical desync events
   - anti-cheat events are emitted back into runtime event stream

## Phase-6 handoff

Enforcement ladder (warn, shadow limit, isolate, ban) can consume these anti-cheat events in a policy service without changing event producers.
