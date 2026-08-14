#!/usr/bin/env bash
# Layer-1 dev loop (macOS/Linux): build and copy the plugin straight into the
# local plugin folders via JUCE's COPY_PLUGIN_AFTER_BUILD. Then rescan in your
# DAW and play — no Git, no release.
set -euo pipefail
cd "$(dirname "$0")/.."

GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
fi

cmake -B build "${GENERATOR_ARGS[@]}" -DCMAKE_BUILD_TYPE=Release -DFOLIE_COPY_PLUGIN=ON
cmake --build build --parallel

echo
echo "Installed:"
if [[ "$(uname)" == "Darwin" ]]; then
    echo "  VST3 -> ~/Library/Audio/Plug-Ins/VST3/Folie.vst3"
    echo "  AU   -> ~/Library/Audio/Plug-Ins/Components/Folie.component"
else
    echo "  VST3 -> ~/.vst3/Folie.vst3"
fi
echo "Rescan plugins in your DAW to pick up the new build."
