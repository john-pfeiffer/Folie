# Folie

Supersaw synthesizer with musically-controlled feedback loops — the second
instrument in the EVS plugin line. A wall of detuned saws (1–16 per voice)
feeding a per-voice feedback loop tuned to the played note, so cranked feedback
screams *in key* instead of just making noise.

Formats: VST3 (macOS + Windows) and AU (macOS).

## Building

Requires CMake ≥ 3.25 and a C++20 toolchain. JUCE is fetched automatically
(pinned in `cmake/GetJUCE.cmake`).

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Local dev install

Copies the freshly built plugin into your system plugin folders, so the loop is
build → rescan in DAW → play:

- macOS: `scripts/dev-install.sh`
- Windows: `scripts/dev-install.ps1` (elevates itself; VST3 goes to Common Files)

## CI builds

Every push to `develop` builds macOS + Windows, runs pluginval, and uploads the
plugins as workflow artifacts named `Folie-<OS>-<short-sha>.zip` — download from
the Actions run page, no release required. Version tags (`v*`) on `main` produce
the real GitHub Release.

## Branches

- `develop` — day-to-day work; every push must keep CI green.
- `main` — stable/release-worthy only (merged via PR).

> Note: the 4-char plugin codes (`Evsy`/`Foli` in CMakeLists.txt) become
> permanent once builds live in real DAW projects — sign off before the first
> public release. v1 binaries are unsigned/un-notarized; macOS users must
> right-click-open or clear quarantine until notarization lands.
