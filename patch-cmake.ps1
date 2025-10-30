#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Patch STM32CubeMX generated CMakeLists.txt with useful post-build commands
.DESCRIPTION
    Adds automatic .bin/.hex generation and size reporting to CMakeLists.txt
.EXAMPLE
    .\patch-cmake.ps1
    .\patch-cmake.ps1 -Path Z:\projects\stm32\NewProject\CMakeLists.txt
#>

param(
    [Parameter()]
    [string]$Path = "CMakeLists.txt"
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Path)) {
    Write-Host "Error: CMakeLists.txt not found at: $Path" -ForegroundColor Red
    exit 1
}

$content = Get-Content $Path -Raw

# Check if already patched
if ($content -match 'CMAKE_OBJCOPY.*\.bin') {
    Write-Host "CMakeLists.txt is already patched!" -ForegroundColor Yellow
    exit 0
}

# Snippet to add
$snippet = @'

# Generate binary and hex files after build
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating binary files and size info"
)
'@

# Add snippet at the end
$content += $snippet
Set-Content -Path $Path -Value $content -NoNewline

Write-Host "✓ Successfully patched CMakeLists.txt!" -ForegroundColor Green
Write-Host "  Added: .bin and .hex file generation" -ForegroundColor Cyan
Write-Host "  Added: Binary size reporting" -ForegroundColor Cyan

