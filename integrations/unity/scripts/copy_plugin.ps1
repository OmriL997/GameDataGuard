<#
.SYNOPSIS
    Copies the built gamedataguard_unity.dll into the Unity project's Plugins folder.

.DESCRIPTION
    Run this script after building the CMake project with GDG_BUILD_UNITY_PLUGIN=ON.
    Close Unity before running this script if the project is already open, because
    Unity holds a file lock on the DLL while it is loaded.

.PARAMETER Configuration
    CMake build configuration: Debug or Release (default: Release).

.PARAMETER BuildDir
    Path to the CMake build directory, relative to the repository root
    (default: build).

.EXAMPLE
    .\copy_plugin.ps1
    .\copy_plugin.ps1 -Configuration Debug
    .\copy_plugin.ps1 -Configuration Release -BuildDir build_release

.NOTES
    Build steps before running this script:
        cmake -B build -DGDG_BUILD_UNITY_PLUGIN=ON
        cmake --build build --config Release --target gamedataguard_unity
#>
param(
    [string]$Configuration = "Release",
    [string]$BuildDir      = "build"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Resolve paths relative to the repository root.
# This script lives at integrations/unity/scripts/copy_plugin.ps1
# so three levels up is the repository root.
# ---------------------------------------------------------------------------
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot ".." ".." "..")

$dllName    = "gamedataguard_unity.dll"
$sourcePath = Join-Path $repoRoot "$BuildDir" "$Configuration" $dllName
$destDir    = Join-Path $repoRoot "integrations" "unity" `
                  "GameDataGuardUnity" "Assets" "Plugins" "x86_64"
$destPath   = Join-Path $destDir $dllName

Write-Host ""
Write-Host "GameDataGuard Unity Plugin Copy"
Write-Host "================================"
Write-Host "  Repository root : $repoRoot"
Write-Host "  Configuration   : $Configuration"
Write-Host "  Source          : $sourcePath"
Write-Host "  Destination     : $destPath"
Write-Host ""

# ---------------------------------------------------------------------------
# Verify that the DLL was built.
# ---------------------------------------------------------------------------
if (-not (Test-Path $sourcePath)) {
    Write-Error @"
DLL not found at: $sourcePath

Build the plugin first:
    cmake -B $BuildDir -DGDG_BUILD_UNITY_PLUGIN=ON
    cmake --build $BuildDir --config $Configuration --target gamedataguard_unity
"@
    exit 1
}

# ---------------------------------------------------------------------------
# Warn if Unity appears to be running (heuristic: lockfile present).
# ---------------------------------------------------------------------------
$lockFile = Join-Path $repoRoot "integrations" "unity" `
                "GameDataGuardUnity" "Temp" "UnityLockfile"
if (Test-Path $lockFile) {
    Write-Warning @"
Unity lockfile detected at:
  $lockFile

Close Unity before replacing the DLL to avoid an access-denied error.
The lockfile is removed automatically when Unity exits.
"@
}

# ---------------------------------------------------------------------------
# Create the destination directory if it does not already exist.
# ---------------------------------------------------------------------------
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
    Write-Host "Created: $destDir"
}

# ---------------------------------------------------------------------------
# Copy the DLL.
# ---------------------------------------------------------------------------
try {
    Copy-Item -Path $sourcePath -Destination $destPath -Force -ErrorAction Stop
}
catch {
    Write-Error @"
Copy failed: $_

If Unity has the DLL locked, close Unity and retry.
"@
    exit 1
}

Write-Host "Copied $dllName -> $destPath" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps"
Write-Host "----------"
Write-Host "  1. Open the Unity project at:"
Write-Host "       integrations/unity/GameDataGuardUnity/"
Write-Host "  2. Unity will reimport the plugin automatically."
Write-Host "  3. In the Plugin Inspector for gamedataguard_unity.dll:"
Write-Host "       - Uncheck 'Any Platform'"
Write-Host "       - Check 'Editor'"
Write-Host "       - Set platform to 'Windows' / 'x86_64'"
Write-Host "       - Click Apply"
Write-Host "  4. Open Tools > GameDataGuard to run the validation window."
Write-Host ""
