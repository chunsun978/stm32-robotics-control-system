#!/usr/bin/env pwsh
<#
.SYNOPSIS
    STM32 Project Build Tool - Similar to idf.py for ESP32
.DESCRIPTION
    Simple wrapper around CMake for STM32 projects
.EXAMPLE
    .\build.ps1 build
    .\build.ps1 clean
    .\build.ps1 flash
#>

param(
    [Parameter(Position=0)]
    [ValidateSet('build', 'clean', 'rebuild', 'configure', 'flash', 'size', 'help')]
    [string]$Command = 'build',
    
    [Parameter()]
    [ValidateSet('Debug', 'Release')]
    [string]$Preset = 'Debug',
    
    [switch]$VerboseOutput
)

$ErrorActionPreference = 'Stop'

function Show-Help {
    Write-Host @"
STM32 Build Tool - Usage:

  .\build.ps1 build      - Build the project (default)
  .\build.ps1 clean      - Clean build artifacts
  .\build.ps1 rebuild    - Clean then build
  .\build.ps1 configure  - Run CMake configuration
  .\build.ps1 flash      - Flash firmware to device
  .\build.ps1 size       - Show binary size information
  .\build.ps1 help       - Show this help message

Options:
  -Preset <Debug|Release>  - Select build preset (default: Debug)
  -VerboseOutput           - Show verbose build output

Examples:
  .\build.ps1
  .\build.ps1 build -Preset Release
  .\build.ps1 rebuild -VerboseOutput
"@
}

function Invoke-Build {
    Write-Host "Building project (Preset: $Preset)..." -ForegroundColor Cyan
    $buildArgs = @('--build', '--preset', $Preset)
    if ($VerboseOutput) { $buildArgs += '--verbose' }
    
    & cmake @buildArgs
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`nBuild completed successfully!" -ForegroundColor Green
    } else {
        Write-Host "`nBuild failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

function Invoke-Clean {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Cyan
    & cmake --build --preset $Preset --target clean
    Write-Host "Clean completed!" -ForegroundColor Green
}

function Invoke-Configure {
    Write-Host "Configuring project (Preset: $Preset)..." -ForegroundColor Cyan
    & cmake --preset $Preset
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Configuration completed!" -ForegroundColor Green
    } else {
        Write-Host "Configuration failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

function Invoke-Rebuild {
    Invoke-Clean
    Invoke-Build
}

function Invoke-Flash {
    $elfFile = "build/$Preset/Test01.elf"
    
    if (-not (Test-Path $elfFile)) {
        Write-Host "Error: $elfFile not found. Build the project first." -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Flashing firmware with OpenOCD..." -ForegroundColor Cyan
    
    # Use OpenOCD (works with WinUSB driver)
    if (Get-Command openocd -ErrorAction SilentlyContinue) {
        & openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init" -c "reset halt" -c "program $elfFile verify reset exit"
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`nFlash completed successfully!" -ForegroundColor Green
        } else {
            Write-Host "`nFlash failed!" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    }
    else {
        Write-Host @"
Error: OpenOCD not found!
Please install OpenOCD:
  choco install openocd

Or manually flash using OpenOCD:
  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $elfFile verify reset exit"
"@ -ForegroundColor Red
        exit 1
    }
}

function Show-Size {
    $elfFile = "build/$Preset/Test01.elf"
    
    if (-not (Test-Path $elfFile)) {
        Write-Host "Error: $elfFile not found. Build the project first." -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Binary size information:" -ForegroundColor Cyan
    & arm-none-eabi-size $elfFile
    Write-Host ""
    & arm-none-eabi-size -A $elfFile
}

# Main execution
switch ($Command) {
    'build'     { Invoke-Build }
    'clean'     { Invoke-Clean }
    'rebuild'   { Invoke-Rebuild }
    'configure' { Invoke-Configure }
    'flash'     { Invoke-Flash }
    'size'      { Show-Size }
    'help'      { Show-Help }
}

