# PvZ YG Phase 7 — UX and Localization Compliance

## Scope

Phase 7 applies moderation/platform compliance hardening for focus-audio and localization behavior.

Implemented files:

- `src/LawnApp.cpp`
- `src/SexyAppFramework/SexyAppBase.cpp`
- `src/emscripten_fetch.cpp`

## Implemented behavior

1. Focus-loss audio pause:
   - `mMuteOnLostFocus` is enabled in web builds (`__EMSCRIPTEN__`) from app construction path.

2. Purchase dialog localization cleanup:
   - removed hardcoded Chinese store purchase header.
   - switched to localization key lookup with Russian fallback:
     - fallback text: `Купить этот предмет?`

3. Case-sensitive resource path hardening:
   - `LoadProperties()` now attempts:
     - `properties/LawnStrings.txt`
     - then `Properties/LawnStrings.txt`
   - Emscripten resource fetch includes lowercase and legacy uppercase `LawnStrings.txt` paths.

## Phase-7 handoff

Content team can now complete full Russian localization coverage through string keys without relying on hardcoded UI literals.
