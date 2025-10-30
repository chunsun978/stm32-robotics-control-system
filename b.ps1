#!/usr/bin/env pwsh
# Ultra-short build script - Just type: .\b
param(
    [Parameter(Position=0)]
    [string]$Command = 'build',
    [Parameter(Position=1)]
    [string]$Preset = 'Debug'
)

# Check if command is 'ninja' - use ninja directly
if ($Command -eq 'ninja') {
    $buildDir = "build/$Preset"
    & ninja -C $buildDir
}
# Check if it's a ninja command variant
elseif ($Command -match '^ninja-(.+)$') {
    $buildDir = "build/$Preset"
    $ninjaCmd = $matches[1]
    switch ($ninjaCmd) {
        'clean'   { & ninja -C $buildDir clean }
        'rebuild' { & ninja -C $buildDir clean; & ninja -C $buildDir }
        default   { & ninja -C $buildDir }
    }
}
# Otherwise use CMake wrapper
else {
    & "$PSScriptRoot\build.ps1" -Command $Command -Preset $Preset
}
