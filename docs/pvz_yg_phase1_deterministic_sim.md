# PvZ YG Phase 1 — Deterministic Simulation Extraction

## 1) Scope

Phase 1 focuses on deterministic extraction around authoritative gameplay loops:

- sun spawning logic,
- zombie wave progression logic,
- deterministic replay/hash checkpoint harness.

Implementation files:

- `src/Lawn/System/DeterministicSim.h`
- `src/Lawn/System/DeterministicSim.cpp`
- `src/Lawn/Board.cpp`

---

## 2) Extracted Deterministic Helpers

### 2.1 Sun-spawning helpers

Added pure helper functions for:

- skip-gate decision (`DeterministicSunShouldSkip`)
- tutorial plant gate decision (`DeterministicSunNeedsPlantTutorialGate`)
- post-tick spawn condition (`DeterministicSunShouldSpawnAfterTick`)
- next countdown formula (`DeterministicComputeNextSunCountDown`)

These mirror existing gameplay behavior but move rule evaluation into deterministic helper code.

### 2.2 Wave-spawning helpers

Added deterministic helper functions for:

- countdown acceleration decision (`DeterministicShouldAccelerateZombieCountdown`)
- health-threshold computation (`DeterministicComputeZombieHealthToNextWave`)
- random countdown offset application (`DeterministicComputeZombieCountdownWithRandomOffset`)

These keep behavior equivalent while isolating core authority math from raw update body flow.

---

## 3) Replay / Determinism Harness

Introduced:

- `DeterministicReplayFrame`
- `DeterministicReplayRecorder`
- stable 64-bit FNV-1a style hash helpers:
  - `DeterministicHashInit`
  - `DeterministicHashCombine`
  - `DeterministicHashFinalize`

In `Board::UpdateGame`, debug builds now record a per-tick deterministic state hash checkpoint for replay parity diagnostics.

---

## 4) Integration Notes

`Board::UpdateSunSpawning` and `Board::UpdateZombieSpawning` now use deterministic helper state builders and helper functions while preserving original game outcomes.

`Board::UpdateGame` appends debug replay checkpoints so future server/client parity tools can compare deterministic traces.

---

## 5) Next Step (Phase 2 handoff)

Phase 2 can reuse this deterministic surface when building the authoritative server tick loop:

- same formulas and gate logic,
- same state-hash checkpoints to detect divergence,
- same replay frame model for command-driven deterministic validation.
