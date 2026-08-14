# Folie

Supersaw synth VST3/AU with per-voice tuned feedback loops (EVS plugin line).
Full product spec and architecture rationale: see the Folie handoff doc / Notion
"Folie — Dev Tracker" database.

## Build

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Artefacts land in `build/Folie_artefacts/Release/`.

## Test

```sh
ctest --test-dir build --output-on-failure   # offline DSP render tests (FolieTests)
```

## Local install (Layer-1 dev loop: build → rescan DAW → play)

- macOS: `scripts/dev-install.sh` (copies VST3 + AU into ~/Library/Audio/Plug-Ins)
- Windows: `scripts/dev-install.ps1` (copies VST3 into Common Files; self-elevates)

Both just configure with `-DFOLIE_COPY_PLUGIN=ON` and build — JUCE's
COPY_PLUGIN_AFTER_BUILD does the copying. CI never sets this flag.

## Rules

- **Zero post FX by design** (post-initial-build handoff): no chorus/delay/reverb —
  ever. The in-loop saturator/damping filter and osc stereo width stay (sound
  generation). The only bus stage is the fixed, parameterless safety soft-clip.
- **Per-voice principle**: the feedback loop, its saturator, and all three envelopes
  run per voice, never on the sum. No optimization may share them across voices.

- Every parameter ID lives in `source/params/ParameterIDs.h` and is registered in
  `createParameterLayout()` — never inline parameter strings anywhere else.
- All parameters are host-automatable APVTS parameters; plugin state is exactly the
  APVTS tree (+ `uiWidth`/`uiHeight` properties). No other stateful storage.
- The editor is a pure function of plugin state: attachments only, zero model state
  held in the editor. It must be constructible/destructible at any time with no
  effect on sound.
- Code in `source/dsp/` stays UI-free and headless-testable (header-only where
  practical); it is exercised by `tests/RenderTests.cpp` without the plugin wrapper.
- No allocations, locks, or logging on the audio thread after `prepareToPlay`.
- JUCE is pinned in `cmake/GetJUCE.cmake` — do not bump without a reason.
- Branch flow: day-to-day work pushes to `develop`; `main` is release-only (John
  PRs manually). CI must be green on every push to `develop`.
- Work tracking: Notion "Folie — Dev Tracker" (FOL-# items). Set items In
  progress/Fixed in develop as work lands; new discovered work gets a tracker item.
