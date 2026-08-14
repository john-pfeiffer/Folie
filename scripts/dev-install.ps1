# Layer-1 dev loop (Windows): build and copy the VST3 into
# C:\Program Files\Common Files\VST3 via JUCE's COPY_PLUGIN_AFTER_BUILD.
# Writing to Common Files needs admin, so this script self-elevates.

$ErrorActionPreference = "Stop"

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Elevating to administrator (VST3 install dir needs it)..."
    Start-Process -FilePath "powershell" -Verb RunAs `
        -ArgumentList "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`""
    exit
}

Set-Location (Join-Path $PSScriptRoot "..")

cmake -B build -DCMAKE_BUILD_TYPE=Release -DFOLIE_COPY_PLUGIN=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Installed: C:\Program Files\Common Files\VST3\Folie.vst3"
Write-Host "Rescan plugins in your DAW to pick up the new build."
