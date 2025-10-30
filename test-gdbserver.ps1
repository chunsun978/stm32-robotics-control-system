#!/usr/bin/env pwsh
# Test ST-LINK GDB Server Connection

$ErrorActionPreference = 'Continue'

$gdbServerPath = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_2.2.200.202505060755\tools\bin\ST-LINK_gdbserver.exe"
$cubeProgrammerPath = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.200.202503041107\tools\bin"

Write-Host "Testing ST-LINK GDB Server..." -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

# Try with minimal args first
Write-Host "Attempt 1: Minimal arguments..." -ForegroundColor Yellow
& $gdbServerPath -p 3333 -cp $cubeProgrammerPath -l 4
Write-Host ""

Write-Host ""
Write-Host "GDB Server stopped." -ForegroundColor Cyan

